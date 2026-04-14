#include <PigeonBase/Object/Actor.h>

#include "Component/SceneComponent.h"
#include "World/World.h"

#if _EDITOR_ONLY
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
		this->RootComponent->Init();
		for(const auto& Component : Components)
		{
			Component->Init();
		}
		for(const auto& child : ChildrenActors)
		{
			child->Init();
		}
		SetInitialized(TRUE);
	}

	void PActor::Uninit()
	{
		PObject::Uninit();
	}

	void PActor::Tick(FLOAT deltaTime)
	{
		UserTick(deltaTime);
	}

	void PActor::FixedTick(FLOAT deltaTime)
	{
		this->RootComponent->TickRender();//something went wrong when tick render in tick.
		if (!IsTickable())// if actor is not tickable will skip tick and fixed tick of components and child actors.
		{
			return;
		}
		for(const auto& Component : Components)
		{
			Component->FixedTick(deltaTime);
		}
		RootComponent->FixedTick(deltaTime);
		for(const auto& child : ChildrenActors)
		{
			child->FixedTick(deltaTime);
		}
	}

#if _EDITOR_ONLY
	void PActor::EditorTick(FLOAT deltaTime)
	{
		this->RootComponent->TickRender();
		if(!IsTickable())
		{
			return;
		}
		for(const auto& Component : Components)
		{
			Component->EditorTick(deltaTime);
		}
		RootComponent->EditorTick(deltaTime);
		for(const auto& child : ChildrenActors)
		{
			child->EditorTick(deltaTime);
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
		for(const auto& elem : this->ChildrenActors)
		{
			Out.Append(elem->GetAllActorsAttached(TRUE));
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
		
		for(auto& elem : ChildrenActors)
		{
			const PActor* Ret = elem->GetActorByUniqueID(InUniqueID, bIncludeChildActor);
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
		for(auto& elem : ActorsAttached)
		{
			elem->DetachFromParentActor();
		}
	}

	void PActor::DestroyActorsAttached()
	{
		TArray<PActor*> ActorsAttached(this->ChildrenActors);
		this->ChildrenActors.Empty();
		for(auto& elem : ActorsAttached)
		{
			if(!elem)
			{
				continue;
			}
			if(elem->AttachedParentActor == this)
			{
				elem->AttachedParentActor = nullptr;
			}
			elem->Destroy();
		}
	}

	void PActor::SetRootComponent(PSceneComponent* NewRoot)
	{
		PE_CHECK(ENGINE_ACTOR_ERROR, "You are setting root component with a nullptr", NewRoot != nullptr);

		ETransform trans;
		NewRoot->SetOwnerActor(this);
		if (RootComponent)
		{
			trans = RootComponent->Transform;
			TSet<PSceneComponent*> TempComponents = RootComponent->ChildrenComponents;
			for (auto& elem : TempComponents)
			{
				elem->AttachToComponent(NewRoot);
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
		
		for(auto& elem : Components)
		{
			if(elem->GetUniqueID() == InUniqueID)
			{
				return elem;
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
		for (const auto& Component : Components)
		{
			if (Component)
			{
				Component->Destroy();
			}
		}
		Components.Empty();
		if (RootComponent)
		{
			RootComponent->Destroy();
			RootComponent = nullptr;
		}
	}

	EBoundAABB PActor::GetBounds()const
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
		if (RootComponent)
		{
			RootComponent->OnAddedToScene(this->GetWorld());
		}
		for (const auto& Component : Components)
		{
			Component->OnAddedToScene(this->GetWorld());
		}
		for (const auto& Child : ChildrenActors)
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
		if (RootComponent)
		{
			RootComponent->OnRemovedFromScene();
		}
		for (const auto& Component : Components)
		{
			Component->OnRemovedFromScene();
		}
		for (const auto& Child : ChildrenActors)
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

#if _EDITOR_ONLY
	
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
			for (const auto& elem : ChildrenActors)
			{
				elem->GenerateWorldOutline(WorldCurrentSelectedActor);
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