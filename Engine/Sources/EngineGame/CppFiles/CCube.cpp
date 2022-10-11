#include "../../../../Entry/EngineMain.h"
#include "../Headers/CCube.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureType.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMesh.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CCamera.h"
#include "../../EngineGame/Headers/CScene.h"

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
	CustomStruct::CRenderInputLayoutDesc desc[4u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	this->m_Mesh = CMeshManager::LoadCubeMesh(desc, 4u);
	this->m_MeshRenderer = new CMeshRenderer();
	this->m_MeshRenderer->Init(this, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS, desc, 4u, CMeshRenderer::RenderTypeEnum::RENDER_TYPE_OPAQUE);

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Cloud001.tga");
	this->m_NormalTexture = CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP);
	this->m_PropertyTexture = CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
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
	//FLOAT pos[3] = { this->GetPosition().X(), this->GetPosition().Y(), this->GetPosition().Z() };
	//FLOAT scale[3] = { this->GetScale().X(), this->GetScale().Y(), this->GetScale().Z() };
	//ImGui::Begin("Cube properties");
	//ImGui::InputFloat3(" Position", pos);
	//ImGui::InputFloat3(" Scale", scale);
	//ImGui::End();
	//this->SetPosition(CustomType::Vector3(pos[0], pos[1], pos[2]));
	//this->SetScale(CustomType::Vector3(scale[0], scale[1], scale[2]));
}
void CCube::FixedUpdate()
{

}
void CCube::PrepareDraw()
{
	CustomType::Matrix4x4 tempWorldMatrix(this->GetLocalToWorldMatrix());
	CustomType::Matrix4x4 tempWorldInverseMatrix(tempWorldMatrix.Inverse());
	this->m_MeshRenderer->SetPerDrawInfo(tempWorldMatrix, tempWorldInverseMatrix, CustomType::Vector4::Zero());
}
void CCube::Draw()
{
	this->PrepareDraw();
	CRenderDevice::BindPSShaderResourceView(this->m_AlbedoTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
	CRenderDevice::BindPSShaderResourceView(this->m_NormalTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_NORMAL_START_SLOT);
	CRenderDevice::BindPSShaderResourceView(this->m_PropertyTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_PROPERTY_START_SLOT);
	this->m_MeshRenderer->Draw();
}
void CCube::DrawExtra()
{
	this->PrepareDraw();
	CRenderPipeline::GetDefaultEmptyPS()->Bind();
	this->m_MeshRenderer->Draw(FALSE);
}