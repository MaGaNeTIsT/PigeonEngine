#include "../../Headers/Component/SceneComponent.h"

#include "../../Headers/Actor.h"

namespace PigeonEngine
{

	PSceneComponent::PSceneComponent()
	{
	}
	PSceneComponent::~PSceneComponent()
	{
	}
	PSceneComponent::PSceneComponent(const PSceneComponent& other)
	{
	}
	PSceneComponent& PSceneComponent::operator=(const PSceneComponent& other)
	{
		//TODO
		return (*this);
	}

	void PSceneComponent::AttachToComponent(PSceneComponent* AttachTo)
	{
		AttachComponentToComponent(this, AttachTo);
	}

	void PSceneComponent::AttachComponentTo(PSceneComponent* Component)
	{
		AttachComponentToComponent(Component, this);
	}

	void PSceneComponent::AttachComponentToComponent(PSceneComponent* Component, PSceneComponent* AttachTo)
	{
		if(!Component || !AttachTo || !AttachTo->GetOwnerActor())
		{
			// Need an alert here!
			return;
		}
		AttachTo->AttachedParentComponent = AttachTo;
		AttachTo->SetOwnerActor(AttachTo->GetOwnerActor());
	}

	BOOL PSceneComponent::ContainTransform()const
	{
		return TRUE;
	}

};