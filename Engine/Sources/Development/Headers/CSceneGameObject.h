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
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
protected:
	enum DefaultMaterialType
	{
		DefaultMaterialType_DefaultLit	= 0,
		DefaultMaterialType_Cloth		= 1,
		DefaultMaterialType_Count
	};
protected:
	class CMeshRendererComponent*	m_MeshRendererComponent;
	class CMeshComponent*			m_MeshComponent;
	INT								m_CurrentMaterialType;
	INT								m_PreviousMaterialType;
	BOOL							m_LoadBaseModel;
	CHAR							m_LoadCustomModelPath[512];
	INT								m_CurrentMeshType;
	INT								m_PreviousMeshType;
#endif
public:
	CSceneGameObject();
	virtual ~CSceneGameObject();
};