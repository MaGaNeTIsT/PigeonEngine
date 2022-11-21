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
	BOOL			HasMesh()const { return (this->m_MeshComponent != NULL); }
	BOOL			HasVertexShader()const { return (this->m_VertexShader != NULL); }
	BOOL			HasPixelShader()const { return (this->m_PixelShader != NULL); }
public:
	virtual void	InitShadersAndInputLayout(const std::string& vertexShaderName, const std::string& pixelShaderName, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, RenderTypeEnum type);
	void			SetMaterialTextures(class CTexture2D* albedoTexture, class CTexture2D* normalTexture, class CTexture2D* propertyTexture);
	void			SetMeshComponent(const CMeshComponent* meshComponent);
	void			SetPerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter)const;
	virtual void	UploadConstantBuffer(const void* data);
	void			BindConstantBuffer(const UINT& startSlot);
public:
	virtual void	Update()const override;
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
	virtual ~CMeshRendererComponent();
protected:
	mutable BOOL										m_CurrentFrameUpload;
	class CTexture2D*									m_AlbedoTexture;
	class CTexture2D*									m_NormalTexture;
	class CTexture2D*									m_PropertyTexture;
protected:
	const CMeshComponent*								m_MeshComponent;
	RenderTypeEnum										m_RenderType;
	std::string											m_VertexShaderName;
	std::string											m_PixelShaderName;
	class CVertexShader*								m_VertexShader;
	class CPixelShader*									m_PixelShader;
	std::vector<CustomStruct::CRenderInputLayoutDesc>	m_InputLayoutDesc;
	mutable RenderPerDrawInfo							m_PerDrawInfo;
	CustomStruct::CRenderBufferDesc						m_ConstantBufferDesc;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_ConstantBuffer;
};