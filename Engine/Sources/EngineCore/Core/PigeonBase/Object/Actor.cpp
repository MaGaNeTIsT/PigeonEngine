#include <PigeonBase/Object/Actor.h>

#include "Component/SceneComponent.h"

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
			RootComponent = new PSceneComponent();
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

	void PActor::AttachToActor(PActor* Another, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(this, Another, RelativeTransform);
	}

	void PActor::AttachActorTo(PActor* Another, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(Another, this, RelativeTransform);
	}

	void PActor::AttachActorToActor(PActor* Actor, PActor* AttachTo, const ETransform& RelativeTransform)
	{
		Check(ENGINE_ACTOR_ERROR, "You are attaching an actor to itself", Actor == AttachTo);
		Check(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching actor to actor", !Actor || !AttachTo || !Actor->GetRootComponent() || !AttachTo->GetRootComponent());
		Actor->AttachedParentActor = AttachTo;
		PSceneComponent::AttachComponentToComponent(Actor->GetRootComponent(), AttachTo->GetRootComponent(), RelativeTransform);
	}

	void PActor::SetRootComponent(PSceneComponent* NewRoot)
	{
		Check(ENGINE_ACTOR_ERROR, "You are setting root component with a nullptr", NewRoot == nullptr);
		NewRoot->SetComponentTransform(*RootComponent->GetTransform());
		delete RootComponent;
		this->RootComponent = NewRoot;
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

	void PActor::AttachComponent(PSceneComponent* Component, const ETransform& RelativeTransform)
	{
		AttachComponentToActor(Component, this, RelativeTransform);
	}

	void PActor::AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform )
	{
		Check(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching", !Component || !Actor || !Actor->GetRootComponent());
		PSceneComponent::AttachComponentToComponent(Component, Actor->GetRootComponent(), RelativeTransform);
	}

	void PActor::AddComponent(PActorComponent* NewComponent)
	{
		Check(ENGINE_ACTOR_ERROR, "Adding null component", NewComponent == nullptr );
		this->Components.Add(NewComponent);
	}

	void PActor::DestoyComponent(PActorComponent* Component)
	{
		Check(ENGINE_ACTOR_ERROR, "You are destroy a component doesn't belonged to this", Components.Contains(Component));
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

	PSceneComponent* PActor::GetRootComponent() const
	{
		return this->RootComponent;
	}
};