#include "LevelSerializer.h"

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/World/World.h>
#include <RapidJson/Headers/JsonObject.h>
#include <Editor/EditorLogManager.h>
#include <IO/FileHelper.h>
#include "JsonArchive.h"
#include "Property.h"

namespace PigeonEngine
{
	namespace
	{
		EString MakeIndexKey(const UINT32 InIndex)
		{
			return EString::FromInt(InIndex);
		}

		// Writes one actor entry. Indexes are into the actor's own component list and
		// actor ids are 1-based into the caller's actor set, so a file is self-contained.
		void WriteActorEntry(EJsonArchiveWriter& Writer, const UINT32 InKeyIndex, const PActor& InActor, const TMap<const PActor*, UINT32>& InActorFileIds)
		{
			const EString ElementKey = MakeIndexKey(InKeyIndex);
			Writer.BeginObject(*ElementKey);

			const size_t ActorClassHash = GetClassHashCode(&InActor);
			EString ActorClassName(GetClassStableName(ActorClassHash));
			Writer.SerializeString(PE_TEXT("Class"), ActorClassName);
			UINT32 ActorClassVersion = EClassPropertyRegistry::GetClassVersion(ActorClassHash);
			Writer.SerializeUInt32(PE_TEXT("ClassVersion"), ActorClassVersion);

			UINT32 ParentActorId = 0u;
			if (const PActor* ParentActor = InActor.GetAttachedParentActor())
			{
				if (const UINT32* Found = InActorFileIds.FindValueAsPtr(ParentActor))
				{
					ParentActorId = (*Found);
				}
			}
			Writer.SerializeUInt32(PE_TEXT("ParentActorID"), ParentActorId);

			// Actor reflected properties live in their own scope, mirroring components. No
			// actor class registers properties yet, but the scope keeps the format stable for
			// when they do and makes ClassVersion meaningful for actors too.
			Writer.BeginObject(PE_TEXT("Properties"));
			EClassPropertyRegistry::SerializeObject(Writer, ActorClassHash, const_cast<PActor*>(&InActor));
			Writer.EndObject();

			const TArray<PActorComponent*> Components = InActor.GetAllComponents();
			UINT32 ComponentCount = Components.Num<UINT32>();
			Writer.BeginArray(PE_TEXT("Components"), ComponentCount);
			for (UINT32 ComponentIndex = 0u; ComponentIndex < ComponentCount; ComponentIndex++)
			{
				PActorComponent* Component = Components[ComponentIndex];
				Writer.BeginObject(*MakeIndexKey(ComponentIndex));

				const size_t ComponentClassHash = GetClassHashCode(Component);
				EString ComponentClassName(GetClassStableName(ComponentClassHash));
				Writer.SerializeString(PE_TEXT("Class"), ComponentClassName);
				UINT32 ComponentClassVersion = EClassPropertyRegistry::GetClassVersion(ComponentClassHash);
				Writer.SerializeUInt32(PE_TEXT("ClassVersion"), ComponentClassVersion);

				BOOL32 bIsRoot = (Component == InActor.GetRootComponent()) ? TRUE : FALSE;
				Writer.SerializeBool(PE_TEXT("Root"), bIsRoot);

				UINT32 ParentComponentIndex = 0u;
				if (PSceneComponent* SceneComponent = dynamic_cast<PSceneComponent*>(Component))
				{
					if (PSceneComponent* ParentComponent = SceneComponent->GetParentComponent())
					{
						const INT32 FoundIndex = Components.Find(ParentComponent);
						if (FoundIndex >= 0)
						{
							ParentComponentIndex = static_cast<UINT32>(FoundIndex) + 1u;
						}
					}
				}
				Writer.SerializeUInt32(PE_TEXT("ParentComponentIndex"), ParentComponentIndex);

				Writer.BeginObject(PE_TEXT("Properties"));
				EClassPropertyRegistry::SerializeObject(Writer, ComponentClassHash, Component);
				Writer.EndObject();

				Writer.EndObject();
			}
			Writer.EndArray();

			Writer.EndObject();
		}
	}

	BOOL32 ELevelSerializer::SaveActors(const TArray<PActor*>& InActors, CJsonObject& OutDocument)
	{
		TMap<const PActor*, UINT32> ActorFileIds;
		for (UINT32 Index = 0u, Num = InActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (InActors[Index])
			{
				ActorFileIds.Add(InActors[Index], Index + 1u);
			}
		}

		EJsonArchiveWriter Writer(OutDocument);
		UINT32 FormatVersion = LevelFormatVersion;
		Writer.SerializeUInt32(PE_TEXT("FormatVersion"), FormatVersion);

		UINT32 ActorCount = InActors.Num<UINT32>();
		Writer.BeginArray(PE_TEXT("Actors"), ActorCount);
		for (UINT32 Index = 0u; Index < ActorCount; Index++)
		{
			PActor* Actor = InActors[Index];
			if (Actor)
			{
				WriteActorEntry(Writer, Index, *Actor, ActorFileIds);
			}
			else
			{
				// Keep array indexes stable even for a null entry.
				Writer.BeginObject(*MakeIndexKey(Index));
				Writer.EndObject();
			}
		}
		Writer.EndArray();
		return TRUE;
	}

