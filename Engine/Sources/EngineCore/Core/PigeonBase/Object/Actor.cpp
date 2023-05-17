#include <PigeonBase/Object/Actor.h>

#include "Component/SceneComponent.h"

namespace PigeonEngine
{
	void PActor::Init()
	{
		PObject::Init();
		
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
		for(auto& elem : Components)
		{
			if(elem)
			{
				delete elem;
			}
		}
		Components.clear();
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
		Check(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching", !Actor || !AttachTo || !Actor->GetRootComponent() || !AttachTo->GetRootComponent());
		Actor->AttachedParentActor = AttachTo;
		PSceneComponent::AttachComponentToComponent(Actor->GetRootComponent(), AttachTo->GetRootComponent(), RelativeTransform);
	}

	void PActor::SetRootComponent(PSceneComponent* NewRoot)
	{
		if(!NewRoot)
		{
			return;
		}
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
		AttachComponentToActor(Component, this);
	}

	void PActor::AttachComponentToActor(PSceneComponent* Component, PActor* Actor, const ETransform& RelativeTransform )
	{
		Check(ENGINE_ACTOR_ERROR, "Something is nullptr when attaching", !Component || !Actor || !Actor->GetRootComponent());
		PSceneComponent::AttachComponentToComponent(Component, Actor->GetRootComponent(), RelativeTransform);
	}

	void PActor::AddComponent(PActorComponent* NewComponent)
	{
		Check(ENGINE_ACTOR_ERROR, "Adding null component", !NewComponent );
		this->Components.insert(NewComponent);
	}

	PSceneComponent* PActor::GetRootComponent() const
	{
		return this->RootComponent;
	}
};