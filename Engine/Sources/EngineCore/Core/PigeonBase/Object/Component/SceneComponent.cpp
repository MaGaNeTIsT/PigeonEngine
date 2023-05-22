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
		Check(ENGINE_COMPONENT_ERROR, "You are attaching an component to itself", Component == AttachTo);
		Check(ENGINE_COMPONENT_ERROR, "Something is nullptr when attaching", !Component || !AttachTo || !AttachTo->GetOwnerActor());

		AttachTo->SetAttachedParentComponent(AttachTo);
		AttachTo->SetOwnerActor(AttachTo->GetOwnerActor());
		AttachTo->AddChildComponent(Component);
		Component->SetComponentTransform(RelativeTransform);
	}

	PSceneComponent* PSceneComponent::GetAttachedParentComponent() const
	{
		return this->AttachedParentComponent;
	}

	void PSceneComponent::SetAttachedParentComponent(PSceneComponent* NewParent)
	{
		Check(ENGINE_COMPONENT_ERROR, "You are setting this component's attached parent to a nullptr. ", NewParent == nullptr);
		this->AttachedParentComponent = NewParent;
		this->SetOwnerActor(NewParent->GetOwnerActor());
	}

	void PSceneComponent::RemoveFromAttachedParent()
	{
		if(AttachedParentComponent)
		{
			AttachedParentComponent->RemoveChildComponent(this);
		}
		AttachedParentComponent = nullptr;
	}

	void PSceneComponent::ClearChildren()
	{
		for(const auto& elem : ChildrenComponents)
		{
			elem->Destroy();
		}
		ChildrenComponents.Clear();
	}

	void PSceneComponent::AddChildComponent(PSceneComponent* NewChild)
	{
		Check(ENGINE_COMPONENT_ERROR, "You are adding a nullptr to this component as a child. ", NewChild == nullptr);
		this->ChildrenComponents.Add(NewChild);
	}

	void PSceneComponent::RemoveChildComponent(PSceneComponent* NewChild)
	{
		ChildrenComponents.Remove(NewChild);
	}

	void PSceneComponent::ReparentChildrenComponents(PSceneComponent* NewParent)
	{
		Check(ENGINE_COMPONENT_ERROR, "You are reparent childrents to a nullptr. ", NewParent == nullptr);
		for(const auto& Child : ChildrenComponents)
		{
			Child->SetAttachedParentComponent(NewParent);
			// Child->SetOwnerActor(NewParent->GetOwnerActor());
		}
		ChildrenComponents.Clear();
	}


	void PSceneComponent::Init()
	{
		PActorComponent::Init();
	}

	void PSceneComponent::Uninit()
	{
		PActorComponent::Uninit();
	}

	void PSceneComponent::Destroy()
	{
		ClearChildren();
		RemoveFromAttachedParent();
		RemoveFromOwnerActor();
		PActorComponent::Destroy();
		
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
		Transform.SetLocation<ECoordinateSpaceType::ECST_LOCAL>(Location, this->AttachedParentComponent);
	}

	void PSceneComponent::SetComponentRotation(const Quaternion& Rotation)
	{
		Transform.SetRotation<ECoordinateSpaceType::ECST_LOCAL>(Rotation, this->AttachedParentComponent);
	}

	void PSceneComponent::SetComponentScale(const Vector3& Scale)
	{
		Transform.SetScaling<ECoordinateSpaceType::ECST_LOCAL>(Scale, this->AttachedParentComponent);
	}

	void PSceneComponent::SetComponentTransform(const ETransform& Trans)
	{
		SetComponentLocation(Trans.GetLocation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentRotation(Trans.GetRotation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentScale(Trans.GetScaling<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent, this->GetOwnerActor()));
	}

	void PSceneComponent::SetComponentWorldLocation(const Vector3& Location)
	{
		Transform.SetLocation<ECoordinateSpaceType::ECST_WORLD>(Location, this->AttachedParentComponent);
	}

	void PSceneComponent::SetComponentWorldRotation(const Quaternion& Rotation)
	{
		Transform.SetRotation<ECoordinateSpaceType::ECST_WORLD>(Rotation, this->AttachedParentComponent);
	}

	void PSceneComponent::SetComponentWorldScale(const Vector3& Scale)
	{
		Transform.SetScaling<ECoordinateSpaceType::ECST_WORLD>(Scale, this->AttachedParentComponent);
	}

	void PSceneComponent::SetComponentWorldTransform(const ETransform& Trans)
	{
		SetComponentWorldLocation(Trans.GetLocation<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentWorldRotation(Trans.GetRotation<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentWorldScale(Trans.GetScaling<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor()));
	}

	Vector3 PSceneComponent::GetComponentLocalLocation() const
	{
		return this->Transform.GetLocation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Quaternion PSceneComponent::GetComponentLocalRotation()const
	{
		return this->Transform.GetRotation<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentLocalScale() const
	{
		return this->Transform.GetScaling<ECoordinateSpaceType::ECST_LOCAL>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentWorldLocation() const
	{
		return this->Transform.GetLocation<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Quaternion PSceneComponent::GetComponentWorldRotation() const
	{
		return this->Transform.GetRotation<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentWorldScale() const
	{
		return this->Transform.GetScaling<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentForwardVector() const
	{
		return this->Transform.GetForwardVector<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentRightVector() const
	{
		return this->Transform.GetRightVector<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentUpVector() const
	{
		return this->Transform.GetUpVector<ECoordinateSpaceType::ECST_WORLD>(this->AttachedParentComponent, this->GetOwnerActor());
	}
};