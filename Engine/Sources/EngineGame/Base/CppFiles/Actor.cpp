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
		if(!bCanTick)
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
	}

	PActor* PActor::GetAttachedParentActor() const
	{
		return this->AttachedParentActor;
	}

	void PActor::AttachToActor(PActor* AttachTo)
	{
		if(!AttachTo)
		{
			return;
		}
		this->AttachedParentActor = AttachTo;
	}

	PSceneComponent* PActor::GetRootComponent() const
	{
		return this->RootComponent;
	}
};