#pragma once

#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CBaseType.h"
#include "../../Headers/Base/CRenderDevice.h"

class CScene;

class CGameObject
{
public:
	enum GameObjectTransparencyEnum
	{
		GAMEOBJECT_OPAQUE		= 0,
		GAMEOBJECT_TRANSPARENT	= 1,
		GAMEOBJECT_SKY			= 2
	};
public:
	void SetTransparent(GameObjectTransparencyEnum transparent = GAMEOBJECT_TRANSPARENT) { m_Transparency = transparent; }
public:
	GameObjectTransparencyEnum		GetTransparency()const { return m_Transparency; }

	const CustomType::Vector3&		GetPosition()const { return m_Position; }
	const CustomType::Quaternion&	GetRotation()const { return m_Rotation; }
	const CustomType::Vector3&		GetScale()const { return m_Scale; }

	void SetPosition(const CustomType::Vector3 Position) { m_Position = Position; }
	void SetRotation(const CustomType::Quaternion& Rotation) { m_Rotation = Rotation; }
	void SetScale(const CustomType::Vector3 Scale) { m_Scale = Scale; }
protected:
	void RecurWorldMatrix(CGameObject* obj, CustomType::Matrix4x4& m)
	{
		CustomType::Matrix4x4 temp(obj->m_Position, obj->m_Rotation, obj->m_Scale);
		m = temp * m;
		if (obj->m_Parent != NULL)
			RecurWorldMatrix(obj->m_Parent, m);
	}
public:
	CustomType::Vector3& GetForwardVector()
	{
		CustomType::Vector3 result(0.f, 0.f, 1.f);
		return (m_Rotation.MultiplyVector(result));
	}
	CustomType::Vector3& GetUpVector()
	{
		CustomType::Vector3 result(0.f, 1.f, 0.f);
		return (m_Rotation.MultiplyVector(result));
	}
	CustomType::Vector3& GetRightVector()
	{
		CustomType::Vector3 result(1.f, 0.f, 0.f);
		return (m_Rotation.MultiplyVector(result));
	}
	CustomType::Matrix4x4& GetLocalToWorldMatrix()
	{
		CustomType::Matrix4x4 result(m_Position, m_Rotation, m_Scale);
		if (m_Parent != NULL)
			RecurWorldMatrix(m_Parent, result);
		return result;
	}
	CustomType::Matrix4x4& GetWorldToLocalMatrix()
	{
		CustomType::Matrix4x4 result(GetLocalToWorldMatrix().Inverse());
		return result;
	}
public:
	void SetScene(CScene* scene) { m_Scene = scene; }
	void SetParent(CGameObject* parent)
	{
		if (m_Parent != NULL)
			m_Parent->RemoveChild(this);
		m_Parent = parent;
		parent->AddChild(this);
	}
	void AddChild(CGameObject* child) { m_Child.insert_or_assign(child->m_UID, child); }
	void RemoveChild(CGameObject* child) { m_Child.erase(child->m_UID); }
protected:
	GameObjectTransparencyEnum			m_Transparency	= GAMEOBJECT_OPAQUE;

	ULONGLONG							m_UID;
	CScene*								m_Scene			= NULL;
	CGameObject*						m_Parent		= NULL;
	std::map<ULONGLONG,CGameObject*>	m_Child;

	CustomType::Vector3					m_Position;
	CustomType::Quaternion				m_Rotation;
	CustomType::Vector3					m_Scale;
public:
	CGameObject(ULONGLONG uid) { m_UID = uid; }
	virtual ~CGameObject() {}
	virtual void	Init()			= 0;
	virtual void	Uninit()		= 0;
	virtual void	Update()		= 0;
	virtual void	Draw()			= 0;
	virtual void	DrawDeferred()	= 0;
	virtual void	DrawShadow()	= 0;
};