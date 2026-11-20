#include <PigeonBase/Object/Actor.h>

#include "Component/SceneComponent.h"
#include "Component/ActorComponent.h"
#include "Component/Character/CharacterInputComponent.h"
#include "Component/Character/MovementComponent.h"
#include "World/World.h"
#include <Editor/EditorLogManager.h>


namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PActor, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

#if 1
	static PActor* CreatePActorClassObject()
	{
		return (new PActor());
	}
	static void RegisterClassFactories()
	{
		SaveClassFactoryFunction<PActor>(&CreatePActorClassObject);
	}
#else
	PE_REGISTER_CLASS_FACTORY_DEFAULT_DECLARE(PActor);

	static void RegisterClassFactories()
	{
		PE_REGISTER_CLASS_FACTORY_DEFAULT_IMPLEMENT(PActor);
	}
#endif

	PE_REGISTER_CLASS_FACTORY(&RegisterClassFactories);

	// Root debug names carry exactly one "(Root)" suffix. The suffix is editor metadata and
	// is written to level files, so installing a serialized root must normalize the name
	// instead of appending to a suffix that is already present.
	static EString StripRootDebugSuffix(const EString& InName)
	{
		const EString Suffix(PE_TEXT("(Root)"));
		EString Result = InName;
		while ((Result.Length() >= Suffix.Length()) && (Result.Right(Suffix.Length()) == Suffix))
		{
			Result = Result.Left(Result.Length() - Suffix.Length());
		}
		return Result;
	}

	static EString EnsureRootDebugName(const EString& InName)
	{
		return StripRootDebugSuffix(InName) + EString(PE_TEXT("(Root)"));
	}

	PActor::PActor()
	{
		if(!RootComponent)
		{
			SetRootComponent(new PSceneComponent());
		}
	}

	PActor::~PActor()
	{
		// Destroy() empties the component list before the memory goes away; a raw delete
		// bypasses that and leaks every component. Catch it here rather than leaking
		// silently.
		PE_CHECK(ENGINE_ACTOR_ERROR, "PActor was deleted without Destroy()", (RootComponent == nullptr) && (Components.Num() == 0));
	}

	void PActor::Init()
	{
		for (PActorComponent* Component : Components)
		{
			Component->Init();
		}
		for (PActor* Child : ChildrenActors)
		{
			Child->Init();
		}
		SetInitialized(TRUE);
	}

	void PActor::Uninit()
	{
		PObject::Uninit();
	}

	void PActor::Tick(FLOAT deltaTime)
	{
		// The scheduler owns component and child-actor entries. This entry is only the
		// actor's own user logic, and the actor flag gates it without affecting components.
		if (IsTickable())
		{
			UserTick(deltaTime);
		}
	}

	void PActor::SetIsTickable(const BOOL32& bTickable)
	{
		PObject::SetIsTickable(bTickable);
		if (bTickable)
		{
			RegisterSelfTickDomains();
		}
		else
		{
			UnregisterSelfTickDomains();
		}
	}

	void PActor::SetTickPriority(const INT32& InPriority)
	{
		PObject::SetTickPriority(InPriority);
		PWorld* World = GetWorld();
		if (!World)
		{
			return;
		}
		FWorldTickScheduler& Scheduler = World->GetTickScheduler();
		Scheduler.SetPriority(VariableTickHandle, InPriority);
#if ENGINE_EDITOR_ONLY
		Scheduler.SetPriority(EditorTickHandle, InPriority);
#endif
	}

	void PActor::RegisterSelfTickDomains()
	{
		PWorld* World = GetWorld();
		if ((!World) || (!IsTickable()))
		{
			return;
		}
		FWorldTickScheduler& Scheduler = World->GetTickScheduler();
		if (!VariableTickHandle.IsValid())
		{
			VariableTickHandle = Scheduler.RegisterActor(*this, ETickDomain::VARIABLE, GetTickPriority());
		}
#if ENGINE_EDITOR_ONLY
		if (!EditorTickHandle.IsValid())
		{
			EditorTickHandle = Scheduler.RegisterActor(*this, ETickDomain::EDITOR, GetTickPriority());
		}
#endif
	}

	void PActor::UnregisterSelfTickDomains()
	{
		PWorld* World = GetWorld();
		if (!World)
		{
			return;
		}
		FWorldTickScheduler& Scheduler = World->GetTickScheduler();
		Scheduler.Unregister(VariableTickHandle);
#if ENGINE_EDITOR_ONLY
		Scheduler.Unregister(EditorTickHandle);
#endif
	}

	void PActor::TickRenderTree()
	{
		if (!bRenderTreeDirty)
		{
			return;
		}
		// Clear before walking: a deferred apply or a render-state send during this pass may
		// dirty the actor again, and that work must be visible on the next frame.
		bRenderTreeDirty = FALSE;
		if (RootComponent)
		{
			RootComponent->TickRender();
		}
		for (UINT32 ChildIndex = 0u, NumChildren = ChildrenActors.Num<UINT32>(); ChildIndex < NumChildren; ChildIndex++)
		{
			PActor* Child = ChildrenActors[ChildIndex];
			if (Child)
			{
				Child->TickRenderTree();
			}
		}
	}

	void PActor::MarkRenderTreeDirty()
	{
		if (bRenderTreeDirty)
		{
			return;
		}
		bRenderTreeDirty = TRUE;
		if (AttachedParentActor)
		{
			AttachedParentActor->MarkRenderTreeDirty();
		}
	}

