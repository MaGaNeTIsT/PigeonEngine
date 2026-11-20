#include <Editor/EditorWorldContext.h>

#if ENGINE_EDITOR_ONLY

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/ActorComponent.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/World/World.h>
#include <Editor/WorldEditorCommand.h>

namespace PigeonEngine
{
	namespace
	{
		struct FEditorWorldContext
		{
			FObjectHandle SelectedActorHandle;
			FObjectHandle SelectedComponentHandle;
			FWorldEditorCommandStack* CommandStack = nullptr;
		};

		TMap<PWorld*, FEditorWorldContext>& GetContextMap()
		{
			static TMap<PWorld*, FEditorWorldContext> Map;
			return Map;
		}

		FEditorWorldContext* FindContext(PWorld* InWorld)
		{
			return (InWorld != nullptr) ? GetContextMap().FindValueAsPtr(InWorld) : nullptr;
		}

		FEditorWorldContext& GetOrCreateContext(PWorld* InWorld)
		{
			FEditorWorldContext* Existing = FindContext(InWorld);
			if (!Existing)
			{
				GetContextMap().Add(InWorld, FEditorWorldContext());
				Existing = FindContext(InWorld);
			}
			return *Existing;
		}

		void RemoveContext(PWorld* InWorld)
		{
			if (GetContextMap().FindValueAsPtr(InWorld))
			{
				GetContextMap().Remove(InWorld);
			}
		}

		// Erases the context of a world that is being deleted, so disposable worlds do not
		// grow the map over an editor session. Installed before main, like the controller
		// factory registration.
		const BOOL32 bWorldContextLifecycleInstalled = []() -> BOOL32
		{
			PObject::OnObjectPreDelete.Add(TFunction<void(PObject*)>([](PObject* InObject) -> void
			{
				if (PWorld* World = dynamic_cast<PWorld*>(InObject))
				{
					RemoveContext(World);
				}
			}));
			return TRUE;
		}();
	}

	namespace EEditorWorldContext
	{
		PActor* GetSelectedActor(PWorld* InWorld)
		{
			const FEditorWorldContext* Context = FindContext(InWorld);
			return ((Context) && (Context->SelectedActorHandle.IsValid())) ? InWorld->ResolveActor(Context->SelectedActorHandle) : nullptr;
		}

		PActorComponent* GetSelectedComponent(PWorld* InWorld)
		{
			const FEditorWorldContext* Context = FindContext(InWorld);
			return ((Context) && (Context->SelectedComponentHandle.IsValid())) ? InWorld->ResolveComponent(Context->SelectedComponentHandle) : nullptr;
		}

		void SetSelectedActor(PWorld* InWorld, PActor* InActor)
		{
			FEditorWorldContext& Context = GetOrCreateContext(InWorld);
			Context.SelectedActorHandle = MakeObjectHandle(InActor);
			if (!InActor)
			{
				SetSelectedComponent(InWorld, nullptr);
				return;
			}

			// If the root has exactly one child component, auto-select it so properties like
			// materials are immediately visible without an extra click.
			PSceneComponent* Root = InActor->GetRootComponent();
			PActorComponent* DefaultComp = Root;
			if (Root)
			{
				TArray<PSceneComponent*> Children = Root->GetChildrenComponents();
				if (Children.Num() == 1)
				{
					DefaultComp = Children[0];
				}
			}
			SetSelectedComponent(InWorld, DefaultComp);
		}

		void SetSelectedComponent(PWorld* InWorld, PActorComponent* InComponent)
		{
			FEditorWorldContext& Context = GetOrCreateContext(InWorld);
			Context.SelectedComponentHandle = MakeObjectHandle(InComponent);
		}

		void ClearSelection(PWorld* InWorld)
		{
			if (FEditorWorldContext* Context = FindContext(InWorld))
			{
				Context->SelectedActorHandle = FObjectHandle();
				Context->SelectedComponentHandle = FObjectHandle();
			}
		}

		void SetEditorCommandStack(PWorld* InWorld, FWorldEditorCommandStack* InCommands)
		{
			GetOrCreateContext(InWorld).CommandStack = InCommands;
		}

		FWorldEditorCommandStack* GetEditorCommandStack(PWorld* InWorld)
		{
			const FEditorWorldContext* Context = FindContext(InWorld);
			return Context ? Context->CommandStack : nullptr;
		}
	}
}

#endif
