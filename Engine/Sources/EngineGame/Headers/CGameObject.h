#pragma once

#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../EngineRender/AssetsManager/Headers/CMesh.h"

class CObjectBase
{
public:
	CObjectBase();
	CObjectBase(const CObjectBase& obj);
	virtual ~CObjectBase();
public:
	const ULONGLONG& GetUniqueID()const;
public:
	BOOL operator==(const CObjectBase& obj);
	BOOL operator!=(const CObjectBase& obj);
protected:
	ULONGLONG m_UniqueID;
};

class CTransform : public CObjectBase
{
public:
	void	InitTransform(std::shared_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent = nullptr);
	void	InitTransform(std::shared_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition, std::shared_ptr<CTransform> parent = nullptr);
	void	InitTransform(std::shared_ptr<class CGameObject> gameObject, const CustomType::Quaternion& worldRotation, std::shared_ptr<CTransform> parent = nullptr);
	void	InitTransform(std::shared_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent = nullptr);
	void	InitTransform(std::shared_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, std::shared_ptr<CTransform> parent = nullptr);
	void	InitTransform(std::shared_ptr<class CGameObject> gameObject, const CustomType::Vector3& worldPosition, const CustomType::Vector3& worldScale, std::shared_ptr<CTransform> parent = nullptr);
public:
	std::shared_ptr<class CGameObject>					GetGameObject();
	std::shared_ptr<CTransform>							GetParent();
	std::shared_ptr<CTransform>							GetChildByUniqueID(const ULONGLONG& id);
	std::vector<std::shared_ptr<CTransform>>			GetChildrenList();
	std::map<ULONGLONG, std::shared_ptr<CTransform>>	GetChildrenMap();
public:
	void	SetGameObject(std::shared_ptr<class CGameObject> gameObject);
	void	SetParent(std::shared_ptr<CTransform> parent);
	void	AddChild(std::shared_ptr<CTransform> child);
public:
	void	RemoveParent();
	void	RemoveChild(std::shared_ptr<CTransform> child);
	void	RemoveChildByUniqueID(const ULONGLONG& id);
	void	RemoveChildren();
protected:
	std::shared_ptr<class CGameObject>					m_GameObject;
	std::shared_ptr<CTransform>							m_Parent;
	std::map<ULONGLONG, std::shared_ptr<CTransform>>	m_Children;
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
	void	RecursionWorldPosition(const std::shared_ptr<CTransform> parent, CustomType::Vector3& position);
	void	RecursionWorldRotation(const std::shared_ptr<CTransform> parent, CustomType::Quaternion& rotation);
	void	RecursionWorldScale(const std::shared_ptr<CTransform> parent, CustomType::Vector3& scale);
	void	CalculateCurrentLocalTransform(std::shared_ptr<CTransform> newParent);
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
	CTransform(const CTransform& transform);
	virtual ~CTransform();
};

