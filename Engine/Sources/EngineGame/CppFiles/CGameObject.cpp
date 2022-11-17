#include "../../../../Entry/EngineMain.h"
#include "../Headers/CGameObject.h"
#include "../Headers/CComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../Headers/CScene.h"

CGameObject::CGameObject()
{
	this->m_Scene.reset();
	this->m_Active					= FALSE;
	this->m_Transform				= nullptr;
	this->m_RenderBoundingBox		= nullptr;
	this->m_RenderBoundingSphere	= nullptr;
	this->m_MeshComponentID			= 0u;
	this->m_MeshRendererComponentID	= 0u;
}
CGameObject::CGameObject(const CGameObject& obj)
{
	this->m_Active					= FALSE;
	this->m_Scene					= obj.m_Scene;
	this->m_Transform				= nullptr;
	this->m_RenderBoundingBox		= obj.m_RenderBoundingBox;
	this->m_RenderBoundingSphere	= obj.m_RenderBoundingSphere;
	this->m_MeshComponentID			= obj.m_MeshComponentID;
	this->m_MeshRendererComponentID	= obj.m_MeshRendererComponentID;
	if (obj.m_Transform)
	{
		this->m_Transform = std::shared_ptr<CTransform>(new CTransform());
		this->m_Transform->InitTransform(this->shared_from_this(), obj.m_Transform->GetWorldPosition(), obj.m_Transform->GetWorldRotation(), obj.m_Transform->GetWorldScale(), obj.m_Transform->GetParent().lock());
	}
}
CGameObject::~CGameObject()
{

}
std::weak_ptr<CScene> CGameObject::GetScene()
{
	return (this->m_Scene);
}
std::weak_ptr<CTransform> CGameObject::GetTransform()
{
	std::weak_ptr<CTransform> weakPtr(this->m_Transform);
	return weakPtr;
}
std::weak_ptr<CustomStruct::CRenderBoundingBox> CGameObject::GetRenderLocalBoundingBox()
{
	std::weak_ptr<CustomStruct::CRenderBoundingBox> weakPtr((this->m_RenderBoundingBox));
	return weakPtr;
}
std::weak_ptr<CustomStruct::CRenderBoundingSphere> CGameObject::GetRenderLocalBoundingSphere()
{
	std::weak_ptr<CustomStruct::CRenderBoundingSphere> weakPtr((this->m_RenderBoundingSphere));
	return weakPtr;
}
void CGameObject::SetScene(std::shared_ptr<CScene> scene)
{
	this->m_Scene = scene;
}
void CGameObject::AddNewTransform(std::shared_ptr<CGameObject> parent)
{
	if (!m_Transform)
	{
		this->m_Transform = std::shared_ptr<CTransform>(new CTransform());
	}
	if (parent)
	{
		this->m_Transform->InitTransform(this->shared_from_this(), CustomType::Vector3::Zero(), parent->GetTransform().lock());
	}
	else if (!this->GetParent().expired())
	{
		this->m_Transform->InitTransform(this->shared_from_this(), CustomType::Vector3::Zero(), this->GetParent().lock()->GetTransform().lock());
	}
	else
	{
		this->m_Transform->InitTransform(this->shared_from_this(), CustomType::Vector3::Zero(), nullptr);
	}
}
void CGameObject::AddNewTransformWithValue(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, std::shared_ptr<CGameObject> parent)
{
	if (!m_Transform)
	{
		this->m_Transform = std::shared_ptr<CTransform>(new CTransform());
	}
	if (parent)
	{
		this->m_Transform->InitTransform(this->shared_from_this(), worldPosition, worldRotation, worldScale, parent->GetTransform().lock());
	}
	else if (!this->GetParent().expired())
	{
		this->m_Transform->InitTransform(this->shared_from_this(), worldPosition, worldRotation, worldScale, this->GetParent().lock()->GetTransform().lock());
	}
	else
	{
		this->m_Transform->InitTransform(this->shared_from_this(), worldPosition, worldRotation, worldScale, nullptr);
	}
}
void CGameObject::SetRenderLocalBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions)
{
	if (!m_RenderBoundingBox)
	{
		m_RenderBoundingBox = std::shared_ptr<CustomStruct::CRenderBoundingBox>(new CustomStruct::CRenderBoundingBox(anchor, dimensions));
		return;
	}
	m_RenderBoundingBox->Anchor = anchor;
	m_RenderBoundingBox->Dimensions = dimensions;
}
void CGameObject::SetRenderLocalBoundingBox(const CustomType::Vector3& dimensions)
{
	CustomType::Vector3 tempAnchor(-dimensions.X(), -dimensions.Y(), -dimensions.Z());
	tempAnchor = tempAnchor * 0.5f;
	if (!m_RenderBoundingBox)
	{
		m_RenderBoundingBox = std::shared_ptr<CustomStruct::CRenderBoundingBox>(new CustomStruct::CRenderBoundingBox(tempAnchor, dimensions));
		return;
	}
	m_RenderBoundingBox->Anchor = tempAnchor;
	m_RenderBoundingBox->Dimensions = dimensions;
}
void CGameObject::GetRenderWorldAABBBoundingBox(CustomType::Vector3& boundingMin, CustomType::Vector3& boundingMax)
{
	if (!m_RenderBoundingBox)
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

		CustomType::Vector3 tempPoint(rightVec * m_RenderBoundingBox->Anchor.X() * scale.X() + upVec * m_RenderBoundingBox->Anchor.Y() * scale.Y() + forwardVec * m_RenderBoundingBox->Anchor.Z() * scale.Z() + position);
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

		rightVec = rightVec * m_RenderBoundingBox->Dimensions.X() * scale.X();
		upVec = upVec * m_RenderBoundingBox->Dimensions.Y() * scale.Y();
		forwardVec = forwardVec * m_RenderBoundingBox->Dimensions.Z() * scale.Z();

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
void CGameObject::SetRenderLocalBoundingSphere(const CustomType::Vector3& anchor, const FLOAT& radius)
{
	if (!m_RenderBoundingSphere)
	{
		m_RenderBoundingSphere = std::shared_ptr<CustomStruct::CRenderBoundingSphere>(new CustomStruct::CRenderBoundingSphere(anchor, radius));
		return;
	}
	m_RenderBoundingSphere->Anchor = anchor;
	m_RenderBoundingSphere->Radius = radius;
}
void CGameObject::SetRenderLocalBoundingSphere(const FLOAT& radius)
{
	if (!m_RenderBoundingSphere)
	{
		m_RenderBoundingSphere = std::shared_ptr<CustomStruct::CRenderBoundingSphere>(new CustomStruct::CRenderBoundingSphere(CustomType::Vector3::Zero(), radius));
		return;
	}
	m_RenderBoundingSphere->Anchor = CustomType::Vector3::Zero();
	m_RenderBoundingSphere->Radius = radius;
}
void CGameObject::GetRenderWorldBoundingSphere(CustomType::Vector3& anchor, FLOAT& radius)
{
	if (!m_RenderBoundingSphere)
	{
		anchor = this->m_Transform->GetWorldPosition();
		radius = 1.f;
		return;
	}
	CustomType::Vector3 position(this->m_Transform->GetWorldPosition());
	CustomType::Vector3 scale(this->m_Transform->GetWorldScale());
	anchor = (m_RenderBoundingSphere->Anchor * scale) + position;
	radius = m_RenderBoundingSphere->Radius * CustomType::CMath::Max(scale.X(), CustomType::CMath::Max(scale.Y(), scale.Z()));
}
CustomType::Vector3 CGameObject::GetForwardVector()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetForwardVector());
	}
	return CustomType::Vector3::ZVector();
}
CustomType::Vector3 CGameObject::GetUpVector()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetUpVector());
	}
	return CustomType::Vector3::YVector();
}
CustomType::Vector3 CGameObject::GetRightVector()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetRightVector());
	}
	return CustomType::Vector3::XVector();
}
CustomType::Matrix4x4 CGameObject::GetLocalToWorldMatrix()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetLocalToWorldMatrix());
	}
	return CustomType::Matrix4x4::Identity();
}
CustomType::Matrix4x4 CGameObject::GetWorldToLocalMatrix()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetWorldToLocalMatrix());
	}
	return CustomType::Matrix4x4::Identity();
}
CustomType::Vector3 CGameObject::GetWorldPosition()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetWorldPosition());
	}
	return CustomType::Vector3::Zero();
}
CustomType::Quaternion CGameObject::GetWorldRotation()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetWorldRotation());
	}
	return CustomType::Quaternion::Identity();
}
CustomType::Vector3 CGameObject::GetWorldScale()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetWorldScale());
	}
	return CustomType::Vector3::One();
}
CustomType::Vector3 CGameObject::GetLocalPosition()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetLocalPosition());
	}
	return CustomType::Vector3::Zero();
}
CustomType::Quaternion CGameObject::GetLocalRotation()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetLocalRotation());
	}
	return CustomType::Quaternion::Identity();
}
CustomType::Vector3 CGameObject::GetLocalScale()
{
	if (this->m_Transform)
	{
		return (this->m_Transform->GetLocalScale());
	}
	return CustomType::Vector3::One();
}
void CGameObject::SetWorldPosition(const CustomType::Vector3& worldPosition)
{
	if (this->m_Transform)
	{
		this->m_Transform->SetWorldPosition(worldPosition);
	}
}
void CGameObject::SetWorldRotation(const CustomType::Quaternion& worldRotation)
{
	if (this->m_Transform)
	{
		this->m_Transform->SetWorldRotation(worldRotation);
	}
}
void CGameObject::SetWorldScale(const CustomType::Vector3& worldScale)
{
	if (this->m_Transform)
	{
		this->m_Transform->SetWorldScale(worldScale);
	}
}
void CGameObject::SetLocalPosition(const CustomType::Vector3& localPosition)
{
	if (this->m_Transform)
	{
		this->m_Transform->SetLocalPosition(localPosition);
	}
}
void CGameObject::SetLocalRotation(const CustomType::Quaternion& localRotation)
{
	if (this->m_Transform)
	{
		this->m_Transform->SetLocalRotation(localRotation);
	}
}
void CGameObject::SetLocalScale(const CustomType::Vector3& localScale)
{
	if (this->m_Transform)
	{
		this->m_Transform->SetLocalScale(localScale);
	}
}
std::weak_ptr<CGameObject> CGameObject::GetParent()
{
	if (this->m_Transform && !this->m_Transform->GetParent().expired())
	{
		return (this->m_Transform->GetParent().lock()->GetGameObject());
	}
	std::weak_ptr<CGameObject> weakPrt;
	return weakPrt;
}
std::vector<std::weak_ptr<CGameObject>> CGameObject::GetChildrenList()
{
	std::vector<std::weak_ptr<CGameObject>> childrenList;
	if (this->m_Transform && this->m_Transform->HasChild())
	{
		std::vector<std::weak_ptr<CTransform>> childrenTransformList = this->m_Transform->GetChildrenList();
		for (const auto& child : childrenTransformList)
		{
			if (!child.expired())
			{
				std::shared_ptr<CTransform> sharedPtr(child.lock());
				if (!sharedPtr->GetGameObject().expired())
				{
					childrenList.push_back(sharedPtr->GetGameObject());
				}
			}
		}
	}
	return childrenList;
}
template<class T>
std::vector<std::weak_ptr<T>> CGameObject::GetChildrenListByType()
{
	std::vector<std::weak_ptr<T>> childrenList;
	if (this->m_Transform && this->m_Transform->HasChild())
	{
		std::vector<std::weak_ptr<CTransform>> childrenTransformList = this->m_Transform->GetChildrenList();
		for (const auto& child : childrenTransformList)
		{
			if (!child.expired())
			{
				std::shared_ptr<CTransform> sharedPrt(child.lock());
				if (!sharedPrt->GetGameObject().expired() && typeid(*sharedPrt) == typeid(T))
				{
					childrenList.push_back(sharedPrt->GetGameObject());
				}
			}
		}
	}
	return childrenList;
}
std::weak_ptr<CGameObject> CGameObject::FindChildByTransformID(const ULONGLONG& id)
{
	if (this->m_Transform && this->m_Transform->HasChild())
	{
		std::weak_ptr<CTransform> tempTransform(this->m_Transform->GetChildByUniqueID(id));
		if (!tempTransform.expired())
		{
			return (tempTransform.lock()->GetGameObject());
		}
	}
	std::weak_ptr<CGameObject> weakPtr;
	return weakPtr;
}
template<class T>
std::weak_ptr<T> CGameObject::FindFirstChildByType()
{
	if (this->m_Transform && this->m_Transform->HasChild())
	{
		std::vector<std::weak_ptr<CTransform>> childrenTransformList = this->m_Transform->GetChildrenList();
		for (const auto& child : childrenTransformList)
		{
			if (!child.expired())
			{
				std::shared_ptr<CTransform> sharedPtr(child.lock());
				if (!sharedPtr->GetGameObject().expired() && typeid(*sharedPtr) == typeid(T))
				{
					return (sharedPtr->GetGameObject());
				}
			}
		}
	}
	std::weak_ptr<T> weakPtr;
	return weakPtr;
}
void CGameObject::SetParent(std::shared_ptr<CGameObject> parent)
{
	if (parent && this->m_Transform && !parent->GetTransform().expired())
	{
		this->m_Transform->SetParent(parent->GetTransform().lock());
	}
}
void CGameObject::AddChild(std::shared_ptr<CGameObject> child)
{
	if (child && this->m_Transform && !child->GetTransform().expired())
	{
		this->m_Transform->AddChild(child->GetTransform().lock());
	}
}
void CGameObject::RemoveParent()
{
	if (this->m_Transform)
	{
		this->m_Transform->RemoveParent();
	}
}
void CGameObject::RemoveChild(std::shared_ptr<CGameObject> child)
{
	if (child && this->m_Transform && !child->GetTransform().expired())
	{
		this->m_Transform->RemoveChild(child->GetTransform().lock());
	}
}
void CGameObject::RemoveChildByTransformID(const ULONGLONG& id)
{
	if (this->m_Transform)
	{
		this->m_Transform->RemoveChildByUniqueID(id);
	}
}
void CGameObject::RemoveChildren()
{
	if (this->m_Transform)
	{
		this->m_Transform->RemoveChildren();
	}
}
BOOL CGameObject::IsBelongTransform(const CTransform* transform)
{
	if (transform && this->m_Transform)
	{
		return (this->m_Transform->GetUniqueID() == transform->GetUniqueID());
	}
	return FALSE;
}
BOOL CGameObject::IsParent(const CGameObject* parent)
{
	if (parent && this->m_Transform && parent->m_Transform)
	{
		return (this->m_Transform->IsParent(parent->m_Transform.get()));
	}
	return FALSE;
}
BOOL CGameObject::IsChild(const CGameObject* child)
{
	if (child && this->m_Transform && child->m_Transform)
	{
		return (this->m_Transform->IsChild(child->m_Transform.get()));
	}
	return FALSE;
}
void CGameObject::AddComponent(std::shared_ptr<CBaseComponent> component)
{
	if (component)
	{
		if (!this->HasComponent() || !this->IsBelongComponent(component.get()))
		{
			//TODO Confirm is CMeshRendererComponent's child class by RTTI in future.
			if (typeid(*component) == typeid(CMeshRendererComponent))
			{
				this->m_MeshRendererComponentID = component->GetUniqueID();
			}
			else if (typeid(*component) == typeid(CMeshComponent))
			{
				this->m_MeshComponentID = component->GetUniqueID();
			}
			this->m_Components.insert_or_assign(component->GetUniqueID(), component);
		}
	}
}
void CGameObject::RemoveComponent(std::shared_ptr<CBaseComponent> component)
{
	if (component)
	{
		if (this->HasComponent() && this->IsBelongComponent(component.get()))
		{
			//TODO Confirm is CMeshRendererComponent's child class by RTTI in future.
			if (typeid(*component) == typeid(CMeshRendererComponent))
			{
				this->m_MeshRendererComponentID = 0u;
			}
			else if (typeid(*component) == typeid(CMeshComponent))
			{
				this->m_MeshComponentID = 0u;
			}
			this->m_Components.erase(component->GetUniqueID());
		}
	}
}
void CGameObject::RemoveComponentByComponentID(const ULONGLONG& id)
{
	if (this->HasComponent())
	{
		std::weak_ptr<CBaseComponent> component(this->FindComponentByComponentID(id));
		if (!component.expired())
		{
			std::shared_ptr<CBaseComponent> sharedPtr(component.lock());
			//TODO Confirm is CMeshRendererComponent's child class by RTTI in future.
			if (typeid(*sharedPtr) == typeid(CMeshRendererComponent))
			{
				this->m_MeshRendererComponentID = 0u;
			}
			else if (typeid(*sharedPtr) == typeid(CMeshComponent))
			{
				this->m_MeshComponentID = 0u;
			}
			this->m_Components.erase(sharedPtr->GetUniqueID());
		}
	}
}
void CGameObject::RemoveComponents()
{
	if (this->HasComponent())
	{
		this->m_Components.clear();
	}
}
std::weak_ptr<CBaseComponent> CGameObject::FindComponentByComponentID(const ULONGLONG& id)
{
	if (this->HasComponent())
	{
		return (this->FindComponentByComponentID(id));
	}
	std::weak_ptr<CBaseComponent> weakPtr;
	return weakPtr;
}
template<class T>
std::weak_ptr<T> CGameObject::FindFirstComponentByType()
{
	std::weak_ptr<T> weakPtr;
	if (this->HasComponent())
	{
		for (const auto& component : this->m_Components)
		{
			if (component.second && typeid(*(component.second)) == typeid(T))
			{
				weakPtr = component.second;
				break;
			}
		}
	}
	return weakPtr;
}
template<class T>
std::vector<std::weak_ptr<T>> CGameObject::GetComponentListByType()
{
	std::vector<std::weak_ptr<T>> componentList;
	if (this->HasComponent())
	{
		for (const auto& component : this->m_Components)
		{
			if (component.second && typeid(*(component.second)) == typeid(T))
			{
				std::weak_ptr<T> weakPtr(component.second);
				componentList.push_back(weakPtr);
			}
		}
	}
	return componentList;
}
template<class T>
std::weak_ptr<T> CGameObject::GetMeshComponent()
{
	if (this->HasMeshComponent())
	{
		return (this->FindComponentByComponentID(this->m_MeshComponentID));
	}
	std::weak_ptr<T> weakPtr;
	return weakPtr;
}
template<class T>
std::weak_ptr<T> CGameObject::GetMeshRendererComponent()
{
	if (this->HasMeshRendererComponent())
	{
		return (this->FindComponentByComponentID(this->m_MeshRendererComponentID));
	}
	std::weak_ptr<T> weakPtr;
	return weakPtr;
}
BOOL CGameObject::IsBelongComponent(const CBaseComponent* component)
{
	std::map<ULONGLONG, std::shared_ptr<CBaseComponent>>::iterator element = this->m_Components.find(component->GetUniqueID());
	if (element != this->m_Components.end())
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CGameObject::HasComponent()
{
	return (this->m_Components.size() > 0);
}
BOOL CGameObject::HasMeshComponent()
{
	return (this->m_MeshComponentID > 0u);
}
BOOL CGameObject::HasMeshRendererComponent()
{
	return (this->m_MeshRendererComponentID > 0u);
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