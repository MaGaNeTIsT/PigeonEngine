/*
#include "../Headers/CSkyBox.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureType.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"

const CBaseMesh<UINT>* CSkyBox::m_FullScreenMesh = NULL;

CSkyBox::CSkyBox()
{
	this->m_PixelShader = NULL;
	this->m_CubeMap = NULL;
	this->m_ConstantBuffer = nullptr;

	CustomStruct::CRenderInputLayoutDesc desc[2u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	this->m_VertexShader = CShaderManager::LoadVertexShader(ENGINE_SHADER_SKY_BOX_VS, desc, 2u);
}
CSkyBox::CSkyBox(const CSkyBox& skyBox)
{
	this->m_SkyBoxInfo = skyBox.m_SkyBoxInfo;
	this->m_PixelShader = skyBox.m_PixelShader;
	this->m_CubeMap = skyBox.m_CubeMap;
	this->m_ConstantBuffer = skyBox.m_ConstantBuffer;

	CustomStruct::CRenderInputLayoutDesc desc[2u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	this->m_VertexShader = CShaderManager::LoadVertexShader(ENGINE_SHADER_SKY_BOX_VS, desc, 2u);
}
CSkyBox::~CSkyBox()
{
	if (this->m_VertexShader != NULL)
	{
		delete (this->m_VertexShader);
		this->m_VertexShader = NULL;
	}
	if (this->m_PixelShader != NULL)
	{
		delete (this->m_PixelShader);
		this->m_PixelShader = NULL;
	}
}
void CSkyBox::Init(const CustomType::Vector2Int& bufferSize, const SkyBoxInfo& skyBoxInfo)
{
	if (CSkyBox::m_FullScreenMesh == NULL)
	{
		CustomStruct::CRenderInputLayoutDesc desc[2u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		CSkyBox::m_FullScreenMesh = CMeshManager::LoadPolygon2D(CustomType::Vector4Int(0, 0, bufferSize.X(), bufferSize.Y()), desc, 2u);
	}
	if (this->m_PixelShader == NULL)
	{
		this->m_PixelShader = CShaderManager::LoadPixelShader(ENGINE_SHADER_SKY_BOX_PS);
	}
	if (this->m_CubeMap == NULL)
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
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Top.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Bottom.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Forward.tga",
			"./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/Sky_001_Back.tga" };
		this->m_CubeMap = CTextureManager::LoadTextureCubeCombine(cubeName);
	}
	this->m_SkyBoxInfo = skyBoxInfo;
	if (!(this->m_ConstantBuffer))
	{
		this->m_ConstantData.Parameter = CustomType::Vector4(0.f, 0.f, 0.f, this->m_SkyBoxInfo.Radius);
		DirectX::XMFLOAT4 tempData(this->m_ConstantData.Parameter.GetXMFLOAT4());
		CRenderDevice::CreateBuffer(this->m_ConstantBuffer, CustomStruct::CRenderBufferDesc(sizeof(DirectX::XMFLOAT4), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)), &(CustomStruct::CRenderSubresourceData(&tempData, 0u, 0u)));
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
	CRenderDevice::BindPSConstantBuffer(this->m_ConstantBuffer, 3u);
	CRenderDevice::BindPSShaderResourceView(this->m_CubeMap->GetShaderResourceView(), 5u);
}
void CSkyBox::Draw()
{
	this->PrepareDraw();
	CRenderDevice::DrawIndexed(CSkyBox::m_FullScreenMesh->GetIndexCount());
}
*/