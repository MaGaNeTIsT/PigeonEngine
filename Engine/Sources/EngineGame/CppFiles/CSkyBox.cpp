#include "../../../../Entry/EngineMain.h"
#include "../Headers/CSkyBox.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"

std::shared_ptr<CVertexShader> CSkyBox::m_VertexShader = nullptr;
std::shared_ptr<CMesh<UINT>> CSkyBox::m_FullScreenMesh = nullptr;

CSkyBox::CSkyBox()
{
	if (!CSkyBox::m_VertexShader)
	{
		CustomStruct::CRenderInputLayoutDesc desc[2u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		CSkyBox::m_VertexShader = CShaderManager::LoadVertexShader(ENGINE_SHADER_SKY_BOX_VS, desc, 2u);
	}
	this->m_PixelShader = nullptr;
}
CSkyBox::CSkyBox(const CSkyBox& skyBox)
{
	if (!CSkyBox::m_VertexShader)
	{
		CustomStruct::CRenderInputLayoutDesc desc[2u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		CSkyBox::m_VertexShader = CShaderManager::LoadVertexShader(ENGINE_SHADER_SKY_BOX_VS, desc, 2u);
	}
	this->m_SkyBoxInfo = skyBox.m_SkyBoxInfo;
	this->m_PixelShader = skyBox.m_PixelShader;
}
void CSkyBox::Init(const CustomType::Vector2Int& bufferSize, const SkyBoxInfo& skyBoxInfo)
{
	if (!CSkyBox::m_FullScreenMesh)
	{
		CustomStruct::CRenderInputLayoutDesc desc[2u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		CSkyBox::m_FullScreenMesh = CMeshManager::LoadPolygon2D(CustomType::Vector4Int(0, 0, bufferSize.X(), bufferSize.Y()), desc, 2u);
	}
	this->m_SkyBoxInfo = skyBoxInfo;
	this->m_PixelShader = CShaderManager::LoadPixelShader(ENGINE_SHADER_SKY_BOX_PS);
}
void CSkyBox::Uninit()
{

}
void CSkyBox::Update()
{

}
void CSkyBox::PrepareDraw()
{
	this->m_VertexShader->Bind();
	CRenderDevice::SetVertexBuffer(CSkyBox::m_FullScreenMesh->GetVertexBuffer(), CSkyBox::m_FullScreenMesh->GetVertexStride());
	CRenderDevice::SetIndexBuffer(CSkyBox::m_FullScreenMesh->GetIndexBuffer());
	this->m_PixelShader->Bind();
}
void CSkyBox::Draw()
{
	this->PrepareDraw();
	CRenderDevice::DrawIndexed(CSkyBox::m_FullScreenMesh->GetIndexCount());
}