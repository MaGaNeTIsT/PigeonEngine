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

CDefaultLitMaterial::CDefaultLitMaterial() : CMaterialBase(typeid(CDefaultLitMaterial).name(), MaterialType::MATERIAL_TYPE_OPAQUE_DEFERRED, 0u, _GDefaultLitMaterialInputLayout, 4u, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS)
{
	this->m_VertexShader	= CShaderManager::LoadVertexShader(ENGINE_SHADER_DEFAULT_VS, _GDefaultLitMaterialInputLayout, 4u);
	this->m_PixelShader		= CShaderManager::LoadPixelShader(ENGINE_SHADER_GBUFFER_WRITE_PS);
#if _DEVELOPMENT_EDITOR
	this->m_AlbedoTextureSelect		= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY;
	this->m_NormalTextureSelect		= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP;
	this->m_PropertyTextureSelect	= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY;
	this->m_AlbedoTexturePath		= "Default Gray";
	this->m_NormalTexturePath		= "Default Bump";
	this->m_PropertyTexturePath		= "Default Property";
#endif
	this->m_AlbedoTexture		= NULL;
	this->m_NormalTexture		= NULL;
	this->m_PropertyTexture		= NULL;
}
CDefaultLitMaterial::~CDefaultLitMaterial()
{

}
void CDefaultLitMaterial::SetAlbedoTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AlbedoTexture = tex;
	}
}
void CDefaultLitMaterial::SetNormalTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_NormalTexture = tex;
	}
}
void CDefaultLitMaterial::SetPropertyTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_PropertyTexture = tex;
	}
}
void CDefaultLitMaterial::Init()
{
#if _DEVELOPMENT_EDITOR
	{
		std::map<INT, std::string> baseEngineTextureItems = {
			{ -1, "None" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE, "Default White" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BLACK, "Default Black" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY, "Default Gray" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_RED, "Default Red" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GREEN, "Default Green" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BLUE, "Default Blue" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP, "Default Bump" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY, "Default Property" },
			{ -2, "Custom Path" } };
		auto pathSelect = [&baseEngineTextureItems](const INT& select, std::string& name, CTexture2D*& texture, const BOOL& sRGB = TRUE) {
			if (select != -2)
			{
				name = baseEngineTextureItems[select];
				if (select != -1)
				{
					texture = CRenderPipeline::GetDefaultTexture(static_cast<CustomStruct::CEngineDefaultTexture2DEnum>(select));
				}
			}
			else
			{
				texture = CTextureManager::LoadTexture2D(name, sRGB);
			}};
		pathSelect(this->m_AlbedoTextureSelect, this->m_AlbedoTexturePath, this->m_AlbedoTexture);
		pathSelect(this->m_NormalTextureSelect, this->m_NormalTexturePath, this->m_NormalTexture);
		pathSelect(this->m_PropertyTextureSelect, this->m_PropertyTexturePath, this->m_PropertyTexture);
	}
#endif

	{
		auto nullTextureSelect = [](CTexture2D*& texture, CustomStruct::CEngineDefaultTexture2DEnum type) {
			if (texture == NULL)
			{
				texture = CRenderPipeline::GetDefaultTexture(type);
			}};
		nullTextureSelect(this->m_AlbedoTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY);
		nullTextureSelect(this->m_NormalTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP);
		nullTextureSelect(this->m_PropertyTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
	}
}
void CDefaultLitMaterial::Bind()const
{
	auto bindTexture = [](const CTexture2D* tex, const UINT& slot, CustomStruct::CEngineDefaultTexture2DEnum type) {
		if (tex != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(tex->GetShaderResourceView(), slot);
		}
		else
		{
			CRenderDevice::BindPSShaderResourceView(CRenderPipeline::GetDefaultTexture(type)->GetShaderResourceView(), slot);
		}};
	bindTexture(this->m_AlbedoTexture, ENGINE_TEXTURE2D_ALBEDO_START_SLOT, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY);
	bindTexture(this->m_NormalTexture, ENGINE_TEXTURE2D_NORMAL_START_SLOT, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP);
	bindTexture(this->m_PropertyTexture, ENGINE_TEXTURE2D_PROPERTY_START_SLOT, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
}
const void* CDefaultLitMaterial::GetConstantData()const
{
	return nullptr;
}
#if _DEVELOPMENT_EDITOR
void CDefaultLitMaterial::SelectedEditorUpdate()
{

}
#endif