	BOOL32 ELevelSerializer::SaveWorld(const PWorld& InWorld, CJsonObject& OutDocument)
	{
		const TArray<PActor*> Actors = InWorld.GetAllActors();
		return SaveActors(Actors, OutDocument);
	}

	BOOL32 ELevelSerializer::SaveActorsToFile(const TArray<PActor*>& InActors, const EString& InPath)
	{
		CJsonObject Document;
		if (!SaveActors(InActors, Document))
		{
			return FALSE;
		}
		return EFileHelper::SaveStringToFile(InPath, Document.ToString(), TRUE, FALSE) ? TRUE : FALSE;
	}

	BOOL32 ELevelSerializer::LoadActors(PWorld& InWorld, const CJsonObject& InDocument, TArray<PActor*>* OutCreatedActors)
	{
		EJsonArchiveReader Reader(InDocument);
		UINT32 FormatVersion = 0u;
		Reader.SerializeUInt32(PE_TEXT("FormatVersion"), FormatVersion);
		if (FormatVersion != LevelFormatVersion)
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level load: unsupported format version ")) + EString::FromInt(FormatVersion));
			return FALSE;
		}

		struct FPendingComponentWire
		{
			PActorComponent* Component = nullptr;
			BOOL32 bIsRoot = FALSE;
			UINT32 ParentComponentIndex = 0u;
		};
		struct FPendingActorAttach
		{
			PActor* Actor = nullptr;
			UINT32 ParentActorId = 0u;
		};

		TArray<PActor*> CreatedActors;
		TArray<FPendingActorAttach> ActorAttachments;
		// File ids survive skipped (unknown-class) entries, so a parent id still maps to
		// the right actor when an earlier entry failed to load.
		TMap<UINT32, PActor*> ActorByFileId;

