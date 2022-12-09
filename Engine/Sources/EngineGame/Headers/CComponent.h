#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CObjectManager.h"

class CTransform : public CObjectBase
{
public:
	void	InitTransform(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale);
	void	InitTransform(const CustomType::Vector3& worldPosition);
	void	InitTransform(const CustomType::Quaternion& worldRotation);
	void	InitTransform(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation);
	void	InitTransform(const CustomType::Vector3& worldPosition, const CustomType::Vector3& worldScale);
public:
	const class CGameObject*					GetGameObject()const;
	const CTransform*							GetParent()const;
	UINT										GetChildrenNum()const;
	const CTransform*							GetChildByUniqueID(const ULONGLONG& id)const;
	std::vector<const CTransform*>				GetChildrenList()const;
	std::map<ULONGLONG, const CTransform*>		GetChildrenMap()const;
public:
	void	SetParent(const CTransform* parent)const;
	void	AddChild(const CTransform* child)const;
public:
	void	RemoveParent()const;
	void	RemoveChild(const CTransform* child)const;
	void	RemoveChildByUniqueID(const ULONGLONG& id)const;
	void	RemoveAllChildren()const;
public:
	BOOL	IsBelongGameObject(const class CGameObject* transform)const;
	BOOL	HasGameObject()const;
	BOOL	HasParent()const;
	BOOL	HasChild()const;
	BOOL	IsParent(const CTransform* parent)const;
	BOOL	IsChild(const CTransform* child)const;
protected:
	void	BaseAddChild(const CTransform* child)const;
	void	BaseRemoveChildByUniqueID(const ULONGLONG& id)const;
	BOOL	BaseFindChildByUniqueID(const ULONGLONG& id)const;
	BOOL	BaseModifyChildByUniqueID(const ULONGLONG& id, const CTransform*& output)const;
	void	ConnectParentAndChild(const CTransform* parent, const CTransform* child)const;
	void	DisconnectParentAndChild(const CTransform* parent, const CTransform* child)const;
	void	CalculateCurrentLocalTransform(const CTransform* newParent)const;
protected:
	const class CGameObject*							m_GameObject;
	mutable const CTransform*							m_Parent;
	mutable std::map<ULONGLONG, const CTransform*>		m_Children;
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
	CustomType::Vector3		GetWorldPosition()const;
	CustomType::Quaternion	GetWorldRotation()const;
	CustomType::Vector3		GetWorldScale()const;
protected:
	void	RecursionWorldPosition(const CTransform* parent, CustomType::Vector3& position)const;
	void	RecursionWorldRotation(const CTransform* parent, CustomType::Quaternion& rotation)const;
	void	RecursionWorldScale(const CTransform* parent, CustomType::Vector3& scale)const;
protected:
	mutable CustomType::Vector3			m_LocalPosition;
	mutable CustomType::Quaternion		m_LocalRotation;
	mutable CustomType::Vector3			m_LocalScale;
public:
	CustomType::Vector3		GetForwardVector()const;
	CustomType::Vector3		GetUpVector()const;
	CustomType::Vector3		GetRightVector()const;
	CustomType::Matrix4x4	GetLocalToWorldMatrix()const;
	CustomType::Matrix4x4	GetWorldToLocalMatrix()const;
#ifdef _DEVELOPMENT_EDITOR
public:
	void	SelectedEditorUpdate();
protected:
	BOOL	m_RealTimeChangeValue;
	FLOAT	m_EditorRotation[3];
#endif
public:
	CTransform();
	virtual ~CTransform() {}
protected:
	friend class CGameObject;
};

class CBaseComponent : public CObjectBase
{
public:
	virtual void	Init()			= 0;
	virtual void	Uninit()		= 0;
	virtual void	Update() {}
	virtual void	FixedUpdate() {}
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate() {}
#endif
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
	const class CGameObject* GetGameObject() { return (this->m_GameObject); }
	void SetGameObject(const class CGameObject* gameObject)
	{
		if (gameObject != NULL)
		{
			this->m_GameObject = gameObject;
		}
	}
protected:
	const class CGameObject*	m_GameObject;
public:
	CBaseComponent(const BOOL& active, const BOOL& needUpdate, const BOOL& needFixedUpdate)
	{
		this->m_NeedUpdate		= needUpdate;
		this->m_NeedFixedUpdate	= needFixedUpdate;
		this->m_Active			= active;
		this->m_GameObject		= NULL;
	}
	virtual ~CBaseComponent() {}
};

class CRenderComponent : public CBaseComponent
{
public:
	virtual void	Init()override {}
	virtual void	Uninit()override {}
	virtual void	Update()override {}
	virtual void	FixedUpdate()override {}
public:
	virtual void	Draw()const			= 0;
	virtual void	DrawExtra()const	= 0;
public:
	CRenderComponent(const BOOL& active, const BOOL& needUpdate, const BOOL& needFixedUpdate) : CBaseComponent(active, needUpdate, needFixedUpdate) {}
	virtual ~CRenderComponent() {}
};