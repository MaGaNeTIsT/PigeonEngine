#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CGameObject.h"

class CScene
{
public:
	enum SceneLayout
	{
		LAYOUT_CAMERA		= 0,
		LAYOUT_LIGHT		= 1,
		LAYOUT_TERRAIN		= 2,
		LAYOUT_OPAQUE		= 3,
		LAYOUT_TRANSPARENT	= 4,
		LAYOUT_SKY			= 5,
		LAYOUT_COUNT
	};
protected:
	std::map<ULONGLONG, CGameObject*> m_GameObject[SceneLayout::LAYOUT_COUNT];
public:
	CScene();
	virtual ~CScene();
public:
	template <typename T>
	T* AddGameObject(const UINT& layout)
	{
		CGameObject* gameObject = new T();
		gameObject->SetScene(this);
		gameObject->Active();
		gameObject->Init();
		ULONGLONG id = gameObject->GetGameObjectID();
		(this->m_GameObject[layout])[id] = gameObject;
		return  (reinterpret_cast<T*>(gameObject));
	}
	template <typename T>
	T* GetGameObjectFirst(const UINT& layout)const
	{
		if (this->m_GameObject[layout].size() < 1)
			return NULL;
		for (const auto& obj : (this->m_GameObject[layout]))
		{
			if ((obj.second) != NULL)
			{
				if (typeid(*(obj.second)) == typeid(T))
					return (reinterpret_cast<T*>(obj.second));
			}
		}
		return NULL;
	}
	template <typename T>
	T* GetGameObjectByIndex(const UINT& layout, const UINT& idx)const
	{
		if (this->m_GameObject[layout].size() < 1)
			return NULL;
		UINT number = idx;
		for (const auto& obj : (this->m_GameObject[layout]))
		{
			if ((obj.second) != NULL)
			{
				if (typeid(*(obj.second)) == typeid(T))
				{
					if (number == 0)
						return (reinterpret_cast<T*>(obj.second));
					else
						number -= 1;
				}
			}
		}
		return NULL;
	}
	template <typename T>
	std::vector<T*>	GetGameObjectAll(const UINT& layout)const
	{
		std::vector<T*> listObj;
		if (this->m_GameObject[layout].size() < 1)
			return listObj;
		for (const auto& obj : (this->m_GameObject[layout]))
		{
			if ((obj.second) != NULL)
			{
				if (typeid(*(obj.second)) == typeid(T))
					listObj.push_back((reinterpret_cast<T*>(obj.second)));
			}
		}
		return listObj;
	}
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	FixedUpdate();
private:
	friend class CRenderPipeline;
};