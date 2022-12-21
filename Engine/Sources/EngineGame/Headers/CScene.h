#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CObjectManager.h"
#include "./CGameObject.h"
//
//class CSceneNode
//{
//
//public:
//
//	BOOL IsHiddenInGame()const;
//	void SetHiddenInGame(BOOL HiddenInGame);
//
//	std::string GetNodeName()const;
//	void SetNodeName(const std::string& NewNodeName);
//
//private:
//
//	BOOL bHiddenInGame;
//	std::string Name;
//
//};

class CScene : public CObjectBase
{
public:
	enum SceneLayout
	{
		LAYOUT_TERRAIN		= 0,
		LAYOUT_OPAQUE		= 1,
		LAYOUT_TRANSPARENT	= 2,
		LAYOUT_COUNT
	};
protected:
	CGameObjectBase*						m_MainCamera;
	std::map<ULONGLONG, CGameObjectBase*>	m_Lights;
	std::map<ULONGLONG, CGameObjectBase*>	m_GameObjects[SceneLayout::LAYOUT_COUNT];
	std::map<ULONGLONG, CGameObjectBase*>	m_AllObjectList;
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	FixedUpdate();
protected:
	template <class T>
	T* GetMainCamera()const
	{
		if (this->m_MainCamera == nullptr)
		{
			return nullptr;
		}
		return (reinterpret_cast<T*>(this->m_MainCamera));
	}
	template <class T>
	T* AddMainCamera()
	{
		ULONGLONG id;
		if (this->m_MainCamera != nullptr)
		{
			id = this->m_MainCamera->GetUniqueID();
			this->m_MainCamera->Uninit();
			{
				this->m_AllObjectList[id] = nullptr;
				this->m_AllObjectList.erase(id);
			}
			delete (this->m_MainCamera);
		}
		this->m_MainCamera = new T(TRUE, this);
		this->m_MainCamera->Active();
		this->m_MainCamera->Init();
		id = this->m_MainCamera->GetUniqueID();
		(this->m_AllObjectList)[id] = this->m_MainCamera;
		return  (reinterpret_cast<T*>(this->m_MainCamera));
	}
	template <class T>
	T* AddLight()
	{
		CGameObject* gameObject = new T(TRUE, this);
		gameObject->Active();
		gameObject->Init();
		ULONGLONG id = gameObject->GetUniqueID();
		(this->m_Lights)[id] = gameObject;
		(this->m_AllObjectList)[id] = gameObject;
		return  (reinterpret_cast<T*>(gameObject));
	}
	template <class T>
	T* AddGameObject(const UINT& layout)
	{
		if (layout >= SceneLayout::LAYOUT_COUNT)
		{
			return nullptr;
		}
		CGameObject* gameObject = new T(TRUE, this);
		gameObject->Active();
		gameObject->Init();
		ULONGLONG id = gameObject->GetUniqueID();
		(this->m_GameObjects[layout])[id] = gameObject;
		(this->m_AllObjectList)[id] = gameObject;
		return  (reinterpret_cast<T*>(gameObject));
	}
	template <class T>
	T* GetGameObjectFirst(const UINT& layout)const
	{
		if (layout >= SceneLayout::LAYOUT_COUNT)
		{
			return nullptr;
		}
		if (this->m_GameObjects[layout].size() < 1)
		{
			return nullptr;
		}
		for (const auto& obj : (this->m_GameObjects[layout]))
		{
			if ((obj.second) != nullptr)
			{
				if (typeid(*(obj.second)) == typeid(T))
				{
					return (reinterpret_cast<T*>(obj.second));
				}
			}
		}
		return nullptr;
	}
	template <class T>
	T* GetGameObjectByIndex(const UINT& layout, const UINT& idx)const
	{
		if (layout >= SceneLayout::LAYOUT_COUNT)
		{
			return nullptr;
		}
		if (this->m_GameObjects[layout].size() < 1)
		{
			return nullptr;
		}
		UINT number = idx;
		for (const auto& obj : (this->m_GameObjects[layout]))
		{
			if ((obj.second) != nullptr)
			{
				if (typeid(*(obj.second)) == typeid(T))
				{
					if (number == 0)
					{
						return (reinterpret_cast<T*>(obj.second));
					}
					else
					{
						number -= 1;
					}
				}
			}
		}
		return nullptr;
	}
	template <class T>
	std::vector<T*>	GetGameObjectAll(const UINT& layout)const
	{
		if (layout >= SceneLayout::LAYOUT_COUNT)
		{
			return std::vector<T*>();
		}
		std::vector<T*> listObj;
		if (this->m_GameObjects[layout].size() < 1)
		{
			return listObj;
		}
		for (const auto& obj : (this->m_GameObjects[layout]))
		{
			if ((obj.second) != nullptr)
			{
				if (typeid(*(obj.second)) == typeid(T))
				{
					listObj.push_back((reinterpret_cast<T*>(obj.second)));
				}
			}
		}
		return listObj;
	}
	template <class T>
	std::vector<T*>	GetGameObjectAll()const
	{
		std::vector<T*> listObj;
		if (this->m_AllObjectList.size() < 1)
		{
			return listObj;
		}
		for (const auto& obj : (this->m_AllObjectList))
		{
			if ((obj.second) != nullptr)
			{
				if (typeid(*(obj.second)) == typeid(T))
				{
					listObj.push_back((reinterpret_cast<T*>(obj.second)));
				}
			}
		}
		return listObj;
	}
public:
	std::vector<CGameObjectBase*> GetGameObjectAll(const UINT& layout)const
	{
		if (layout >= SceneLayout::LAYOUT_COUNT)
		{
			return std::vector<CGameObjectBase*>();
		}
		std::vector<CGameObjectBase*> listObj;
		if (this->m_GameObjects[layout].size() < 1)
		{
			return listObj;
		}
		for (const auto& obj : (this->m_GameObjects[layout]))
		{
			if ((obj.second) != nullptr)
			{
				listObj.push_back(obj.second);
			}
		}
		return listObj;
	}
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	EditorUpdate();
protected:
	virtual void	OnClickReselectSceneObjectWithBound(const INT& mouseX, const INT& mouseY);
protected:
	BOOL				m_ShowSceneInfo;
	BOOL				m_ShowCameraInfo;
	BOOL				m_ShowLightInfo;
	BOOL				m_ShowSelectObjectInfo;
	CGameObjectBase*	m_SelectedObject;
#endif
public:
	CScene();
	virtual ~CScene();
protected:
	friend class CRenderPipeline;
};