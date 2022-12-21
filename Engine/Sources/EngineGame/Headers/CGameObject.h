#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CObjectManager.h"
#include "./CComponent.h"
#include "./CGameBoundComponent.h"

class CGameObjectBase : public CObjectBase
{
public:
	virtual void	Init()			= 0;
	virtual void	Uninit()		= 0;
	virtual void	Update()		= 0;
	virtual void	FixedUpdate()	= 0;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate() = 0;
#endif
public:
	BOOL					HasScene()const;
	BOOL					IsBelongScene(const class CScene* scene)const;
	const class CScene*		GetScene()const;
protected:
	const class CScene*		m_Scene;
public:
	const BOOL&		IsActive()const;
	void			Active();
	void			Inactive();
protected:
	BOOL			m_Active;
public:
	CGameObjectBase(const BOOL& active, const class CScene* scene);
	virtual ~CGameObjectBase();
};

class CGameObjectTransformBase : public CGameObjectBase
{
public:
	virtual void	Init()override {}
	virtual void	Uninit()override {}
	virtual void	Update()override {}
	virtual void	FixedUpdate()override {}
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override {}
#endif
public:
	CustomType::Vector3		GetForwardVector()const;
	CustomType::Vector3		GetUpVector()const;
	CustomType::Vector3		GetRightVector()const;
	CustomType::Matrix4x4	GetLocalToWorldMatrix()const;
	CustomType::Matrix4x4	GetWorldToLocalMatrix()const;
public:
	CustomType::Vector3		GetWorldPosition()const;
	CustomType::Quaternion	GetWorldRotation()const;
	CustomType::Vector3		GetWorldScale()const;
	CustomType::Vector3		GetLocalPosition()const;
	CustomType::Quaternion	GetLocalRotation()const;
	CustomType::Vector3		GetLocalScale()const;
public:
	void	SetWorldPosition(const CustomType::Vector3& worldPosition);
	void	SetWorldRotation(const CustomType::Quaternion& worldRotation);
	void	SetWorldScale(const CustomType::Vector3& worldScale);
	void	SetLocalPosition(const CustomType::Vector3& localPosition);
	void	SetLocalRotation(const CustomType::Quaternion& localRotation);
	void	SetLocalScale(const CustomType::Vector3& localScale);
public:
	BOOL			HasTransform()const;
	BOOL			IsBelongTransform(const ULONGLONG& transformID)const;
	CTransform*		GetTransform()const;
	ULONGLONG		GetTransformID()const;
	void			AddNewTransform(CGameObjectTransformBase* parent = nullptr);
	void			AddNewTransformWithValue(const CustomType::Vector3& worldPosition, const CustomType::Quaternion& worldRotation, const CustomType::Vector3& worldScale, CGameObjectTransformBase* parent = nullptr);
	void			RemoveTransform();
protected:
	void	BaseRemoveTransform(const BOOL& bDeleteTransform);
	void	ConnectGameObjectAndTransform(CGameObjectTransformBase* gameObject, CTransform* transform);
	void	DisconnectGameObjectAndTransform(CGameObjectTransformBase* gameObject, CTransform* transform);
public:
	BOOL	HasParent()const;
	BOOL	HasChild()const;
	BOOL	IsParent(const ULONGLONG& parentID)const;
	BOOL	IsChild(const ULONGLONG& childID)const;
public:
	void	SetParent(CGameObjectTransformBase* parent);
	void	AddChild(CGameObjectTransformBase* child);
public:
	void	RemoveParent();
	void	RemoveChild(CGameObjectTransformBase* child);
	void	RemoveChildByTransformID(const ULONGLONG& id);
	void	RemoveChildren();
public:
	CGameObjectTransformBase*					GetParent()const;
	std::vector<CGameObjectTransformBase*>		GetChildrenList()const;
	CGameObjectTransformBase*					GetChildByTransformID(const ULONGLONG& id)const;
	template<class T>
	std::vector<T*> GetChildrenListByType()const
	{
		std::vector<T*> childrenList;
		if (this->HasChild())
		{
			std::vector<CTransform*> childrenTransformList = this->m_Transform->GetChildrenList();
			for (const auto& child : childrenTransformList)
			{
				if (child != nullptr && child->HasGameObject())
				{
					CGameObjectTransformBase* childGameObject = child->GetGameObject();
					if (childGameObject != nullptr && typeid(*childGameObject) == typeid(T))
					{
						childrenList.push_back(reinterpret_cast<T*>(childGameObject));
					}
				}
			}
		}
		return childrenList;
	}
	template<class T>
	T* GetFirstChildByType()const
	{
		if (this->HasChild())
		{
			std::vector<CTransform*> childrenTransformList = this->m_Transform->GetChildrenList();
			for (const auto& child : childrenTransformList)
			{
				if (child != nullptr && child->HasGameObject())
				{
					CGameObjectTransformBase* childGameObject = child->GetGameObject();
					if (childGameObject != nullptr && typeid(*childGameObject) == typeid(T))
					{
						return (reinterpret_cast<T*>(childGameObject));
					}
				}
			}
		}
		return nullptr;
	}
protected:
	CTransform*		m_Transform;
public:
	CGameObjectTransformBase(const BOOL& active, const class CScene* scene);
	virtual ~CGameObjectTransformBase();
};

