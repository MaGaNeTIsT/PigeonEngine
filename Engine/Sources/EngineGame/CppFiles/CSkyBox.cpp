#include "../../../../Entry/EngineMain.h"
#include "../Headers/CSkyBox.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureType.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
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
	this->m_CubeMap = NULL;
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
	this->m_CubeMap = skyBox.m_CubeMap;
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
	{
		// Face[0] : +X Right face.
		// Face[1] : -X Left face.
		// Face[2] : +Y Top face.
		// Face[3] : -Y Bottom face.
		// Face[4] : +Z Forward face.
		// Face[5] : -Z Back face.
		std::vector<std::string> cubeName = {
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Right.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Left.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Up.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Bottom.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Forward.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Back.tga" };
		this->m_CubeMap = CTextureManager::LoadTextureCubeCombine(cubeName, TRUE);
	}
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
	CRenderDevice::BindPSShaderResourceView(this->m_CubeMap->GetShaderResourceView(), 3u);
}
void CSkyBox::Draw()
{
	this->PrepareDraw();
	CRenderDevice::DrawIndexed(CSkyBox::m_FullScreenMesh->GetIndexCount());
}