#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CTestModel.h"
#include "../../Headers/Base/CManager.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CTexture2D.h"
#include "../../Headers/Base/CTextureManager.h"
#include "../..//Headers/Game/CMesh.h"
#include "../..//Headers/Game/CMeshRenderer.h"
#include "../../Headers/Game/CMeshManager.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Game/CScene.h"

CTestModel::CTestModel()
{
	this->m_AlbedoTexture = NULL;
	this->m_NormalTexture = NULL;
	this->m_PropertyTexture = NULL;
}
CTestModel::~CTestModel()
{
}
void CTestModel::Init()
{
	this->m_MeshRenderer = new CMeshRenderer(this, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS);
	this->m_Mesh = CMeshManager::LoadMeshFromFile("./Assets/EngineModels/Base/sphere.obj");
	this->m_MeshRenderer->LoadShader();
	this->m_MeshRenderer->CreateConstantBuffer(sizeof(CustomStruct::ConstantBufferPerDraw));
	this->m_MeshRenderer->LoadExtraShader(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_EMPTY_PS);

	this->m_AlbedoTexture = CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_GRAY);
	this->m_NormalTexture = CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_BUMP);
	this->m_PropertyTexture = CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
}
void CTestModel::Uninit()
{
	if (this->m_MeshRenderer != NULL)
	{
		delete (this->m_MeshRenderer);
		this->m_MeshRenderer = NULL;
	}
}
void CTestModel::Update()
{

}
void CTestModel::FixedUpdate()
{

}
void CTestModel::PrepareDraw()
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
void CTestModel::Draw()
{
	this->PrepareDraw();
	this->m_MeshRenderer->Draw();
}
void CTestModel::DrawExtra()
{
	this->PrepareDraw();
	this->m_MeshRenderer->DrawExtra();
}