		UINT32 ActorCount = 0u;
		Reader.BeginArray(PE_TEXT("Actors"), ActorCount);
		for (UINT32 ActorIndex = 0u; ActorIndex < ActorCount; ActorIndex++)
		{
			Reader.BeginObject(*MakeIndexKey(ActorIndex));

			EString ActorClassName;
			Reader.SerializeString(PE_TEXT("Class"), ActorClassName);
			UINT32 ActorClassVersion = 1u;
			Reader.SerializeUInt32(PE_TEXT("ClassVersion"), ActorClassVersion);
			UINT32 ParentActorId = 0u;
			Reader.SerializeUInt32(PE_TEXT("ParentActorID"), ParentActorId);

			PActor* Actor = (ActorClassName.Length() > 0u) ? CreateClassObjectByStableName<PActor>(*ActorClassName) : nullptr;
			if (!Actor && (ActorClassName.Length() > 0u))
			{
				PE_LOG_WARN(EString(PE_TEXT("Level load: unknown actor class ")) + ActorClassName);
			}

			Reader.BeginObject(PE_TEXT("Properties"));
			if (Actor)
			{
				const size_t ActorClassHash = GetClassHashCode(Actor);
				EClassPropertyRegistry::SerializeObject(Reader, ActorClassHash, Actor);
				if (ActorClassVersion < EClassPropertyRegistry::GetClassVersion(ActorClassHash))
				{
					if (const TFunction<void(void*, UINT32)>* Migrator = EClassPropertyRegistry::FindClassMigrator(ActorClassHash))
					{
						(*Migrator)(Actor, ActorClassVersion);
					}
				}
			}
			Reader.EndObject(); // actor Properties scope

			TArray<FPendingComponentWire> ComponentWires;
			UINT32 ComponentCount = 0u;
			Reader.BeginArray(PE_TEXT("Components"), ComponentCount);
			for (UINT32 ComponentIndex = 0u; ComponentIndex < ComponentCount; ComponentIndex++)
			{
				Reader.BeginObject(*MakeIndexKey(ComponentIndex));

				EString ComponentClassName;
				Reader.SerializeString(PE_TEXT("Class"), ComponentClassName);
				UINT32 ComponentClassVersion = 1u;
				Reader.SerializeUInt32(PE_TEXT("ClassVersion"), ComponentClassVersion);
				BOOL32 bIsRoot = FALSE;
				Reader.SerializeBool(PE_TEXT("Root"), bIsRoot);
				UINT32 ParentComponentIndex = 0u;
				Reader.SerializeUInt32(PE_TEXT("ParentComponentIndex"), ParentComponentIndex);

				PActorComponent* Component = nullptr;
				if (Actor && (ComponentClassName.Length() > 0u))
				{
					Component = CreateClassObjectByStableName<PActorComponent>(*ComponentClassName);
					if (!Component)
					{
						PE_LOG_WARN(EString(PE_TEXT("Level load: unknown component class ")) + ComponentClassName);
					}
				}

				Reader.BeginObject(PE_TEXT("Properties"));
				if (Component)
				{
					const size_t ComponentClassHash = GetClassHashCode(Component);
					EClassPropertyRegistry::SerializeObject(Reader, ComponentClassHash, Component);
					if (ComponentClassVersion < EClassPropertyRegistry::GetClassVersion(ComponentClassHash))
					{
						if (const TFunction<void(void*, UINT32)>* Migrator = EClassPropertyRegistry::FindClassMigrator(ComponentClassHash))
						{
							(*Migrator)(Component, ComponentClassVersion);
						}
					}
					FPendingComponentWire Wire;
					Wire.Component = Component;
					Wire.bIsRoot = bIsRoot;
					Wire.ParentComponentIndex = ParentComponentIndex;
					ComponentWires.Add(Wire);
				}
				Reader.EndObject(); // Properties scope
				Reader.EndObject(); // component entry
			}
			Reader.EndArray();

			if (Actor)
			{
				// Wire the component tree before the actor joins the world. The root is
				// installed first without the default root's transform carry-over, then the
				// others re-parent under their serialized parent; every local transform was
				// already loaded, so KEEP_CURRENT keeps exactly what the file stored.
				PActorComponent* RootComponent = nullptr;
				for (UINT32 WireIndex = 0u, WireNum = ComponentWires.Num<UINT32>(); WireIndex < WireNum; WireIndex++)
				{
					if (ComponentWires[WireIndex].bIsRoot)
					{
						RootComponent = ComponentWires[WireIndex].Component;
						break;
					}
				}
				if (PSceneComponent* RootSceneComponent = dynamic_cast<PSceneComponent*>(RootComponent))
				{
					Actor->ReplaceRootComponentForLoad(RootSceneComponent);
				}
				else if (RootComponent)
				{
					PE_LOG_WARN(PE_TEXT("Level load: serialized root component is not a scene component."));
				}
				for (UINT32 WireIndex = 0u, WireNum = ComponentWires.Num<UINT32>(); WireIndex < WireNum; WireIndex++)
				{
					FPendingComponentWire& Wire = ComponentWires[WireIndex];
					if (Wire.Component == RootComponent)
					{
						continue;
					}
					Actor->AddComponent(Wire.Component);
					PSceneComponent* SceneComponent = dynamic_cast<PSceneComponent*>(Wire.Component);
					if (SceneComponent && (Wire.ParentComponentIndex > 0u) && (Wire.ParentComponentIndex <= WireNum))
					{
						PSceneComponent* ParentComponent = dynamic_cast<PSceneComponent*>(ComponentWires[Wire.ParentComponentIndex - 1u].Component);
						if (ParentComponent)
						{
							SceneComponent->AttachToComponent(ParentComponent);
						}
					}
				}

				InWorld.AddActor(Actor, EAttachmentRuleType::KEEP_CURRENT, ETransform());
				CreatedActors.Add(Actor);
				ActorByFileId.Add(ActorIndex + 1u, Actor);

				FPendingActorAttach Attachment;
				Attachment.Actor = Actor;
				Attachment.ParentActorId = ParentActorId;
				ActorAttachments.Add(Attachment);
			}
			Reader.EndObject(); // actor entry
		}
		Reader.EndArray();

		// Actor hierarchy last: ids are 1-based into the file's actor list, not into the
		// created list, so skipped entries do not shift the mapping.
		for (UINT32 AttachIndex = 0u, AttachNum = ActorAttachments.Num<UINT32>(); AttachIndex < AttachNum; AttachIndex++)
		{
			const FPendingActorAttach& Attachment = ActorAttachments[AttachIndex];
			if (Attachment.ParentActorId == 0u)
			{
				continue;
			}
			if (PActor** FoundParent = ActorByFileId.FindValueAsPtr(Attachment.ParentActorId))
			{
				if (*FoundParent)
				{
					(*FoundParent)->AttachActorTo(Attachment.Actor);
				}
			}
		}

		if (OutCreatedActors)
		{
			for (UINT32 Index = 0u, Num = CreatedActors.Num<UINT32>(); Index < Num; Index++)
			{
				OutCreatedActors->Add(CreatedActors[Index]);
			}
		}
		return TRUE;
	}

	BOOL32 ELevelSerializer::LoadActorsFromFile(PWorld& InWorld, const EString& InPath, TArray<PActor*>* OutCreatedActors)
	{
		EString DocumentText;
		if (!EFileHelper::ReadFileAsString(InPath, DocumentText))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level load: cannot read ")) + InPath);
			return FALSE;
		}
		CJsonObject Document(DocumentText);
		if (!Document.IsValid())
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level load: invalid JSON in ")) + InPath);
			return FALSE;
		}
		return LoadActors(InWorld, Document, OutCreatedActors);
	}

}
