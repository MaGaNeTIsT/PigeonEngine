#include "../Headers/CMeshRenderer.h"
#include "../Headers/CRenderDevice.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CMesh.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../../EngineGame/Headers/CGameObject.h"

CMeshRenderer::CMeshRenderer()
{
	this->m_GameObject			= NULL;
	this->m_RenderType			= RenderTypeEnum::RENDER_TYPE_OPAQUE;
	this->m_VertexShaderName	= ENGINE_SHADER_NONE;
	this->m_PixelShaderName		= ENGINE_SHADER_NONE;
	this->m_VertexShader		= nullptr;
	this->m_PixelShader			= nullptr;
	//this->m_InputLayoutDesc.clear();
	//::ZeroMemory(&(this->m_PerDrawInfo), sizeof(this->m_PerDrawInfo));
	//this->m_ConstantBufferDesc = CustomStruct::CRenderBufferDesc();
	this->m_ConstantBuffer		= nullptr;
}
CMeshRenderer::CMeshRenderer(const CMeshRenderer& renderer)
{
	this->m_GameObject			= renderer.m_GameObject;
	this->m_RenderType			= renderer.m_RenderType;
	this->m_VertexShaderName	= renderer.m_VertexShaderName;
	this->m_PixelShaderName		= renderer.m_PixelShaderName;
	this->m_VertexShader		= renderer.m_VertexShader;
	this->m_PixelShader			= renderer.m_PixelShader;
	this->m_InputLayoutDesc		= renderer.m_InputLayoutDesc;
	this->m_PerDrawInfo			= renderer.m_PerDrawInfo;
	this->m_ConstantBufferDesc	= renderer.m_ConstantBufferDesc;
	this->m_ConstantBuffer		= renderer.m_ConstantBuffer;
}
CMeshRenderer::~CMeshRenderer()
{
}
void CMeshRenderer::Init(CGameObject* gameObject, const std::string& vertexShaderName, const std::string& pixelShaderName, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, RenderTypeEnum type)
{
	this->m_RenderType			= type;
	this->m_VertexShaderName	= vertexShaderName;
	this->m_PixelShaderName		= pixelShaderName;
	this->m_GameObject			= gameObject;
	this->SetInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	CRenderDevice::CreateBuffer(this->m_PerDrawInfo.PerDrawBuffer, CustomStruct::CRenderBufferDesc(sizeof(CustomStruct::CShaderGlobalPerDraw), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	this->LoadShader();
}
void CMeshRenderer::SetPerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter)
{
	this->m_PerDrawInfo.PerDrawData.WorldMatrix				= worldMatrix.GetGPUUploadFloat4x4();
	this->m_PerDrawInfo.PerDrawData.WorldInvMatrix			= worldInvMatrix.GetGPUUploadFloat4x4();
	this->m_PerDrawInfo.PerDrawData.WorldInvTransposeMatrix	= worldInvMatrix.GetXMFLOAT4X4();
	this->m_PerDrawInfo.PerDrawData.CustomParameter			= customParameter.GetXMFLOAT4();
}
void CMeshRenderer::UploadConstantBuffer(const void* ptrData)
{
	CRenderDevice::UploadBuffer(this->m_ConstantBuffer, ptrData);
}
void CMeshRenderer::BindConstantBuffer(const UINT& startSlot)
{
	CRenderDevice::BindVSConstantBuffer(this->m_ConstantBuffer, startSlot);
	CRenderDevice::BindPSConstantBuffer(this->m_ConstantBuffer, startSlot);
}
void CMeshRenderer::Draw(const BOOL& needPixelShader)
{
	this->Bind(needPixelShader);
	if (this->m_GameObject->GetMesh()->GetSubMeshInfo().size() < 1)
	{
		CRenderDevice::DrawIndexed(this->m_GameObject->GetMesh()->GetIndexCount());
	}
	else
	{
		for (UINT i = 0; i < static_cast<UINT>(this->m_GameObject->GetMesh()->GetSubMeshInfo().size()); i++)
		{
			const CustomStruct::CSubMeshInfo& subInfo = (this->m_GameObject->GetMesh()->GetSubMeshInfo())[i];
			CRenderDevice::DrawIndexed(subInfo.IndexCount, subInfo.IndexStart, subInfo.VertexStart);
		}
	}
}
void CMeshRenderer::SetInputLayoutDesc(const CustomStruct::CRenderInputLayoutDesc* layoutDesc, const UINT& layoutNum)
{
	if (this->m_InputLayoutDesc.size() != layoutNum)
	{
		this->m_InputLayoutDesc.resize(layoutNum);
	}
	for (UINT i = 0u; i < layoutNum; i++)
	{
		this->m_InputLayoutDesc[i] = layoutDesc[i];
	}
}
void CMeshRenderer::LoadShader()
{
	this->m_VertexShader = CShaderManager::LoadVertexShader(this->m_VertexShaderName, this->m_InputLayoutDesc.data(), static_cast<UINT>(this->m_InputLayoutDesc.size()));
	this->m_PixelShader = CShaderManager::LoadPixelShader(this->m_PixelShaderName);
}
BOOL CMeshRenderer::CreateConstantBuffer(const UINT& size)
{
	this->m_ConstantBufferDesc = CustomStruct::CRenderBufferDesc(size, CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT));
	return (CRenderDevice::CreateBuffer(m_ConstantBuffer, this->m_ConstantBufferDesc));
}
void CMeshRenderer::UploadPerDrawConstantBuffer()
{
	CRenderDevice::UploadBuffer(this->m_PerDrawInfo.PerDrawBuffer, static_cast<const void*>(&(this->m_PerDrawInfo.PerDrawData)));
}
void CMeshRenderer::Bind(const BOOL& needPixelShader)
{
	if (this->m_GameObject == NULL || this->m_GameObject->GetMesh() == NULL || this->m_VertexShader == nullptr)
		return;
	this->UploadPerDrawConstantBuffer();
	CRenderDevice::BindVSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	CRenderDevice::BindPSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	this->m_VertexShader->Bind();
	CRenderDevice::SetVertexBuffer(this->m_GameObject->GetMesh()->GetVertexBuffer(), this->m_GameObject->GetMesh()->GetVertexStride());
	CRenderDevice::SetIndexBuffer(this->m_GameObject->GetMesh()->GetIndexBuffer());
	if (needPixelShader && this->m_PixelShader != nullptr)
		this->m_PixelShader->Bind();
}