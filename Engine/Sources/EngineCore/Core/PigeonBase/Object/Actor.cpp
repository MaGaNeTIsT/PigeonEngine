#include <PigeonBase/Object/Actor.h>

#include "Component/SceneComponent.h"
#include "World/World.h"

#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#endif

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

	PActor::PActor()
	{
		if(!RootComponent)
		{
			SetRootComponent(new PSceneComponent());
		}
	}

	PActor::~PActor()
	{
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
		this->RootComponent->TickRender();
		if (!IsTickable())// if actor is not tickable will skip tick and fixed tick of components and child actors.
		{
			return;
		}
		for (PActorComponent* Component : Components)
		{
			if (Component && Component->IsTickable())
			{
				Component->Tick(deltaTime);
			}
		}
		// By index rather than by reference: a tick may spawn actors - a scene builder does exactly
		// that - and appending to ChildrenActors reallocates the array, so a range over it would keep
		// a reference into the buffer the reallocation freed. An index into the live array cannot
		// dangle, and re-reading the count lets an actor spawned during this tick be ticked by it.
		for (UINT32 ChildIndex = 0u, NumChildren = ChildrenActors.Num<UINT32>(); ChildIndex < NumChildren; ChildIndex++)
		{
			PActor* Child = ChildrenActors[ChildIndex];
			if (Child && Child->IsTickable())
			{
				Child->Tick(deltaTime);
			}
		}
		UserTick(deltaTime);
	}

	void PActor::FixedTick(FLOAT deltaTime)
	{
		if (!IsTickable())// if actor is not tickable will skip tick and fixed tick of components and child actors.
		{
			return;
		}
		for (PActorComponent* Component : Components)
		{
			if (Component && Component->IsTickable())
			{
				Component->FixedTick(deltaTime);
			}
		}
		// Same reason as Tick: a fixed tick may spawn actors too, so the list is walked by index.
		for (UINT32 ChildIndex = 0u, NumChildren = ChildrenActors.Num<UINT32>(); ChildIndex < NumChildren; ChildIndex++)
		{
			PActor* Child = ChildrenActors[ChildIndex];
			if (Child && Child->IsTickable())
			{
				Child->FixedTick(deltaTime);
			}
		}
	}

#if ENGINE_EDITOR_ONLY
	void PActor::EditorTick(FLOAT deltaTime)
	{
		this->RootComponent->TickRender();
		if(!IsTickable())
		{
			return;
		}
		for (PActorComponent* Component : Components)
		{
			if (Component && Component->IsTickable())
			{
				Component->EditorTick(deltaTime);
			}
		}
		// Same reason as Tick: the editor tick is the one a scene builder runs inside, so this is
		// exactly the loop that can be reallocated under itself.
		for (UINT32 ChildIndex = 0u, NumChildren = ChildrenActors.Num<UINT32>(); ChildIndex < NumChildren; ChildIndex++)
		{
			PActor* Child = ChildrenActors[ChildIndex];
			if (Child && Child->IsTickable())
			{
				Child->EditorTick(deltaTime);
			}
		}
	}
