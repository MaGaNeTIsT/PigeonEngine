#include "../../../../Entry/EngineMain.h"
#include "../Headers/CPlane.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTexture2D.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMesh.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CCamera.h"
#include "../../EngineGame/Headers/CScene.h"

CPlane::CPlane()
{
	this->m_AlbedoTexture	= NULL;
	this->m_NormalTexture	= NULL;
	this->m_PropertyTexture	= NULL;
	this->m_PlaneMeshInfo.Length		= 1.f;
	this->m_PlaneMeshInfo.UV			= 1.f;
	this->m_PlaneMeshInfo.VertexCount	= 2;
}
CPlane::CPlane(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	this->m_AlbedoTexture	= NULL;
	this->m_NormalTexture	= NULL;
	this->m_PropertyTexture = NULL;
	this->m_PlaneMeshInfo.Length		= length;
	this->m_PlaneMeshInfo.UV			= uv;
	this->m_PlaneMeshInfo.VertexCount	= vertexCount;
}
CPlane::~CPlane()
{
}
void CPlane::Init()
{
	CustomStruct::CRenderInputLayoutDesc desc[4u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	this->m_Mesh = CMeshManager::LoadPlaneMesh(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV, desc, 4u);
	this->m_MeshRenderer = new CMeshRenderer();
	this->m_MeshRenderer->Init(this, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS, desc, 4u, CMeshRenderer::RenderTypeEnum::RENDER_TYPE_OPAQUE);

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Field001.tga");
	this->m_NormalTexture = CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP);
	this->m_PropertyTexture = CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
}
void CPlane::Uninit()
{
	if (this->m_MeshRenderer != NULL)
	{
		delete (this->m_MeshRenderer);
		this->m_MeshRenderer = NULL;
	}
}
void CPlane::Update()
{
	//FLOAT tempR = this->m_TestRotate;

	//CHAR tempName[256];
	//_itoa_s(static_cast<INT>(this->m_UID), tempName, 256, 10);
	//std::string name = tempName;
	//name = "Plane data : " + name;
	//ImGui::Begin(name.c_str());
	//ImGui::SliderFloat("Rotation", &tempR, 0.f, 360.f);
	//ImGui::End();

	//this->m_TestRotate = tempR;
	//this->SetRotation(CustomType::Quaternion(CustomType::Vector3(0.f, 1.f, 0.f), this->m_TestRotate * CustomType::CMath::GetDegToRad()));
}
void CPlane::PrepareDraw()
{
	CustomType::Matrix4x4 tempWorldMatrix(this->GetLocalToWorldMatrix());
	CustomType::Matrix4x4 tempWorldInverseMatrix(tempWorldMatrix.Inverse());
	this->m_MeshRenderer->SetPerDrawInfo(tempWorldMatrix, tempWorldInverseMatrix, CustomType::Vector4::Zero());
}
void CPlane::Draw()
{
	this->PrepareDraw();
	CRenderDevice::BindPSShaderResourceView(this->m_AlbedoTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
	CRenderDevice::BindPSShaderResourceView(this->m_NormalTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_NORMAL_START_SLOT);
	CRenderDevice::BindPSShaderResourceView(this->m_PropertyTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_PROPERTY_START_SLOT);
	this->m_MeshRenderer->Draw();
}
void CPlane::DrawExtra()
{
	this->PrepareDraw();
	CRenderPipeline::GetDefaultEmptyPS()->Bind();
	this->m_MeshRenderer->Draw(FALSE);
}
void CPlane::SetMeshInfo(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	this->m_PlaneMeshInfo.Length		= length;
	this->m_PlaneMeshInfo.UV			= uv;
	this->m_PlaneMeshInfo.VertexCount	= vertexCount;
	CustomStruct::CRenderInputLayoutDesc desc[4u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	if (this->m_Mesh != nullptr)
		this->m_Mesh = CMeshManager::LoadPlaneMesh(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV, desc, 4u);
}