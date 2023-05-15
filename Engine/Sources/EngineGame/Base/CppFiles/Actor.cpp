#include "../Headers/Actor.h"
#include "../Headers/Component/SceneComponent.h"

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

	void PActor::AttachToActor(PActor* AttachTo)
	{
		PActor::AttachActorToActor(this, AttachTo);
	}

	void PActor::AttachActorTo(PActor* Actor)
	{
		PActor::AttachActorToActor(Actor, this);
	}

	void PActor::AttachActorToActor(PActor* Actor, PActor* AttachTo)
	{
		if(!Actor || !AttachTo)
		{
			// Need an alert here
			return;
		}
		Actor->AttachedParentActor = AttachTo;
		PSceneComponent::AttachComponentToComponent(Actor->GetRootComponent(), AttachTo->GetRootComponent());
	}

	void PActor::SetRootComponent(PSceneComponent* NewRoot)
	{
		if(!NewRoot)
		{
			return;
		}
		if(RootComponent)
		{
			delete RootComponent;
		}
		this->RootComponent = NewRoot;
	}


	void PActor::AttachComponent(PSceneComponent* Component)
	{
		AttachComponentToActor(Component, this);
	}

	void PActor::AttachComponentToActor(PSceneComponent* Component, PActor* Actor)
	{
		if(!Component || !Actor)
		{
			// Need an alert here
			return;
		}
		PSceneComponent::AttachComponentToComponent(Component, Actor->GetRootComponent());
	}

	void PActor::AddComponent(PActorComponent* NewComponent)
	{
		if(!NewComponent)
		{
			// Need an alert here
			return;
		}
		this->Components.insert(NewComponent);
	}

	PSceneComponent* PActor::GetRootComponent() const
	{
		return this->RootComponent;
	}
};