#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "./CGameObjectManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMesh.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"

class CGameObject
{
public:
	struct CGameBoundingBox
	{
		CGameBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions)
		{
			Anchor		= anchor;
			Dimensions	= dimensions;
		}
		CustomType::Vector3 Anchor;
		CustomType::Vector3 Dimensions;
	};
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
	const BOOL& IsActive()const { return m_Active; }
	void Active() { m_Active = TRUE; }
	void Inactive() { m_Active = FALSE; }
	void SetScene(class CScene* scene) { m_Scene = scene; }
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
	CMesh* GetMesh()const { return m_Mesh; }
	CMeshRenderer* GetMeshRenderer()const { return m_MeshRenderer; }
protected:
	ULONGLONG							m_UID;
	BOOL								m_Active;
	class CScene*						m_Scene			= NULL;
	CGameObject*						m_Parent		= NULL;
	std::map<ULONGLONG,CGameObject*>	m_Child;

	CMesh*								m_Mesh			= NULL;
	CMeshRenderer*						m_MeshRenderer	= NULL;

	CGameBoundingBox*					m_BoundingBox	= NULL;

	CustomType::Vector3					m_Position;
	CustomType::Quaternion				m_Rotation;
	CustomType::Vector3					m_Scale			= CustomType::Vector3(1.f, 1.f, 1.f);
public:
	CGameObject() { m_UID = CGameObjectManager::GetGameObjectID(); m_Active = FALSE; }
	virtual ~CGameObject()
	{
		if (m_MeshRenderer != NULL)
		{
			delete m_MeshRenderer;
			m_MeshRenderer = NULL;
		}
		if (m_BoundingBox != NULL)
		{
			delete m_BoundingBox;
			m_BoundingBox = NULL;
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
public:
	void SetBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions)
	{
		if (m_BoundingBox == NULL)
		{
			m_BoundingBox = new CGameBoundingBox(anchor, dimensions);
			return;
		}
		m_BoundingBox->Anchor = anchor;
		m_BoundingBox->Dimensions = dimensions;
	}
	void SetBoundingBox(const CustomType::Vector3& dimensions)
	{
		CustomType::Vector3 tempAnchor(-dimensions.X(), -dimensions.Y(), -dimensions.Z());
		tempAnchor = tempAnchor * 0.5f;
		if (m_BoundingBox == NULL)
		{
			m_BoundingBox = new CGameBoundingBox(tempAnchor, dimensions);
			return;
		}
		m_BoundingBox->Anchor = tempAnchor;
		m_BoundingBox->Dimensions = dimensions;
	}
	CGameBoundingBox* GetBoundingBox() { return m_BoundingBox; }
	void GetAABBBoundingBox(CustomType::Vector3& boundingMin, CustomType::Vector3& boundingMax)
	{
		if (m_BoundingBox == NULL)
		{
			boundingMin = m_Position;
			boundingMax = CustomType::Vector3::Zero();
			return;
		}
		FLOAT points[24];
		{
			CustomType::Vector3 rightVec(GetRightVector());
			CustomType::Vector3 upVec(GetUpVector());
			CustomType::Vector3 forwardVec(GetForwardVector());

			CustomType::Vector3 tempPoint(rightVec * m_BoundingBox->Anchor.X() * m_Scale.X() + upVec * m_BoundingBox->Anchor.Y() * m_Scale.Y() + forwardVec * m_BoundingBox->Anchor.Z() * m_Scale.Z() + m_Position);
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

			rightVec = rightVec * m_BoundingBox->Dimensions.X() * m_Scale.X();
			upVec = upVec * m_BoundingBox->Dimensions.Y() * m_Scale.Y();
			forwardVec = forwardVec * m_BoundingBox->Dimensions.Z() * m_Scale.Z();

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
};