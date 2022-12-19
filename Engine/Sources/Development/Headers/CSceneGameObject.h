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
	class CMeshRendererComponent*	GetMeshRendererComponentNotConst();
	class CMeshComponent*			GetMeshComponentNotConst();
public:
	virtual void	SelectedEditorUpdate()override;
protected:
	enum DefaultMaterialType
	{
		DefaultMaterialType_DefaultLit			= 0,
		DefaultMaterialType_Anisotropic			= 1,
		DefaultMaterialType_ClearCoat			= 2,
		DefaultMaterialType_Cloth				= 3,
		DefaultMaterialType_ClothAnisotropic	= 4,
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
	CHAR							m_SaveMaterialPath[512];
	CHAR							m_LoadMaterialPath[512];
#endif
public:
	CSceneGameObject();
	virtual ~CSceneGameObject();
};