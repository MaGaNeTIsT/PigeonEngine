#include "WorldEditorCommand.h"

#if ENGINE_EDITOR_ONLY

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Controller/Controller.h>
#include <PigeonBase/Object/World/World.h>
#include <Reflection/LevelSerializer.h>
#include <Reflection/Property.h>
#include <RapidJson/Headers/JsonObject.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{
	namespace
	{
		void CollectActorSubtree(PActor* InActor, TArray<PActor*>& OutActors)
		{
			if (!InActor)
			{
				return;
			}
			OutActors.Add(InActor);
			const TArray<PActor*> Children = InActor->GetAllActorsAttached(FALSE);
			for (UINT32 Index = 0u, Num = Children.Num<UINT32>(); Index < Num; Index++)
			{
				CollectActorSubtree(Children[Index], OutActors);
			}
		}

		BOOL32 SaveActorSubtree(PActor* InActor, CJsonObject& OutDocument)
		{
			TArray<PActor*> Actors;
			CollectActorSubtree(InActor, Actors);
			return ELevelSerializer::SaveActors(Actors, OutDocument);
		}

		// Loads a subtree document and finds its root (the created actor whose parent is not
		// another created actor). The root is attached to InParent when that handle resolves.
		PActor* LoadActorSubtree(FWorldEditorCommandContext& InContext, const CJsonObject& InDocument, const FObjectHandle& InParentHandle)
		{
			if (!InContext.World)
			{
				return nullptr;
			}
			TArray<PActor*> CreatedActors;
			if (!ELevelSerializer::LoadActors(*InContext.World, InDocument, &CreatedActors))
			{
				return nullptr;
			}
			PActor* Root = nullptr;
			for (UINT32 Index = 0u, Num = CreatedActors.Num<UINT32>(); Index < Num; Index++)
			{
				PActor* Actor = CreatedActors[Index];
				if (!Actor)
				{
					continue;
				}
				BOOL32 bHasCreatedParent = FALSE;
				for (UINT32 OtherIndex = 0u; OtherIndex < Num; OtherIndex++)
				{
					if (CreatedActors[OtherIndex] == Actor->GetAttachedParentActor())
					{
						bHasCreatedParent = TRUE;
						break;
					}
				}
				if (!bHasCreatedParent)
				{
					Root = Actor;
					break;
				}
			}
			if (Root)
			{
				PActor* Parent = InContext.World->ResolveActor(InContext.Resolve(InParentHandle));
				if (Parent)
				{
					Parent->AttachActorTo(Root);
				}
			}
			return Root;
		}

		class FSpawnActorCommand final : public IWorldEditorCommand
		{
		public:
			FSpawnActorCommand(const CJsonObject& InDocument, const FObjectHandle& InParentHandle, const FObjectHandle& InOriginalHandle, const EString& InLabel)
				: Document(InDocument), ParentHandle(InParentHandle), OriginalHandle(InOriginalHandle), CurrentHandle(InOriginalHandle), Label(InLabel)
			{
			}

			virtual BOOL32 Undo(FWorldEditorCommandContext& InContext) override
			{
				PActor* Actor = InContext.World ? InContext.World->ResolveActor(InContext.Resolve(CurrentHandle)) : nullptr;
				if (Actor)
				{
					InContext.World->DestroyActor(Actor);
					InContext.World->FlushPendingDestroy();
				}
				CurrentHandle = FObjectHandle();
				return TRUE;
			}
			virtual BOOL32 Redo(FWorldEditorCommandContext& InContext) override
			{
				PActor* Root = LoadActorSubtree(InContext, Document, ParentHandle);
				if (!Root)
				{
					return FALSE;
				}
				CurrentHandle = MakeObjectHandle(Root);
				InContext.AddRemap(OriginalHandle, CurrentHandle);
				InContext.World->SetSelectedActor(Root);
				return TRUE;
			}
			virtual EString GetLabel() const override { return Label; }

		private:
			CJsonObject Document;
			FObjectHandle ParentHandle;
			FObjectHandle OriginalHandle;
			FObjectHandle CurrentHandle;
			EString Label;
		};

		class FDeleteActorCommand final : public IWorldEditorCommand
		{
		public:
			FDeleteActorCommand(const CJsonObject& InDocument, const FObjectHandle& InParentHandle, const FObjectHandle& InOriginalHandle, const EString& InLabel)
				: Document(InDocument), ParentHandle(InParentHandle), OriginalHandle(InOriginalHandle), Label(InLabel)
			{
			}

			virtual BOOL32 Undo(FWorldEditorCommandContext& InContext) override
			{
				PActor* Root = LoadActorSubtree(InContext, Document, ParentHandle);
				if (!Root)
				{
					return FALSE;
				}
				CurrentHandle = MakeObjectHandle(Root);
				InContext.AddRemap(OriginalHandle, CurrentHandle);
				InContext.World->SetSelectedActor(Root);
				return TRUE;
			}
			virtual BOOL32 Redo(FWorldEditorCommandContext& InContext) override
			{
				PActor* Actor = InContext.World ? InContext.World->ResolveActor(InContext.Resolve(CurrentHandle)) : nullptr;
				if (Actor)
				{
					InContext.World->DestroyActor(Actor);
					InContext.World->FlushPendingDestroy();
				}
				CurrentHandle = FObjectHandle();
				return TRUE;
			}
			virtual EString GetLabel() const override { return Label; }

		private:
			CJsonObject Document;
			FObjectHandle ParentHandle;
			FObjectHandle OriginalHandle;
			FObjectHandle CurrentHandle;
			EString Label;
		};

		class FAddComponentCommand final : public IWorldEditorCommand
		{
		public:
			FAddComponentCommand(const CJsonObject& InDocument, const FObjectHandle& InActorHandle, const FObjectHandle& InParentComponentHandle, const FObjectHandle& InOriginalHandle, const EString& InLabel)
				: Document(InDocument), ActorHandle(InActorHandle), ParentComponentHandle(InParentComponentHandle), OriginalHandle(InOriginalHandle), CurrentHandle(InOriginalHandle), Label(InLabel)
			{
			}

			virtual BOOL32 Undo(FWorldEditorCommandContext& InContext) override
			{
				PActorComponent* Component = InContext.World ? InContext.World->ResolveComponent(InContext.Resolve(CurrentHandle)) : nullptr;
				if (Component)
				{
					InContext.World->DestroyComponent(Component);
					InContext.World->FlushPendingDestroy();
				}
				CurrentHandle = FObjectHandle();
				return TRUE;
			}
			virtual BOOL32 Redo(FWorldEditorCommandContext& InContext) override
			{
				if (!InContext.World)
				{
					return FALSE;
				}
				PActor* Actor = InContext.World->ResolveActor(InContext.Resolve(ActorHandle));
				if (!Actor)
				{
					return FALSE;
				}
				PSceneComponent* Parent = InContext.World->ResolveComponent<PSceneComponent>(InContext.Resolve(ParentComponentHandle));
				PActorComponent* Component = nullptr;
				if (!ELevelSerializer::LoadComponentIntoActor(*Actor, Document, Parent, Component) || (!Component))
				{
					return FALSE;
				}
				CurrentHandle = MakeObjectHandle(Component);
				InContext.AddRemap(OriginalHandle, CurrentHandle);
				InContext.World->SetSelectedComponent(Component);
				return TRUE;
			}
			virtual EString GetLabel() const override { return Label; }

		private:
			CJsonObject Document;
			FObjectHandle ActorHandle;
			FObjectHandle ParentComponentHandle;
			FObjectHandle OriginalHandle;
			FObjectHandle CurrentHandle;
			EString Label;
		};

		class FDeleteComponentCommand final : public IWorldEditorCommand
		{
		public:
			FDeleteComponentCommand(const CJsonObject& InDocument, const FObjectHandle& InActorHandle, const FObjectHandle& InParentComponentHandle, const FObjectHandle& InOriginalHandle, const EString& InLabel)
				: Document(InDocument), ActorHandle(InActorHandle), ParentComponentHandle(InParentComponentHandle), OriginalHandle(InOriginalHandle), Label(InLabel)
			{
			}

			virtual BOOL32 Undo(FWorldEditorCommandContext& InContext) override
			{
				if (!InContext.World)
				{
					return FALSE;
				}
				PActor* Actor = InContext.World->ResolveActor(InContext.Resolve(ActorHandle));
				if (!Actor)
				{
					return FALSE;
				}
				PSceneComponent* Parent = InContext.World->ResolveComponent<PSceneComponent>(InContext.Resolve(ParentComponentHandle));
				PActorComponent* Component = nullptr;
				if (!ELevelSerializer::LoadComponentIntoActor(*Actor, Document, Parent, Component) || (!Component))
				{
					return FALSE;
				}
				CurrentHandle = MakeObjectHandle(Component);
				InContext.AddRemap(OriginalHandle, CurrentHandle);
				InContext.World->SetSelectedComponent(Component);
				return TRUE;
			}
			virtual BOOL32 Redo(FWorldEditorCommandContext& InContext) override
			{
				PActorComponent* Component = InContext.World ? InContext.World->ResolveComponent(InContext.Resolve(CurrentHandle)) : nullptr;
				if (Component)
				{
					InContext.World->DestroyComponent(Component);
					InContext.World->FlushPendingDestroy();
				}
				CurrentHandle = FObjectHandle();
				return TRUE;
			}
			virtual EString GetLabel() const override { return Label; }

		private:
			CJsonObject Document;
			FObjectHandle ActorHandle;
			FObjectHandle ParentComponentHandle;
			FObjectHandle OriginalHandle;
			FObjectHandle CurrentHandle;
			EString Label;
		};

		class FSetPropertyCommand final : public IWorldEditorCommand
		{
		public:
			FSetPropertyCommand(const FObjectHandle& InObjectHandle, const BOOL32& bInIsComponent, const size_t& InClassHash,
				const EString& InPropertyName, const FPropertyValue& InBefore, const FPropertyValue& InAfter, const EString& InLabel)
				: ObjectHandle(InObjectHandle), bIsComponent(bInIsComponent), ClassHash(InClassHash)
				, PropertyName(InPropertyName), Before(InBefore), After(InAfter), Label(InLabel)
			{
			}

			virtual BOOL32 Undo(FWorldEditorCommandContext& InContext) override
			{
				return Apply(InContext, Before);
			}
			virtual BOOL32 Redo(FWorldEditorCommandContext& InContext) override
			{
				return Apply(InContext, After);
			}
			virtual EString GetLabel() const override { return Label; }

		private:
			BOOL32 Apply(FWorldEditorCommandContext& InContext, const FPropertyValue& InValue)
			{
				if (!InContext.World)
				{
					return FALSE;
				}
				const FObjectHandle Resolved = InContext.Resolve(ObjectHandle);
				void* Object = bIsComponent
					? static_cast<void*>(InContext.World->ResolveComponent(Resolved))
					: static_cast<void*>(InContext.World->ResolveActor(Resolved));
				if (!Object)
				{
					return FALSE;
				}
				return EClassPropertyRegistry::SetPropertyValue(ClassHash, Object, PropertyName, InValue);
			}

			FObjectHandle ObjectHandle;
			BOOL32 bIsComponent = FALSE;
			size_t ClassHash = static_cast<size_t>(-1);
			EString PropertyName;
			FPropertyValue Before;
			FPropertyValue After;
			EString Label;
		};

		void DeleteCommands(TArray<IWorldEditorCommand*>& InCommands)
		{
			for (UINT32 Index = 0u, Num = InCommands.Num<UINT32>(); Index < Num; Index++)
			{
				delete InCommands[Index];
			}
			InCommands.Empty();
		}
	}

	FObjectHandle FWorldEditorCommandContext::Resolve(const FObjectHandle& InHandle) const
	{
		FObjectHandle Result = InHandle;
		for (UINT32 Depth = 0u; (Depth < 32u) && Result.IsValid(); Depth++)
		{
			ObjectIdentityType MappedId = 0u;
			if (!HandleRemap.FindValue(Result.UniqueID, MappedId))
			{
				break;
			}
			Result.UniqueID = MappedId;
		}
		return Result;
	}

	void FWorldEditorCommandContext::AddRemap(const FObjectHandle& InOldHandle, const FObjectHandle& InNewHandle)
	{
		if (InOldHandle.IsValid() && InNewHandle.IsValid())
		{
			HandleRemap.Add(InOldHandle.UniqueID, InNewHandle.UniqueID);
		}
	}

	void FWorldEditorCommandContext::ClearRemap()
	{
		HandleRemap.Empty();
	}

	FWorldEditorCommandStack::~FWorldEditorCommandStack()
	{
		Clear();
	}

	void FWorldEditorCommandStack::Clear()
	{
		DeleteCommands(UndoStack);
		DeleteCommands(RedoStack);
		Context.ClearRemap();
	}

	void FWorldEditorCommandStack::PushExecuted(IWorldEditorCommand* InCommand)
	{
		if (!InCommand)
		{
			return;
		}
		DeleteCommands(RedoStack);
		UndoStack.Add(InCommand);
		HistoryRevision += 1u;
		if (UndoStack.Num<UINT32>() > MaxHistory)
		{
			delete UndoStack[0];
			UndoStack.RemoveAt(0u);
		}
	}

	BOOL32 FWorldEditorCommandStack::CanUndo() const
	{
		return (UndoStack.Num<UINT32>() > 0u) ? TRUE : FALSE;
	}

	BOOL32 FWorldEditorCommandStack::CanRedo() const
	{
		return (RedoStack.Num<UINT32>() > 0u) ? TRUE : FALSE;
	}

	EString FWorldEditorCommandStack::GetUndoLabel() const
	{
		return CanUndo() ? UndoStack[UndoStack.Num<UINT32>() - 1u]->GetLabel() : EString();
	}

	EString FWorldEditorCommandStack::GetRedoLabel() const
	{
		return CanRedo() ? RedoStack[RedoStack.Num<UINT32>() - 1u]->GetLabel() : EString();
	}

	BOOL32 FWorldEditorCommandStack::Undo()
	{
		if (!CanUndo())
		{
			return FALSE;
		}
		IWorldEditorCommand* Command = UndoStack[UndoStack.Num<UINT32>() - 1u];
		UndoStack.RemoveAt(UndoStack.Num<UINT32>() - 1u);
		if (!Command->Undo(Context))
		{
			delete Command;
			return FALSE;
		}
		RedoStack.Add(Command);
		HistoryRevision += 1u;
		return TRUE;
	}

	BOOL32 FWorldEditorCommandStack::Redo()
	{
		if (!CanRedo())
		{
			return FALSE;
		}
		IWorldEditorCommand* Command = RedoStack[RedoStack.Num<UINT32>() - 1u];
		RedoStack.RemoveAt(RedoStack.Num<UINT32>() - 1u);
		if (!Command->Redo(Context))
		{
			delete Command;
			return FALSE;
		}
		UndoStack.Add(Command);
		HistoryRevision += 1u;
		return TRUE;
	}

	FWorldEditorCommandContext& FWorldEditorCommandStack::GetContext()
	{
		return Context;
	}

	UINT64 FWorldEditorCommandStack::GetHistoryRevision() const
	{
		return HistoryRevision;
	}

	BOOL32 PushSpawnActorCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActor* InActor)
	{
		if (!InActor)
		{
			return FALSE;
		}
		CJsonObject Document;
		if (!SaveActorSubtree(InActor, Document))
		{
			return FALSE;
		}
		const FObjectHandle ParentHandle = MakeObjectHandle(InActor->GetAttachedParentActor());
		const FObjectHandle ActorHandle = MakeObjectHandle(InActor);
		InStack.PushExecuted(new FSpawnActorCommand(Document, ParentHandle, ActorHandle, EString(PE_TEXT("Create Actor"))));
		return TRUE;
	}

	BOOL32 PushDeleteActorCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActor* InActor)
	{
		if ((!InActor) || (!InActor->ShouldSerializeToLevel()) || (InActor == InWorld.GetController()))
		{
			return FALSE;
		}
		// The controller and characters cannot be recreated by the level format, so their
		// deletion is not undoable; the UI refuses it for the same reason.
		CJsonObject Document;
		if (!SaveActorSubtree(InActor, Document))
		{
			return FALSE;
		}
		const FObjectHandle ParentHandle = MakeObjectHandle(InActor->GetAttachedParentActor());
		const FObjectHandle ActorHandle = MakeObjectHandle(InActor);
		InStack.PushExecuted(new FDeleteActorCommand(Document, ParentHandle, ActorHandle, EString(PE_TEXT("Delete Actor"))));
		InWorld.DestroyActor(InActor);
		InWorld.FlushPendingDestroy();
		return TRUE;
	}

	BOOL32 PushAddComponentCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActor* InActor, PActorComponent* InComponent)
	{
		if ((!InActor) || (!InComponent))
		{
			return FALSE;
		}
		CJsonObject Document;
		if (!ELevelSerializer::SaveComponent(InComponent, InActor, Document))
		{
			return FALSE;
		}
		FObjectHandle ParentComponentHandle;
		if (PSceneComponent* SceneComponent = dynamic_cast<PSceneComponent*>(InComponent))
		{
			ParentComponentHandle = MakeObjectHandle(SceneComponent->GetParentComponent());
		}
		InStack.PushExecuted(new FAddComponentCommand(Document, MakeObjectHandle(InActor), ParentComponentHandle,
			MakeObjectHandle(InComponent), EString(PE_TEXT("Add Component"))));
		return TRUE;
	}

	BOOL32 PushDeleteComponentCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActorComponent* InComponent)
	{
		if (!InComponent)
		{
			return FALSE;
		}
		PActor* Owner = InComponent->GetOwnerActor();
		if ((!Owner) || (Owner->GetRootComponent() == InComponent))
		{
			return FALSE;
		}
		CJsonObject Document;
		if (!ELevelSerializer::SaveComponent(InComponent, Owner, Document))
		{
			return FALSE;
		}
		FObjectHandle ParentComponentHandle;
		if (PSceneComponent* SceneComponent = dynamic_cast<PSceneComponent*>(InComponent))
		{
			ParentComponentHandle = MakeObjectHandle(SceneComponent->GetParentComponent());
		}
		InStack.PushExecuted(new FDeleteComponentCommand(Document, MakeObjectHandle(Owner), ParentComponentHandle,
			MakeObjectHandle(InComponent), EString(PE_TEXT("Remove Component"))));
		InWorld.DestroyComponent(InComponent);
		InWorld.FlushPendingDestroy();
		return TRUE;
	}

	BOOL32 PushSetPropertyCommand(FWorldEditorCommandStack& InStack, const FObjectHandle& InObjectHandle, const BOOL32& bInIsComponent,
		const size_t& InClassHash, const EString& InPropertyName, const FPropertyValue& InBefore, const FPropertyValue& InAfter)
	{
		if (!InObjectHandle.IsValid())
		{
			return FALSE;
		}
		InStack.PushExecuted(new FSetPropertyCommand(InObjectHandle, bInIsComponent, InClassHash, InPropertyName,
			InBefore, InAfter, EString(PE_TEXT("Set ")) + InPropertyName));
		return TRUE;
	}
}

#endif
