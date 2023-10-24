#include "./SceneComponent.h"

#include <PigeonBase/Object/Actor.h>
#include "../../../../IO/SerializationHelper.h"
#include "../World/World.h"
namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSceneComponent, PActorComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PSceneComponent::PSceneComponent()
	{
		POBJ_DEBUGNAME_SET(this, "SceneComponent");
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
		PE_CHECK(ENGINE_COMPONENT_ERROR, "You are attaching an component to itself", Child != Parent);
		PE_CHECK(ENGINE_COMPONENT_ERROR, "Child is nullptr when attaching", Child );
		PE_CHECK(ENGINE_COMPONENT_ERROR, "Parent is nullptr when attaching", Parent);
		PE_CHECK(ENGINE_COMPONENT_ERROR, "Parent->GetOwnerActor() is nullptr when attaching", Parent->GetOwnerActor());

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
			
			(*elem)->ParentComponent = nullptr;
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
		NewChild->ParentComponent = nullptr;
		
	}

	TSet<PSceneComponent*> PSceneComponent::GetChildrenComponents() const
	{
		return this->ChildrenComponents;
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
		DestroyRenderState();
		DetachFromParentComponent();
		ClearChildren();
		PActorComponent::Destroy();
	}

	void PSceneComponent::FixedTick(FLOAT deltaTime)
	{
		PActorComponent::FixedTick(deltaTime);
		for(const auto& elem : ChildrenComponents)
		{
			elem->FixedTick(deltaTime);
		}
		// this->TickRender();
	}

	void PSceneComponent::EditorTick(FLOAT deltaTime)
	{
		PActorComponent::EditorTick(deltaTime);
		for(const auto& elem : ChildrenComponents)
        {
        	elem->EditorTick(deltaTime);
        }
		// this->TickRender();
	}

	void PSceneComponent::TickRender()
	{
		if(IsRenderStateDirty())
		{
			SendUpdateRenderState();
		}
		for(const auto& elem : ChildrenComponents)
		{
			elem->TickRender();
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
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetLocation_Local(Location);
		
		MarkRenderTransformAsDirty();
		for (const auto& elem : ChildrenComponents)
		{
			elem->MarkRenderTransformAsDirty();
		}
	}

	void PSceneComponent::SetComponentRotation(const Quaternion& Rotation)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentRotation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetRotation_Local(Rotation);
		MarkRenderTransformAsDirty();
		for (const auto& elem : ChildrenComponents)
		{
			elem->MarkRenderTransformAsDirty();
		}
	}

	void PSceneComponent::SetComponentScale(const Vector3& Scale)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentScale : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetScaling_Local(Scale);
		MarkRenderTransformAsDirty();
		for (const auto& elem : ChildrenComponents)
		{
			elem->MarkRenderTransformAsDirty();
		}
	}

	void PSceneComponent::SetComponentTransform(const ETransform& Trans)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentTransform : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		SetComponentLocation(Trans.GetLocation_Local());
		SetComponentRotation(Trans.GetRotation_Local());
		SetComponentScale(Trans.GetScaling_Local());
	}

	void PSceneComponent::SetComponentWorldLocation(const Vector3& Location)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);

		Transform.SetLocation_World(Location, this->ParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
		for (const auto& elem : ChildrenComponents)
		{
			elem->MarkRenderTransformAsDirty();
		}
	}

	void PSceneComponent::SetComponentWorldRotation(const Quaternion& Rotation)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldRotation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetRotation_World(Rotation, this->ParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
		for (const auto& elem : ChildrenComponents)
		{
			elem->MarkRenderTransformAsDirty();
		}
	}

	void PSceneComponent::SetComponentWorldScale(const Vector3& Scale)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldScale : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetScaling_World(Scale, this->ParentComponent, this->GetOwnerActor());
		MarkRenderTransformAsDirty();
		for (const auto& elem : ChildrenComponents)
		{
			elem->MarkRenderTransformAsDirty();
		}
	}

	void PSceneComponent::SetComponentWorldTransform(const ETransform& Trans)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldTransform : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		SetComponentWorldLocation(Trans.GetLocation_Local());
		SetComponentWorldRotation(Trans.GetRotation_Local());
		SetComponentWorldScale(Trans.GetScaling_Local());
		
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
		return this->Transform.GetLocation_World(this->ParentComponent, this->GetOwnerActor() ? this->GetOwnerActor()->GetAttachedParentActor() : nullptr);
	}

	Quaternion PSceneComponent::GetComponentWorldRotation() const
	{
		return this->Transform.GetRotation_World(this->ParentComponent, this->GetOwnerActor() ? this->GetOwnerActor()->GetAttachedParentActor() : nullptr);
	}

	Vector3 PSceneComponent::GetComponentWorldScale() const
	{
		return this->Transform.GetScaling_World(this->ParentComponent, this->GetOwnerActor() ? this->GetOwnerActor()->GetAttachedParentActor() : nullptr);
	}

	Vector3 PSceneComponent::GetComponentForwardVector() const
	{
		return this->Transform.GetForwardVector_World(this->ParentComponent, this->GetOwnerActor() ? this->GetOwnerActor()->GetAttachedParentActor() : nullptr);
	}

	Vector3 PSceneComponent::GetComponentRightVector() const
	{
		return this->Transform.GetRightVector_World(this->ParentComponent, this->GetOwnerActor() ? this->GetOwnerActor()->GetAttachedParentActor() : nullptr);
	}

	Vector3 PSceneComponent::GetComponentUpVector() const
	{
		return this->Transform.GetUpVector_World(this->ParentComponent, this->GetOwnerActor() ? this->GetOwnerActor()->GetAttachedParentActor() : nullptr);
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

	static FLOAT LocX;
	static FLOAT LocY;
	static FLOAT LocZ;

	static Euler Rot;
	static FLOAT ScaleX;
	static FLOAT ScaleY;
	static FLOAT ScaleZ;

	void PSceneComponent::GenerateComponentOutline(const PActorComponent* WorldCurrentSelectedComponent)
	{
		BOOL8 bSelectedComponent = WorldCurrentSelectedComponent == this;
		
		ImGuiTreeNodeFlags TreeNodeFlag = (ChildrenComponents.Length() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf) | ImGuiTreeNodeFlags_DefaultOpen;
		BOOL8 bTreeNodeExpand = ImGui::TreeNodeEx(*(EString("##") + (POBJ_DEBUGNAME_GET(this)) + EString("_TreeNode")), TreeNodeFlag);

		ImGui::SameLine();
		BOOL8 bSelected = ImGui::Selectable(*(POBJ_DEBUGNAME_GET(this)), &bSelectedComponent);

		if (bTreeNodeExpand)
		{
			// child actors
			for (const auto& elem : ChildrenComponents)
			{
				elem->GenerateComponentOutline(WorldCurrentSelectedComponent);
			}
			ImGui::TreePop();
		}
		
		if (bSelected)
		{
			this->GetOwnerActor()->GetWorld()->SetSelectedComponent(this);
		}
	}

	void PSceneComponent::GenerateComponentDetail()
	{
		BOOL8 bTransformExpand = ImGui::TreeNodeEx("Transform") ;
		if (bTransformExpand)
		{
			if (ImGui::TreeNodeEx("Location"))
			{
				const BOOL8 bChangedX = ImGui::DragScalar("##LocX", ImGuiDataType_Float, &LocX, 0.05f, NULL, NULL, "X : %.6f");
				const BOOL8 bChangedY = ImGui::DragScalar("##LocY", ImGuiDataType_Float, &LocY, 0.05f, NULL, NULL, "Y : %.6f");
				const BOOL8 bChangedZ = ImGui::DragScalar("##LocZ", ImGuiDataType_Float, &LocZ, 0.05f, NULL, NULL, "Z : %.6f");
				if (bChangedX || bChangedY || bChangedZ)
				{
					this->SetComponentLocation(Vector3(LocX, LocY, LocZ));
					
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Rotation"))
			{
				const FLOAT Min = -180.0f;
				const FLOAT Max = 180.0f;
				
				const BOOL8 bChangedRoll  = ImGui::DragScalar("##Roll",  ImGuiDataType_Float, &Rot.Roll, 0.05f, &Min, &Max, "Roll : %.6f");
				const BOOL8 bChangedPitch = ImGui::DragScalar("##Pitch", ImGuiDataType_Float, &Rot.Pitch, 0.05f, &Min, &Max, "Pitch : %.6f");
				const BOOL8 bChangedYaw   = ImGui::DragScalar("##Yaw",   ImGuiDataType_Float, &Rot.Yaw,   0.05f, &Min, &Max, "Yaw : %.6f");

				if (bChangedRoll || bChangedPitch || bChangedYaw)
				{
					this->SetComponentRotation(MakeQuaternion(Euler(Rot.Pitch, Rot.Yaw, Rot.Roll)));
					// Rot = MakeEuler(this->GetComponentLocalRotation());
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Scale"))
			{
				BOOL8 bChangedX = ImGui::DragScalar("##ScaleX", ImGuiDataType_Float, &ScaleX, 0.05f, NULL, NULL, "X : %.6f");
				BOOL8 bChangedY = ImGui::DragScalar("##ScaleY", ImGuiDataType_Float, &ScaleY, 0.05f, NULL, NULL, "Y : %.6f");
				BOOL8 bChangedZ = ImGui::DragScalar("##ScaleZ", ImGuiDataType_Float, &ScaleZ, 0.05f, NULL, NULL, "Z : %.6f");

				if (bChangedX || bChangedY || bChangedZ)
				{
					this->SetComponentScale(Vector3(ScaleX, ScaleY, ScaleZ));
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		
	}

	void PSceneComponent::OnSelectedByImGui()
	{
		LocX = this->GetComponentLocalLocation().x;
		LocY = this->GetComponentLocalLocation().y;
		LocZ = this->GetComponentLocalLocation().z;
		
		Rot = MakeEuler(this->GetComponentLocalRotation());

		ScaleX = this->GetComponentLocalScale().x;
		ScaleY = this->GetComponentLocalScale().y;
		ScaleZ = this->GetComponentLocalScale().z;
	}

};