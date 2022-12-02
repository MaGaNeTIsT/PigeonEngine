#include "../Headers/CDefaultLitMaterial.h"
#include "../../RenderBase/Headers/CRenderStructCommon.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderBase/Headers/CRenderDevice.h"
#include "../../RenderBase/Headers/CRenderPipeline.h"

const static CustomStruct::CRenderInputLayoutDesc _GDefaultLitMaterialInputLayout[4u] = {
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

CDefaultLitMaterial::CDefaultLitMaterial() : CMaterialBase(typeid(CDefaultLitMaterial).name(), MaterialType::MATERIAL_TYPE_OPAQUE_FORWARD, sizeof(CDefaultLitMaterial::RenderParams), _GDefaultLitMaterialInputLayout, 4u, ENGINE_DEFAULT_SHADER_FORWARD_VS, ENGINE_DEFAULT_SHADER_FORWARD_PS)
{
	this->m_VertexShader	= CShaderManager::LoadVertexShader(ENGINE_DEFAULT_SHADER_FORWARD_VS, _GDefaultLitMaterialInputLayout, 4u);
	this->m_PixelShader		= CShaderManager::LoadPixelShader(ENGINE_DEFAULT_SHADER_FORWARD_PS);
#ifdef _DEVELOPMENT_EDITOR

	this->m_NormalSelect				= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP;
	this->m_AlbedoSelect				= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_EmissiveSelect				= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK;
	this->m_RoughnessSelect				= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_MetallicnessSelect			= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_AmbientOcclusionSelect		= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_AnisotropySelect			= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK;
	this->m_AnisotropyDirectionSelect	= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP;
	this->m_NormalPath				= "Default Bump";
	this->m_AlbedoPath				= "Default Gray";
	this->m_EmissivePath			= "Default Black";
	this->m_RoughnessPath			= "Default White";
	this->m_MetallicnessPath		= "Default White";
	this->m_AmbientOcclusionPath	= "Default White";
	this->m_AnisotropyPath			= "Default Black";
	this->m_AnisotropyDirectionPath	= "Default Bump";
#endif
	this->m_NormalTexture				= NULL;
	this->m_AlbedoTexture				= NULL;
	this->m_EmissiveTexture				= NULL;
	this->m_RoughnessTexture			= NULL;
	this->m_MetallicnessTexture			= NULL;
	this->m_AmbientOcclusionTexture		= NULL;
	this->m_AnisotropyTexture			= NULL;
	this->m_AnisotropyDirectionTexture	= NULL;
}
CDefaultLitMaterial::~CDefaultLitMaterial()
{

}
void CDefaultLitMaterial::SetNormalTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_NormalTexture = tex;
	}
}
void CDefaultLitMaterial::SetAlbedoTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AlbedoTexture = tex;
	}
}
void CDefaultLitMaterial::SetEmissiveTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_EmissiveTexture = tex;
	}
}
void CDefaultLitMaterial::SetRoughnessTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_RoughnessTexture = tex;
	}
}
void CDefaultLitMaterial::SetMetallicnessTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_MetallicnessTexture = tex;
	}
}
void CDefaultLitMaterial::SetAmbientOcclusionTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AmbientOcclusionTexture = tex;
	}
}
void CDefaultLitMaterial::SetAnisotropyTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AnisotropyTexture = tex;
	}
}
void CDefaultLitMaterial::SetAnisotropyDirectionTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AnisotropyDirectionTexture = tex;
	}
}
void CDefaultLitMaterial::Init()
{
#ifdef _DEVELOPMENT_EDITOR
	{
		std::map<INT, std::string> baseEngineTextureItems = {
			{ -1, "None" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE, "Default White" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK, "Default Black" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY, "Default Gray" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_RED, "Default Red" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GREEN, "Default Green" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLUE, "Default Blue" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP, "Default Bump" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_PROPERTY, "Default Property" },
			{ -2, "Custom Path" } };
		auto pathSelect = [&baseEngineTextureItems](const INT& select, std::string& name, CTexture2D*& texture, const BOOL& sRGB = TRUE) {
			if (select != -2)
			{
				name = baseEngineTextureItems[select];
				if (select != -1)
				{
					texture = CRenderPipeline::GetDefaultTexture(static_cast<CustomStruct::CEngineDefaultTexture2DType>(select));
				}
			}
			else
			{
				texture = CTextureManager::LoadTexture2D(name, sRGB);
			}};
		pathSelect(this->m_NormalSelect, this->m_NormalPath, this->m_NormalTexture);
		pathSelect(this->m_AlbedoSelect, this->m_AlbedoPath, this->m_AlbedoTexture);
		pathSelect(this->m_EmissiveSelect, this->m_EmissivePath, this->m_EmissiveTexture);
		pathSelect(this->m_RoughnessSelect, this->m_RoughnessPath, this->m_RoughnessTexture);
		pathSelect(this->m_MetallicnessSelect, this->m_MetallicnessPath, this->m_MetallicnessTexture);
		pathSelect(this->m_AmbientOcclusionSelect, this->m_AmbientOcclusionPath, this->m_AmbientOcclusionTexture);
		pathSelect(this->m_AnisotropySelect, this->m_AnisotropyPath, this->m_AnisotropyTexture);
		pathSelect(this->m_AnisotropyDirectionSelect, this->m_AnisotropyDirectionPath, this->m_AnisotropyDirectionTexture);
	}
#endif

	{
		auto nullTextureSelect = [](CTexture2D*& texture, CustomStruct::CEngineDefaultTexture2DType type) {
			if (texture == NULL)
			{
				texture = CRenderPipeline::GetDefaultTexture(type);
			}};
		nullTextureSelect(this->m_NormalTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP);
		nullTextureSelect(this->m_AlbedoTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
		nullTextureSelect(this->m_EmissiveTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK);
		nullTextureSelect(this->m_RoughnessTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK);
		nullTextureSelect(this->m_MetallicnessTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK);
		nullTextureSelect(this->m_AmbientOcclusionTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_AnisotropyTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK);
		nullTextureSelect(this->m_AnisotropyDirectionTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
	}

	this->m_RenderParams.BaseColorRoughness					= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	this->m_RenderParams.EmissiveAmbientOcclusion			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	this->m_RenderParams.MetallicnessReflectanceAnisotropy	= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.f);
	this->m_RenderParams.ClearCoatStrengthRoughness			= DirectX::XMFLOAT4(1.f, 1.f, 0.f, 0.f);
}
void CDefaultLitMaterial::Bind()const
{
	auto bindTexture = [](const CTexture2D* tex, const UINT& slot, CustomStruct::CEngineDefaultTexture2DType type) {
		if (tex != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(tex->GetShaderResourceView(), slot);
		}
		else
		{
			CRenderDevice::BindPSShaderResourceView(CRenderPipeline::GetDefaultTexture(type)->GetShaderResourceView(), slot);
		}};
	bindTexture(this->m_NormalTexture, ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP);
	bindTexture(this->m_AlbedoTexture, ENGINE_TEXTURE2D_CUSTOM_B_START_SLOT, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
	bindTexture(this->m_EmissiveTexture, ENGINE_TEXTURE2D_CUSTOM_C_START_SLOT, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
	bindTexture(this->m_AmbientOcclusionTexture, ENGINE_TEXTURE2D_CUSTOM_D_START_SLOT, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
	bindTexture(this->m_RoughnessTexture, 9u, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
	bindTexture(this->m_MetallicnessTexture, 10u, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
	bindTexture(this->m_AnisotropyTexture, 11u, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
	bindTexture(this->m_AnisotropyDirectionTexture, 12u, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY);
}
const void* CDefaultLitMaterial::GetConstantData()const
{
	return (&(this->m_RenderParams));
}
#ifdef _DEVELOPMENT_EDITOR
void CDefaultLitMaterial::SelectedEditorUpdate()
{

}
#endif