#include "../../../../Entry/EngineMain.h"
#include "../Headers/CComponent.h"
#include "../Headers/CGameObject.h"

CTransform::CTransform()
{
	this->m_GameObject.reset();
	this->m_Parent.reset();
	this->m_LocalPosition	= CustomType::Vector3::Zero();
	this->m_LocalRotation	= CustomType::Quaternion::Identity();
	this->m_LocalScale		= 1.f;
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent)
{
	this->m_Parent.reset();
	this->SetGameObject(gameObject);
	this->SetWorldPosition(worldPosition);
	this->SetWorldRotation(worldRotation);
	this->SetWorldScale(worldScale);
	this->SetParent(parent);
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldPosition, std::shared_ptr<CTransform> parent)
{
	this->InitTransform(gameObject, worldPosition, CustomType::Quaternion::Identity(), CustomType::Vector3::One(), parent);
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Quaternion& worldRotation, std::shared_ptr<CTransform> parent)
{
	this->InitTransform(gameObject, CustomType::Vector3::Zero(), worldRotation, CustomType::Vector3::One(), parent);
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, std::shared_ptr<CTransform> parent)
{
	this->InitTransform(gameObject, worldPosition, worldRotation, CustomType::Vector3::One(), parent);
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent)
{
	this->InitTransform(gameObject, worldPosition, CustomType::Quaternion::Identity(), CustomType::Vector3::One(), parent);
}
std::weak_ptr<CGameObject> CTransform::GetGameObject()
{
	return (this->m_GameObject);
}
std::weak_ptr<CTransform> CTransform::GetParent()
{
	return (this->m_Parent);
}
UINT CTransform::GetChildrenNum()
{
	return (static_cast<UINT>(this->m_Children.size()));
}
std::weak_ptr<CTransform> CTransform::GetChildByUniqueID(const ULONGLONG& id)
{
	std::weak_ptr<CTransform> child;
	if (this->HasChild())
	{
		std::map<ULONGLONG, std::weak_ptr<CTransform>>::iterator element = this->m_Children.find(id);
		if (element == this->m_Children.end())
		{
			return child;
		}
		return (element->second);
	}
	return child;
}
std::vector<std::weak_ptr<CTransform>> CTransform::GetChildrenList()
{
	std::vector<std::weak_ptr<CTransform>> childrenList;
	if (this->HasChild())
	{
		for (const auto& child : this->m_Children)
		{
			if (!child.second.expired())
			{
				childrenList.push_back(child.second);
			}
		}
	}
	return childrenList;
}
std::map<ULONGLONG, std::weak_ptr<CTransform>> CTransform::GetChildrenMap()
{
	return (this->m_Children);
}
void CTransform::SetGameObject(std::shared_ptr<CGameObject> gameObject)
{
	this->m_GameObject = gameObject;
}
void CTransform::SetParent(std::shared_ptr<CTransform> parent)
{
	if (parent)
	{
		this->m_Parent = parent;
		this->CalculateCurrentLocalTransform(parent);
	}
}
void CTransform::AddChild(std::shared_ptr<CTransform> child)
{
	if (child)
	{
		BOOL needNewAdd = TRUE;
		if (this->HasChild())
		{
			std::map<ULONGLONG, std::weak_ptr<CTransform>>::iterator element = this->m_Children.find(child->GetUniqueID());
			if (element != this->m_Children.end())
			{
				needNewAdd = FALSE;
			}
		}
		if (needNewAdd)
		{
			std::weak_ptr weakPtrChild(child);
			this->m_Children.insert_or_assign(child->GetUniqueID(), weakPtrChild);
		}
	}
}
void CTransform::RemoveParent()
{
	if (!this->m_Parent.expired())
	{
		this->CalculateCurrentLocalTransform(nullptr);
		this->m_Parent.reset();
	}
}
void CTransform::RemoveChild(std::shared_ptr<CTransform> child)
{
	if (child && this->HasChild())
	{
		this->m_Children.erase(child->GetUniqueID());
	}
}
void CTransform::RemoveChildByUniqueID(const ULONGLONG& id)
{
	if (this->HasChild())
	{
		this->m_Children.erase(id);
	}
}
void CTransform::RemoveChildren()
{
	if (this->HasChild())
	{
		this->m_Children.clear();
	}
}
BOOL CTransform::IsBelongGameObject(const CGameObject* gameObject)
{
	if (gameObject && !this->m_GameObject.expired())
	{
		return (this->m_GameObject.lock()->GetUniqueID() == gameObject->GetUniqueID());
	}
	return FALSE;
}
BOOL CTransform::HasChild()
{
	return (this->m_Children.size() > 0);
}
BOOL CTransform::IsParent(const CTransform* parent)
{
	if (parent && !this->m_Parent.expired())
	{
		return (this->m_Parent.lock()->GetUniqueID() == parent->GetUniqueID());
	}
	return FALSE;
}
BOOL CTransform::IsChild(const CTransform* child)
{
	if (child && this->HasChild())
	{
		std::map<ULONGLONG, std::weak_ptr<CTransform>>::iterator element = this->m_Children.find(child->GetUniqueID());
		if (element == this->m_Children.end())
		{
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
const CustomType::Vector3& CTransform::GetLocalPosition()const
{
	return (this->m_LocalPosition);
}
const CustomType::Quaternion& CTransform::GetLocalRotation()const
{
	return (this->m_LocalRotation);
}
const CustomType::Vector3& CTransform::GetLocalScale()const
{
	return (this->m_LocalScale);
}
void CTransform::SetLocalPosition(const CustomType::Vector3& localPosition)
{
	this->m_LocalPosition = localPosition;
}
void CTransform::SetLocalRotation(const CustomType::Quaternion& localRotation)
{
	this->m_LocalRotation = localRotation;
}
void CTransform::SetLocalScale(const CustomType::Vector3& localScale)
{
	this->m_LocalScale = localScale;
}
void CTransform::SetWorldPosition(const CustomType::Vector3& worldPosition)
{
	if (!this->m_Parent.expired())
	{
		CustomType::Vector3 parentWorldPosition(-(this->m_Parent.lock()->GetWorldPosition()));
		this->m_LocalPosition = parentWorldPosition + worldPosition;
	}
	else
	{
		this->m_LocalPosition = worldPosition;
	}
}
void CTransform::SetWorldRotation(const CustomType::Quaternion& worldRotation)
{
	if (!this->m_Parent.expired())
	{
		this->m_LocalRotation = CustomType::Quaternion::MultiplyQuaternion(this->m_Parent.lock()->GetWorldRotation().Inverse(), worldRotation);
	}
	else
	{
		this->m_LocalRotation = worldRotation;
	}
}
void CTransform::SetWorldScale(const CustomType::Vector3& worldScale)
{
	if (!this->m_Parent.expired())
	{
		this->m_LocalScale = CustomType::Vector3::Reciprocal(this->m_Parent.lock()->GetWorldScale()) * worldScale;
	}
	else
	{
		this->m_LocalScale = worldScale;
	}
}
CustomType::Vector3 CTransform::GetWorldPosition()
{
	CustomType::Vector3 result(this->m_LocalPosition);
	this->RecursionWorldPosition(this->m_Parent, result);
	return result;
}
CustomType::Quaternion CTransform::GetWorldRotation()
{
	CustomType::Quaternion result(this->m_LocalRotation);
	this->RecursionWorldRotation(this->m_Parent, result);
	return result;
}
CustomType::Vector3 CTransform::GetWorldScale()
{
	CustomType::Vector3 result(this->m_LocalScale);
	this->RecursionWorldScale(this->m_Parent, result);
	return result;
}
void CTransform::RecursionWorldPosition(const std::weak_ptr<CTransform> parent, CustomType::Vector3& position)
{
	if (parent.expired())
	{
		return;
	}
	std::shared_ptr<CTransform> sharedParent(parent.lock());
	position += sharedParent->m_LocalPosition;
	this->RecursionWorldPosition(sharedParent->m_Parent, position);
}
void CTransform::RecursionWorldRotation(const std::weak_ptr<CTransform> parent, CustomType::Quaternion& rotation)
{
	if (parent.expired())
	{
		return;
	}
	std::shared_ptr<CTransform> sharedParent(parent.lock());
	rotation = CustomType::Quaternion::MultiplyQuaternion(sharedParent->m_LocalRotation, rotation);
	this->RecursionWorldRotation(sharedParent->m_Parent, rotation);
}
void CTransform::RecursionWorldScale(const std::weak_ptr<CTransform> parent, CustomType::Vector3& scale)
{
	if (parent.expired())
	{
		return;
	}
	std::shared_ptr<CTransform> sharedParent(parent.lock());
	scale *= sharedParent->m_LocalScale;
	this->RecursionWorldScale(sharedParent->m_Parent, scale);
}
void CTransform::CalculateCurrentLocalTransform(std::shared_ptr<CTransform> newParent)
{
	if (newParent)
	{
		this->m_LocalPosition = this->GetWorldPosition() - newParent->GetWorldPosition();
		this->m_LocalRotation = CustomType::Quaternion::MultiplyQuaternion(newParent->GetWorldRotation().Inverse(), this->GetWorldRotation());
		this->m_LocalScale = CustomType::Vector3::Reciprocal(newParent->GetWorldScale()) * this->GetWorldScale();
		return;
	}
	this->m_LocalPosition = this->GetWorldPosition();
	this->m_LocalRotation = this->GetWorldRotation();
	this->m_LocalScale = this->GetWorldScale();
}
CustomType::Vector3 CTransform::GetForwardVector()
{
	return (this->GetWorldRotation().MultiplyVector(CustomType::Vector3::ZVector()));
}
CustomType::Vector3 CTransform::GetUpVector()
{
	return (this->GetWorldRotation().MultiplyVector(CustomType::Vector3::YVector()));
}
CustomType::Vector3 CTransform::GetRightVector()
{
	return (this->GetWorldRotation().MultiplyVector(CustomType::Vector3::XVector()));
}
CustomType::Matrix4x4 CTransform::GetLocalToWorldMatrix()
{
	return (CustomType::Matrix4x4(this->GetWorldPosition(), this->GetWorldRotation(), this->GetWorldScale()));
}
CustomType::Matrix4x4 CTransform::GetWorldToLocalMatrix()
{
	return (this->GetLocalToWorldMatrix().Inverse());
}