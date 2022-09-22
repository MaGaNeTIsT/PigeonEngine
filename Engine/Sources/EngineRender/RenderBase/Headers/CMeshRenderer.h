#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "./CStructCommon.h"

class CGameObject;

class CMeshRenderer
{
public:
	struct RenderPerDrawInfo
	{
		CustomStruct::ConstantBufferPerDraw		PerDrawData;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	PerDrawBuffer;
	};
public:
	enum CRenderTypeEnum
	{
		RENDER_TYPE_OPAQUE				= 0,
		RENDER_TYPE_OPAQUE_FORWARD		= 1,
		RENDER_TYPE_TRANSPARENT_FORWARD	= 2,
		RENDER_TYPE_SKY					= 3
	};
public:
	CRenderTypeEnum	GetRenderType() { return this->m_RenderType; }
	void			SetRenderType(CRenderTypeEnum type) { this->m_RenderType = type; }
public:
	void			LoadShader();
	void			SetInputLayoutDesc(const std::vector<D3D11_INPUT_ELEMENT_DESC>& desc);
	BOOL			CreateConstantBuffer(const UINT& size);
	void			UploadPerDrawConstantBuffer(const CustomStruct::ConstantBufferPerDraw& data);
	virtual void	UploadConstantBuffer(const void* data);
	virtual void	Bind(const BOOL& needPixelShader = TRUE);
public:
	virtual void	Draw(const BOOL& needPixelShader = TRUE);
public:
	CMeshRenderer(CGameObject* gameObject, const std::string& vertexShaderName, const std::string& pixelShaderName, CRenderTypeEnum type = CRenderTypeEnum::RENDER_TYPE_OPAQUE);
	virtual ~CMeshRenderer();
protected:
	CGameObject*							m_GameObject;
	CRenderTypeEnum							m_RenderType;
	std::string								m_VertexShaderName;
	std::string								m_PixelShaderName;
	std::shared_ptr<class CVertexShader>	m_VertexShader;
	std::shared_ptr<class CPixelShader>		m_PixelShader;
	std::vector<D3D11_INPUT_ELEMENT_DESC>	m_InputLayoutDesc;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	m_PerDrawBuffer;
	CustomStruct::CRenderBufferDesc			m_ConstantBufferDesc;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	m_ConstantBuffer;
};