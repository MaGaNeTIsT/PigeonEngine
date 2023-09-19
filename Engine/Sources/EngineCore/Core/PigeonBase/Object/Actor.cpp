#include <PigeonBase/Object/Actor.h>

#include "Component/SceneComponent.h"
#include "World/World.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PActor, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);
	
	PActor::PActor()
	{
		if(!RootComponent)
		{
			this->SetRootComponent(new PSceneComponent());
		}
	}
	
	PActor::~PActor()
	{
	}
	
	void PActor::Init()
	{
		
	}

	void PActor::Uninit()
	{
		PObject::Uninit();
	}

	void PActor::Tick(FLOAT deltaTime)
	{
#if _EDITOR_ONLY
		EditorTick(deltaTime);
		return;
#endif
		FixedTick(deltaTime);
	}

	void PActor::FixedTick(FLOAT deltaTime)
	{
		if(!IsTickable())
		{
			return;
		}
		for(const auto& Component : Components)
		{
			Component->FixedTick(deltaTime);
		}
		UserTick(deltaTime);
	}

	void PActor::EditorTick(FLOAT deltaTime)
	{
		if(!IsTickable())
		{
			return;
		}
		for(const auto& Component : Components)
		{
			Component->EditorTick(deltaTime);
		}
	}

	PActor* PActor::GetAttachedParentActor() const
	{
		return this->AttachedParentActor;
	}

	void PActor::DetachFromParentActor()
	{
		Check(ENGINE_ACTOR_ERROR, "This actor belong to no world", this->GetWorld() != nullptr);
		ETransform WolrdTrans = this->GetActorWorldTransform();
		if(this->AttachedParentActor)
		{
			this->AttachedParentActor->ChildrenActors.Remove(this);
		}
		this->RootComponent->DetachFromParentComponent();
		this->GetWorld()->AddActor(this);
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
		Check(ENGINE_ACTOR_ERROR, "You are attaching an actor to itself", Child != Parent);
		Check(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching actor to actor", Child && Parent && Child->GetRootComponent() && Parent->GetRootComponent());
		
		PSceneComponent::AttachComponentToComponent(Child->GetRootComponent(), Parent->GetRootComponent(), RelativeTransform);
		Parent->ChildrenActors.Add(Child);
		
	}

	void PActor::SetRootComponent(PSceneComponent* NewRoot)
	{
		Check(ENGINE_ACTOR_ERROR, "You are setting root component with a nullptr", NewRoot != nullptr);
		// NewRoot->SetComponentTransform(RootComponent->GetTransform());
		// RootComponent->ReparentChildren(NewRoot);
		if (RootComponent)
		{
			RootComponent->Destroy();

		}
		
		this->RootComponent = NewRoot;
		this->RootComponent->SetOwnerActor(this);
		if(this->GetWorld())
		{
			this->RootComponent->BeginAddedToScene(this->GetWorld());
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
		Check(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching", !(!Component || !Actor || !Actor->GetRootComponent()));
		PSceneComponent::AttachComponentToComponent(Component, Actor->GetRootComponent(), RelativeTransform);
		
	}

	void PActor::AddComponent(PActorComponent* NewComponent, const ETransform& RelativeTransform)
	{
		Check(ENGINE_ACTOR_ERROR, "Adding null component", NewComponent != nullptr );
		
		
		if(PSceneComponent* SceneComp = dynamic_cast<PSceneComponent*>(NewComponent))
		{
			AttachComponentToActor(SceneComp, this, RelativeTransform);
			//if (this->GetWorld())
			//{
			//	SceneComp->BeginAddedToScene(MyWorld);
			//}
		}
		this->Components.Add(NewComponent);
		
	}

	void PActor::DestoyComponent(PActorComponent* Component)
	{
		Check(ENGINE_ACTOR_ERROR, "You are destroy a component doesn't belonged to this", !Components.Contains(Component));
		Component->Destroy();
	}

	void PActor::ClearComponents()
	{
		for (auto elem = Components.Begin(); elem != Components.End(); elem++)
		{
			DestoyComponent(*elem);
		}
		Components.Clear();
	}

	EBoundAABB PActor::GetComponentsBoundingBox()
	{
		//TODO
		return EBoundAABB();
	}

	void PActor::Destroy()
	{
		// RemoveFromScene
		ClearComponents();
		PObject::Destroy();
	}

	void PActor::BeginAddedToScene(PWorld* World)
	{
		this->SetWorld(World);
		this->RootComponent->BeginAddedToScene(this->GetWorld());
		this->UserBeginPlay();
	}

	void PActor::RemovedFromScene()
	{
		this->RootComponent->RemovedFromScene();
		this->UserEndPlay();
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