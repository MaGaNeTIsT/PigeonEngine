#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineGame/Headers/CGameObject.h"

class CSceneSkeletonMeshObject : public CGameObject
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	FixedUpdate()override;
#ifdef _DEVELOPMENT_EDITOR
public:
	class CMeshRendererComponent*		GetMeshRendererComponentNotConst();
	class CMeshComponent*				GetMeshComponentNotConst();
	class CSkeletonComponent*			GetSkeletonComponentNotConst();
	class CSkeletonAnimationComponent*	GetSkeletonAnimationComponentNotConst();
public:
	virtual void	SelectedEditorUpdate()override;
protected:
	class CMeshRendererComponent*		m_MeshRendererComponent;
	class CSkeletonMeshComponent*		m_MeshComponent;
	class CSkeletonComponent*			m_SkeletonComponent;
	class CSkeletonAnimationComponent*	m_SkeletonAnimationComponent;
	CHAR								m_SaveMaterialPath[512];
	CHAR								m_LoadMaterialPath[512];
#endif
public:
	CSceneSkeletonMeshObject(const BOOL& active, const class CScene* scene);
	virtual ~CSceneSkeletonMeshObject();
};