#include "../Headers/CMeshRenderer.h"
#include "../Headers/CRenderDevice.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CMesh.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../../EngineGame/Headers/CGameObject.h"

CMeshRenderer::CMeshRenderer(CGameObject* gameObject, const std::string& vertexShaderName, const std::string& pixelShaderName, CRenderTypeEnum type)
{
	this->m_RenderType			= type;
	this->m_VertexShaderName	= vertexShaderName;
	this->m_PixelShaderName		= pixelShaderName;
	this->m_VertexShader		= nullptr;
	this->m_PixelShader			= nullptr;
	this->m_GameObject			= gameObject;
	this->m_InputLayoutDesc.resize(5u);
	this->m_InputLayoutDesc[0] = { "POSITION", 0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
	this->m_InputLayoutDesc[1] = { "NORMAL",   0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
	this->m_InputLayoutDesc[2] = { "TANGENT",  0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
	this->m_InputLayoutDesc[3] = { "COLOR",    0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
	this->m_InputLayoutDesc[4] = { "TEXCOORD", 0u, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,       0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
	CRenderDevice::CreateBuffer(this->m_PerDrawBuffer, CustomStruct::CRenderBufferDesc(sizeof(CustomStruct::ConstantBufferPerDraw), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
}
CMeshRenderer::~CMeshRenderer()
{
}
void CMeshRenderer::LoadShader()
{
	this->m_VertexShader = CShaderManager::LoadVertexShader(this->m_VertexShaderName, &(this->m_InputLayoutDesc));
	this->m_PixelShader = CShaderManager::LoadPixelShader(this->m_PixelShaderName);
}
void CMeshRenderer::SetInputLayoutDesc(const std::vector<D3D11_INPUT_ELEMENT_DESC>& desc)
{
	this->m_InputLayoutDesc = desc;
}
BOOL CMeshRenderer::CreateConstantBuffer(const UINT& size)
{
	this->m_ConstantBufferDesc = CustomStruct::CRenderBufferDesc(size, CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT));
	return (CRenderDevice::CreateBuffer(m_ConstantBuffer, this->m_ConstantBufferDesc));
}
void CMeshRenderer::UploadPerDrawConstantBuffer(const CustomStruct::ConstantBufferPerDraw& data)
{
	CRenderDevice::UploadBuffer(this->m_PerDrawBuffer, static_cast<const void*>(&data));
}
void CMeshRenderer::UploadConstantBuffer(const void* ptrData)
{
	CRenderDevice::UploadBuffer(this->m_ConstantBuffer, ptrData);
}
void CMeshRenderer::Bind(const BOOL& needPixelShader)
{
	if (this->m_GameObject == NULL || this->m_GameObject->GetMesh() == NULL || this->m_VertexShader == nullptr)
		return;
	CRenderDevice::BindVSConstantBuffer(this->m_PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	CRenderDevice::BindPSConstantBuffer(this->m_PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	this->m_VertexShader->Bind();
	CRenderDevice::SetVertexBuffer(this->m_GameObject->GetMesh()->GetVertexBuffer());
	CRenderDevice::SetIndexBuffer(this->m_GameObject->GetMesh()->GetIndexBuffer());
	if (needPixelShader && this->m_PixelShader != nullptr)
		this->m_PixelShader->Bind();
}
void CMeshRenderer::Draw(const BOOL& needPixelShader)
{
	this->Bind(needPixelShader);
	if (this->m_GameObject->GetMesh()->GetSubMeshInfo().size() < 1)
		CRenderDevice::DrawIndexed(this->m_GameObject->GetMesh()->GetIndexCount());
	else
	{
		for (UINT i = 0; i < static_cast<UINT>(this->m_GameObject->GetMesh()->GetSubMeshInfo().size()); i++)
		{
			const CustomStruct::CSubMeshInfo& subInfo = (this->m_GameObject->GetMesh()->GetSubMeshInfo())[i];
			CRenderDevice::DrawIndexed(subInfo.IndexCount, subInfo.IndexStart, subInfo.VertexStart);
		}
	}
}