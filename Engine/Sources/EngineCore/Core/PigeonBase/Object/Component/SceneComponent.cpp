#include "./SceneComponent.h"

#include <PigeonBase/Object/Actor.h>

#include "../../../../IO/SerializationHelper.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSceneComponent, PActorComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

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
		for (auto elem = ChildrenComponents.Begin(); elem != ChildrenComponents.End(); elem++)
		{
			(*elem)->Destroy();
		}
		ChildrenComponents.Clear();
	}

	TSharedPtr<CJsonObject> PSceneComponent::Serialize()
	{
		TSharedPtr<CJsonObject> JsonObj =  PActorComponent::Serialize();
		
		TSharedPtr<CJsonObject> TransFormJson = ESerializationHelper::SerializeTransform(Transform);
		JsonObj->SetObjectField("Transform", TransFormJson.Get());
		return JsonObj;
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
		for (auto Child = ChildrenComponents.Begin(); Child != ChildrenComponents.End(); Child++)
		{
			(*Child)->SetAttachedParentComponent(NewParent);
			//(*Child)->SetOwnerActor(NewParent->GetOwnerActor());
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

	EMobilityType PSceneComponent::GetMobility() const
	{
		return Mobility;
	}

	void PSceneComponent::SetMobility(const EMobilityType& NewMobility)
	{
		this->Mobility = NewMobility;
	}

	BOOL32 PSceneComponent::ContainTransform()const
	{
		return TRUE;
	}
	const ETransform* PSceneComponent::GetTransform()const
	{
		return (&Transform);
	}

	void PSceneComponent::SetComponentLocation(const Vector3& Location)
	{
		Transform.SetLocation_Local(Location);
	}

	void PSceneComponent::SetComponentRotation(const Quaternion& Rotation)
	{
		Transform.SetRotation_Local(Rotation);
	}

	void PSceneComponent::SetComponentScale(const Vector3& Scale)
	{
		Transform.SetScaling_Local(Scale);
	}

	void PSceneComponent::SetComponentTransform(const ETransform& Trans)
	{
		SetComponentLocation(Trans.GetLocation_Local());
		SetComponentRotation(Trans.GetRotation_Local());
		SetComponentScale(Trans.GetScaling_Local());
	}

	void PSceneComponent::SetComponentWorldLocation(const Vector3& Location)
	{
		Transform.SetLocation_World(Location, this->AttachedParentComponent, this->GetOwnerActor());
	}

	void PSceneComponent::SetComponentWorldRotation(const Quaternion& Rotation)
	{
		Transform.SetRotation_World(Rotation, this->AttachedParentComponent, this->GetOwnerActor());
	}

	void PSceneComponent::SetComponentWorldScale(const Vector3& Scale)
	{
		Transform.SetScaling_World(Scale, this->AttachedParentComponent, this->GetOwnerActor());
	}

	void PSceneComponent::SetComponentWorldTransform(const ETransform& Trans)
	{
		SetComponentWorldLocation(Trans.GetLocation_World(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentWorldRotation(Trans.GetRotation_World(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentWorldScale(Trans.GetScaling_World(this->AttachedParentComponent, this->GetOwnerActor()));
	}

	Vector3 PSceneComponent::GetComponentLocalLocation() const
	{
		return this->Transform.GetLocation_Local();
	}

	Quaternion PSceneComponent::GetComponentLocalRotation()const
	{
		return this->Transform.GetRotation_Local();
	}

	Vector3 PSceneComponent::GetComponentLocalScale() const
	{
		return this->Transform.GetScaling_Local();
	}

	Vector3 PSceneComponent::GetComponentWorldLocation() const
	{
		return this->Transform.GetLocation_World(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Quaternion PSceneComponent::GetComponentWorldRotation() const
	{
		return this->Transform.GetRotation_World(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentWorldScale() const
	{
		return this->Transform.GetScaling_World(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentForwardVector() const
	{
		return this->Transform.GetForwardVector_World(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentRightVector() const
	{
		return this->Transform.GetRightVector_World(this->AttachedParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentUpVector() const
	{
		return this->Transform.GetUpVector_World(this->AttachedParentComponent, this->GetOwnerActor());
	}

	// Render proxy functions START
	BOOL32 PSceneComponent::IsRenderTransformDirty()const
	{
		return RenderTransformDirty;
	}
	void PSceneComponent::CreateRenderState()
	{
		PActorComponent::CreateRenderState();
		if (ShouldRender())
		{
			//TODO
		}
	}
	void PSceneComponent::DestroyRenderState()
	{
		//TODO
		PActorComponent::DestroyRenderState();
	}
	void PSceneComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			//TODO
		}
		PActorComponent::SendUpdateRenderState();
	}
	void PSceneComponent::MarkRenderTransformAsDirty()
	{
		RenderTransformDirty = TRUE;
		MarkRenderStateAsDirty();
	}
	void PSceneComponent::MarkRenderStateAsDirty()
	{
		PActorComponent::MarkRenderStateAsDirty();
	}
	void PSceneComponent::CleanMarkRenderStateDirty()
	{
		PActorComponent::CleanMarkRenderStateDirty();
		RenderTransformDirty = FALSE;
	}
	// Render proxy functions END

};