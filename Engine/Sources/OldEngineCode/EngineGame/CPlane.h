/*
#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CGameObject.h"

class CTexture2D;

class CPlane : public CGameObject
{
	EClass(CPlane,CGameObject)
public:
	struct CPlaneMeshInfo
	{
		CustomType::Vector2		Length;
		CustomType::Vector2		UV;
		CustomType::Vector2Int	VertexCount;
	};
public:
	virtual void	Init()override;
	virtual void	Update()override;
public:
	void			SetMeshInfo(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv);
public:
	CPlane(const BOOL32& active, const class CScene* scene);
	CPlane(const BOOL32& active, const class CScene* scene, const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv);
	virtual ~CPlane();
protected:
	class CMeshComponent*		m_MeshComponent;
	CPlaneMeshInfo				m_PlaneMeshInfo;
};
*/