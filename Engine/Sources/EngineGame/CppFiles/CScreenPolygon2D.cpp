#include "../Headers/CScreenPolygon2D.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"

CScreenPolygon2D::CScreenPolygon2D(const std::string& vertexShaderName, const std::string& pixelShaderName, CustomType::Vector4 screenPosition)
{
	this->m_2DPosition = screenPosition;
	{
		CMeshRendererComponent* meshRendererComponent = new CMeshRendererComponent();
		CMeshComponent* meshComponent = new CMeshComponent();
		this->AddComponent(meshRendererComponent);
		this->AddComponent(meshComponent);
		CustomStruct::CRenderInputLayoutDesc desc[2u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		meshComponent->SetMesh(CMeshManager::LoadPolygon2D(this->m_2DPosition, desc, 2u));
		meshRendererComponent->InitShadersAndInputLayout(vertexShaderName, pixelShaderName, desc, 2u, CMeshRendererComponent::RenderTypeEnum::RENDER_TYPE_OPAQUE_FORWARD);
		meshRendererComponent->SetMeshComponent(meshComponent);
	}
}
CScreenPolygon2D::~CScreenPolygon2D()
{
}
void CScreenPolygon2D::Update()
{
	CGameObject::Update();
}