#if ENGINE_EDITOR_ONLY
	void PActor::EditorTick(FLOAT deltaTime)
	{
		// Actor self editor logic only. Component and child-actor editor entries are
		// scheduled independently; subclasses keep their existing overrides.
		(void)deltaTime;
	}
#endif

	PActor* PActor::GetAttachedParentActor() const
	{
		return this->AttachedParentActor;
	}

	void PActor::DetachFromParentActor()
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "This actor belong to no world", this->GetWorld() != nullptr);
		const ETransform WorldTrans = this->GetActorWorldTransform();
		if(this->AttachedParentActor)
		{
			this->AttachedParentActor->ChildrenActors.Remove(this);
			AttachedParentActor = nullptr;
		}
		this->RootComponent->DetachFromParentComponent();
		this->GetWorld()->AddActor(this);
		// Re-home under the world root and keep the world transform: the root actor's
		// transform is part of the parent chain now, so the local one has to be recomputed
		// against it (setting the world values as local ones was only correct when the
		// root actor happened to be at identity).
		this->RootComponent->SetComponentWorldTransform(WorldTrans);
	}

	void PActor::AttachToActor(PActor* Parent, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(this, Parent, RelativeTransform);
	}

	void PActor::AttachToActor(PActor* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(this, Parent, InRule, RelativeTransform);
	}

	void PActor::AttachActorTo(PActor* Child, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(Child, this, RelativeTransform);
	}

	void PActor::AttachActorTo(PActor* Child, EAttachmentRuleType InRule, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(Child, this, InRule, RelativeTransform);
	}

	void PActor::AttachActorToActor(PActor* Child, PActor* Parent, const ETransform& RelativeTransform)
	{
		// Historic overload: keep whatever transform the child already has.
		PActor::AttachActorToActor(Child, Parent, EAttachmentRuleType::KEEP_CURRENT, RelativeTransform);
	}

	void PActor::AttachActorToActor(PActor* Child, PActor* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "You are attaching an actor to itself", Child != Parent);
		PE_CHECK(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching actor to actor", Child && Parent && Child->GetRootComponent() && Parent->GetRootComponent());

		// Cycle check on the actor chain: the tick and transform walks recurse actor
		// parents, so attaching an ancestor below its own descendant is an infinite
		// loop, not a misplacement. Refused with a warning - not a check - because it is
		// recoverable (the engine keeps the current hierarchy) and the framework sanity
		// check exercises this exact path.
		for (const PActor* Ancestor = Parent; Ancestor; Ancestor = Ancestor->GetAttachedParentActor())
		{
			if (Ancestor == Child)
			{
				PE_LOG_WARN(EString(PE_TEXT("AttachActorToActor: refused, the new parent is a descendant of the child.")));
				return;
			}
		}

		if(Child->AttachedParentActor)
		{
			Child->AttachedParentActor->ChildrenActors.Remove(Child);
		}
		PSceneComponent::AttachComponentToComponent(Child->GetRootComponent(), Parent->GetRootComponent(), InRule, RelativeTransform);
		if(!Parent->ChildrenActors.Contains(Child))
		{
			Parent->ChildrenActors.Add(Child);
		}
		Child->AttachedParentActor = Parent;
		// The actor and its whole subtree just changed scene transform; make sure the parent
		// walk reaches it even if the child's flag was already set before the attach.
		Child->MarkRenderTreeDirty();
		Parent->MarkRenderTreeDirty();
	}

	TArray<PActor*> PActor::GetAllActorsAttached(BOOL8 bRecursive) const
	{
		if(!bRecursive)
		{
			return this->ChildrenActors;
		}
		TArray<PActor*> Out = this->ChildrenActors;
		for (PActor* Element : this->ChildrenActors)
		{
			Out.Append(Element->GetAllActorsAttached(TRUE));
		}
		return Out;
	}

	const PActor* PActor::GetActorByUniqueID(const ObjectIdentityType& InUniqueID, const BOOL8& bIncludeChildActor) const
	{
		if(this->GetUniqueID() == InUniqueID)
		{
			return this;
		}
		if(!bIncludeChildActor)
		{
			return nullptr;
		}

		for (PActor* Element : ChildrenActors)
		{
			const PActor* Ret = Element->GetActorByUniqueID(InUniqueID, bIncludeChildActor);
			if(Ret)
			{
				return Ret;
			}

		}
		return nullptr;
	}

	void PActor::DetachActorsAttached()
	{
		TArray<PActor*> ActorsAttached(this->ChildrenActors);
		for (PActor* Actor : ActorsAttached)
		{
			Actor->DetachFromParentActor();
		}
	}

	void PActor::DestroyActorsAttached()
	{
		TArray<PActor*> ActorsAttached(this->ChildrenActors);
		this->ChildrenActors.Empty();
		for (PActor* Actor : ActorsAttached)
		{
			if(!Actor)
			{
				continue;
			}
			if(Actor->AttachedParentActor == this)
			{
				Actor->AttachedParentActor = nullptr;
			}
			Actor->Destroy();
		}
	}

	BOOL32 PActor::ShouldSerializeToLevel() const
	{
		// Only classes with an on-disk identity can be created again on load. Concrete actors
		// are registered in SerializableClassRegistry; editor-only helpers are not.
		const CHAR* StableName = GetClassStableName(GetClassHashCode(this));
		return ((StableName) && (StableName[0] != '\0')) ? TRUE : FALSE;
	}

	void PActor::PrepareForDeserialize()
	{
		// The class factory constructs a playable default actor, including its root and any
		// components a subclass constructor creates. A level file owns the actor's complete
		// component set, so the loader clears all construction-created components before
		// installing the serialized ones. Subclasses that cache other constructor state
		// override this and reset those caches around the base call.
		ClearComponents();
	}

	void PActor::PostDeserialize()
	{
		// The base actor has no runtime state to rebuild after its components are installed.
	}

	void PActor::ReplaceRootComponentForLoad(PSceneComponent* NewRoot)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "Replacing root component with a nullptr", NewRoot != nullptr);
		PE_CHECK(ENGINE_ACTOR_ERROR, "Replacing a root component requires PrepareForDeserialize first", RootComponent == nullptr);
		if ((!NewRoot) || (RootComponent != nullptr))
		{
			return;
		}
		// The loader calls PrepareForDeserialize before installing a serialized root. This
		// method only adopts the file's root; it never interprets a pre-existing root as a
		// construction default.
		NewRoot->SetOwnerActor(this);
		if (!Components.Contains(NewRoot))
		{
			Components.Add(NewRoot);
		}
		this->RootComponent = NewRoot;
		this->RootComponent->SetOwnerActor(this);
		POBJ_DEBUGNAME_SET(this->RootComponent, EnsureRootDebugName(POBJ_DEBUGNAME_GET(this->RootComponent)));
		MarkRenderTreeDirty();
	}

	TArray<PActorComponent*> PActor::GetAllComponents() const
	{
		return Components;
	}

	void PActor::SetRootComponent(PSceneComponent* NewRoot)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "You are setting root component with a nullptr", NewRoot != nullptr);

		ETransform trans;
		NewRoot->SetOwnerActor(this);
		if (!Components.Contains(NewRoot))
		{
			Components.Add(NewRoot);
		}
		if (RootComponent)
		{
			trans = RootComponent->Transform;
			TArray<PSceneComponent*> TempComponents = RootComponent->ChildrenComponents;
			for (PSceneComponent* Component : TempComponents)
			{
				Component->AttachToComponent(NewRoot);
			}
			this->RootComponent->AttachToComponent(NewRoot);
			POBJ_DEBUGNAME_SET(this->RootComponent, StripRootDebugSuffix(POBJ_DEBUGNAME_GET(this->RootComponent)));
			this->RootComponent->SetComponentTransform(ETransform());
			this->RootComponent = nullptr;
		}

		this->RootComponent = NewRoot;
		this->RootComponent->SetOwnerActor(this);
		POBJ_DEBUGNAME_SET(this->RootComponent, EnsureRootDebugName(POBJ_DEBUGNAME_GET(this->RootComponent)));
		this->RootComponent->SetComponentTransform(trans);

		if(this->GetWorld())
		{
			if (!this->RootComponent->IsInitialized())
			{
				this->RootComponent->Init();
			}
			this->RootComponent->OnAddedToScene(this->GetWorld());
		}

		// The component list changed (possibly from the constructor, before any world).
		MarkRenderTreeDirty();
	}

	Vector3 PActor::GetActorLocation() const
	{
		return this->RootComponent->GetComponentLocalLocation();
	}

	Quaternion PActor::GetActorRotation() const
	{
		return this->RootComponent->GetComponentLocalRotation();
	}

	Vector3 PActor::GetActorScale() const
	{
		return this->RootComponent->GetComponentLocalScale();
	}

	PE_NODISCARD Vector3 PActor::GetActorWorldLocation() const
	{
		return this->RootComponent->GetComponentWorldLocation();
	}

	PE_NODISCARD Quaternion PActor::GetActorWorldRotation() const
	{
		return this->RootComponent->GetComponentWorldRotation();
	}

	PE_NODISCARD Vector3 PActor::GetActorWorldScale() const
	{
		return this->RootComponent->GetComponentWorldScale();
	}

	const ETransform& PActor::GetActorTransform() const
	{
		return this->RootComponent->GetTransform();
	}

	ETransform PActor::GetActorWorldTransform() const
	{
		return this->RootComponent->GetWorldTransform();
	}

	void PActor::SetActorLocation(const Vector3& Location)
	{
		this->RootComponent->SetComponentLocation(Location);
	}

	void PActor::SetActorRotation(const Quaternion& Rotation)
	{
		this->RootComponent->SetComponentRotation(Rotation);
	}

	void PActor::SetActorScale(const Vector3& Scale)
	{
		this->RootComponent->SetComponentScale(Scale);
	}

	Vector3 PActor::GetActorForwardVector() const
	{
		return this->RootComponent->GetComponentForwardVector();
	}

	Vector3 PActor::GetActorRightVector() const
	{
		return this->RootComponent->GetComponentRightVector();
	}

	Vector3 PActor::GetActorUpVector() const
	{
		return this->RootComponent->GetComponentUpVector();
	}

	EMobilityType PActor::GetMobility() const
	{
		return RootComponent->GetMobility();
	}


	void PActor::AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform )
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching", !(!Component || !Actor || !Actor->GetRootComponent()));
		PSceneComponent::AttachComponentToComponent(Component, Actor->GetRootComponent(), RelativeTransform);

	}

	void PActor::AddComponent(PActorComponent* NewComponent, const ETransform& RelativeTransform)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "Adding null component", NewComponent != nullptr );

		// Ownership is exclusive: a component that already belongs to another actor would
		// be destroyed twice, once per owner. Assigning the owner here for every
		// component - not just scene components - also removes the trap where
		// PPhysicsComponent::Init dereferenced a null owner because only the scene path
		// set one.
		if (PActor* CurrentOwner = NewComponent->GetOwnerActor())
		{
			if (CurrentOwner != this)
			{
				PE_CHECK(ENGINE_ACTOR_ERROR, "Component already belongs to another actor", FALSE);
				return;
			}
		}
		else
		{
			NewComponent->SetOwnerActor(this);
		}

		if(PSceneComponent* SceneComp = dynamic_cast<PSceneComponent*>(NewComponent))
		{
			AttachComponentToActor(SceneComp, this, RelativeTransform);
		}
		this->Components.Add(NewComponent);
		this->MarkRenderTreeDirty();
		if (this->GetWorld())
		{
			if (!NewComponent->IsInitialized())
			{
				NewComponent->Init();
			}
			NewComponent->OnAddedToScene(this->GetWorld());
		}

	}

	const PActorComponent* PActor::GetComponentByUniqueID(const ObjectIdentityType& InUniqueID, const BOOL8& bIncludeChildComponent) const
	{
		const PActorComponent* Ret = RootComponent->GetComponentByUniqueID(InUniqueID, bIncludeChildComponent);

		if(Ret)
		{
			return Ret;
		}

		for (PActorComponent* Component : Components)
		{
			if(Component->GetUniqueID() == InUniqueID)
			{
				return Component;
			}
		}
		return nullptr;
	}

	void PActor::DestroyComponent(PActorComponent* Component)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "Destroying a null component", Component != nullptr);
		// Not contained means one of two things, and neither may run Destroy: the
		// component belongs to another actor, or it was already destroyed and removed
		// from this list. The old assertion here was inverted - it fired on the normal
		// case and stayed silent on the dangerous one.
		if (!Components.Contains(Component))
		{
			PE_LOG_WARN(EString(PE_TEXT("DestroyComponent: component is not in this actor's list, skipped.")));
			return;
		}
		// With a world, destruction defers to its pending-destroy queue: a caller inside
		// a tick, a physics callback or UserEndPlay cannot know whether some loop still
		// holds this component. Without one (teardown, detached actor), destroy now.
		if (PWorld* World = GetWorld())
		{
			World->DestroyComponent(Component);
			return;
		}
		Component->Destroy();
	}

	void PActor::RemoveComponentReference(PActorComponent* Component)
	{
		// The component's OnPreDelete calls this as its final destroy step, so the list
		// never keeps a pointer to memory that is about to go away.
		Components.Remove(Component);
	}

	void PActor::ClearComponents()
	{
		// Snapshot + destroy-roots-only: a scene component whose ParentComponent is set
		// dies with that parent (its ClearChildren destroys it recursively), so calling
		// Destroy on it here as well would destroy it twice. Whether a parent happens to
		// sort before its child in the pointer-ordered set is heap luck, which is exactly
		// why the old loop double-freed only sometimes.
		TArray<PActorComponent*> ComponentsSnapshot = Components;
		for (UINT32 Index = 0u, Num = ComponentsSnapshot.Num<UINT32>(); Index < Num; Index++)
		{
			PActorComponent* Component = ComponentsSnapshot[Index];
			if (!Component || (Component == RootComponent))
			{
				continue;
			}
			PSceneComponent* SceneComponent = dynamic_cast<PSceneComponent*>(Component);
			if (SceneComponent && SceneComponent->GetParentComponent())
			{
				continue;
			}
			Component->Destroy();
		}
		if (RootComponent)
		{
			RootComponent->Destroy();
			RootComponent = nullptr;
		}
		Components.Empty();
	}

	EBoundAABB PActor::GetBounds() const
	{
		//TODO
		return this->RootComponent->GetComponentBound();
	}

	void PActor::OnDestroy()
	{
		// RemovedFromScene already ran (the orchestrator calls it first), so UserEndPlay
		// and every component's scene cleanup happened while the object was still whole.
		// What is left is structural teardown.
		if(AttachedParentActor)
		{
			AttachedParentActor->ChildrenActors.Remove(this);
			AttachedParentActor = nullptr;
		}

		DestroyActorsAttached();
		ClearComponents();
	}

	void PActor::BeginAddedToScene(PWorld* World)
	{
		const BOOL32 bWasAddedToScene = IsAddedToScene();
		PObject::BeginAddedToScene(World);
		MarkRenderTreeDirty();
		for (PActorComponent* Component : Components)
		{
			Component->OnAddedToScene(this->GetWorld());
		}
		RegisterSelfTickDomains();
		for (PActor* Child : ChildrenActors)
		{
			Child->BeginAddedToScene(this->GetWorld());
		}
		if (!bWasAddedToScene)
		{
			this->UserBeginPlay();
		}
	}

	void PActor::RemovedFromScene()
	{
		// Unregister before the early-return: a repeated removal may still have a live
		// scheduler handle even though the scene membership flag is already clear.
		UnregisterSelfTickDomains();
		// The world pointer is still set at entry (PObject::RemovedFromScene clears it at
		// the end of this chain), so this is the one spot every destroy path funnels
		// through where the world's actor registry can be cleaned - including direct
		// Destroy() calls that never went through PWorld::RemoveActor.
		if (PWorld* World = GetWorld())
		{
			World->UnregisterActor(this);
		}
		if (!IsAddedToScene())
		{
			return;
		}
		for (PActorComponent* Component : Components)
		{
			Component->OnRemovedFromScene();
		}
		for (PActor* Child : ChildrenActors)
		{
			Child->RemovedFromScene();
		}
		UserEndPlay();
		PObject::RemovedFromScene();
	}


	void PActor::UserBeginPlay()
	{
	}

	void PActor::UserEndPlay()
	{
	}

	void PActor::UserTick(FLOAT deltaTime)
	{

	}



	PSceneComponent* PActor::GetRootComponent() const
	{
		return this->RootComponent;
	}


};
