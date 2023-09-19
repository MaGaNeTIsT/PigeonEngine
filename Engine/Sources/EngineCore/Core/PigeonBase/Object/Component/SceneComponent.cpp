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

	void PSceneComponent::DetachFromParentComponent()
	{
		if(this->ParentComponent)
		{
			this->ParentComponent->ChildrenComponents.Remove(this);
		}
		this->ParentComponent = nullptr;
	}


	void PSceneComponent::AttachComponentToComponent(PSceneComponent* Child, PSceneComponent* Parent, const ETransform& RelativeTransform)
	{
		Check(ENGINE_COMPONENT_ERROR, "You are attaching an component to itself", Child != Parent);
		Check(ENGINE_COMPONENT_ERROR, "Child is nullptr when attaching", Child );
		Check(ENGINE_COMPONENT_ERROR, "Parent is nullptr when attaching", Parent);
		Check(ENGINE_COMPONENT_ERROR, "Parent->GetOwnerActor() is nullptr when attaching", Parent->GetOwnerActor());

		// Remove from current parent
		Child->DetachFromParentComponent();
		Child->ParentComponent = Parent;
		
		Parent->ChildrenComponents.Add(Child);

		if(!Child->GetOwnerActor())
		{
			Child->SetOwnerActor(Parent->GetOwnerActor());
		}
		
		Child->SetComponentTransform(RelativeTransform);
		
	}

	PSceneComponent* PSceneComponent::GetParentComponent() const
	{
		return this->ParentComponent;
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
		DetachFromParentComponent();
		ClearChildren();
		RemovedFromScene();
		PActorComponent::Destroy();
	}

	void PSceneComponent::FixedTick(FLOAT deltaTime)
	{
		PActorComponent::FixedTick(deltaTime);
		if(IsRenderStateDirty())
		{
			SendUpdateRenderState();
		}
	}

	void PSceneComponent::EditorTick(FLOAT deltaTime)
	{

		PActorComponent::EditorTick(deltaTime);
		if(IsRenderStateDirty())
		{
			SendUpdateRenderState();
		}
	}

	void PSceneComponent::BeginAddedToScene(PWorld* World)
	{
		PActorComponent::BeginAddedToScene(World);
		this->CreateRenderState();
		for(const auto& child : this->ChildrenComponents)
		{
			child->BeginAddedToScene(MyWolrd);
		}
		
	}

	void PSceneComponent::RemovedFromScene()
	{
		
		for(const auto& child : this->ChildrenComponents)
		{
			child->RemovedFromScene();
		}
		
		this->DestroyRenderState();
		PActorComponent::RemovedFromScene();
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
	const ETransform& PSceneComponent::GetTransform()const
	{
		return Transform;
	}

	ETransform PSceneComponent::GetWorldTransform() const
	{
		return ETransform(GetComponentWorldLocation(), GetComponentWorldRotation(), GetComponentWorldScale());
	}

	void PSceneComponent::SetComponentLocation(const Vector3& Location)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetLocation_Local(Location);
		
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentRotation(const Quaternion& Rotation)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentRotation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetRotation_Local(Rotation);
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentScale(const Vector3& Scale)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentScale : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetScaling_Local(Scale);
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentTransform(const ETransform& Trans)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentTransform : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		SetComponentLocation(Trans.GetLocation_Local());
		SetComponentRotation(Trans.GetRotation_Local());
		SetComponentScale(Trans.GetScaling_Local());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldLocation(const Vector3& Location)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);

		Transform.SetLocation_World(Location, this->ParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldRotation(const Quaternion& Rotation)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldRotation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetRotation_World(Rotation, this->ParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldScale(const Vector3& Scale)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldScale : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetScaling_World(Scale, this->ParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
	}

	void PSceneComponent::SetComponentWorldTransform(const ETransform& Trans)
	{
		Check(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldTransform : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		SetComponentWorldLocation(Trans.GetLocation_World(this->ParentComponent, this->GetOwnerActor()));
		SetComponentWorldRotation(Trans.GetRotation_World(this->ParentComponent, this->GetOwnerActor()));
		SetComponentWorldScale(Trans.GetScaling_World(this->ParentComponent, this->GetOwnerActor()));
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
		return this->Transform.GetLocation_World(this->ParentComponent, this->GetOwnerActor());
	}

	Quaternion PSceneComponent::GetComponentWorldRotation() const
	{
		return this->Transform.GetRotation_World(this->ParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentWorldScale() const
	{
		return this->Transform.GetScaling_World(this->ParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentForwardVector() const
	{
		return this->Transform.GetForwardVector_World(this->ParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentRightVector() const
	{
		return this->Transform.GetRightVector_World(this->ParentComponent, this->GetOwnerActor());
	}

	Vector3 PSceneComponent::GetComponentUpVector() const
	{
		return this->Transform.GetUpVector_World(this->ParentComponent, this->GetOwnerActor());
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