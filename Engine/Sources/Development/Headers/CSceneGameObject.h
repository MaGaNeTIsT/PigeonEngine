#pragma once

#include "../../EngineGame/Headers/CGameObject.h"

class CSceneGameObject : public CGameObject
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	FixedUpdate()override;
public:
	CSceneGameObject();
	CSceneGameObject(const CSceneGameObject& sceneGameObject);
	virtual ~CSceneGameObject();
};