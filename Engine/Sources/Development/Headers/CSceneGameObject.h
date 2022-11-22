#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineGame/Headers/CGameObject.h"

class CSceneGameObject : public CGameObject
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	FixedUpdate()override;
#if _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
protected:
	class CMeshRendererComponent*	m_MeshRendererComponent;
	class CMeshComponent*			m_MeshComponent;
	BOOL							m_LoadBaseModel;
	INT								m_CurrentMeshType;
	INT								m_PreviousMeshType;
#endif
public:
	CSceneGameObject();
	virtual ~CSceneGameObject();
};