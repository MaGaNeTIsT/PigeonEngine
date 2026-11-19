#include "LevelSerializer.h"

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/World/World.h>
#include <RapidJson/Headers/JsonObject.h>
#include <Editor/EditorLogManager.h>
#include <IO/FileHelper.h>
#include "JsonArchive.h"
#include "ObjectReference.h"
#include "Property.h"

namespace PigeonEngine
{
	namespace
	{
		EString MakeIndexKey(const UINT32 InIndex)
		{
			return EString::FromInt(InIndex);
		}

		struct FLevelSaveReferenceHandler : public IObjectReferenceHandler
		{
			const TMap<ObjectIdentityType, EString>* References = nullptr;

			virtual BOOL32 WriteReference(EArchive& InArchive, const CHAR* InName, const FObjectHandle& InHandle) override
			{
				EString Reference;
				if (InHandle.IsValid() && References)
				{
					References->FindValue(InHandle.UniqueID, Reference);
				}
				InArchive.SerializeString(InName, Reference);
				return TRUE;
			}
			virtual BOOL32 ReadReference(EArchive& InArchive, const CHAR* InName, const TFunction<void(const FObjectHandle&)>& InApply) override
			{
				(void)InArchive;
				(void)InName;
				(void)InApply;
				return FALSE;
			}
		};

		struct FPendingReference
		{
			UINT32 ActorFileId = 0u;
			UINT32 ComponentIndex = 0u;
			TFunction<void(const FObjectHandle&)> Apply;
		};

		struct FLevelLoadReferenceHandler : public IObjectReferenceHandler
		{
			TArray<FPendingReference> Pending;

			virtual BOOL32 WriteReference(EArchive& InArchive, const CHAR* InName, const FObjectHandle& InHandle) override
			{
				(void)InArchive;
				(void)InName;
				(void)InHandle;
				return FALSE;
			}
			virtual BOOL32 ReadReference(EArchive& InArchive, const CHAR* InName, const TFunction<void(const FObjectHandle&)>& InApply) override
			{
				EString Reference;
				InArchive.SerializeString(InName, Reference);

				FPendingReference PendingReference;
				PendingReference.Apply = InApply;
				if (Reference.Length() > 0u)
				{
					ParseObjectReferenceString(Reference, PendingReference.ActorFileId, PendingReference.ComponentIndex);
				}
				Pending.Add(PendingReference);
				return TRUE;
			}
		};

		void BuildReferenceMap(const TArray<PActor*>& InActors, TMap<ObjectIdentityType, EString>& OutReferences)
		{
			for (UINT32 ActorIndex = 0u, ActorNum = InActors.Num<UINT32>(); ActorIndex < ActorNum; ActorIndex++)
			{
				PActor* Actor = InActors[ActorIndex];
				if (!Actor)
				{
					continue;
				}
				const UINT32 ActorFileId = ActorIndex + 1u;
				OutReferences.Add(Actor->GetUniqueID(), MakeObjectReferenceString(ActorFileId, 0u));

				const TArray<PActorComponent*> Components = Actor->GetAllComponents();
				for (UINT32 ComponentIndex = 0u, ComponentNum = Components.Num<UINT32>(); ComponentIndex < ComponentNum; ComponentIndex++)
				{
					if (Components[ComponentIndex])
					{
						OutReferences.Add(Components[ComponentIndex]->GetUniqueID(), MakeObjectReferenceString(ActorFileId, ComponentIndex + 1u));
					}
				}
			}
		}

