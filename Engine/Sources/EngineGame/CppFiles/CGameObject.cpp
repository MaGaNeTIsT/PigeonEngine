#include "../Headers/CGameObject.h"
#include "../Headers/CComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../Headers/CScene.h"

CGameObject::CGameObject()
{
	this->m_Active						= FALSE;
	this->m_Scene						= NULL;
	this->m_Transform					= NULL;
	this->m_RenderBoundingBox			= NULL;
	this->m_RenderBoundingSphere		= NULL;
	this->m_MeshComponentID				= 0u;
	this->m_MeshRendererComponentID		= 0u;
	this->m_GameBoundComponentID		= 0u;
}
CGameObject::~CGameObject()
{
	if (this->HasTransform())
	{
		this->RemoveParent();
		this->RemoveChildren();
		delete (this->m_Transform);
		this->m_Transform = NULL;
	}
	if (this->HasRenderBoundingBox())
	{
		delete (this->m_RenderBoundingBox);
		this->m_RenderBoundingBox = NULL;
	}
	if (this->HasRenderBoundingSphere())
	{
		delete (this->m_RenderBoundingSphere);
		this->m_RenderBoundingSphere = NULL;
	}
	if (this->HasComponent())
	{
		this->RemoveComponents();
	}
}
const CScene* CGameObject::GetScene()const
{
	return (this->m_Scene);
}
const CTransform* CGameObject::GetTransform()const
{
	return (this->m_Transform);
}
const CustomStruct::CRenderBoundingBox* CGameObject::GetRenderLocalBoundingBox()const
{
	return (this->m_RenderBoundingBox);
}
const CustomStruct::CRenderBoundingSphere* CGameObject::GetRenderLocalBoundingSphere()const
{
	return (this->m_RenderBoundingSphere);
}
void CGameObject::SetScene(const CScene* scene)const
{
	this->m_Scene = scene;
}
void CGameObject::AddNewTransform(const CGameObject* parent)const
{
	if (!this->HasTransform())
	{
		this->m_Transform = new CTransform();
		this->ConnectGameObjectAndTransform(this, this->m_Transform);
	}
	if (parent != NULL)
	{
		this->m_Transform->InitTransform(CustomType::Vector3::Zero());
		this->SetParent(parent);
	}
	else
	{
		this->m_Transform->InitTransform(CustomType::Vector3::Zero());
	}
}
void CGameObject::AddNewTransformWithValue(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, const CGameObject* parent)const
{
	if (!this->HasTransform())
	{
		this->m_Transform = new CTransform();
		this->ConnectGameObjectAndTransform(this, this->m_Transform);
	}
	if (parent != NULL)
	{
		this->m_Transform->InitTransform(worldPosition, worldRotation, worldScale);
		this->SetParent(parent);
	}
	else
	{
		this->m_Transform->InitTransform(worldPosition, worldRotation, worldScale);
	}
}
void CGameObject::SetRenderLocalBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions)const
{
	if (!this->HasRenderBoundingBox())
	{
		this->m_RenderBoundingBox = new CustomStruct::CRenderBoundingBox(anchor, dimensions);
	}
	else
	{
		this->m_RenderBoundingBox->Anchor = anchor;
		this->m_RenderBoundingBox->Dimensions = dimensions;
	}
}
void CGameObject::SetRenderLocalBoundingBox(const CustomType::Vector3& dimensions)const
{
	CustomType::Vector3 tempAnchor(-dimensions.X(), -dimensions.Y(), -dimensions.Z());
	tempAnchor = tempAnchor * 0.5f;
	if (!this->HasRenderBoundingBox())
	{
		this->m_RenderBoundingBox = new CustomStruct::CRenderBoundingBox(tempAnchor, dimensions);
	}
	else
	{
		this->m_RenderBoundingBox->Anchor = tempAnchor;
		this->m_RenderBoundingBox->Dimensions = dimensions;
	}
}
void CGameObject::GetRenderWorldAABBBoundingBox(CustomType::Vector3& boundingMin, CustomType::Vector3& boundingMax)const
{
	if (!this->HasRenderBoundingBox())
	{
		CustomType::Vector3 position(this->m_Transform->GetWorldPosition());
		boundingMin = position - 0.5f;
		boundingMax = position + 0.5f;
		return;
	}
	FLOAT points[24];
	{
		CustomType::Vector3 position(this->m_Transform->GetWorldPosition());
		CustomType::Vector3 scale(this->m_Transform->GetWorldScale());
		CustomType::Vector3 rightVec(this->m_Transform->GetRightVector());
		CustomType::Vector3 upVec(this->m_Transform->GetUpVector());
		CustomType::Vector3 forwardVec(this->m_Transform->GetForwardVector());

		CustomType::Vector3 tempPoint(rightVec * this->m_RenderBoundingBox->Anchor.X() * scale.X() + upVec * this->m_RenderBoundingBox->Anchor.Y() * scale.Y() + forwardVec * this->m_RenderBoundingBox->Anchor.Z() * scale.Z() + position);
		/*
		//            5--------6                Y(up vector)
		//           /|       /|                ^
		//          / |      / |  (dimension y) |
		//         4--------7  |                |    Z(forward vector)
		//         |  1-----|--2                |   /
		//         | /      | /                 |  /(dimension z)
		//         |/       |/                  | /
		// (anchor)0--------3                   0---------------->X(right vector)
		//                                         (dimension x)
		*/
		points[0u * 3u + 0u] = tempPoint.X();
		points[0u * 3u + 1u] = tempPoint.Y();
		points[0u * 3u + 2u] = tempPoint.Z();

		rightVec = rightVec * this->m_RenderBoundingBox->Dimensions.X() * scale.X();
		upVec = upVec * this->m_RenderBoundingBox->Dimensions.Y() * scale.Y();
		forwardVec = forwardVec * this->m_RenderBoundingBox->Dimensions.Z() * scale.Z();

		points[1u * 3u + 0u] = points[0u * 3u + 0u] + forwardVec.X();
		points[1u * 3u + 1u] = points[0u * 3u + 1u] + forwardVec.Y();
		points[1u * 3u + 2u] = points[0u * 3u + 2u] + forwardVec.Z();
		points[2u * 3u + 0u] = points[1u * 3u + 0u] + rightVec.X();
		points[2u * 3u + 1u] = points[1u * 3u + 1u] + rightVec.Y();
		points[2u * 3u + 2u] = points[1u * 3u + 2u] + rightVec.Z();
		points[3u * 3u + 0u] = points[0u * 3u + 0u] + rightVec.X();
		points[3u * 3u + 1u] = points[0u * 3u + 1u] + rightVec.Y();
		points[3u * 3u + 2u] = points[0u * 3u + 2u] + rightVec.Z();
		points[4u * 3u + 0u] = points[0u * 3u + 0u] + upVec.X();
		points[4u * 3u + 1u] = points[0u * 3u + 1u] + upVec.Y();
		points[4u * 3u + 2u] = points[0u * 3u + 2u] + upVec.Z();
		points[5u * 3u + 0u] = points[4u * 3u + 0u] + forwardVec.X();
		points[5u * 3u + 1u] = points[4u * 3u + 1u] + forwardVec.Y();
		points[5u * 3u + 2u] = points[4u * 3u + 2u] + forwardVec.Z();
		points[7u * 3u + 0u] = points[4u * 3u + 0u] + rightVec.X();
		points[7u * 3u + 1u] = points[4u * 3u + 1u] + rightVec.Y();
		points[7u * 3u + 2u] = points[4u * 3u + 2u] + rightVec.Z();
		points[6u * 3u + 0u] = points[7u * 3u + 0u] + forwardVec.X();
		points[6u * 3u + 1u] = points[7u * 3u + 1u] + forwardVec.Y();
		points[6u * 3u + 2u] = points[7u * 3u + 2u] + forwardVec.Z();
	}

	{
		FLOAT minPoint[3] = { points[0], points[1], points[2] };
		FLOAT maxPoint[3] = { points[0], points[1], points[2] };
		for (INT i = 1; i < 8; i++)
		{
			for (INT d = 0; d < 3; d++)
			{
				minPoint[d] = CustomType::CMath::Min(points[i * 3 + d], minPoint[d]);
				maxPoint[d] = CustomType::CMath::Max(points[i * 3 + d], maxPoint[d]);
			}
		}
		boundingMin = CustomType::Vector3(minPoint[0], minPoint[1], minPoint[2]);
		boundingMax = CustomType::Vector3(maxPoint[0], maxPoint[1], maxPoint[2]);
	}
}
void CGameObject::SetRenderLocalBoundingSphere(const CustomType::Vector3& anchor, const FLOAT& radius)const
{
	if (!this->HasRenderBoundingSphere())
	{
		this->m_RenderBoundingSphere = new CustomStruct::CRenderBoundingSphere(anchor, radius);
	}
	else
	{
		this->m_RenderBoundingSphere->Anchor = anchor;
		this->m_RenderBoundingSphere->Radius = radius;
	}
}
void CGameObject::SetRenderLocalBoundingSphere(const FLOAT& radius)const
{
	if (!this->HasRenderBoundingSphere())
	{
		this->m_RenderBoundingSphere = new CustomStruct::CRenderBoundingSphere(CustomType::Vector3::Zero(), radius);
	}
	else
	{
		this->m_RenderBoundingSphere->Anchor = CustomType::Vector3::Zero();
		this->m_RenderBoundingSphere->Radius = radius;
	}
}
void CGameObject::GetRenderWorldBoundingSphere(CustomType::Vector3& anchor, FLOAT& radius)const
{
	if (!this->HasRenderBoundingSphere())
	{
		anchor = this->m_Transform->GetWorldPosition();
		radius = 1.f;
		return;
	}
	CustomType::Vector3 position(this->m_Transform->GetWorldPosition());
	CustomType::Vector3 scale(this->m_Transform->GetWorldScale());
	anchor = (this->m_RenderBoundingSphere->Anchor * scale) + position;
	radius = this->m_RenderBoundingSphere->Radius * CustomType::CMath::Max(scale.X(), CustomType::CMath::Max(scale.Y(), scale.Z()));
}
void CGameObject::RemoveTransform()const
{
	if (this->HasTransform())
	{
		this->DisconnectGameObjectAndTransform(this, this->m_Transform);
	}
}
BOOL CGameObject::IsBelongTransform(const CTransform* transform)const
{
	if (transform != NULL && this->HasTransform())
	{
		return (this->m_Transform->GetUniqueID() == transform->GetUniqueID());
	}
	return FALSE;
}
BOOL CGameObject::HasScene()const
{
	return (this->m_Scene != NULL);
}
BOOL CGameObject::HasTransform()const
{
	return (this->m_Transform != NULL);
}
BOOL CGameObject::HasRenderBoundingBox()const
{
	return (this->m_RenderBoundingBox != NULL);
}
BOOL CGameObject::HasRenderBoundingSphere()const
{
	return (this->m_RenderBoundingSphere != NULL);
}
const CGameObject* CGameObject::GetParent()const
{
	if (this->HasParent())
	{
		return (this->m_Transform->GetParent()->GetGameObject());
	}
	return NULL;
}
std::vector<const CGameObject*> CGameObject::GetChildrenList()const
{
	std::vector<const CGameObject*> childrenList;
	if (this->HasChild())
	{
		std::vector<const CTransform*> childrenTransformList = this->m_Transform->GetChildrenList();
		for (const auto& child : childrenTransformList)
		{
			if (child != NULL)
			{
				if (child->HasGameObject())
				{
					childrenList.push_back(child->GetGameObject());
				}
			}
		}
	}
	return childrenList;
}
const CGameObject* CGameObject::GetChildByTransformID(const ULONGLONG& id)const
{
	if (this->HasChild())
	{
		const CTransform* tempTransform = this->m_Transform->GetChildByUniqueID(id);
		if (tempTransform != NULL)
		{
			return (tempTransform->GetGameObject());
		}
	}
	return NULL;
}
void CGameObject::SetParent(const CGameObject* parent)const
{
	if (parent != NULL && this->HasTransform() && parent->HasTransform())
	{
		this->m_Transform->SetParent(parent->GetTransform());
	}
}
void CGameObject::AddChild(const CGameObject* child)const
{
	if (child != NULL && this->HasTransform() && child->HasTransform())
	{
		this->m_Transform->AddChild(child->GetTransform());
	}
}
void CGameObject::RemoveParent()const
{
	if (this->HasTransform())
	{
		this->m_Transform->RemoveParent();
	}
}
void CGameObject::RemoveChild(const CGameObject* child)const
{
	if (child != NULL && this->HasTransform() && child->HasTransform())
	{
		this->m_Transform->RemoveChild(child->GetTransform());
	}
}
void CGameObject::RemoveChildByTransformID(const ULONGLONG& id)const
{
	if (this->HasTransform())
	{
		this->m_Transform->RemoveChildByUniqueID(id);
	}
}
void CGameObject::RemoveChildren()const
{
	if (this->HasTransform())
	{
		this->m_Transform->RemoveAllChildren();
	}
}
BOOL CGameObject::IsParent(const CGameObject* parent)const
{
	if (parent != NULL && this->HasTransform() && parent->HasTransform())
	{
		return (this->m_Transform->IsParent(parent->m_Transform));
	}
	return FALSE;
}
BOOL CGameObject::IsChild(const CGameObject* child)const
{
	if (child != NULL && this->HasTransform() && child->HasTransform())
	{
		return (this->m_Transform->IsChild(child->m_Transform));
	}
	return FALSE;
}
BOOL CGameObject::HasParent()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->HasParent());
	}
	return FALSE;
}
BOOL CGameObject::HasChild()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->HasChild());
	}
	return FALSE;
}
CustomType::Vector3 CGameObject::GetForwardVector()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetForwardVector());
	}
	return CustomType::Vector3::ZVector();
}
CustomType::Vector3 CGameObject::GetUpVector()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetUpVector());
	}
	return CustomType::Vector3::YVector();
}
CustomType::Vector3 CGameObject::GetRightVector()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetRightVector());
	}
	return CustomType::Vector3::XVector();
}
CustomType::Matrix4x4 CGameObject::GetLocalToWorldMatrix()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetLocalToWorldMatrix());
	}
	return CustomType::Matrix4x4::Identity();
}
CustomType::Matrix4x4 CGameObject::GetWorldToLocalMatrix()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetWorldToLocalMatrix());
	}
	return CustomType::Matrix4x4::Identity();
}
CustomType::Vector3 CGameObject::GetWorldPosition()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetWorldPosition());
	}
	return CustomType::Vector3::Zero();
}
CustomType::Quaternion CGameObject::GetWorldRotation()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetWorldRotation());
	}
	return CustomType::Quaternion::Identity();
}
CustomType::Vector3 CGameObject::GetWorldScale()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetWorldScale());
	}
	return CustomType::Vector3::One();
}
CustomType::Vector3 CGameObject::GetLocalPosition()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetLocalPosition());
	}
	return CustomType::Vector3::Zero();
}
CustomType::Quaternion CGameObject::GetLocalRotation()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetLocalRotation());
	}
	return CustomType::Quaternion::Identity();
}
CustomType::Vector3 CGameObject::GetLocalScale()const
{
	if (this->HasTransform())
	{
		return (this->m_Transform->GetLocalScale());
	}
	return CustomType::Vector3::One();
}
void CGameObject::SetWorldPosition(const CustomType::Vector3& worldPosition)
{
	if (this->HasTransform())
	{
		this->m_Transform->SetWorldPosition(worldPosition);
	}
}
void CGameObject::SetWorldRotation(const CustomType::Quaternion& worldRotation)
{
	if (this->HasTransform())
	{
		this->m_Transform->SetWorldRotation(worldRotation);
	}
}
void CGameObject::SetWorldScale(const CustomType::Vector3& worldScale)
{
	if (this->HasTransform())
	{
		this->m_Transform->SetWorldScale(worldScale);
	}
}
void CGameObject::SetLocalPosition(const CustomType::Vector3& localPosition)
{
	if (this->HasTransform())
	{
		this->m_Transform->SetLocalPosition(localPosition);
	}
}
void CGameObject::SetLocalRotation(const CustomType::Quaternion& localRotation)
{
	if (this->HasTransform())
	{
		this->m_Transform->SetLocalRotation(localRotation);
	}
}
void CGameObject::SetLocalScale(const CustomType::Vector3& localScale)
{
	if (this->HasTransform())
	{
		this->m_Transform->SetLocalScale(localScale);
	}
}
void CGameObject::BaseRemoveTransform(const UINT& deleteTransform)const
{
	if (this->HasTransform())
	{
		if (this->m_Transform->HasGameObject())
		{
			this->m_Transform->m_GameObject = NULL;
		}
		if (deleteTransform)
		{
			delete (this->m_Transform);
		}
		this->m_Transform = NULL;
	}
}
void CGameObject::ConnectGameObjectAndTransform(const CGameObject* gameObject, CTransform* transform)const
{
	if (gameObject == NULL || transform == NULL)
	{
		return;
	}
	if (gameObject->HasTransform() && transform->HasGameObject() && gameObject->GetUniqueID() == transform->m_GameObject->GetUniqueID() && gameObject->m_Transform->GetUniqueID() == transform->GetUniqueID())
	{
		return;
	}
	if (gameObject->HasTransform() && gameObject->m_Transform->GetUniqueID() != transform->GetUniqueID())
	{
		gameObject->BaseRemoveTransform(TRUE);
	}
	if (transform->HasGameObject() && transform->m_GameObject->GetUniqueID() != gameObject->GetUniqueID())
	{
		transform->m_GameObject->BaseRemoveTransform(FALSE);
	}
	transform->m_GameObject = gameObject;
	gameObject->m_Transform = transform;
}
void CGameObject::DisconnectGameObjectAndTransform(const CGameObject* gameObject, CTransform* transform)const
{
	if (gameObject == NULL || transform == NULL)
	{
		return;
	}
	if (transform->HasGameObject() && transform->m_GameObject->GetUniqueID() == gameObject->GetUniqueID())
	{
		transform->m_GameObject = NULL;
	}
	if (gameObject->HasTransform() && gameObject->m_Transform->GetUniqueID() == transform->GetUniqueID())
	{
		gameObject->BaseRemoveTransform(TRUE);
	}
}
void CGameObject::AddComponent(CBaseComponent* component)
{
	if (component != NULL && component->GetGameObject() == NULL)
	{
		if (!this->HasComponent() || !this->IsBelongComponent(component))
		{
			//TODO Confirm is CMeshRendererComponent's child class by RTTI in future.
			if (typeid(*component) == typeid(CMeshRendererComponent))
			{
				if (this->HasMeshRendererComponent())
				{
					this->RemoveComponentByComponentID(this->m_MeshRendererComponentID);
				}
				this->m_MeshRendererComponentID = component->GetUniqueID();
			}
			else if (typeid(*component) == typeid(CMeshComponent))
			{
				if (this->HasMeshComponent())
				{
					this->RemoveComponentByComponentID(this->m_MeshComponentID);
				}
				this->m_MeshComponentID = component->GetUniqueID();
			}
			else if (typeid(*component) == typeid(CGameBoundSphereComponent))
			{
				if (this->HasGameBoundComponent())
				{
					this->RemoveComponentByComponentID(this->m_GameBoundComponentID);
				}
				this->m_GameBoundComponentID = component->GetUniqueID();
			}
			else if (typeid(*component) == typeid(CGameBoundBoxComponent))
			{
				if (this->HasGameBoundComponent())
				{
					this->RemoveComponentByComponentID(this->m_GameBoundComponentID);
				}
				this->m_GameBoundComponentID = component->GetUniqueID();
			}
			this->m_Components.insert_or_assign(component->GetUniqueID(), component);
			component->SetGameObject(this);
		}
	}
}
void CGameObject::RemoveComponent(const CBaseComponent* component)
{
	if (component != NULL)
	{
		if (this->HasComponent())
		{
			CBaseComponent* resultComponent = NULL;
			if (this->FindComponentByComponentID(component->GetUniqueID(), resultComponent))
			{
				//TODO Confirm is CMeshRendererComponent's child class by RTTI in future.
				if (typeid(*resultComponent) == typeid(CMeshRendererComponent))
				{
					this->m_MeshRendererComponentID = 0u;
				}
				else if (typeid(*resultComponent) == typeid(CMeshComponent))
				{
					this->m_MeshComponentID = 0u;
				}
				else if (typeid(*resultComponent) == typeid(CGameBoundSphereComponent))
				{
					this->m_GameBoundComponentID = 0u;
				}
				else if (typeid(*resultComponent) == typeid(CGameBoundBoxComponent))
				{
					this->m_GameBoundComponentID = 0u;
				}
				delete resultComponent;
				this->m_Components.erase(component->GetUniqueID());
			}
		}
	}
}
void CGameObject::RemoveComponentByComponentID(const ULONGLONG& id)
{
	if (this->HasComponent())
	{
		CBaseComponent* resultComponent = NULL;
		if (this->FindComponentByComponentID(id, resultComponent))
		{
			//TODO Confirm is CMeshRendererComponent's child class by RTTI in future.
			if (typeid(*resultComponent) == typeid(CMeshRendererComponent))
			{
				this->m_MeshRendererComponentID = 0u;
			}
			else if (typeid(*resultComponent) == typeid(CMeshComponent))
			{
				this->m_MeshComponentID = 0u;
			}
			else if (typeid(*resultComponent) == typeid(CGameBoundSphereComponent))
			{
				this->m_GameBoundComponentID = 0u;
			}
			else if (typeid(*resultComponent) == typeid(CGameBoundBoxComponent))
			{
				this->m_GameBoundComponentID = 0u;
			}
			delete resultComponent;
			this->m_Components.erase(id);
		}
	}
}
void CGameObject::RemoveComponents()
{
	if (this->HasComponent())
	{
		for (auto& component : this->m_Components)
		{
			if (component.second != NULL)
			{
				delete (component.second);
			}
		}
		this->m_MeshComponentID = 0u;
		this->m_MeshRendererComponentID = 0u;
		this->m_GameBoundComponentID = 0u;
		this->m_Components.clear();
	}
}
CBaseComponent* CGameObject::GetComponentByComponentID(const ULONGLONG& id)const
{
	if (this->HasComponent())
	{
		auto& element = this->m_Components.find(id);
		if (element != this->m_Components.end())
		{
			return (element->second);
		}
	}
	return NULL;
}
CBaseComponent* CGameObject::BaseGetComponentByComponentID(const ULONGLONG& id)const
{
	if (this->HasComponent())
	{
		auto& element = this->m_Components.find(id);
		if (element != this->m_Components.end())
		{
			return (element->second);
		}
	}
	return NULL;
}
BOOL CGameObject::IsBelongComponent(const CBaseComponent* component)const
{
	if (component != NULL && this->HasComponent())
	{
		auto& element = this->m_Components.find(component->GetUniqueID());
		if (element != this->m_Components.end())
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL CGameObject::HasComponent()const
{
	return (this->m_Components.size() > 0);
}
BOOL CGameObject::HasMeshComponent()const
{
	return (this->m_MeshComponentID > 0u);
}
BOOL CGameObject::HasMeshRendererComponent()const
{
	return (this->m_MeshRendererComponentID > 0u);
}
BOOL CGameObject::HasGameBoundComponent()const
{
	return (this->m_GameBoundComponentID > 0u);
}
BOOL CGameObject::FindComponentByComponentID(const ULONGLONG& id, CBaseComponent*& component)
{
	if (this->HasComponent())
	{
		auto& element = this->m_Components.find(id);
		if (element != this->m_Components.end())
		{
			component = element->second;
			return TRUE;
		}
	}
	return FALSE;
}
const BOOL& CGameObject::IsActive()const
{
	return (this->m_Active);
}
void CGameObject::Active()
{
	this->m_Active = TRUE;
}
void CGameObject::Inactive()
{
	this->m_Active = FALSE;
}
void CGameObject::Init()
{
	if (this->HasComponent())
	{
		for (const auto& component : this->m_Components)
		{
			if (component.second)
			{
				component.second->Init();
			}
		}
	}
}
void CGameObject::Uninit()
{
	if (this->HasComponent())
	{
		for (const auto& component : this->m_Components)
		{
			if (component.second)
			{
				component.second->Uninit();
			}
		}
	}
}
void CGameObject::Update()
{
	if (this->HasComponent())
	{
		for (const auto& component : this->m_Components)
		{
			if (component.second && component.second->NeedUpdate() && component.second->IsActive())
			{
				component.second->Update();
			}
		}
	}
}
void CGameObject::FixedUpdate()
{
	if (this->HasComponent())
	{
		for (const auto& component : this->m_Components)
		{
			if (component.second && component.second->NeedFixedUpdate() && component.second->IsActive())
			{
				component.second->FixedUpdate();
			}
		}
	}
}
#ifdef _DEVELOPMENT_EDITOR
void CGameObject::SelectedEditorUpdate_RenderBounding()
{
	ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
	if (ImGui::TreeNode("RenderBounding"))
	{
		FLOAT boxBoundMin[3] = { 0.f, 0.f, 0.f }, boxBoundMax[3] = { 0.f, 0.f, 0.f };
		if (this->HasRenderBoundingBox())
		{
			CustomType::Vector3 boundMin(this->GetRenderLocalBoundingBox()->Anchor), boundMax(this->GetRenderLocalBoundingBox()->Dimensions);
			boxBoundMin[0] = boundMin.X(); boxBoundMin[1] = boundMin.Y(); boxBoundMin[2] = boundMin.Z();
			boxBoundMax[0] = boundMax.X(); boxBoundMax[1] = boundMax.Y(); boxBoundMax[2] = boundMax.Z();
		}
		FLOAT sphereBoundAnchor[3] = { 0.f, 0.f, 0.f }, sphereBoundRadius = 0.f;
		if (this->HasRenderBoundingSphere())
		{
			CustomType::Vector3 boundAnchor(this->GetRenderLocalBoundingSphere()->Anchor);
			sphereBoundRadius = this->GetRenderLocalBoundingSphere()->Radius;
			sphereBoundAnchor[0] = boundAnchor.X(); sphereBoundAnchor[1] = boundAnchor.Y(); sphereBoundAnchor[2] = boundAnchor.Z();
		}
		ImGui::Text("RenderBoundingBox & BoundingSphere");
		ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(320, 105), ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		ImGui::InputFloat3("Box Min", boxBoundMin);
		ImGui::InputFloat3("Box Max", boxBoundMax);
		ImGui::InputFloat3("Sphere Anchor", sphereBoundAnchor);
		ImGui::InputFloat("Sphere Radius", &sphereBoundRadius);
		ImGui::EndChild();
		if (ImGui::Button("ApplyBounding"))
		{
			this->SetRenderLocalBoundingBox(CustomType::Vector3(boxBoundMin[0], boxBoundMin[1], boxBoundMin[2]), CustomType::Vector3(boxBoundMax[0], boxBoundMax[1], boxBoundMax[2]));
			this->SetRenderLocalBoundingSphere(CustomType::Vector3(sphereBoundAnchor[0], sphereBoundAnchor[1], sphereBoundAnchor[2]), sphereBoundRadius);
		}
		ImGui::TreePop();
	}
}
void CGameObject::SelectedEditorUpdate()
{
	if (this->HasTransform())
	{
		this->m_Transform->SelectedEditorUpdate();
	}
	this->SelectedEditorUpdate_RenderBounding();
	if (this->HasComponent())
	{
		for (const auto& component : this->m_Components)
		{
			if (component.second && component.second->IsActive())
			{
				component.second->SelectedEditorUpdate();
			}
		}
	}
}
#endif