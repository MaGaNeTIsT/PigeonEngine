#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CCube.h"
#include "../../Headers/Base/CManager.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CTexture2D.h"
#include "../../Headers/Base/CTextureManager.h"
#include "../..//Headers/Game/CMesh.h"
#include "../..//Headers/Game/CMeshRenderer.h"
#include "../../Headers/Game/CMeshManager.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Game/CScene.h"

CCube::CCube()
{
	this->m_AlbedoTexture = NULL;
}
CCube::~CCube()
{
}
void CCube::Init()
{
	this->m_MeshRenderer = new CMeshRenderer(this, "./HLSLCSO/EngineCubeVS.cso", ENGINE_SHADER_GBUFFER_WRITE_PS);
	this->m_Mesh = CMeshManager::LoadCubeMesh();
	this->m_MeshRenderer->LoadShader();
	this->m_MeshRenderer->CreateConstantBuffer(sizeof(CustomStruct::ConstantBufferPerDraw));
	this->m_MeshRenderer->LoadExtraShader("./HLSLCSO/EngineCubeVS.cso", "./HLSLCSO/EngineCubePS.cso");

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Assets/EngineTextures/Test/Cloud.tga");
}
void CCube::Uninit()
{
	if (this->m_MeshRenderer != NULL)
	{
		delete (this->m_MeshRenderer);
		this->m_MeshRenderer = NULL;
	}
}
void CCube::Update()
{

}
void CCube::PrepareDraw()
{
	CustomType::Matrix4x4 tempWorldMatrix(this->GetLocalToWorldMatrix());
	CustomType::Matrix4x4 tempWorldInverseMatrix(tempWorldMatrix.Inverse());
	this->m_ConstantBuffer.WorldMatrix = tempWorldMatrix.GetGPUUploadFloat4x4();
	this->m_ConstantBuffer.WorldInvMatrix = tempWorldInverseMatrix.GetGPUUploadFloat4x4();
	this->m_ConstantBuffer.WorldInvTransposeMatrix = tempWorldInverseMatrix.GetXMFLOAT4X4();
	this->m_MeshRenderer->UploadConstantBuffer(&(this->m_ConstantBuffer));
	this->m_MeshRenderer->BindConstantBuffer(ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);

	if (this->m_AlbedoTexture != NULL)
		CRenderDevice::BindTexture(this->m_AlbedoTexture, 8u);
	CRenderDevice::BindTexture(CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_BUMP), 9u);
	CRenderDevice::BindTexture(CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_PROPERTY), 10u);
}
void CCube::Draw()
{
	this->PrepareDraw();
	this->m_MeshRenderer->Draw();
}
void CCube::DrawExtra()
{
	this->PrepareDraw();
	this->m_MeshRenderer->DrawExtra();
}