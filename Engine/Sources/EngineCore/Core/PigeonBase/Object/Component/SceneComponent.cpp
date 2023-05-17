#include "./SceneComponent.h"

#include <PigeonBase/Object/Actor.h>

namespace PigeonEngine
{

	PSceneComponent::PSceneComponent()
	{
	}
	PSceneComponent::~PSceneComponent()
	{
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
		AttachTo->ChildrenComponents.insert(Component);
	}

	PSceneComponent* PSceneComponent::GetAttachedParentComponent() const
	{
		return this->AttachedParentComponent;
	}

	BOOL PSceneComponent::ContainTransform()const
	{
		return TRUE;
	}
	const ETransform* PSceneComponent::GetTransform()const
	{
		return (&Transform);
	}

	Vector3 PSceneComponent::GetComponentLocalLocation() const
	{
		return this->Transform.GetLocation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent);
	}

	Quaternion PSceneComponent::GetComponentLocalRotation()const
	{
		return this->Transform.GetRotation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent);
	}

	Vector3 PSceneComponent::GetComponentLocalScale() const
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

	Vector3 PSceneComponent::GetComponentForwardVector() const
	{
		return this->Transform.GetForwardVector<ECoordinateSpaceType::ECST_WORLD>();
	}

	Vector3 PSceneComponent::GetComponentRightVector() const
	{
		return this->Transform.GetRightVector<ECoordinateSpaceType::ECST_WORLD>();
	}

	Vector3 PSceneComponent::GetComponentUpVector() const
	{
		return this->Transform.GetUpVector<ECoordinateSpaceType::ECST_WORLD>();
	}
};