#include "./SceneComponent.h"

#include <PigeonBase/Object/Actor.h>
#include "../../../../IO/SerializationHelper.h"
#include "../World/World.h"
#include <Editor/EditorLogManager.h>

#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#endif

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSceneComponent, PActorComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PSceneComponent::PSceneComponent()
		: PActorComponent()
	{
		POBJ_DEBUGNAME_SET(this, "SceneComponent");
	}

	PSceneComponent::~PSceneComponent()
	{
	}


	void PSceneComponent::AttachToComponent(PSceneComponent* Parent, const ETransform& RelativeTransform)
	{
		AttachComponentToComponent(this, Parent, EAttachmentRuleType::KEEP_CURRENT, RelativeTransform);
	}

	void PSceneComponent::AttachToComponent(PSceneComponent* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform)
	{
		AttachComponentToComponent(this, Parent, InRule, RelativeTransform);
	}

	void PSceneComponent::DetachFromParentComponent()
	{
		if(this->ParentComponent)
		{
			this->ParentComponent->ChildrenComponents.Remove(this);
		}
		this->ParentComponent = nullptr;
		// The local transform is kept, so the world one just changed for this subtree.
		MarkTransformDirtyRecursive();
	}


	void PSceneComponent::AttachComponentToComponent(PSceneComponent* Child, PSceneComponent* Parent, const ETransform& RelativeTransform)
	{
		// Historic overload: keep whatever local transform the child already has.
		AttachComponentToComponent(Child, Parent, EAttachmentRuleType::KEEP_CURRENT, RelativeTransform);
	}

	void PSceneComponent::AttachComponentToComponent(PSceneComponent* Child, PSceneComponent* Parent, EAttachmentRuleType InRule, const ETransform& RelativeTransform)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "You are attaching an component to itself", Child != Parent);
		PE_CHECK(ENGINE_COMPONENT_ERROR, "Child is nullptr when attaching", Child );
		PE_CHECK(ENGINE_COMPONENT_ERROR, "Parent is nullptr when attaching", Parent);
		PE_CHECK(ENGINE_COMPONENT_ERROR, "Parent->GetOwnerActor() is nullptr when attaching", Parent->GetOwnerActor());

		// Cycle check: transform composition recurses the parent chain, so attaching a
		// component under its own descendant is an infinite loop, not a misplacement.
		// Refused with a warning - not a check - because it is recoverable (the engine
		// keeps the current hierarchy) and the framework sanity check exercises this
		// exact path.
		for (PSceneComponent* Ancestor = Parent; Ancestor; Ancestor = Ancestor->GetParentComponent())
		{
			if (Ancestor == Child)
			{
				PE_LOG_WARN(EString(PE_TEXT("AttachComponentToComponent: refused, the new parent is a descendant of the child.")));
				return;
			}
		}

		// KEEP_WORLD needs the child's world matrix before the parent chain changes.
		Matrix4x4 ChildWorldMatrix = Matrix4x4::Identity();
		if (InRule == EAttachmentRuleType::KEEP_WORLD)
		{
			ChildWorldMatrix = Child->GetCachedLocalToWorldMatrix();
		}

		// Remove from current parent
		Child->DetachFromParentComponent();
		Child->ParentComponent = Parent;

		Parent->ChildrenComponents.Add(Child);

		if(!Child->GetOwnerActor())
		{
			Child->SetOwnerActor(Parent->GetOwnerActor());
		}

		switch (InRule)
		{
		case EAttachmentRuleType::KEEP_CURRENT:
			// The local transform stays exactly as it was; the world one follows the
			// new parent. This is what every existing attach call site expects.
			break;
		case EAttachmentRuleType::KEEP_WORLD:
		{
			// Row-vector convention: World = Local * ParentWorld, so Local = World * ParentWorld⁻¹.
			const Matrix4x4 ParentWorld = Parent->GetCachedLocalToWorldMatrix();
			Child->Transform = ETransform::FromMatrix(ChildWorldMatrix * ParentWorld.Inverse());
			break;
		}
		case EAttachmentRuleType::KEEP_RELATIVE:
			Child->Transform = RelativeTransform;
			break;
		case EAttachmentRuleType::SNAP_TO_TARGET:
			Child->Transform = ETransform();
			break;
		}

		// Any rule except KEEP_CURRENT changed either the local transform or the parent
		// chain; in all cases the subtree has to re-compose and re-submit.
		Child->MarkTransformDirtyRecursive();

		// A live attach into an in-scene actor must bring the subtree into the scene too.
		// During AddActor the owner actor is not in scene yet, so this stays out of that
		// path and the normal Init/BeginAddedToScene sequence still owns it.
		PActor* OwnerActor = Parent->GetOwnerActor();
		if (OwnerActor && OwnerActor->IsAddedToScene() && (!Child->IsAddedToScene()))
		{
			if (!Child->IsInitialized())
			{
				Child->Init();
			}
			Child->OnAddedToScene(OwnerActor->GetWorld());
		}
	}

	PSceneComponent* PSceneComponent::GetParentComponent() const
	{
		return this->ParentComponent;
	}

	void PSceneComponent::ClearChildren()
	{
		// Snapshot first: destroying a child detaches it from this live list, and walking
		// the list while its rows disappear would skip entries.
		TArray<PSceneComponent*> ChildrenSnapshot = ChildrenComponents;
		for (UINT32 Index = 0u, Num = ChildrenSnapshot.Num<UINT32>(); Index < Num; Index++)
		{
			PSceneComponent* Child = ChildrenSnapshot[Index];
			if (!Child)
			{
				continue;
			}
			// Nulled before the call so the dying child does not reach back into this
			// mid-destroy parent through its own DetachFromParentComponent.
			Child->ParentComponent = nullptr;
			Child->Destroy();
		}
		ChildrenComponents.Empty();
	}

	TSharedPtr<CJsonObject> PSceneComponent::Serialize()
	{
		TSharedPtr<CJsonObject> JsonObj =  PActorComponent::Serialize();

		TSharedPtr<CJsonObject> TransFormJson = ESerializationHelper::SerializeTransform(Transform);
		JsonObj->SetObjectField("Transform", EMemory::GetPtr(TransFormJson));
		return JsonObj;
	}

	void PSceneComponent::RemoveChildComponent(PSceneComponent* NewChild)
	{
		ChildrenComponents.Remove(NewChild);
		NewChild->ParentComponent = nullptr;

	}

	const PSceneComponent* PSceneComponent::GetComponentByUniqueID(const ObjectIdentityType& InUniqueID,
		const BOOL8& bIncludeChildComponent) const
	{
		if(this->GetUniqueID() == InUniqueID)
		{
			return this;
		}
		if(!bIncludeChildComponent)
		{
			return nullptr;
		}
		for (PSceneComponent* Component : ChildrenComponents)
		{
			const PSceneComponent* Ret = Component->GetComponentByUniqueID(InUniqueID, bIncludeChildComponent);
			if(Ret)
			{
				return Ret;
			}
		}
		return nullptr;
	}

	TArray<PSceneComponent*> PSceneComponent::GetChildrenComponents() const
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

	void PSceneComponent::OnDestroy()
	{
		// Backstop only: scene exit (RemovedFromScene) already released the state while
		// the world pointer was still valid. This covers a component destroyed while
		// still flagged as scene-registered.
		if (bRenderStateCreated)
		{
			DestroyRenderState();
			bRenderStateCreated = FALSE;
		}
		DetachFromParentComponent();
		ClearChildren();
	}

	void PSceneComponent::Tick(FLOAT deltaTime)
	{
		PActorComponent::Tick(deltaTime);
	}

	void PSceneComponent::FixedTick(FLOAT deltaTime)
	{
		PActorComponent::FixedTick(deltaTime);
		// this->TickRender();
	}