		void WriteLevelHeader(EJsonArchiveWriter& Writer, const FLevelHeader& InHeader)
		{
			Writer.BeginObject(PE_TEXT("Level"));
			EString LevelName = InHeader.LevelName;
			Writer.SerializeString(PE_TEXT("Name"), LevelName);
			Vector3 GravityDirection = InHeader.GravityDirection;
			SerializeVector3(Writer, PE_TEXT("GravityDirection"), GravityDirection);
			FLOAT GravityStrength = InHeader.GravityStrength;
			Writer.SerializeFloat(PE_TEXT("GravityStrength"), GravityStrength);
			BOOL32 bUseUpVector = InHeader.bUseUpVector;
			Writer.SerializeBool(PE_TEXT("UseUpVector"), bUseUpVector);
			Vector3 UpVector = InHeader.UpVector;
			SerializeVector3(Writer, PE_TEXT("UpVector"), UpVector);
			Writer.EndObject();
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

#if ENGINE_EDITOR_ONLY
			EString ActorName = InActor.GetDebugName();
			Writer.SerializeString(PE_TEXT("Name"), ActorName);
#endif

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

#if ENGINE_EDITOR_ONLY
				EString ComponentName = Component->GetDebugName();
				Writer.SerializeString(PE_TEXT("Name"), ComponentName);
#endif

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

		void WriteActorArray(EJsonArchiveWriter& Writer, const TArray<PActor*>& InActors)
		{
			TMap<const PActor*, UINT32> ActorFileIds;
			for (UINT32 Index = 0u, Num = InActors.Num<UINT32>(); Index < Num; Index++)
			{
				if (InActors[Index])
				{
					ActorFileIds.Add(InActors[Index], Index + 1u);
				}
			}

			TMap<ObjectIdentityType, EString> References;
			BuildReferenceMap(InActors, References);
			FLevelSaveReferenceHandler ReferenceHandler;
			ReferenceHandler.References = &References;
			Writer.SetObjectReferenceHandler(&ReferenceHandler);

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

			Writer.SetObjectReferenceHandler(nullptr);
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

		BOOL32 LoadActorArray(EJsonArchiveReader& Reader, PWorld& InWorld, TArray<PActor*>* OutCreatedActors)
		{
			TArray<PActor*> CreatedActors;
			TArray<FPendingActorAttach> ActorAttachments;
			// File ids survive skipped (unknown-class) entries, so a parent id still maps to
			// the right actor when an earlier entry failed to load.
			TMap<UINT32, PActor*> ActorByFileId;
			// Component file indexes are resolved against the list in file order, not against
			// the loaded actor's component list (the default root shifts that one).
			TMap<UINT32, TArray<PActorComponent*>> ComponentsByActorFileId;

			FLevelLoadReferenceHandler ReferenceHandler;
			Reader.SetObjectReferenceHandler(&ReferenceHandler);

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
#if ENGINE_EDITOR_ONLY
				if (Actor)
				{
					EString ActorName;
					Reader.SerializeString(PE_TEXT("Name"), ActorName);
					if (ActorName.Length() > 0u)
					{
						Actor->SetDebugName(ActorName);
					}
				}
#endif

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
#if ENGINE_EDITOR_ONLY
					if (Component)
					{
						EString ComponentName;
						Reader.SerializeString(PE_TEXT("Name"), ComponentName);
						if (ComponentName.Length() > 0u)
						{
							Component->SetDebugName(ComponentName);
						}
					}
#endif

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

					TArray<PActorComponent*> FileOrderComponents;
					for (UINT32 WireIndex = 0u, WireNum = ComponentWires.Num<UINT32>(); WireIndex < WireNum; WireIndex++)
					{
						FileOrderComponents.Add(ComponentWires[WireIndex].Component);
					}
					ComponentsByActorFileId.Add(ActorIndex + 1u, FileOrderComponents);

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

			// Object references, now that every actor and component exists.
			for (UINT32 PendingIndex = 0u, PendingNum = ReferenceHandler.Pending.Num<UINT32>(); PendingIndex < PendingNum; PendingIndex++)
			{
				const FPendingReference& Pending = ReferenceHandler.Pending[PendingIndex];
				FObjectHandle Resolved;
				PActor** FoundActor = ActorByFileId.FindValueAsPtr(Pending.ActorFileId);
				if (FoundActor && (*FoundActor))
				{
					if (Pending.ComponentIndex == 0u)
					{
						Resolved = MakeObjectHandle(*FoundActor);
					}
					else
					{
						TArray<PActorComponent*>* FoundComponents = ComponentsByActorFileId.FindValueAsPtr(Pending.ActorFileId);
						if (FoundComponents && (Pending.ComponentIndex <= FoundComponents->Num<UINT32>()))
						{
							Resolved = MakeObjectHandle((*FoundComponents)[Pending.ComponentIndex - 1u]);
						}
					}
				}
				if (Pending.Apply)
				{
					Pending.Apply(Resolved);
				}
			}
			Reader.SetObjectReferenceHandler(nullptr);

			if (OutCreatedActors)
			{
				for (UINT32 Index = 0u, Num = CreatedActors.Num<UINT32>(); Index < Num; Index++)
				{
					OutCreatedActors->Add(CreatedActors[Index]);
				}
			}
			return TRUE;
		}
	}

	BOOL32 ELevelSerializer::SaveActors(const TArray<PActor*>& InActors, CJsonObject& OutDocument)
	{
		EJsonArchiveWriter Writer(OutDocument);
		UINT32 FormatVersion = LevelFormatVersion;
		Writer.SerializeUInt32(PE_TEXT("FormatVersion"), FormatVersion);
		WriteActorArray(Writer, InActors);
		return TRUE;
	}

	BOOL32 ELevelSerializer::SaveWorld(const PWorld& InWorld, const EString& InLevelName, CJsonObject& OutDocument)
	{
		FLevelHeader Header;
		Header.LevelName = InLevelName;
		Header.UpVector = InWorld.GetUpVector();
		const FWorldGravitySettings& GravitySettings = InWorld.GetGravitySettings();
		Header.GravityDirection = GravitySettings.GravityDirection;
		Header.GravityStrength = GravitySettings.GravityStrength;
		Header.bUseUpVector = GravitySettings.bUseUpVector;

		TArray<PActor*> Actors;
		const TArray<PActor*> AllActors = InWorld.GetAllActors();
		for (UINT32 Index = 0u, Num = AllActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (AllActors[Index] && AllActors[Index]->ShouldSerializeToLevel())
			{
				Actors.Add(AllActors[Index]);
			}
		}

		EJsonArchiveWriter Writer(OutDocument);
		UINT32 FormatVersion = LevelFormatVersion;
		Writer.SerializeUInt32(PE_TEXT("FormatVersion"), FormatVersion);
		WriteLevelHeader(Writer, Header);
		WriteActorArray(Writer, Actors);
		return TRUE;
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

	BOOL32 ELevelSerializer::SaveWorldToFile(const PWorld& InWorld, const EString& InLevelName, const EString& InPath)
	{
		CJsonObject Document;
		if (!SaveWorld(InWorld, InLevelName, Document))
		{
			return FALSE;
		}
		return EFileHelper::SaveStringToFile(InPath, Document.ToString(), TRUE, FALSE) ? TRUE : FALSE;
	}

	BOOL32 ELevelSerializer::ReadLevelHeader(const CJsonObject& InDocument, FLevelHeader& OutHeader)
	{
		EJsonArchiveReader Reader(InDocument);
		UINT32 FormatVersion = 0u;
		Reader.SerializeUInt32(PE_TEXT("FormatVersion"), FormatVersion);
		if (FormatVersion < 2u)
		{
			// v1 had no header; the defaults are the world's current settings and are left in
			// place by PWorld::LoadLevel.
			return FALSE;
		}
		Reader.BeginObject(PE_TEXT("Level"));
		Reader.SerializeString(PE_TEXT("Name"), OutHeader.LevelName);
		SerializeVector3(Reader, PE_TEXT("GravityDirection"), OutHeader.GravityDirection);
		Reader.SerializeFloat(PE_TEXT("GravityStrength"), OutHeader.GravityStrength);
		Reader.SerializeBool(PE_TEXT("UseUpVector"), OutHeader.bUseUpVector);
		SerializeVector3(Reader, PE_TEXT("UpVector"), OutHeader.UpVector);
		Reader.EndObject();
		return TRUE;
	}

	BOOL32 ELevelSerializer::LoadActors(PWorld& InWorld, const CJsonObject& InDocument, TArray<PActor*>* OutCreatedActors)
	{
		EJsonArchiveReader Reader(InDocument);
		UINT32 FormatVersion = 0u;
		Reader.SerializeUInt32(PE_TEXT("FormatVersion"), FormatVersion);
		if ((FormatVersion != 1u) && (FormatVersion != LevelFormatVersion))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level load: unsupported format version ")) + EString::FromInt(FormatVersion));
			return FALSE;
		}
		return LoadActorArray(Reader, InWorld, OutCreatedActors);
	}

	BOOL32 ELevelSerializer::LoadWorld(PWorld& InWorld, const CJsonObject& InDocument, FLevelHeader* OutHeader, TArray<PActor*>* OutCreatedActors)
	{
		EJsonArchiveReader Reader(InDocument);
		UINT32 FormatVersion = 0u;
		Reader.SerializeUInt32(PE_TEXT("FormatVersion"), FormatVersion);
		if ((FormatVersion != 1u) && (FormatVersion != LevelFormatVersion))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Level load: unsupported format version ")) + EString::FromInt(FormatVersion));
			return FALSE;
		}

		FLevelHeader Header;
		if (FormatVersion >= 2u)
		{
			ReadLevelHeader(InDocument, Header);
		}
		if (!LoadActorArray(Reader, InWorld, OutCreatedActors))
		{
			return FALSE;
		}
		if (OutHeader)
		{
			*OutHeader = Header;
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

	BOOL32 ELevelSerializer::LoadWorldFromFile(PWorld& InWorld, const EString& InPath, FLevelHeader* OutHeader, TArray<PActor*>* OutCreatedActors)
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
		return LoadWorld(InWorld, Document, OutHeader, OutCreatedActors);
	}

	BOOL32 ELevelSerializer::SaveComponent(const PActorComponent* InComponent, const PActor* InOwner, CJsonObject& OutDocument)
	{
		if ((!InComponent) || (!InOwner))
		{
			return FALSE;
		}
		EJsonArchiveWriter Writer(OutDocument);
		Writer.BeginObject(PE_TEXT("Component"));

		const size_t ComponentClassHash = GetClassHashCode(InComponent);
		EString ComponentClassName(GetClassStableName(ComponentClassHash));
		Writer.SerializeString(PE_TEXT("Class"), ComponentClassName);
		UINT32 ComponentClassVersion = EClassPropertyRegistry::GetClassVersion(ComponentClassHash);
		Writer.SerializeUInt32(PE_TEXT("ClassVersion"), ComponentClassVersion);
#if ENGINE_EDITOR_ONLY
		EString ComponentName = InComponent->GetDebugName();
		Writer.SerializeString(PE_TEXT("Name"), ComponentName);
#endif
		BOOL32 bIsRoot = (InComponent == InOwner->GetRootComponent()) ? TRUE : FALSE;
		Writer.SerializeBool(PE_TEXT("Root"), bIsRoot);
		UINT32 ParentComponentIndex = 0u;
		Writer.SerializeUInt32(PE_TEXT("ParentComponentIndex"), ParentComponentIndex);
		Writer.BeginObject(PE_TEXT("Properties"));
		EClassPropertyRegistry::SerializeObject(Writer, ComponentClassHash, const_cast<PActorComponent*>(InComponent));
		Writer.EndObject();

		Writer.EndObject();
		return TRUE;
	}

	BOOL32 ELevelSerializer::LoadComponentIntoActor(PActor& InActor, const CJsonObject& InDocument, PSceneComponent* InParent, PActorComponent*& OutComponent)
	{
		OutComponent = nullptr;
		EJsonArchiveReader Reader(InDocument);
		Reader.BeginObject(PE_TEXT("Component"));

		EString ComponentClassName;
		Reader.SerializeString(PE_TEXT("Class"), ComponentClassName);
		UINT32 ComponentClassVersion = 1u;
		Reader.SerializeUInt32(PE_TEXT("ClassVersion"), ComponentClassVersion);
		BOOL32 bIsRoot = FALSE;
		Reader.SerializeBool(PE_TEXT("Root"), bIsRoot);
		UINT32 ParentComponentIndex = 0u;
		Reader.SerializeUInt32(PE_TEXT("ParentComponentIndex"), ParentComponentIndex);

		PActorComponent* Component = (ComponentClassName.Length() > 0u)
			? CreateClassObjectByStableName<PActorComponent>(*ComponentClassName)
			: nullptr;
		if (!Component)
		{
			Reader.EndObject();
			return FALSE;
		}
#if ENGINE_EDITOR_ONLY
		EString ComponentName;
		Reader.SerializeString(PE_TEXT("Name"), ComponentName);
		if (ComponentName.Length() > 0u)
		{
			Component->SetDebugName(ComponentName);
		}
#endif
		Reader.BeginObject(PE_TEXT("Properties"));
		const size_t ComponentClassHash = GetClassHashCode(Component);
		EClassPropertyRegistry::SerializeObject(Reader, ComponentClassHash, Component);
		if (ComponentClassVersion < EClassPropertyRegistry::GetClassVersion(ComponentClassHash))
		{
			if (const TFunction<void(void*, UINT32)>* Migrator = EClassPropertyRegistry::FindClassMigrator(ComponentClassHash))
			{
				(*Migrator)(Component, ComponentClassVersion);
			}
		}
		Reader.EndObject();
		Reader.EndObject();

		PSceneComponent* SceneComponent = dynamic_cast<PSceneComponent*>(Component);
		if (bIsRoot && SceneComponent)
		{
			InActor.ReplaceRootComponentForLoad(SceneComponent);
		}
		else
		{
			InActor.AddComponent(Component);
			if (SceneComponent && InParent)
			{
				SceneComponent->AttachToComponent(InParent);
			}
		}
		OutComponent = Component;
		return TRUE;
	}

}
