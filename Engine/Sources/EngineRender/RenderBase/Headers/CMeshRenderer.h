#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "./CStructCommon.h"

class CShader;
class CGameObject;

class CMeshRenderer
{
public:
	enum CRenderTypeEnum
	{
		RENDER_TYPE_OPAQUE_DEFERRED	= 0,
		RENDER_TYPE_OPAQUE_FORWARD	= 1,
		RENDER_TYPE_SKY				= 2,
		RENDER_TYPE_TRANSPARENT		= 3
	};
public:
	CRenderTypeEnum	GetRenderType() { return this->m_RenderType; }
	void			SetRenderType(CRenderTypeEnum type) { this->m_RenderType = type; }
public:
	void	LoadShader();
	BOOL	CreateConstantBuffer(const UINT& sizeData);
	void	UploadConstantBuffer(const void* ptrData);
	void	BindConstantBuffer(const UINT& startSlot);
public:
	void	LoadExtraShader(const std::string& vertexShaderName, const std::string& pixelShaderName);
public:
	void	Draw();
	void	DrawExtra();
public:
	CMeshRenderer(CGameObject* gameObject, const std::string& vertexShaderName, const std::string& pixelShaderName, CRenderTypeEnum type = RENDER_TYPE_OPAQUE_DEFERRED);
	virtual ~CMeshRenderer();
protected:
	CRenderTypeEnum		m_RenderType;
	std::string			m_VertexShaderName;
	std::string			m_PixelShaderName;
	CShader*			m_Shader;
	CShader*			m_ExtraShader;
	CGameObject*		m_GameObject;
};