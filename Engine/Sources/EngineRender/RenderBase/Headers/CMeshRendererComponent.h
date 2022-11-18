#pragma once

#include "./CRenderStructCommon.h"
#include "../../../EngineGame/Headers/CComponent.h"
#include "../../AssetsManager/Headers/CMeshComponent.h"

class CMeshRendererComponent : public CRenderComponent
{
public:
	struct RenderPerDrawInfo
	{
		CustomStruct::CShaderGlobalPerDraw		PerDrawData;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	PerDrawBuffer;
	};
public:
	enum RenderTypeEnum
	{
		RENDER_TYPE_OPAQUE			= 0,
		RENDER_TYPE_OPAQUE_FORWARD	= 1,
		RENDER_TYPE_TRANSPARENT		= 2
	};
public:
	RenderTypeEnum	GetRenderType()const { return this->m_RenderType; }
	void			SetRenderType(RenderTypeEnum type) { this->m_RenderType = type; }
public:
	virtual void	InitShadersAndInputLayout(const std::string& vertexShaderName, const std::string& pixelShaderName, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, RenderTypeEnum type);
	void			SetMeshComponent(std::shared_ptr<CMeshComponent> meshComponent);
	void			SetPerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter);
	virtual void	UploadConstantBuffer(const void* data);
	void			BindConstantBuffer(const UINT& startSlot);
public:
	virtual void	Draw()const override;
	virtual void	DrawExtra()const override;
private:
	void			SetInputLayoutDesc(const CustomStruct::CRenderInputLayoutDesc* layoutDesc, const UINT& layoutNum);
	void			LoadShader();
	BOOL			CreateConstantBuffer(const UINT& size);
	void			UploadPerDrawConstantBuffer()const;
	virtual void	Bind(const BOOL& needPixelShader = TRUE)const;
public:
	CMeshRendererComponent();
	CMeshRendererComponent(const CMeshRendererComponent& renderer);
	virtual ~CMeshRendererComponent();
protected:
	std::weak_ptr<CMeshComponent>						m_MeshComponent;
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