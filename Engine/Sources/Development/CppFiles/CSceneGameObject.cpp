#include "../../../../Entry/EngineMain.h"
#include "../Headers/CSceneGameObject.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"

CSceneGameObject::CSceneGameObject()
{
	this->AddNewTransform();
	{
		CMeshRendererComponent* meshRendererComponent = new CMeshRendererComponent();
		CMeshComponent* meshComponent = new CMeshComponent();
		this->AddComponent(meshRendererComponent);
		this->AddComponent(meshComponent);
		CustomStruct::CRenderInputLayoutDesc desc[4u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		meshComponent->SetMesh(CMeshManager::LoadMeshFromFile("./Engine/Assets/EngineModels/BaseShape/SphereSmooth.obj", desc, 4u));
		meshRendererComponent->InitShadersAndInputLayout(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS, desc, 4u, CMeshRendererComponent::RenderTypeEnum::RENDER_TYPE_OPAQUE);
		meshRendererComponent->SetMeshComponent(meshComponent);
	}
}
CSceneGameObject::~CSceneGameObject()
{
}
void CSceneGameObject::Init()
{
	CGameObject::Init();
}
void CSceneGameObject::Uninit()
{
	CGameObject::Uninit();
}
void CSceneGameObject::Update()
{
	CGameObject::Update();
}
void CSceneGameObject::FixedUpdate()
{
	CGameObject::FixedUpdate();
}