class CGameObject : public CObjectBase
{
public:
	std::shared_ptr<class CScene>	GetScene();
	void							SetScene(std::shared_ptr<class CScene> scene);
	const BOOL&						IsActive()const { return (this->m_Active); }
	void							Active() { this->m_Active = TRUE; }
	void							Inactive() { this->m_Active = FALSE; }
public:
	std::shared_ptr<CTransform>		GetTransform() { return (this->m_Transform); }
protected:
	std::shared_ptr<class CScene>	m_Scene;
	BOOL							m_Active;
protected:
	std::shared_ptr<CTransform>								m_Transform;
	std::shared_ptr<CustomStruct::CRenderBoundingBox>		m_BoundingBox;
	std::shared_ptr<CustomStruct::CRenderBoundingSphere>	m_BoundingSphere;
public:
	CGameObject();
	CGameObject(const CGameObject& obj);
	virtual ~CGameObject();
//	virtual void	Init()			= 0;
//	virtual void	Uninit()		= 0;
//	virtual void	Update()		{}
//	virtual void	FixedUpdate()	{}
//	virtual void	Draw()			{}
//	virtual void	DrawExtra()		{}
//protected:
//	virtual void	PrepareDraw()	{}
//public:
//	void SetBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions)
//	{
//		if (m_BoundingBox == NULL)
//		{
//			m_BoundingBox = new CGameBoundingBox(anchor, dimensions);
//			return;
//		}
//		m_BoundingBox->Anchor = anchor;
//		m_BoundingBox->Dimensions = dimensions;
//	}
//	void SetBoundingBox(const CustomType::Vector3& dimensions)
//	{
//		CustomType::Vector3 tempAnchor(-dimensions.X(), -dimensions.Y(), -dimensions.Z());
//		tempAnchor = tempAnchor * 0.5f;
//		if (m_BoundingBox == NULL)
//		{
//			m_BoundingBox = new CGameBoundingBox(tempAnchor, dimensions);
//			return;
//		}
//		m_BoundingBox->Anchor = tempAnchor;
//		m_BoundingBox->Dimensions = dimensions;
//	}
//	CGameBoundingBox* GetBoundingBox() { return m_BoundingBox; }
//	void GetAABBBoundingBox(CustomType::Vector3& boundingMin, CustomType::Vector3& boundingMax)
//	{
//		if (m_BoundingBox == NULL)
//		{
//			boundingMin = m_Position;
//			boundingMax = CustomType::Vector3::Zero();
//			return;
//		}
//		FLOAT points[24];
//		{
//			CustomType::Vector3 rightVec(GetRightVector());
//			CustomType::Vector3 upVec(GetUpVector());
//			CustomType::Vector3 forwardVec(GetForwardVector());
//
//			CustomType::Vector3 tempPoint(rightVec * m_BoundingBox->Anchor.X() * m_Scale.X() + upVec * m_BoundingBox->Anchor.Y() * m_Scale.Y() + forwardVec * m_BoundingBox->Anchor.Z() * m_Scale.Z() + m_Position);
//			/*
//			//            5--------6                Y(up vector)
//			//           /|       /|                ^
//			//          / |      / |  (dimension y) |
//			//         4--------7  |                |    Z(forward vector)
//			//         |  1-----|--2                |   /
//			//         | /      | /                 |  /(dimension z)
//			//         |/       |/                  | /
//			// (anchor)0--------3                   0---------------->X(right vector)
//			//                                         (dimension x)
//			*/
//			points[0u * 3u + 0u] = tempPoint.X();
//			points[0u * 3u + 1u] = tempPoint.Y();
//			points[0u * 3u + 2u] = tempPoint.Z();
//
//			rightVec = rightVec * m_BoundingBox->Dimensions.X() * m_Scale.X();
//			upVec = upVec * m_BoundingBox->Dimensions.Y() * m_Scale.Y();
//			forwardVec = forwardVec * m_BoundingBox->Dimensions.Z() * m_Scale.Z();
//
//			points[1u * 3u + 0u] = points[0u * 3u + 0u] + forwardVec.X();
//			points[1u * 3u + 1u] = points[0u * 3u + 1u] + forwardVec.Y();
//			points[1u * 3u + 2u] = points[0u * 3u + 2u] + forwardVec.Z();
//			points[2u * 3u + 0u] = points[1u * 3u + 0u] + rightVec.X();
//			points[2u * 3u + 1u] = points[1u * 3u + 1u] + rightVec.Y();
//			points[2u * 3u + 2u] = points[1u * 3u + 2u] + rightVec.Z();
//			points[3u * 3u + 0u] = points[0u * 3u + 0u] + rightVec.X();
//			points[3u * 3u + 1u] = points[0u * 3u + 1u] + rightVec.Y();
//			points[3u * 3u + 2u] = points[0u * 3u + 2u] + rightVec.Z();
//			points[4u * 3u + 0u] = points[0u * 3u + 0u] + upVec.X();
//			points[4u * 3u + 1u] = points[0u * 3u + 1u] + upVec.Y();
//			points[4u * 3u + 2u] = points[0u * 3u + 2u] + upVec.Z();
//			points[5u * 3u + 0u] = points[4u * 3u + 0u] + forwardVec.X();
//			points[5u * 3u + 1u] = points[4u * 3u + 1u] + forwardVec.Y();
//			points[5u * 3u + 2u] = points[4u * 3u + 2u] + forwardVec.Z();
//			points[7u * 3u + 0u] = points[4u * 3u + 0u] + rightVec.X();
//			points[7u * 3u + 1u] = points[4u * 3u + 1u] + rightVec.Y();
//			points[7u * 3u + 2u] = points[4u * 3u + 2u] + rightVec.Z();
//			points[6u * 3u + 0u] = points[7u * 3u + 0u] + forwardVec.X();
//			points[6u * 3u + 1u] = points[7u * 3u + 1u] + forwardVec.Y();
//			points[6u * 3u + 2u] = points[7u * 3u + 2u] + forwardVec.Z();
//		}
//
//		{
//			FLOAT minPoint[3] = { points[0], points[1], points[2] };
//			FLOAT maxPoint[3] = { points[0], points[1], points[2] };
//			for (INT i = 1; i < 8; i++)
//			{
//				for (INT d = 0; d < 3; d++)
//				{
//					minPoint[d] = CustomType::CMath::Min(points[i * 3 + d], minPoint[d]);
//					maxPoint[d] = CustomType::CMath::Max(points[i * 3 + d], maxPoint[d]);
//				}
//			}
//			boundingMin = CustomType::Vector3(minPoint[0], minPoint[1], minPoint[2]);
//			boundingMax = CustomType::Vector3(maxPoint[0], maxPoint[1], maxPoint[2]);
//		}
//	}
//	void SetBoundingSphere(const CustomType::Vector3& anchor, const FLOAT& radius)
//	{
//		if (m_BoundingSphere == NULL)
//		{
//			m_BoundingSphere = new CGameBoundingSphere(anchor, radius);
//			return;
//		}
//		m_BoundingSphere->Anchor = anchor;
//		m_BoundingSphere->Radius = radius;
//	}
//	void SetBoundingSphere(const FLOAT& radius)
//	{
//		if (m_BoundingSphere == NULL)
//		{
//			m_BoundingSphere = new CGameBoundingSphere(CustomType::Vector3::Zero(), radius);
//			return;
//		}
//		m_BoundingSphere->Anchor = CustomType::Vector3::Zero();
//		m_BoundingSphere->Radius = radius;
//	}
//	void SetBoundingSphere(const CGameBoundingBox* boundingBox)
//	{
//		CustomType::Vector3 tempVec = boundingBox->Dimensions;
//		tempVec = tempVec * 0.5f;
//		if (m_BoundingSphere == NULL)
//		{
//			m_BoundingSphere = new CGameBoundingSphere(tempVec + boundingBox->Anchor, tempVec.Length());
//			return;
//		}
//		m_BoundingSphere->Anchor = tempVec + boundingBox->Anchor;
//		m_BoundingSphere->Radius = tempVec.Length();
//	}
//	CGameBoundingSphere* GetBoundingSphere() { return m_BoundingSphere; }
//	void GetBoundingSphere(CustomType::Vector3& anchor, FLOAT& radius)
//	{
//		if (m_BoundingSphere == NULL)
//		{
//			anchor = m_Position;
//			radius = 1.f;
//			return;
//		}
//		anchor = (m_BoundingSphere->Anchor * m_Scale) + m_Position;
//		radius = m_BoundingSphere->Radius * CustomType::CMath::Max(m_Scale.X(), CustomType::CMath::Max(m_Scale.Y(), m_Scale.Z()));
//	}
};