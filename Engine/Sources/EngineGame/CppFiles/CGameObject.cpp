#include "../../../../Entry/EngineMain.h"
#include "../Headers/CGameObject.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"
#include "../Headers/CObjectManager.h"
#include "../Headers/CScene.h"

CObjectBase::CObjectBase()
{
	this->m_UniqueID = CUniqueIDManager::GetUniqueID();
}
CObjectBase::CObjectBase(const CObjectBase& obj)
{
	this->m_UniqueID = CUniqueIDManager::GetUniqueID();
}
CObjectBase::~CObjectBase()
{
}
const ULONGLONG& CObjectBase::GetUniqueID()const
{
	return (this->m_UniqueID);
}
BOOL CObjectBase::operator==(const CObjectBase& obj)
{
	return (this->m_UniqueID == obj.m_UniqueID);
}
BOOL CObjectBase::operator!=(const CObjectBase& obj)
{
	return (this->m_UniqueID != obj.m_UniqueID);
}

CTransform::CTransform()
{
	this->m_GameObject		= nullptr;
	this->m_Parent			= nullptr;
	this->m_LocalPosition	= CustomType::Vector3::Zero();
	this->m_LocalRotation	= CustomType::Quaternion::Identity();
	this->m_LocalScale		= CustomType::Vector3(1.f);
}
CTransform::CTransform(const CTransform& transform)
{
	this->m_GameObject		= transform.m_GameObject;
	this->m_Parent			= transform.m_Parent;
	this->m_LocalPosition	= transform.m_LocalPosition;
	this->m_LocalRotation	= transform.m_LocalRotation;
	this->m_LocalScale		= transform.m_LocalScale;
}
CTransform::~CTransform()
{
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent)
{
	this->m_Parent = nullptr;
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
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent)
{
	this->InitTransform(gameObject, CustomType::Vector3::Zero(), CustomType::Quaternion::Identity(), worldScale, parent);
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, std::shared_ptr<CTransform> parent)
{
	this->InitTransform(gameObject, worldPosition, worldRotation, CustomType::Vector3::One(), parent);
}
void CTransform::InitTransform(std::shared_ptr<CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent)
{
	this->InitTransform(gameObject, worldPosition, CustomType::Quaternion::Identity(), CustomType::Vector3::One(), parent);
}
std::shared_ptr<CGameObject> CTransform::GetGameObject()
{
	return (this->m_GameObject);
}
std::shared_ptr<CTransform> CTransform::GetParent()
{
	return (this->m_Parent);
}
std::shared_ptr<CTransform> CTransform::GetChildByUniqueID(const ULONGLONG& id)
{
	if (this->m_Children.size() > 0)
	{
		std::map<ULONGLONG, std::shared_ptr<CTransform>>::iterator element = this->m_Children.find(id);
		if (element == this->m_Children.end())
		{
			return nullptr;
		}
		return (element->second);
	}
	return nullptr;
}
std::vector<std::shared_ptr<CTransform>> CTransform::GetChildrenList()
{
	std::vector<std::shared_ptr<CTransform>> childrenList;
	if (this->m_Children.size() > 0)
	{
		for (const auto& child : this->m_Children)
		{
			if (child.second)
			{
				childrenList.push_back(child.second);
			}
		}
	}
	return childrenList;
}
std::map<ULONGLONG, std::shared_ptr<CTransform>> CTransform::GetChildrenMap()
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
	BOOL needNewAdd = TRUE;
	if (this->m_Children.size() > 0)
	{
		std::map<ULONGLONG, std::shared_ptr<CTransform>>::iterator element = this->m_Children.find(child->GetUniqueID());
		if (element != this->m_Children.end())
		{
			needNewAdd = FALSE;
		}
	}
	if (needNewAdd)
	{
		this->m_Children.insert_or_assign(child->GetUniqueID(), child);
	}
}
void CTransform::RemoveParent()
{
	if (this->m_Parent)
	{
		this->CalculateCurrentLocalTransform(nullptr);
		this->m_Parent = nullptr;
	}
}
void CTransform::RemoveChild(std::shared_ptr<CTransform> child)
{
	if (this->m_Children.size() > 0)
	{
		this->m_Children.erase(child->GetUniqueID());
	}
}
void CTransform::RemoveChildByUniqueID(const ULONGLONG& id)
{
	if (this->m_Children.size() > 0)
	{
		this->m_Children.erase(id);
	}
}
void CTransform::RemoveChildren()
{
	if (this->m_Children.size() > 0)
	{
		this->m_Children.clear();
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
	if (this->m_Parent)
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
	if (this->m_Parent)
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
	if (this->m_Parent)
	{
		this->m_LocalScale = CustomType::Vector3::Reciprocal(this->m_Parent->GetWorldScale()) * worldScale;
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
void CTransform::RecursionWorldPosition(const std::shared_ptr<CTransform> parent, CustomType::Vector3& position)
{
	if (!parent)
	{
		return;
	}
	position += parent->m_LocalPosition;
	this->RecursionWorldPosition(parent->m_Parent, position);
}
void CTransform::RecursionWorldRotation(const std::shared_ptr<CTransform> parent, CustomType::Quaternion& rotation)
{
	if (!parent)
	{
		return;
	}
	rotation = CustomType::Quaternion::MultiplyQuaternion(parent->m_LocalRotation, rotation);
	this->RecursionWorldRotation(parent->m_Parent, rotation);
}
void CTransform::RecursionWorldScale(const std::shared_ptr<CTransform> parent, CustomType::Vector3& scale)
{
	if (!parent)
	{
		return;
	}
	scale *= parent->m_LocalScale;
	this->RecursionWorldScale(parent->m_Parent, scale);
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

CGameObject::CGameObject()
{
	this->m_Active			= FALSE;
	this->m_Scene			= nullptr;
	this->m_Transform		= nullptr;
	this->m_BoundingBox		= nullptr;
	this->m_BoundingSphere	= nullptr;
}
CGameObject::CGameObject(const CGameObject& obj)
{
	this->m_Active = FALSE;
	this->m_Scene = obj.m_Scene;
	this->m_Transform = std::shared_ptr<CTransform>(new CTransform((*obj.m_Transform)));
	this->m_BoundingBox = obj.m_BoundingBox;
	this->m_BoundingSphere = obj.m_BoundingSphere;
}
CGameObject::~CGameObject()
{

}
std::shared_ptr<CScene> CGameObject::GetScene()
{
	return (this->m_Scene);
}
void CGameObject::SetScene(std::shared_ptr<CScene> scene)
{
	this->m_Scene = scene;
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
std::shared_ptr<CTransform> CGameObject::GetTransform()
{
	return (this->m_Transform);
}