#endif

	PActor* PActor::GetAttachedParentActor() const
	{
		return this->AttachedParentActor;
	}

	void PActor::DetachFromParentActor()
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "This actor belong to no world", this->GetWorld() != nullptr);
		ETransform WolrdTrans = this->GetActorWorldTransform();
		if(this->AttachedParentActor)
		{
			this->AttachedParentActor->ChildrenActors.Remove(this);
			AttachedParentActor = nullptr;
		}
		this->RootComponent->DetachFromParentComponent();
		this->GetWorld()->AddActor(this);
		this->SetActorLocation(WolrdTrans.GetLocation_Local());
		this->SetActorRotation(WolrdTrans.GetRotation_Local());
		this->SetActorScale(WolrdTrans.GetScaling_Local());
	}

	void PActor::AttachToActor(PActor* Parent, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(this, Parent, RelativeTransform);
	}

	void PActor::AttachActorTo(PActor* Child, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(Child, this, RelativeTransform);
	}

	void PActor::AttachActorToActor(PActor* Child, PActor* Parent, const ETransform& RelativeTransform)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "You are attaching an actor to itself", Child != Parent);
		PE_CHECK(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching actor to actor", Child && Parent && Child->GetRootComponent() && Parent->GetRootComponent());

		if(Child->AttachedParentActor)
		{
			Child->AttachedParentActor->ChildrenActors.Remove(Child);
		}
		PSceneComponent::AttachComponentToComponent(Child->GetRootComponent(), Parent->GetRootComponent(), RelativeTransform);
		if(!Parent->ChildrenActors.Contains(Child))
		{
			Parent->ChildrenActors.Add(Child);
		}
		Child->AttachedParentActor = Parent;
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
			TSet<PSceneComponent*> TempComponents = RootComponent->ChildrenComponents;
			for (PSceneComponent* Component : TempComponents)
			{
				Component->AttachToComponent(NewRoot);
			}
			this->RootComponent->AttachToComponent(NewRoot);
			POBJ_DEBUGNAME_SET(this->RootComponent, (POBJ_DEBUGNAME_GET(this->RootComponent)).Replace("(Root)", ""));
			this->RootComponent->SetComponentTransform(ETransform());
			this->RootComponent = nullptr;
		}

		this->RootComponent = NewRoot;
		this->RootComponent->SetOwnerActor(this);
		POBJ_DEBUGNAME_SET(this->RootComponent, (POBJ_DEBUGNAME_GET(this->RootComponent)) + "(Root)");
		this->RootComponent->SetComponentTransform(trans);

		if(this->GetWorld())
		{
			if (!this->RootComponent->IsInitialized())
			{
				this->RootComponent->Init();
			}
			this->RootComponent->OnAddedToScene(this->GetWorld());
		}

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


		if(PSceneComponent* SceneComp = dynamic_cast<PSceneComponent*>(NewComponent))
		{
			AttachComponentToActor(SceneComp, this, RelativeTransform);
		}
		this->Components.Add(NewComponent);
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

	void PActor::DestoyComponent(PActorComponent* Component)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "You are destroy a component doesn't belonged to this", !Components.Contains(Component));
		Component->Destroy();
	}

	void PActor::ClearComponents()
	{
		for (PActorComponent* Component : Components)
		{
			if (Component && Component != RootComponent)
			{
				Component->Destroy();
			}
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

	void PActor::Destroy()
	{
		if(AttachedParentActor)
		{
			AttachedParentActor->ChildrenActors.Remove(this);
			AttachedParentActor = nullptr;
		}

		DestroyActorsAttached();
		ClearComponents();
		if (GetWorld())
		{
			RemovedFromScene();
		}
		PObject::Destroy();
	}

	void PActor::BeginAddedToScene(PWorld* World)
	{
		const BOOL32 bWasAddedToScene = IsAddedToScene();
		PObject::BeginAddedToScene(World);
		for (PActorComponent* Component : Components)
		{
			Component->OnAddedToScene(this->GetWorld());
		}
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

#if ENGINE_EDITOR_ONLY

	void PActor::GenerateWorldOutline(const PActor* WorldCurrentSelectedActor)
	{
		BOOL8 bSelectedActor	= WorldCurrentSelectedActor == this;
		ImGuiTreeNodeFlags TreeNodeFlag = (ChildrenActors.Num() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);

		ImGui::PushID((void*)this);
		BOOL8 bTreeNodeExpand	= ImGui::TreeNodeEx("##node", TreeNodeFlag);
		ImGui::SameLine();
		BOOL8 bSelected			= ImGui::Selectable(*(POBJ_DEBUGNAME_GET(this)), &bSelectedActor);
		ImGui::PopID();

		if (bTreeNodeExpand)
		{
			// child actors
			for (PActor* Element : ChildrenActors)
			{
				Element->GenerateWorldOutline(WorldCurrentSelectedActor);
			}
			ImGui::TreePop();
		}

		if (bSelected)
		{
			this->GetWorld()->SetSelectedActor(this);
		}
	}

	void PActor::GenerateDetails(const PActorComponent* WorldCurrentSelectedComponent)
	{
		if(RootComponent)
		{
			BOOL8 bTreeNodeExpand = ImGui::TreeNodeEx("Components", ImGuiTreeNodeFlags_DefaultOpen);
			if (bTreeNodeExpand)
			{
				RootComponent->GenerateComponentOutline(WorldCurrentSelectedComponent);
				ImGui::TreePop();
			}
		}
	}
#endif

};
