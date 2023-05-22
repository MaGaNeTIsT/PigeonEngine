#include <PigeonBase/Object/Actor.h>

#include "Component/SceneComponent.h"

namespace PigeonEngine
{
	void PActor::Init()
	{
		PObject::Init();
		
	}

	void PActor::Uninit()
	{
		PObject::Uninit();
	}

	void PActor::FixedTick(FLOAT deltaTime)
	{
		if(!IsTickable())
		{
			return;
		}
	}

	PActor::PActor()
	{
		if(!RootComponent)
		{
			RootComponent = new PSceneComponent();
		}
	}
	PActor::~PActor()
	{
		// for(auto& elem : Components)
		// {
		// 	if(elem)
		// 	{
		// 		delete elem;
		// 	}
		// }
		// Components.clear();
		
	}

	PActor* PActor::GetAttachedParentActor() const
	{
		return this->AttachedParentActor;
	}

	void PActor::AttachToActor(PActor* AttachTo, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(this, AttachTo, RelativeTransform);
	}

	void PActor::AttachActorTo(PActor* Actor, const ETransform& RelativeTransform)
	{
		PActor::AttachActorToActor(Actor, this, RelativeTransform);
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
		for(const auto& elem : Components)
		{
			DestoyComponent(elem);
		}
		Components.Clear();
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