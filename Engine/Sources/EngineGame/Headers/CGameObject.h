#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CObjectManager.h"
#include "./CComponent.h"
#include "./CGameBoundComponent.h"

class CGameObject : public CObjectBase
{
public:
	const class CScene*								GetScene()const;
	const CTransform*								GetTransform()const;
	const CustomStruct::CRenderBoundingBox*			GetRenderLocalBoundingBox()const;
	const CustomStruct::CRenderBoundingSphere*		GetRenderLocalBoundingSphere()const;
public:
	void	SetScene(const class CScene* scene)const;
	void	AddNewTransform(const CGameObject* parent = NULL)const;
	void	AddNewTransformWithValue(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, const CGameObject* parent = NULL)const;
	void	SetRenderLocalBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions)const;
	void	SetRenderLocalBoundingBox(const CustomType::Vector3& dimensions)const;
	void	GetRenderWorldAABBBoundingBox(CustomType::Vector3& boundingMin, CustomType::Vector3& boundingMax)const;
	void	SetRenderLocalBoundingSphere(const CustomType::Vector3& anchor, const FLOAT& radius)const;
	void	SetRenderLocalBoundingSphere(const FLOAT& radius)const;
	void	GetRenderWorldBoundingSphere(CustomType::Vector3& anchor, FLOAT& radius)const;
public:
	void	RemoveTransform()const;
public:
	BOOL	IsBelongTransform(const CTransform* gameObject)const;
	BOOL	HasScene()const;
	BOOL	HasTransform()const;
	BOOL	HasRenderBoundingBox()const;
	BOOL	HasRenderBoundingSphere()const;
public:
	const CGameObject*					GetParent()const;
	std::vector<const CGameObject*>		GetChildrenList()const;
	const CGameObject*					GetChildByTransformID(const ULONGLONG& id)const;
	template<class T>
	std::vector<const T*> GetChildrenListByType()const
	{
		std::vector<const T*> childrenList;
		if (this->HasChild())
		{
			std::vector<const CTransform*> childrenTransformList = this->m_Transform->GetChildrenList();
			for (const auto& child : childrenTransformList)
			{
				if (child != NULL && child->HasGameObject())
				{
					const CGameObject* childGameObject = child->GetGameObject();
					if (childGameObject != NULL && typeid(*childGameObject) == typeid(T))
					{
						childrenList.push_back(reinterpret_cast<const T*>(childGameObject));
					}
				}
			}
		}
		return childrenList;
	}
	template<class T>
	const T* GetFirstChildByType()const
	{
		if (this->HasChild())
		{
			std::vector<const CTransform*> childrenTransformList = this->m_Transform->GetChildrenList();
			for (const auto& child : childrenTransformList)
			{
				if (child != NULL && child->HasGameObject())
				{
					const CGameObject* childGameObject = child->GetGameObject();
					if (childGameObject != NULL && typeid(*childGameObject) == typeid(T))
					{
						return (reinterpret_cast<const T*>(childGameObject));
					}
				}
			}
		}
		return NULL;
	}
public:
	void	SetParent(const CGameObject* parent)const;
	void	AddChild(const CGameObject* child)const;
public:
	void	RemoveParent()const;
	void	RemoveChild(const CGameObject* child)const;
	void	RemoveChildByTransformID(const ULONGLONG& id)const;
	void	RemoveChildren()const;
public:
	BOOL	IsParent(const CGameObject* parent)const;
	BOOL	IsChild(const CGameObject* child)const;
	BOOL	HasParent()const;
	BOOL	HasChild()const;
public:
	CustomType::Vector3			GetForwardVector()const;
	CustomType::Vector3			GetUpVector()const;
	CustomType::Vector3			GetRightVector()const;
	CustomType::Matrix4x4		GetLocalToWorldMatrix()const;
	CustomType::Matrix4x4		GetWorldToLocalMatrix()const;
	CustomType::Vector3			GetWorldPosition()const;
	CustomType::Quaternion		GetWorldRotation()const;
	CustomType::Vector3			GetWorldScale()const;
	CustomType::Vector3			GetLocalPosition()const;
	CustomType::Quaternion		GetLocalRotation()const;
	CustomType::Vector3			GetLocalScale()const;
public:
	void	SetWorldPosition(const CustomType::Vector3& worldPosition);
	void	SetWorldRotation(const CustomType::Quaternion& worldRotation);
	void	SetWorldScale(const CustomType::Vector3& worldScale);
	void	SetLocalPosition(const CustomType::Vector3& localPosition);
	void	SetLocalRotation(const CustomType::Quaternion& localRotation);
	void	SetLocalScale(const CustomType::Vector3& localScale);
protected:
	void	BaseRemoveTransform(const UINT& deleteTransform)const;
	void	ConnectGameObjectAndTransform(const CGameObject* gameObject, CTransform* transform)const;
	void	DisconnectGameObjectAndTransform(const CGameObject* gameObject, CTransform* transform)const;
