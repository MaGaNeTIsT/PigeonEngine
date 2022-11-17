#pragma once

#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CObjectManager.h"
#include "./CComponent.h"

class CGameObject : public CObjectBase, public std::enable_shared_from_this<CGameObject>
{
public:
	std::weak_ptr<class CScene>								GetScene();
	std::weak_ptr<CTransform>								GetTransform();
	std::weak_ptr<CustomStruct::CRenderBoundingBox>			GetRenderLocalBoundingBox();
	std::weak_ptr<CustomStruct::CRenderBoundingSphere>		GetRenderLocalBoundingSphere();
public:
	void	SetScene(std::shared_ptr<class CScene> scene);
	void	AddNewTransform(std::shared_ptr<CGameObject> parent = nullptr);
	void	AddNewTransformWithValue(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, std::shared_ptr<CGameObject> parent = nullptr);
public:
	CustomType::Vector3			GetForwardVector();
	CustomType::Vector3			GetUpVector();
	CustomType::Vector3			GetRightVector();
	CustomType::Matrix4x4		GetLocalToWorldMatrix();
	CustomType::Matrix4x4		GetWorldToLocalMatrix();
	CustomType::Vector3			GetWorldPosition();
	CustomType::Quaternion		GetWorldRotation();
	CustomType::Vector3			GetWorldScale();
	CustomType::Vector3			GetLocalPosition();
	CustomType::Quaternion		GetLocalRotation();
	CustomType::Vector3			GetLocalScale();
public:
	void	SetWorldPosition(const CustomType::Vector3& worldPosition);
	void	SetWorldRotation(const CustomType::Quaternion& worldRotation);
	void	SetWorldScale(const CustomType::Vector3& worldScale);
	void	SetLocalPosition(const CustomType::Vector3& localPosition);
	void	SetLocalRotation(const CustomType::Quaternion& localRotation);
	void	SetLocalScale(const CustomType::Vector3& localScale);
public:
	std::weak_ptr<CGameObject>					GetParent();
	std::vector<std::weak_ptr<CGameObject>>		GetChildrenList();
	template<class T>
	std::vector<std::weak_ptr<T>>				GetChildrenListByType();
	std::weak_ptr<CGameObject>					FindChildByTransformID(const ULONGLONG& id);
	template<class T>
	std::weak_ptr<T>							FindFirstChildByType();
public:
	void	SetParent(std::shared_ptr<CGameObject> parent);
	void	AddChild(std::shared_ptr<CGameObject> child);
public:
	void	RemoveParent();
	void	RemoveChild(std::shared_ptr<CGameObject> child);
	void	RemoveChildByTransformID(const ULONGLONG& id);
	void	RemoveChildren();
public:
	BOOL	IsBelongTransform(const CTransform* gameObject);
	BOOL	IsParent(const CGameObject* parent);
	BOOL	IsChild(const CGameObject* child);
public:
	void	SetRenderLocalBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions);
	void	SetRenderLocalBoundingBox(const CustomType::Vector3& dimensions);
	void	GetRenderWorldAABBBoundingBox(CustomType::Vector3& boundingMin, CustomType::Vector3& boundingMax);
	void	SetRenderLocalBoundingSphere(const CustomType::Vector3& anchor, const FLOAT& radius);
	void	SetRenderLocalBoundingSphere(const FLOAT& radius);
	void	GetRenderWorldBoundingSphere(CustomType::Vector3& anchor, FLOAT& radius);
protected:
	std::weak_ptr<class CScene>								m_Scene;
	std::shared_ptr<CTransform>								m_Transform;
	std::shared_ptr<CustomStruct::CRenderBoundingBox>		m_RenderBoundingBox;
	std::shared_ptr<CustomStruct::CRenderBoundingSphere>	m_RenderBoundingSphere;
public:
	void								AddComponent(std::shared_ptr<CBaseComponent> component);
	void								RemoveComponent(std::shared_ptr<CBaseComponent> component);
	void								RemoveComponentByComponentID(const ULONGLONG& id);
	void								RemoveComponents();
	std::weak_ptr<CBaseComponent>		FindComponentByComponentID(const ULONGLONG& id);
	template<class T>
	std::weak_ptr<T>					FindFirstComponentByType();
	template<class T>
	std::vector<std::weak_ptr<T>>		GetComponentListByType();
	template<class T>
	std::weak_ptr<T>					GetMeshComponent();
	template<class T>
	std::weak_ptr<T>					GetMeshRendererComponent();
public:
	BOOL	IsBelongComponent(const CBaseComponent* component);
	BOOL	HasComponent();
	BOOL	HasMeshComponent();
	BOOL	HasMeshRendererComponent();
protected:
	ULONGLONG												m_MeshComponentID;
	ULONGLONG												m_MeshRendererComponentID;
	std::map<ULONGLONG, std::shared_ptr<CBaseComponent>>	m_Components;
public:
	const BOOL&		IsActive()const;
	void			Active();
	void			Inactive();
protected:
	BOOL	m_Active;
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	FixedUpdate();
public:
	CGameObject();
	CGameObject(const CGameObject& obj);
	virtual ~CGameObject();
};