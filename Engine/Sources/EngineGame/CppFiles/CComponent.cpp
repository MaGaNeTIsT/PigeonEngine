#include "../../../../Entry/EngineMain.h"
#include "../Headers/CComponent.h"
#include "../Headers/CGameObject.h"

CTransform::CTransform()
{
	this->m_GameObject		= NULL;
	this->m_Parent			= NULL;
	this->m_LocalPosition	= CustomType::Vector3::Zero();
	this->m_LocalRotation	= CustomType::Quaternion::Identity();
	this->m_LocalScale		= 1.f;
}
void CTransform::InitTransform(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale)
{
	this->SetWorldPosition(worldPosition);
	this->SetWorldRotation(worldRotation);
	this->SetWorldScale(worldScale);
}
void CTransform::InitTransform(const CustomType::Vector3& worldPosition)
{
	this->InitTransform(worldPosition, CustomType::Quaternion::Identity(), CustomType::Vector3::One());
}
void CTransform::InitTransform(const CustomType::Quaternion& worldRotation)
{
	this->InitTransform(CustomType::Vector3::Zero(), worldRotation, CustomType::Vector3::One());
}
void CTransform::InitTransform(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation)
{
	this->InitTransform(worldPosition, worldRotation, CustomType::Vector3::One());
}
void CTransform::InitTransform(const CustomType::Vector3& worldPosition, const CustomType::Vector3& worldScale)
{
	this->InitTransform(worldPosition, CustomType::Quaternion::Identity(), CustomType::Vector3::One());
}
const CGameObject* CTransform::GetGameObject()const
{
	return (this->m_GameObject);
}
const CTransform* CTransform::GetParent()const
{
	return (this->m_Parent);
}
UINT CTransform::GetChildrenNum()const
{
	return (static_cast<UINT>(this->m_Children.size()));
}
const CTransform* CTransform::GetChildByUniqueID(const ULONGLONG& id)const
{
	if (this->HasChild())
	{
		auto& element = this->m_Children.find(id);
		if (element == this->m_Children.end())
		{
			return NULL;
		}
		return (element->second);
	}
	return NULL;
}
std::vector<const CTransform*> CTransform::GetChildrenList()const
{
	std::vector<const CTransform*> childrenList;
	if (this->HasChild())
	{
		for (const auto& child : this->m_Children)
		{
			if (child.second != NULL)
			{
				childrenList.push_back(child.second);
			}
		}
	}
	return childrenList;
}
std::map<ULONGLONG, const CTransform*> CTransform::GetChildrenMap()const
{
	return (this->m_Children);
}
void CTransform::SetParent(const CTransform* parent)const
{
	if (parent != NULL)
	{
		if (this->HasParent())
		{
			this->DisconnectParentAndChild(this->m_Parent, this);
		}
		this->ConnectParentAndChild(parent, this);
	}
}
void CTransform::AddChild(const CTransform* child)const
{
	if (child != NULL)
	{
		if (child->HasParent())
		{
			this->DisconnectParentAndChild(child->m_Parent, child);
		}
		this->ConnectParentAndChild(this, child);
	}
}
void CTransform::RemoveParent()const
{
	if (this->HasParent())
	{
		this->DisconnectParentAndChild(this->m_Parent, this);
	}
}
void CTransform::RemoveChild(const CTransform* child)const
{
	if (child != NULL && this->HasChild())
	{
		this->DisconnectParentAndChild(this, child);
	}
}
void CTransform::RemoveChildByUniqueID(const ULONGLONG& id)const
{
	if (this->HasChild())
	{
		const CTransform* child = NULL;
		if (this->BaseModifyChildByUniqueID(id, child))
		{
			this->DisconnectParentAndChild(this, child);
		}
	}
}
void CTransform::RemoveAllChildren()const
{
	if (this->HasChild())
	{
		for (auto& child : this->m_Children)
		{
			if (child.second != NULL)
			{
				this->DisconnectParentAndChild(this, child.second);
			}
		}
		this->m_Children.clear();
	}
}
BOOL CTransform::IsBelongGameObject(const CGameObject* gameObject)const
{
	if (gameObject != NULL && this->HasGameObject())
	{
		return (this->m_GameObject->GetUniqueID() == gameObject->GetUniqueID());
	}
	return FALSE;
}
BOOL CTransform::HasGameObject()const
{
	return (this->m_GameObject != NULL);
}
BOOL CTransform::HasParent()const
{
	return (this->m_Parent != NULL);
}
BOOL CTransform::HasChild()const
{
	return (this->m_Children.size() > 0);
}
BOOL CTransform::IsParent(const CTransform* parent)const
{
	if (parent != NULL && this->HasParent())
	{
		return (this->m_Parent->GetUniqueID() == parent->GetUniqueID());
	}
	return FALSE;
}
BOOL CTransform::IsChild(const CTransform* child)const
{
	if (child != NULL && this->HasChild())
	{
		auto& element = this->m_Children.find(child->GetUniqueID());
		if (element == this->m_Children.end())
		{
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
void CTransform::BaseAddChild(const CTransform* child)const
{
	if (child != NULL)
	{
		if (!this->BaseFindChildByUniqueID(child->GetUniqueID()))
		{
			this->m_Children.insert_or_assign(child->GetUniqueID(), child);
		}
	}
}
void CTransform::BaseRemoveChildByUniqueID(const ULONGLONG& id)const
{
	if (this->HasChild())
	{
		if (this->BaseFindChildByUniqueID(id))
		{
			this->m_Children.erase(id);
		}
	}
}
BOOL CTransform::BaseFindChildByUniqueID(const ULONGLONG& id)const
{
	if (this->HasChild())
	{
		auto& element = this->m_Children.find(id);
		if (element == this->m_Children.end())
		{
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
BOOL CTransform::BaseModifyChildByUniqueID(const ULONGLONG& id, const CTransform*& output)const
{
	if (this->HasChild())
	{
		auto& element = this->m_Children.find(id);
		if (element == this->m_Children.end())
		{
			return FALSE;
		}
		output = element->second;
		return TRUE;
	}
	return FALSE;
}
void CTransform::ConnectParentAndChild(const CTransform* parent, const CTransform* child)const
{
	if (parent == NULL || child == NULL)
	{
		return;
	}
	if (child->HasParent())
	{
		const CTransform* oldParent = child->m_Parent;
		if ((parent->GetUniqueID() == oldParent->GetUniqueID()) && parent->BaseFindChildByUniqueID(child->GetUniqueID()))
		{
			return;
		}
		child->CalculateCurrentLocalTransform(NULL);
		oldParent->BaseRemoveChildByUniqueID(child->GetUniqueID());
		child->m_Parent = NULL;
	}
	child->CalculateCurrentLocalTransform(parent);
	child->m_Parent = parent;
	parent->BaseAddChild(child);
}
void CTransform::DisconnectParentAndChild(const CTransform* parent, const CTransform* child)const
{
	if (parent == NULL || child == NULL)
	{
		return;
	}
	if (child->HasParent())
	{
		if (parent->GetUniqueID() == child->m_Parent->GetUniqueID())
		{
			child->CalculateCurrentLocalTransform(NULL);
			child->m_Parent = NULL;
		}
	}
	parent->BaseRemoveChildByUniqueID(child->GetUniqueID());
}
void CTransform::CalculateCurrentLocalTransform(const CTransform* newParent)const
{
	if (newParent != NULL)
	{
		this->m_LocalPosition = this->GetWorldPosition() - newParent->GetWorldPosition();
		this->m_LocalRotation = CustomType::Quaternion::MultiplyQuaternion(newParent->GetWorldRotation().Inverse(), this->GetWorldRotation());
		this->m_LocalScale = CustomType::Vector3::Reciprocal(newParent->GetWorldScale()) * this->GetWorldScale();
	}
	else
	{
		this->m_LocalPosition = this->GetWorldPosition();
		this->m_LocalRotation = this->GetWorldRotation();
		this->m_LocalScale = this->GetWorldScale();
	}
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
	if (this->HasParent())
	{
		CustomType::Vector3 parentWorldPosition(-(this->m_Parent->GetWorldPosition()));
		this->m_LocalPosition = parentWorldPosition + worldPosition;
	}
	else
	{
		this->m_LocalPosition = worldPosition;
	}
}
void CTransform::SetWorldRotation(const CustomType::Quaternion& worldRotation)
{
	if (this->HasParent())
	{
		this->m_LocalRotation = CustomType::Quaternion::MultiplyQuaternion(this->m_Parent->GetWorldRotation().Inverse(), worldRotation);
	}
	else
	{
		this->m_LocalRotation = worldRotation;
	}
}
void CTransform::SetWorldScale(const CustomType::Vector3& worldScale)
{
	if (this->HasParent())
	{
		this->m_LocalScale = CustomType::Vector3::Reciprocal(this->m_Parent->GetWorldScale()) * worldScale;
	}
	else
	{
		this->m_LocalScale = worldScale;
	}
}
CustomType::Vector3 CTransform::GetWorldPosition()const
{
	CustomType::Vector3 result(this->m_LocalPosition);
	this->RecursionWorldPosition(this->m_Parent, result);
	return result;
}
CustomType::Quaternion CTransform::GetWorldRotation()const
{
	CustomType::Quaternion result(this->m_LocalRotation);
	this->RecursionWorldRotation(this->m_Parent, result);
	return result;
}
CustomType::Vector3 CTransform::GetWorldScale()const
{
	CustomType::Vector3 result(this->m_LocalScale);
	this->RecursionWorldScale(this->m_Parent, result);
	return result;
}
void CTransform::RecursionWorldPosition(const CTransform* parent, CustomType::Vector3& position)const
{
	if (parent == NULL)
	{
		return;
	}
	position += parent->m_LocalPosition;
	this->RecursionWorldPosition(parent->m_Parent, position);
}
void CTransform::RecursionWorldRotation(const CTransform* parent, CustomType::Quaternion& rotation)const
{
	if (parent == NULL)
	{
		return;
	}
	rotation = CustomType::Quaternion::MultiplyQuaternion(parent->m_LocalRotation, rotation);
	this->RecursionWorldRotation(parent->m_Parent, rotation);
}
void CTransform::RecursionWorldScale(const CTransform* parent, CustomType::Vector3& scale)const
{
	if (parent == NULL)
	{
		return;
	}
	scale *= parent->m_LocalScale;
	this->RecursionWorldScale(parent->m_Parent, scale);
}
CustomType::Vector3 CTransform::GetForwardVector()const
{
	return (this->GetWorldRotation().MultiplyVector(CustomType::Vector3::ZVector()));
}
CustomType::Vector3 CTransform::GetUpVector()const
{
	return (this->GetWorldRotation().MultiplyVector(CustomType::Vector3::YVector()));
}
CustomType::Vector3 CTransform::GetRightVector()const
{
	return (this->GetWorldRotation().MultiplyVector(CustomType::Vector3::XVector()));
}
CustomType::Matrix4x4 CTransform::GetLocalToWorldMatrix()const
{
	return (CustomType::Matrix4x4(this->GetWorldPosition(), this->GetWorldRotation(), this->GetWorldScale()));
}
CustomType::Matrix4x4 CTransform::GetWorldToLocalMatrix()const
{
	return (this->GetLocalToWorldMatrix().Inverse());
}