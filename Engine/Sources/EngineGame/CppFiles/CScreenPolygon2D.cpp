#include "../Headers/CScreenPolygon2D.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"

CScreenPolygon2D::CScreenPolygon2D(const std::string& vertexShaderName, const std::string& pixelShaderName, CustomType::Vector4 screenPosition)
{
	this->m_2DPosition = screenPosition;
	CustomStruct::CRenderInputLayoutDesc desc[2u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	this->m_Mesh = CMeshManager::LoadPolygon2D(this->m_2DPosition, desc, 2u);
	this->m_MeshRenderer = new CMeshRenderer();
	this->m_MeshRenderer->Init(this, vertexShaderName, pixelShaderName, desc, 2u, CMeshRenderer::RenderTypeEnum::RENDER_TYPE_OPAQUE_FORWARD);
}
CScreenPolygon2D::~CScreenPolygon2D()
{
	if (this->m_MeshRenderer != NULL)
	{
		delete (this->m_MeshRenderer);
		this->m_MeshRenderer = NULL;
	}
}
void CScreenPolygon2D::Draw()
{
	this->m_MeshRenderer->Draw();
}