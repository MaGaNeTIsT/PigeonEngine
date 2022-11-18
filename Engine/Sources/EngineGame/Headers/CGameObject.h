#pragma once

#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CObjectManager.h"
#include "./CComponent.h"

class CGameObject : public CObjectBase, protected std::enable_shared_from_this<CGameObject>
{
public:
	std::weak_ptr<class CScene>								GetScene();
	std::weak_ptr<CTransform>								GetTransform();
	std::weak_ptr<CustomStruct::CRenderBoundingBox>			GetRenderLocalBoundingBox();
	std::weak_ptr<CustomStruct::CRenderBoundingSphere>		GetRenderLocalBoundingSphere();
public:
	BOOL	IsBelongTransform(const CTransform* gameObject);
	BOOL	HasScene()const;
	BOOL	HasTransform()const;
	BOOL	HasRenderBoundingBox()const;
	BOOL	HasRenderBoundingSphere()const;
public:
	void	SetScene(std::weak_ptr<class CScene> scene);
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
	std::weak_ptr<CGameObject>					FindChildByTransformID(const ULONGLONG& id);
	template<class T>
	std::vector<const T*> GetChildrenListByType()const
	{
		std::vector<const T*> childrenList;
		if (this->HasTransform() && this->HasChild())
		{
			std::vector<std::weak_ptr<CTransform>> childrenTransformList = this->m_Transform->GetChildrenList();
			for (const auto& child : childrenTransformList)
			{
				std::shared_ptr<CTransform> sharedPrtTransform(child.lock());
				if (sharedPrtTransform && sharedPrtTransform->HasGameObject())
				{
					std::shared_ptr<CGameObject> sharedPrtGameObject(sharedPrtTransform->GetGameObject().lock());
					if (sharedPrtGameObject && typeid(*sharedPrtGameObject) == typeid(T))
					{
						childrenList.push_back(reinterpret_cast<const T*>(sharedPrtGameObject.get()));
					}
				}
			}
		}
		return childrenList;
	}
	template<class T>
	const T* FindFirstChildByType()const
	{
		if (this->HasTransform() && this->HasChild())
		{
			std::vector<std::weak_ptr<CTransform>> childrenTransformList = this->m_Transform->GetChildrenList();
			for (const auto& child : childrenTransformList)
			{
				std::shared_ptr<CTransform> sharedPrtTransform(child.lock());
				if (sharedPrtTransform && sharedPrtTransform->HasGameObject())
				{
					std::shared_ptr<CGameObject> sharedPrtGameObject(sharedPrtTransform->GetGameObject().lock());
					if (sharedPrtGameObject && typeid(*sharedPrtGameObject) == typeid(T))
					{
						return (reinterpret_cast<const T*>(sharedPrtGameObject.get()));
					}
				}
			}
		}
		return nullptr;
	}
public:
	void	SetParent(std::shared_ptr<CGameObject> parent);
	void	AddChild(std::shared_ptr<CGameObject> child);
public:
	void	RemoveParent();
	void	RemoveChild(std::shared_ptr<CGameObject> child);
	void	RemoveChildByTransformID(const ULONGLONG& id);
	void	RemoveChildren();
public:
	BOOL	IsParent(const CGameObject* parent);
	BOOL	IsChild(const CGameObject* child);
	BOOL	HasParent()const;
	BOOL	HasChild()const;
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
public:
	template<class T>
	const T* FindFirstComponentByType()const
	{
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second && typeid(*(component.second)) == typeid(T))
				{
					return (reinterpret_cast<const T*>(component.second.get()));
				}
			}
		}
		return nullptr;
	}
	template<class T>
	std::vector<const T*> GetComponentListByType()const
	{
		std::vector<const T*> componentList;
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second && typeid(*(component.second)) == typeid(T))
				{
					componentList.push_back(reinterpret_cast<const T*>(component.second.get()));
				}
			}
		}
		return componentList;
	}
	template<class T>
	const T* GetMeshComponent()const
	{
		if (this->HasComponent() && this->HasMeshComponent())
		{
			auto element = this->m_Components.find(this->m_MeshComponentID);
			if (element != this->m_Components.end())
			{
				if (typeid(*(element->second)) == typeid(T))
				{
					return (reinterpret_cast<const T*>((element->second).get()));
				}
			}
		}
		return nullptr;
	}
	template<class T>
	const T* GetMeshRendererComponent()const
	{
		if (this->HasComponent() && this->HasMeshRendererComponent())
		{
			auto element = this->m_Components.find(this->m_MeshRendererComponentID);
			if (element != this->m_Components.end())
			{
				if (typeid(*(element->second)) == typeid(T))
				{
					return (reinterpret_cast<const T*>((element->second).get()));
				}
			}
		}
		return nullptr;
	}
public:
	BOOL	IsBelongComponent(const CBaseComponent* component);
	BOOL	HasComponent()const;
	BOOL	HasMeshComponent()const;
	BOOL	HasMeshRendererComponent()const;
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