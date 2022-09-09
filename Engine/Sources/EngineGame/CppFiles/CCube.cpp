#include "../../../../Entry/EngineMain.h"
#include "../Headers/CCube.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CTexture2D.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMesh.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"
#include "../Headers/CCamera.h"
#include "../Headers/CScene.h"

CCube::CCube()
{
	this->m_AlbedoTexture = NULL;
	this->m_NormalTexture = NULL;
	this->m_PropertyTexture = NULL;
}
CCube::~CCube()
{
}
void CCube::Init()
{
	this->m_MeshRenderer = new CMeshRenderer(this, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS);
	this->m_Mesh = CMeshManager::LoadCubeMesh();
	this->m_MeshRenderer->LoadShader();
	this->m_MeshRenderer->CreateConstantBuffer(sizeof(CustomStruct::ConstantBufferPerDraw));
	this->m_MeshRenderer->LoadExtraShader(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_EMPTY_PS);

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Cloud001.tga");
	this->m_NormalTexture = CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_BUMP);
	this->m_PropertyTexture = CRenderDevice::GetEngineDefaultTexture2D(CRenderDevice::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
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
void CCube::FixedUpdate()
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

	CRenderDevice::BindTexture(this->m_AlbedoTexture, ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
	CRenderDevice::BindTexture(this->m_NormalTexture, ENGINE_TEXTURE2D_NORMAL_START_SLOT);
	CRenderDevice::BindTexture(this->m_PropertyTexture, ENGINE_TEXTURE2D_PROPERTY_START_SLOT);
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