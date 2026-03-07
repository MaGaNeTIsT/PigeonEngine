/*
#include "../Headers/CScreenPolygon2DMaterial.h"
#include "../../RenderBase/Headers/CRenderStructCommon.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderBase/Headers/CRenderDevice.h"
#include "../../RenderBase/Headers/CRenderPipeline.h"

CustomStruct::CRenderInputLayoutDesc _GScreenPolygon2DMaterialInputLayout[2u] = {
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

CScreenPolygon2DMaterial::CScreenPolygon2DMaterial() : CMaterialBase(typeid(CScreenPolygon2DMaterial).name(), MaterialType::MATERIAL_TYPE_OPAQUE_FORWARD, 0u, _GScreenPolygon2DMaterialInputLayout, 2u, ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_NONE)
{
	this->m_VertexShader		= CShaderManager::LoadVertexShader(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, _GScreenPolygon2DMaterialInputLayout, 2u);
	this->m_PixelShader			= NULL;
	this->m_AlbedoTexture		= NULL;
}
CScreenPolygon2DMaterial::~CScreenPolygon2DMaterial()
{

}
void CScreenPolygon2DMaterial::SetPixelShader(CPixelShader* shader)
{
	if (shader != NULL)
	{
		if (this->HasPixelShader())
		{
			delete (this->m_PixelShader);
			this->m_PixelShader = NULL;
		}
		this->m_PixelShader = shader;
	}
}
void CScreenPolygon2DMaterial::SetAlbedoTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AlbedoTexture = tex;
	}
}
void CScreenPolygon2DMaterial::Init()
{

}
void CScreenPolygon2DMaterial::Bind()const
{
	if (this->m_AlbedoTexture != NULL)
	{
		CRenderDevice::BindPSShaderResourceView(this->m_AlbedoTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT);
	}
}
const void* CScreenPolygon2DMaterial::GetConstantData()const
{
	return nullptr;
}
#ifdef _DEVELOPMENT_EDITOR
void CScreenPolygon2DMaterial::SelectedEditorUpdate()
{

}
#endif
*/