class CGameObject : public CGameObjectTransformBase
{
public:
	BOOL	HasRenderBoundingBox()const;
	BOOL	HasRenderBoundingSphere()const;
public:
	const CustomStruct::CRenderBoundingBox*		GetRenderLocalBoundingBox()const;
	const CustomStruct::CRenderBoundingSphere*	GetRenderLocalBoundingSphere()const;
public:
	void	GetRenderWorldAABBBoundingBox(CustomType::Vector3& boundingMin, CustomType::Vector3& boundingMax)const;
	void	SetRenderLocalBoundingBox(const CustomType::Vector3& anchor, const CustomType::Vector3& dimensions);
	void	SetRenderLocalBoundingBox(const CustomType::Vector3& dimensions);
public:
	void	GetRenderWorldBoundingSphere(CustomType::Vector3& anchor, FLOAT& radius)const;
	void	SetRenderLocalBoundingSphere(const CustomType::Vector3& anchor, const FLOAT& radius);
	void	SetRenderLocalBoundingSphere(const FLOAT& radius);
protected:
	CustomStruct::CRenderBoundingBox*		m_RenderBoundingBox;
	CustomStruct::CRenderBoundingSphere*	m_RenderBoundingSphere;
public:
	void				AddComponent(CBaseComponent* component);
	void				RemoveComponent(const CBaseComponent* component);
	void				RemoveComponentByComponentID(const ULONGLONG& id);
	void				RemoveComponents();
	CBaseComponent*		GetComponentByComponentID(const ULONGLONG& id)const;
public:
	template<class T>
	T* GetFirstComponentByType()const
	{
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second != nullptr && typeid(*(component.second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(component.second));
				}
			}
		}
		return nullptr;
	}
	template<class T>
	std::vector<T*> GetComponentListByType()const
	{
		std::vector<T*> componentList;
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second != nullptr && typeid(*(component.second)) == typeid(T))
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
		return nullptr;
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
		return nullptr;
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
		return nullptr;
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
				if (component.second != nullptr && typeid(*(component.second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(component.second));
				}
			}
		}
		return nullptr;
	}
	template<class T>
	std::vector<T*> BaseGetComponentListByType()const
	{
		std::vector<T*> componentList;
		if (this->HasComponent())
		{
			for (const auto& component : this->m_Components)
			{
				if (component.second != nullptr && typeid(*(component.second)) == typeid(T))
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
		return nullptr;
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
		return nullptr;
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
		return nullptr;
	}
public:
	BOOL	IsBelongComponent(const CBaseComponent* component)const;
	BOOL	IsBelongComponent(const ULONGLONG& componentID)const;
	BOOL	HasComponent()const;
	BOOL	HasMeshComponent()const;
	BOOL	HasMeshRendererComponent()const;
	BOOL	HasGameBoundComponent()const;
protected:
	BOOL	FindComponentByComponentID(const ULONGLONG& id, CBaseComponent*& component)const;
protected:
	ULONGLONG								m_MeshComponentID;
	ULONGLONG								m_MeshRendererComponentID;
	ULONGLONG								m_GameBoundComponentID;
	std::map<ULONGLONG, CBaseComponent*>	m_Components;
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	FixedUpdate()override;
#ifdef _DEVELOPMENT_EDITOR
protected:
	virtual void	SelectedEditorUpdate_RenderBounding();
public:
	virtual void	SelectedEditorUpdate()override;
#endif
public:
	CGameObject(const BOOL& active, const class CScene* scene);
	virtual ~CGameObject();
};