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
	T* AddGameObject(const UINT& layout);
	template <typename T>
	T* GetGameObjectFirst(const UINT& layout)const;
	template <typename T>
	T* GetGameObjectByIndex(const UINT& layout, const UINT& idx)const;
	template <typename T>
	std::vector<T*>	GetGameObjectAll(const UINT& layout)const;
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	FixedUpdate();
};