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
	

	void PSceneComponent::AttachToComponent(PSceneComponent* Parent, const ETransform& RelativeTransform)
	{
		AttachComponentToComponent(this, Parent, RelativeTransform);
	}


	void PSceneComponent::AttachComponentToComponent(PSceneComponent* Child, PSceneComponent* Parent, const ETransform& RelativeTransform)
	{
		Check(ENGINE_COMPONENT_ERROR, "You are attaching an component to itself", Child == Parent);
		Check(ENGINE_COMPONENT_ERROR, "Something is nullptr when attaching", !Child || !Parent || !Parent->GetOwnerActor());

		
		Child->SetAttachedParentComponent(Parent);
		Child->SetOwnerActor(Parent->GetOwnerActor());
		Parent->ChildrenComponents.Add(Child);
		Child->SetComponentTransform(RelativeTransform);
	}

	PSceneComponent* PSceneComponent::GetAttachedParentComponent() const
	{
		return this->AttachedParentComponent;
	}

	void PSceneComponent::SetAttachedParentComponent(PSceneComponent* NewParent)
	{
		Check(ENGINE_COMPONENT_ERROR, "You are setting this component's attached parent to a nullptr. ", NewParent == nullptr);
		this->RemoveFromAttachedParent();
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

	void PSceneComponent::ReparentChildren(PSceneComponent* Another)
	{
		for(auto& child : this->ChildrenComponents)
		{
			child->AttachToComponent(Another);
		}
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

	void PSceneComponent::RemoveChildComponent(PSceneComponent* NewChild)
	{
		ChildrenComponents.Remove(NewChild);
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
		RemovedFromScene();
		PActorComponent::Destroy();
	}

	void PSceneComponent::BeginAddedToScene(PWorld* World)
	{
		this->CreateRenderState();
		this->MyWolrd = World;
		for(const auto& child : this->ChildrenComponents)
		{
			child->BeginAddedToScene(MyWolrd);
		}
	}

	void PSceneComponent::RemovedFromScene()
	{
		for(const auto& child : this->ChildrenComponents)
		{
			child->DestroyRenderState();
		}
		this->DestroyRenderState();
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
		Check(ENGINE_WORLD_ERROR, "PSceneComponent::SetComponentLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetLocation_Local(Location);
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentRotation(const Quaternion& Rotation)
	{
		Check(ENGINE_WORLD_ERROR, "PSceneComponent::SetComponentLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetRotation_Local(Rotation);
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentScale(const Vector3& Scale)
	{
		Check(ENGINE_WORLD_ERROR, "PSceneComponent::SetComponentLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetScaling_Local(Scale);
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentTransform(const ETransform& Trans)
	{
		Check(ENGINE_WORLD_ERROR, "PSceneComponent::SetComponentLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		SetComponentLocation(Trans.GetLocation_Local());
		SetComponentRotation(Trans.GetRotation_Local());
		SetComponentScale(Trans.GetScaling_Local());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldLocation(const Vector3& Location)
	{
		Transform.SetLocation_World(Location, this->AttachedParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldRotation(const Quaternion& Rotation)
	{
		Transform.SetRotation_World(Rotation, this->AttachedParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldScale(const Vector3& Scale)
	{
		Transform.SetScaling_World(Scale, this->AttachedParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldTransform(const ETransform& Trans)
	{
		SetComponentWorldLocation(Trans.GetLocation_World(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentWorldRotation(Trans.GetRotation_World(this->AttachedParentComponent, this->GetOwnerActor()));
		SetComponentWorldScale(Trans.GetScaling_World(this->AttachedParentComponent, this->GetOwnerActor()));
		MarkRenderTransformAsDirty();
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
		RenderTransformDirty = FALSE;
		PActorComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

};