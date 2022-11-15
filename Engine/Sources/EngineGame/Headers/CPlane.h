#pragma once

#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CGameObject.h"

class CTexture2D;

class CPlane : public CGameObject
{
public:
	struct CPlaneMeshInfo
	{
		CustomType::Vector2		Length;
		CustomType::Vector2		UV;
		CustomType::Vector2Int	VertexCount;
	};
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	Draw()override;
	virtual void	DrawExtra()override;
protected:
	virtual void	PrepareDraw()override;
public:
	void			SetMeshInfo(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv);
public:
	CPlane();
	CPlane(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv);
	virtual ~CPlane();
protected:
	CTexture2D*		m_AlbedoTexture;
	CTexture2D*		m_NormalTexture;
	CTexture2D*		m_PropertyTexture;
protected:
	CPlaneMeshInfo	m_PlaneMeshInfo;
	FLOAT m_TestRotate = 0;
};