protected:
	mutable const class CScene*							m_Scene;
	mutable CTransform*									m_Transform;
	mutable CustomStruct::CRenderBoundingBox*			m_RenderBoundingBox;
	mutable CustomStruct::CRenderBoundingSphere*		m_RenderBoundingSphere;
public:
	void						AddComponent(CBaseComponent* component);
	void						RemoveComponent(const CBaseComponent* component);
	void						RemoveComponentByComponentID(const ULONGLONG& id);
	void						RemoveComponents();
	CBaseComponent*				GetComponentByComponentID(const ULONGLONG& id)const;
public:
	template<class T>
	T* GetFirstComponentByType()const
	{
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second != NULL && typeid(*(component.second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(component.second));
				}
			}
		}
		return NULL;
	}
	template<class T>
	std::vector<T*> GetComponentListByType()const
	{
		std::vector<T*> componentList;
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second != NULL && typeid(*(component.second)) == typeid(T))
				{
					componentList.push_back(reinterpret_cast<T*>(component.second));
				}
			}
		}
		return componentList;
	}
	template<class T>
	T* GetMeshComponent()const
	{
		if (this->HasComponent() && this->HasMeshComponent())
		{
			auto& element = this->m_Components.find(this->m_MeshComponentID);
			if (element != this->m_Components.end())
			{
				if (typeid(*(element->second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(element->second));
				}
			}
		}
		return NULL;
	}
	template<class T>
	T* GetMeshRendererComponent()const
	{
		if (this->HasComponent() && this->HasMeshRendererComponent())
		{
			auto& element = this->m_Components.find(this->m_MeshRendererComponentID);
			if (element != this->m_Components.end())
			{
				if (typeid(*(element->second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(element->second));
				}
			}
		}
		return NULL;
	}
	template<class T>
	T* GetGameBoundComponent()const
	{
		if (this->HasComponent() && this->HasGameBoundComponent())
		{
			auto& element = this->m_Components.find(this->m_GameBoundComponentID);
			if (element != this->m_Components.end())
			{
				//TODO if parent type matched.
				return (reinterpret_cast<T*>(element->second));
			}
		}
		return NULL;
	}
protected:
	CBaseComponent* BaseGetComponentByComponentID(const ULONGLONG& id)const;
	template<class T>
	T* BaseGetFirstComponentByType()const
	{
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second != NULL && typeid(*(component.second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(component.second));
				}
			}
		}
		return NULL;
	}
	template<class T>
	std::vector<T*> BaseGetComponentListByType()const
	{
		std::vector<T*> componentList;
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second != NULL && typeid(*(component.second)) == typeid(T))
				{
					componentList.push_back(reinterpret_cast<T*>(component.second));
				}
			}
		}
		return componentList;
	}
	template<class T>
	T* BaseGetMeshComponent()const
	{
		if (this->HasComponent() && this->HasMeshComponent())
		{
			auto& element = this->m_Components.find(this->m_MeshComponentID);
			if (element != this->m_Components.end())
			{
				if (typeid(*(element->second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(element->second));
				}
			}
		}
		return NULL;
	}
	template<class T>
	T* BaseGetMeshRendererComponent()const
	{
		if (this->HasComponent() && this->HasMeshRendererComponent())
		{
			auto& element = this->m_Components.find(this->m_MeshRendererComponentID);
			if (element != this->m_Components.end())
			{
				if (typeid(*(element->second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(element->second));
				}
			}
		}
		return NULL;
	}
	template<class T>
	T* BaseGetGameBoundComponent()const
	{
		if (this->HasComponent() && this->HasGameBoundComponent())
		{
			auto& element = this->m_Components.find(this->m_GameBoundComponentID);
			if (element != this->m_Components.end())
			{
				if (typeid(*(element->second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(element->second));
				}
			}
		}
		return NULL;
	}
public:
	BOOL	IsBelongComponent(const CBaseComponent* component)const;
	BOOL	HasComponent()const;
	BOOL	HasMeshComponent()const;
	BOOL	HasMeshRendererComponent()const;
	BOOL	HasGameBoundComponent()const;
protected:
	BOOL	FindComponentByComponentID(const ULONGLONG& id, CBaseComponent*& component);
protected:
	ULONGLONG								m_MeshComponentID;
	ULONGLONG								m_MeshRendererComponentID;
	ULONGLONG								m_GameBoundComponentID;
	std::map<ULONGLONG, CBaseComponent*>	m_Components;
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
#ifdef _DEVELOPMENT_EDITOR
protected:
	virtual void	SelectedEditorUpdate_RenderBounding();
public:
	virtual void	SelectedEditorUpdate();
#endif
public:
	CGameObject();
	virtual ~CGameObject();
};