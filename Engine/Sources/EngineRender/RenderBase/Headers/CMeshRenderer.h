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
	enum RenderTypeEnum
	{
		RENDER_TYPE_OPAQUE				= 0,
		RENDER_TYPE_OPAQUE_FORWARD		= 1,
		RENDER_TYPE_TRANSPARENT_FORWARD = 2,
		RENDER_TYPE_SKY					= 3
	};
public:
	RenderTypeEnum	GetRenderType() { return this->m_RenderType; }
	void			SetRenderType(RenderTypeEnum type) { this->m_RenderType = type; }
public:
	virtual void	Init(CGameObject* gameObject, const std::string& vertexShaderName, const std::string& pixelShaderName, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, RenderTypeEnum type);
	void			SetPerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter);
	virtual void	UploadConstantBuffer(const void* data);
	void			BindConstantBuffer(const UINT& startSlot);
	virtual void	Draw(const BOOL& needPixelShader = TRUE);
private:
	void			SetInputLayoutDesc(const CustomStruct::CRenderInputLayoutDesc* layoutDesc, const UINT& layoutNum);
	void			LoadShader();
	BOOL			CreateConstantBuffer(const UINT& size);
	void			UploadPerDrawConstantBuffer();
	virtual void	Bind(const BOOL& needPixelShader = TRUE);
public:
	CMeshRenderer();
	CMeshRenderer(const CMeshRenderer& renderer);
	virtual ~CMeshRenderer();
protected:
	CGameObject*										m_GameObject;
	RenderTypeEnum										m_RenderType;
	std::string											m_VertexShaderName;
	std::string											m_PixelShaderName;
	std::shared_ptr<class CVertexShader>				m_VertexShader;
	std::shared_ptr<class CPixelShader>					m_PixelShader;
	std::vector<CustomStruct::CRenderInputLayoutDesc>	m_InputLayoutDesc;
	RenderPerDrawInfo									m_PerDrawInfo;
	CustomStruct::CRenderBufferDesc						m_ConstantBufferDesc;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_ConstantBuffer;
};