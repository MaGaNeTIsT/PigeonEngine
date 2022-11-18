#pragma once

#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CObjectManager.h"

class CTransform : public CObjectBase, protected std::enable_shared_from_this<CTransform>
{
public:
	void	InitTransform(std::weak_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale);
	void	InitTransform(std::weak_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition);
	void	InitTransform(std::weak_ptr<class CGameObject> gameObject, const CustomType::Quaternion& worldRotation);
	void	InitTransform(std::weak_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation);
	void	InitTransform(std::weak_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Vector3& worldScale);
public:
	std::weak_ptr<class CGameObject>					GetGameObject();
	std::weak_ptr<CTransform>							GetParent();
	UINT												GetChildrenNum();
	std::weak_ptr<CTransform>							GetChildByUniqueID(const ULONGLONG& id);
	std::vector<std::weak_ptr<CTransform>>				GetChildrenList();
	std::map<ULONGLONG, std::weak_ptr<CTransform>>		GetChildrenMap();
public:
	void	SetGameObject(std::weak_ptr<class CGameObject> gameObject);
	void	SetParent(std::shared_ptr<CTransform> parent);
	void	AddChild(std::shared_ptr<CTransform> child);
public:
	void	RemoveParent();
	void	RemoveChild(std::shared_ptr<CTransform> child);
	void	RemoveChildByUniqueID(const ULONGLONG& id);
	void	RemoveAllChildren();
public:
	BOOL	IsBelongGameObject(const class CGameObject* transform);
	BOOL	HasGameObject();
	BOOL	HasParent();
	BOOL	HasChild();
	BOOL	IsParent(const CTransform* parent);
	BOOL	IsChild(const CTransform* child);
protected:
	void	BaseAddChild(std::shared_ptr<CTransform> child);
	void	BaseRemoveChildByUniqueID(const ULONGLONG& id);
	BOOL	BaseFindChildByUniqueID(const ULONGLONG& id);
	BOOL	BaseModifyChildByUniqueID(const ULONGLONG& id, std::weak_ptr<CTransform>& output);
	void	ConnectParentAndChild(std::weak_ptr<CTransform> parent, std::weak_ptr<CTransform> child);
	void	DisconnectParentAndChild(std::weak_ptr<CTransform> parent, std::weak_ptr<CTransform> child);
	void	CalculateCurrentLocalTransform(std::shared_ptr<CTransform> newParent);
protected:
	std::weak_ptr<class CGameObject>					m_GameObject;
	std::weak_ptr<CTransform>							m_Parent;
	std::map<ULONGLONG, std::weak_ptr<CTransform>>		m_Children;
public:
	const CustomType::Vector3&		GetLocalPosition()const;
	const CustomType::Quaternion&	GetLocalRotation()const;
	const CustomType::Vector3&		GetLocalScale()const;
public:
	void	SetLocalPosition(const CustomType::Vector3& localPosition);
	void	SetLocalRotation(const CustomType::Quaternion& localRotation);
	void	SetLocalScale(const CustomType::Vector3& localScale);
public:
	void	SetWorldPosition(const CustomType::Vector3& worldPosition);
	void	SetWorldRotation(const CustomType::Quaternion& worldRotation);
	void	SetWorldScale(const CustomType::Vector3& worldScale);
public:
	CustomType::Vector3		GetWorldPosition();
	CustomType::Quaternion	GetWorldRotation();
	CustomType::Vector3		GetWorldScale();
protected:
	void	RecursionWorldPosition(const std::weak_ptr<CTransform> parent, CustomType::Vector3& position);
	void	RecursionWorldRotation(const std::weak_ptr<CTransform> parent, CustomType::Quaternion& rotation);
	void	RecursionWorldScale(const std::weak_ptr<CTransform> parent, CustomType::Vector3& scale);
protected:
	CustomType::Vector3			m_LocalPosition;
	CustomType::Quaternion		m_LocalRotation;
	CustomType::Vector3			m_LocalScale;
public:
	CustomType::Vector3		GetForwardVector();
	CustomType::Vector3		GetUpVector();
	CustomType::Vector3		GetRightVector();
	CustomType::Matrix4x4	GetLocalToWorldMatrix();
	CustomType::Matrix4x4	GetWorldToLocalMatrix();
public:
	CTransform();
	virtual ~CTransform() {}
};

class CBaseComponent : public CObjectBase
{
public:
	virtual void	Init()			= 0;
	virtual void	Uninit()		= 0;
	virtual void	Update()const {}
	virtual void	FixedUpdate()const {}
public:
	const BOOL&		NeedUpdate()const { return (this->m_NeedUpdate); }
	const BOOL&		NeedFixedUpdate()const { return (this->m_NeedFixedUpdate); }
public:
	const BOOL&		IsActive()const { return (this->m_Active); }
	void			Active() { this->m_Active = TRUE; }
	void			Inactive() { this->m_Active = FALSE; }
protected:
	BOOL	m_NeedUpdate;
	BOOL	m_NeedFixedUpdate;
	BOOL	m_Active;
public:
	std::weak_ptr<class CGameObject> GetGameObject() { return (this->m_GameObject); }
	void SetGameObject(std::weak_ptr<class CGameObject> gameObject)
	{
		if (!gameObject.expired())
		{
			this->m_GameObject = gameObject;
		}
	}
protected:
	std::weak_ptr<class CGameObject>	m_GameObject;
public:
	CBaseComponent(const BOOL& active, const BOOL& needUpdate, const BOOL& needFixedUpdate)
	{
		this->m_NeedUpdate		= needUpdate;
		this->m_NeedFixedUpdate	= needFixedUpdate;
		this->m_Active			= active;
		this->m_GameObject.reset();
	}
	virtual ~CBaseComponent() {}
};

class CRenderComponent : public CBaseComponent
{
public:
	virtual void	Init()override {}
	virtual void	Uninit()override {}
	virtual void	Update()const override {}
	virtual void	FixedUpdate()const override {}
public:
	virtual void	Draw()const			= 0;
	virtual void	DrawExtra()const	= 0;
public:
	CRenderComponent(const BOOL& active, const BOOL& needUpdate, const BOOL& needFixedUpdate) : CBaseComponent(active, needUpdate, needFixedUpdate) {}
	virtual ~CRenderComponent() {}
};