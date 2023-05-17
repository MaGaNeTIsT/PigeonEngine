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
	

	void PSceneComponent::AttachToComponent(PSceneComponent* AttachTo, const ETransform& RelativeTransform)
	{
		AttachComponentToComponent(this, AttachTo, RelativeTransform);
	}

	void PSceneComponent::AttachComponentTo(PSceneComponent* Component, const ETransform& RelativeTransform)
	{
		AttachComponentToComponent(Component, this, RelativeTransform);
	}

	void PSceneComponent::AttachComponentToComponent(PSceneComponent* Component, PSceneComponent* AttachTo, const ETransform& RelativeTransform)
	{
		Check(ENGINE_COMPONENT_ERROR, "Something is nullptr when attaching", !Component || !AttachTo || !AttachTo->GetOwnerActor());
		AttachTo->AttachedParentComponent = AttachTo;
		AttachTo->SetOwnerActor(AttachTo->GetOwnerActor());
		AttachTo->ChildrenComponents.insert(Component);
		Component->SetComponentTransform(RelativeTransform);
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

	void PSceneComponent::SetComponentLocation(const Vector3& Location)
	{
		Transform.SetLocation<ECoordinateSpaceType::ECST_LOCAL>(Location);
	}

	void PSceneComponent::SetComponentRotation(const Quaternion& Rotation)
	{
		Transform.SetRotation<ECoordinateSpaceType::ECST_LOCAL>(Rotation);
	}

	void PSceneComponent::SetComponentScale(const Vector3& Scale)
	{
		Transform.SetScaling<ECoordinateSpaceType::ECST_LOCAL>(Scale);
	}

	void PSceneComponent::SetComponentTransform(const ETransform& Trans)
	{
		SetComponentLocation(Trans.GetLocation<ECoordinateSpaceType::ECST_LOCAL>());
		SetComponentRotation(Trans.GetRotation<ECoordinateSpaceType::ECST_LOCAL>());
		SetComponentScale(Trans.GetScaling<ECoordinateSpaceType::ECST_LOCAL>());
	}

	void PSceneComponent::SetComponentWorldLocation(const Vector3& Location)
	{
		Transform.SetLocation<ECoordinateSpaceType::ECST_WORLD>(Location);
	}

	void PSceneComponent::SetComponentWorldRotation(const Quaternion& Rotation)
	{
		Transform.SetRotation<ECoordinateSpaceType::ECST_WORLD>(Rotation);
	}

	void PSceneComponent::SetComponentWorldScale(const Vector3& Scale)
	{
		Transform.SetScaling<ECoordinateSpaceType::ECST_WORLD>(Scale);
	}

	void PSceneComponent::SetComponentWorldTransform(const ETransform& Trans)
	{
		SetComponentWorldLocation(Trans.GetLocation<ECoordinateSpaceType::ECST_WORLD>());
		SetComponentWorldRotation(Trans.GetRotation<ECoordinateSpaceType::ECST_WORLD>());
		SetComponentWorldScale(Trans.GetScaling<ECoordinateSpaceType::ECST_WORLD>());
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