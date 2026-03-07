/*
#include "../Headers/CScreenPolygon2D.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/RenderMaterials/Headers/CScreenPolygon2DMaterial.h"

CScreenPolygon2D::CScreenPolygon2D(const BOOL32& active, const class CScene* scene, const std::string& pixelShaderName, CustomType::Vector4 screenPosition) : CPolygon2DBase(active, scene)
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
		meshRendererComponent->SetMeshComponent(meshComponent);
		CScreenPolygon2DMaterial* material = meshRendererComponent->AddMaterial<CScreenPolygon2DMaterial>();
		material->SetPixelShader(CShaderManager::LoadPixelShader(pixelShaderName));
	}
}
CScreenPolygon2D::~CScreenPolygon2D()
{
}
void CScreenPolygon2D::Update()
{
	CGameObject::Update();
}
*/