#if ENGINE_EDITOR_ONLY
	void PSceneComponent::EditorTick(FLOAT deltaTime)
	{
		PActorComponent::EditorTick(deltaTime);
		// this->TickRender();
	}
#endif

	void PSceneComponent::AddDeferredApply(TFunction<BOOL32()> InStep)
	{
		if (!InStep.IsValid())
		{
			return;
		}
		DeferredApplies.Add(static_cast<TFunction<BOOL32()>&&>(InStep));
	}
	void PSceneComponent::RunDeferredApplies()
	{
		// Walked backwards so a step that finishes and removes itself does not shift the ones after it.
		for (INT32 StepIndex = static_cast<INT32>(DeferredApplies.Num<UINT32>()) - 1; StepIndex >= 0; StepIndex--)
		{
			if (DeferredApplies[static_cast<UINT32>(StepIndex)]())
			{
				DeferredApplies.RemoveAt(static_cast<UINT32>(StepIndex));
			}
		}
	}

	void PSceneComponent::TickRender()
	{
		// Deferred applies first: a step that resolves its asset sets it on this component, and the
		// dirty mark that follows is what the implementation below then sends.
		if (DeferredApplies.Num<UINT32>() > 0u)
		{
			RunDeferredApplies();
		}
		TickRenderImpl();
	}
	void PSceneComponent::TickRenderImpl()
	{
		if(IsRenderStateDirty())
		{
			SendUpdateRenderState();
		}
		for (PSceneComponent* Component : ChildrenComponents)
		{
			if(Component->GetOwnerActor() == this->GetOwnerActor())
			{
				Component->TickRender();
			}
		}
	}

	void PSceneComponent::BeginAddedToScene(PWorld* World)
	{
		PActorComponent::BeginAddedToScene(World);
		this->CreateRenderState();
		for (PSceneComponent* Child : this->ChildrenComponents)
		{
			Child->BeginAddedToScene(World);
		}
	}

	void PSceneComponent::RemovedFromScene()
	{
		// Scene exit releases the render state HERE, while this component's world pointer
		// is still set (PObject::RemovedFromScene clears it only after this override
		// returns). The typed render paths - camera, lights - hand their proxies back
		// through the world's render scene, so releasing the state any later, at OnDestroy
		// after a deferred-destroy mark already cleared the world, skipped the removal
		// entirely and left the proxy alive past its component (the camera's destructor
		// check caught exactly that). Ending the state at exit is also the semantic a
		// remove-without-destroy needs: an actor that leaves the world stops rendering.
		if (bRenderStateCreated)
		{
			DestroyRenderState();
			bRenderStateCreated = FALSE;
		}
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

	BOOL32 PSceneComponent::ContainTransform() const
	{
		return TRUE;
	}
	const ETransform& PSceneComponent::GetTransform() const
	{
		return Transform;
	}

	ETransform PSceneComponent::GetWorldTransform() const
	{
		return GetCachedWorldTransform();
	}

	const Matrix4x4& PSceneComponent::GetCachedLocalToWorldMatrix() const
	{
		UpdateWorldTransformCache();
		return CachedLocalToWorld;
	}

	const ETransform& PSceneComponent::GetCachedWorldTransform() const
	{
		UpdateWorldTransformCache();
		return CachedWorldTransform;
	}

	void PSceneComponent::UpdateWorldTransformCache() const
	{
		if (!bWorldTransformDirty)
		{
			return;
		}
		const Matrix4x4 LocalMatrix = Transform.ToMatrix();
		if (ParentComponent)
		{
			// Row-vector convention: the child's transform is applied before the parent's,
			// so the local matrix sits on the LEFT of the parent world matrix.
			CachedLocalToWorld = LocalMatrix * ParentComponent->GetCachedLocalToWorldMatrix();
		}
		else
		{
			CachedLocalToWorld = LocalMatrix;
		}
		CachedWorldTransform = ETransform::FromMatrix(CachedLocalToWorld);
		bWorldTransformDirty = FALSE;
	}

	void PSceneComponent::MarkWorldTransformDirty()
	{
		bWorldTransformDirty = TRUE;
		for (PSceneComponent* Component : ChildrenComponents)
		{
			Component->MarkWorldTransformDirty();
		}
	}

	void PSceneComponent::MarkTransformDirtyRecursive()
	{
		MarkRenderTransformAsDirty();
		bWorldTransformDirty = TRUE;
		for (PSceneComponent* Component : ChildrenComponents)
		{
			Component->MarkTransformDirtyRecursive();
		}
	}

	void PSceneComponent::SetWorldTransformPreservingParent(const ETransform& InWorldTransform)
	{
		// Row-vector convention: World = Local * ParentWorld, so Local = World * ParentWorld⁻¹.
		const Matrix4x4 WorldMatrix = InWorldTransform.ToMatrix();
		const Matrix4x4 ParentWorld = ParentComponent ? ParentComponent->GetCachedLocalToWorldMatrix() : Matrix4x4::Identity();
		Transform = ETransform::FromMatrix(WorldMatrix * ParentWorld.Inverse());
		MarkTransformDirtyRecursive();
	}

	void PSceneComponent::SetComponentLocation(const Vector3& Location)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetLocation_Local(Location);
		MarkTransformDirtyRecursive();
	}

	void PSceneComponent::SetComponentRotation(const Quaternion& Rotation)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentRotation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetRotation_Local(Rotation);
		MarkTransformDirtyRecursive();
	}

	void PSceneComponent::SetComponentScale(const Vector3& Scale)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentScale : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform.SetScaling_Local(Scale);
		MarkTransformDirtyRecursive();
	}

	void PSceneComponent::SetComponentTransform(const ETransform& Trans)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentTransform : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		Transform = Trans;
		MarkTransformDirtyRecursive();
	}

	void PSceneComponent::SetComponentWorldLocation(const Vector3& Location)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldLocation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		const ETransform CurrentWorld = GetCachedWorldTransform();
		SetWorldTransformPreservingParent(ETransform(Location, CurrentWorld.GetRotation_Local(), CurrentWorld.GetScaling_Local()));
	}

	void PSceneComponent::SetComponentWorldRotation(const Quaternion& Rotation)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldRotation : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		const ETransform CurrentWorld = GetCachedWorldTransform();
		SetWorldTransformPreservingParent(ETransform(CurrentWorld.GetLocation_Local(), Rotation, CurrentWorld.GetScaling_Local()));
	}

	void PSceneComponent::SetComponentWorldScale(const Vector3& Scale)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldScale : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		const ETransform CurrentWorld = GetCachedWorldTransform();
		SetWorldTransformPreservingParent(ETransform(CurrentWorld.GetLocation_Local(), CurrentWorld.GetRotation_Local(), Scale));
	}

	void PSceneComponent::SetComponentWorldTransform(const ETransform& Trans)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "PSceneComponent::SetComponentWorldTransform : this is not a dynamic component.", GetMobility() == EMobilityType::EMT_DYNAMIC);
		SetWorldTransformPreservingParent(Trans);
	}

	Vector3 PSceneComponent::GetComponentLocalLocation() const
	{
		return this->Transform.GetLocation_Local();
	}

	Quaternion PSceneComponent::GetComponentLocalRotation() const
	{
		return this->Transform.GetRotation_Local();
	}

	Vector3 PSceneComponent::GetComponentLocalScale() const
	{
		return this->Transform.GetScaling_Local();
	}

	Vector3 PSceneComponent::GetComponentWorldLocation() const
	{
		return GetCachedWorldTransform().GetLocation_Local();
	}

	Quaternion PSceneComponent::GetComponentWorldRotation() const
	{
		return GetCachedWorldTransform().GetRotation_Local();
	}

	Vector3 PSceneComponent::GetComponentWorldScale() const
	{
		return GetCachedWorldTransform().GetScaling_Local();
	}

	Vector3 PSceneComponent::GetComponentForwardVector() const
	{
		return GetCachedWorldTransform().GetForwardVector_Local();
	}

	Vector3 PSceneComponent::GetComponentRightVector() const
	{
		return GetCachedWorldTransform().GetRightVector_Local();
	}

	Vector3 PSceneComponent::GetComponentUpVector() const
	{
		return GetCachedWorldTransform().GetUpVector_Local();
	}

	PE_NODISCARD const EBoundAABB& PSceneComponent::GetLocalBound() const
	{
		return LocalBound;
	}
	void PSceneComponent::SetLocalBound(const EBoundAABB& InNewBound)
	{
		LocalBound = InNewBound;
	}

	const EBoundAABB PSceneComponent::GetComponentBound() const
	{
		EBoundAABB ResultBound(this->GetLocalBound());

		for (PSceneComponent* ChildComponent : this->ChildrenComponents)
		{
			const Vector3		TempLocalLocation(ChildComponent->GetComponentLocalLocation());
			const Quaternion	TempLocalRotation(ChildComponent->GetComponentLocalRotation());
			const Vector3		TempLocalScaling(ChildComponent->GetComponentLocalScale());
			EBoundAABB			TempLocalBound = ChildComponent->GetComponentBound();

			TArray<Vector3> TempBoundPoints;
			TempLocalBound.CalculateBoundWithSpace(
				TempLocalLocation,
				TempLocalRotation,
				TempLocalScaling,
				TempBoundPoints
			);

			for (INT32 i = 0, n = TempBoundPoints.Num(); i < n; i++)
			{
				ResultBound.AABBMin = MinVector3(TempBoundPoints[i], ResultBound.AABBMin);
				ResultBound.AABBMax = MaxVector3(TempBoundPoints[i], ResultBound.AABBMax);
			}
		}

		return ResultBound;
	}

	// Render proxy functions START
	BOOL32 PSceneComponent::IsRenderTransformDirty() const
	{
		return RenderTransformDirty;
	}
	void PSceneComponent::CreateRenderState()
	{
		PActorComponent::CreateRenderState();
		// Every subclass chain routes through here on the way in (each override calls its
		// base first), so this is the one place that knows a render state exists.
		bRenderStateCreated = TRUE;
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
#if ENGINE_EDITOR_ONLY
	static Vector3 Location;
	static Euler   Rotation;
	static Vector3 Scale;

	void PSceneComponent::GenerateComponentOutline(const PActorComponent* WorldCurrentSelectedComponent)
	{
		BOOL8 bSelectedComponent = WorldCurrentSelectedComponent == this;

		const ImGuiTreeNodeFlags TreeNodeFlag = (ChildrenComponents.Num() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf) | ImGuiTreeNodeFlags_DefaultOpen;
		ImGui::PushID((void*)this);
		const BOOL8 bTreeNodeExpand = ImGui::TreeNodeEx("##node", TreeNodeFlag);
		ImGui::SameLine();
		const BOOL8 bSelected = ImGui::Selectable(*(POBJ_DEBUGNAME_GET(this)), &bSelectedComponent);
		ImGui::PopID();

		if (bTreeNodeExpand)
		{
			// child actors
			for (PSceneComponent* Component : ChildrenComponents)
			{
				Component->GenerateComponentOutline(WorldCurrentSelectedComponent);
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
		Location = this->GetComponentLocalLocation();
		Rotation = MakeEuler(this->GetComponentLocalRotation());
		Scale    = this->GetComponentLocalScale();
		const BOOL8 bTransformExpand = ImGui::TreeNodeEx("Transform") ;
		if (bTransformExpand)
		{
			if (ImGui::TreeNodeEx("Location"))
			{
				const BOOL8 bChangedX = ImGui::DragScalar("##LocX", ImGuiDataType_Float, &Location.x, 0.05f, NULL, NULL, "X : %.6f");
				const BOOL8 bChangedY = ImGui::DragScalar("##LocY", ImGuiDataType_Float, &Location.y, 0.05f, NULL, NULL, "Y : %.6f");
				const BOOL8 bChangedZ = ImGui::DragScalar("##LocZ", ImGuiDataType_Float, &Location.z, 0.05f, NULL, NULL, "Z : %.6f");
				const BOOL8 bRest     = ImGui::Button("Reset");
				if (bChangedX || bChangedY || bChangedZ)
				{
					this->SetComponentLocation(Location);
				}
				if(bRest)
				{
					Location = Vector3::Zero();
					this->SetComponentLocation(Location);
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Rotation"))
			{
				const FLOAT Min = -180.0f;
				const FLOAT Max = 180.0f;

				const BOOL8 bChangedPitch = ImGui::DragScalar("##Pitch", ImGuiDataType_Float, &Rotation.Pitch, 0.05f, &Min, &Max, "Pitch : %.6f");
				const BOOL8 bChangedYaw   = ImGui::DragScalar("##Yaw",   ImGuiDataType_Float, &Rotation.Yaw,   0.05f, &Min, &Max, "Yaw   : %.6f");
				const BOOL8 bChangedRoll  = ImGui::DragScalar("##Roll",  ImGuiDataType_Float, &Rotation.Roll,  0.05f, &Min, &Max, "Roll  : %.6f");
				const BOOL8 bRest         = ImGui::Button("Reset");
				if (bChangedRoll || bChangedPitch || bChangedYaw)
				{
					this->SetComponentRotation(MakeQuaternion(Rotation));
				}

				if(bRest)
				{
					Rotation = Euler::Zero();
					this->SetComponentRotation(MakeQuaternion(Rotation));
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Scale"))
			{
				const BOOL8 bChangedX = ImGui::DragScalar("##ScaleX", ImGuiDataType_Float, &Scale.x, 0.05f, NULL, NULL, "X : %.6f");
				const BOOL8 bChangedY = ImGui::DragScalar("##ScaleY", ImGuiDataType_Float, &Scale.y, 0.05f, NULL, NULL, "Y : %.6f");
				const BOOL8 bChangedZ = ImGui::DragScalar("##ScaleZ", ImGuiDataType_Float, &Scale.z, 0.05f, NULL, NULL, "Z : %.6f");
				const BOOL8 bRest     = ImGui::Button("Reset");

				if (bChangedX || bChangedY || bChangedZ)
				{
					this->SetComponentScale(Scale);
				}
				if(bRest)
				{
					Scale = Vector3::One();
					this->SetComponentScale(Scale);
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}

	void PSceneComponent::OnSelectedByImGui()
	{
		Location = this->GetComponentLocalLocation();
		Rotation = MakeEuler(this->GetComponentLocalRotation());
		Scale    = this->GetComponentLocalScale();
	}
#endif
};
