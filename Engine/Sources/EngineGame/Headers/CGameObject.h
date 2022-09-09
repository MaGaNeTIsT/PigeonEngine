#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../Headers/CGameObjectManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"

class CScene;

class CGameObject
{
public:
	const CustomType::Vector3&		GetPosition()const { return m_Position; }
	const CustomType::Quaternion&	GetRotation()const { return m_Rotation; }
	const CustomType::Vector3&		GetScale()const { return m_Scale; }

	void SetPosition(const CustomType::Vector3& Position) { m_Position = Position; }
	void SetRotation(const CustomType::Quaternion& Rotation) { m_Rotation = Rotation; }
	void SetScale(const CustomType::Vector3& Scale) { m_Scale = Scale; }
protected:
	void RecurWorldMatrix(CGameObject* obj, CustomType::Matrix4x4& m)
	{
		CustomType::Matrix4x4 temp(obj->m_Position, obj->m_Rotation, obj->m_Scale);
		m = temp * m;
		if (obj->m_Parent != NULL)
			RecurWorldMatrix(obj->m_Parent, m);
	}
public:
	CustomType::Vector3 GetForwardVector()
	{
		CustomType::Vector3 result(0.f, 0.f, 1.f);
		return (m_Rotation.MultiplyVector(result));
	}
	CustomType::Vector3 GetUpVector()
	{
		CustomType::Vector3 result(0.f, 1.f, 0.f);
		return (m_Rotation.MultiplyVector(result));
	}
	CustomType::Vector3 GetRightVector()
	{
		CustomType::Vector3 result(1.f, 0.f, 0.f);
		return (m_Rotation.MultiplyVector(result));
	}
	CustomType::Matrix4x4 GetLocalToWorldMatrix()
	{
		CustomType::Matrix4x4 result(m_Position, m_Rotation, m_Scale);
		if (m_Parent != NULL)
			RecurWorldMatrix(m_Parent, result);
		return result;
	}
	CustomType::Matrix4x4 GetWorldToLocalMatrix()
	{
		CustomType::Matrix4x4 result(GetLocalToWorldMatrix().Inverse());
		return result;
	}
public:
	const ULONGLONG& GetGameObjectID()const { return m_UID; }
	void SetScene(CScene* scene) { m_Scene = scene; }
	void SetParent(CGameObject* parent)
	{
		if (m_Parent != NULL)
			m_Parent->RemoveChild(this);
		m_Parent = parent;
		parent->AddChild(this);
	}
	void AddChild(CGameObject* child) { m_Child[child->m_UID] = child; }
	void RemoveChild(CGameObject* child) { m_Child.erase(child->m_UID); }
public:
	void AddMesh(CMesh* mesh) { m_Mesh = mesh; }
	void AddMeshRenderer(CMeshRenderer* meshRenderer) { m_MeshRenderer = meshRenderer; }
	CMesh*			GetMesh()const { return m_Mesh; }
	CMeshRenderer*	GetMeshRenderer()const { return m_MeshRenderer; }
protected:
	ULONGLONG							m_UID;
	CScene*								m_Scene			= NULL;
	CGameObject*						m_Parent		= NULL;
	std::map<ULONGLONG,CGameObject*>	m_Child;

	CMesh*								m_Mesh			= NULL;
	CMeshRenderer*						m_MeshRenderer	= NULL;

	CustomType::Vector3					m_Position;
	CustomType::Quaternion				m_Rotation;
	CustomType::Vector3					m_Scale			= CustomType::Vector3(1.f, 1.f, 1.f);
public:
	CGameObject() { m_UID = CGameObjectManager::GetGameObjectID(); }
	virtual ~CGameObject()
	{
		if (m_MeshRenderer != NULL)
		{
			delete m_MeshRenderer;
			m_MeshRenderer = NULL;
		}
	}
	virtual void	Init()			= 0;
	virtual void	Uninit()		= 0;
	virtual void	Update()		{}
	virtual void	FixedUpdate()	{}
	virtual void	Draw()			{}
	virtual void	DrawExtra()		{}
protected:
	virtual void	PrepareDraw()	{}
};