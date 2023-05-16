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

	ETransform PSceneComponent::GetComponentTransform() const
	{
		return this->Transform;
	}

	Vector3 PSceneComponent::GetComponentLocation() const
	{
		return this->Transform.GetLocation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent);
	}

	Quaternion PSceneComponent::GetComponentRotation()const
	{
		return this->Transform.GetRotation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent);
	}

	Vector3 PSceneComponent::GetComponentScale() const
	{
		return this->Transform.GetScaling<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent);
	}

	Vector3 PSceneComponent::GetComponentWorldLocation() const
	{
		return this->Transform.GetLocation<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent);
	}

	Quaternion PSceneComponent::GetComponentWorldRotation() const
	{
		return this->Transform.GetRotation<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent);
	}

	Vector3 PSceneComponent::GetComponentWorldScale() const
	{
		return this->Transform.GetScaling<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent);
	}
};