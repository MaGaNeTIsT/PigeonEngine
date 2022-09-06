#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CPlane.h"
#include "../../Headers/Base/CManager.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CTexture2D.h"
#include "../../Headers/Base/CTextureManager.h"
#include "../..//Headers/Game/CMesh.h"
#include "../..//Headers/Game/CMeshRenderer.h"
#include "../../Headers/Game/CMeshManager.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Game/CScene.h"

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
	this->m_MeshRenderer = new CMeshRenderer(this, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS);
	this->m_Mesh = CMeshManager::LoadPlaneMesh(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV);
	this->m_MeshRenderer->LoadShader();
	this->m_MeshRenderer->CreateConstantBuffer(sizeof(CustomStruct::ConstantBufferPerDraw));
	this->m_MeshRenderer->LoadExtraShader(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_EMPTY_PS);

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Assets/EngineTextures/Test/Field001.tga");
	this->m_NormalTexture = CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_BUMP);
	this->m_PropertyTexture = CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
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
	FLOAT tempR = this->m_TestRotate;

	CHAR tempName[256];
	_itoa_s(static_cast<INT>(this->m_UID), tempName, 256, 10);
	std::string name = tempName;
	name = "Plane data : " + name;
	ImGui::Begin(name.c_str());
	ImGui::SliderFloat("Rotation", &tempR, 0.f, 360.f);
	ImGui::End();

	this->m_TestRotate = tempR;
	this->SetRotation(CustomType::Quaternion(CustomType::Vector3(0.f, 1.f, 0.f), this->m_TestRotate * CustomType::CMath::GetDegToRad()));
}
void CPlane::PrepareDraw()
{
	CustomType::Matrix4x4 tempWorldMatrix(this->GetLocalToWorldMatrix());
	CustomType::Matrix4x4 tempWorldInverseMatrix(tempWorldMatrix.Inverse());
	this->m_ConstantBuffer.WorldMatrix = tempWorldMatrix.GetGPUUploadFloat4x4();
	this->m_ConstantBuffer.WorldInvMatrix = tempWorldInverseMatrix.GetGPUUploadFloat4x4();
	this->m_ConstantBuffer.WorldInvTransposeMatrix = tempWorldInverseMatrix.GetXMFLOAT4X4();
	this->m_MeshRenderer->UploadConstantBuffer(&(this->m_ConstantBuffer));
	this->m_MeshRenderer->BindConstantBuffer(ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);

	CRenderDevice::BindTexture(this->m_AlbedoTexture, ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
	CRenderDevice::BindTexture(this->m_NormalTexture, ENGINE_TEXTURE2D_NORMAL_START_SLOT);
	CRenderDevice::BindTexture(this->m_PropertyTexture, ENGINE_TEXTURE2D_PROPERTY_START_SLOT);
}
void CPlane::Draw()
{
	this->PrepareDraw();
	this->m_MeshRenderer->Draw();
}
void CPlane::DrawExtra()
{
	this->PrepareDraw();
	this->m_MeshRenderer->DrawExtra();
}
void CPlane::SetMeshInfo(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	this->m_PlaneMeshInfo.Length		= length;
	this->m_PlaneMeshInfo.UV			= uv;
	this->m_PlaneMeshInfo.VertexCount	= vertexCount;
	if (this->m_Mesh != NULL)
		this->m_Mesh = CMeshManager::LoadPlaneMesh(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV);
}