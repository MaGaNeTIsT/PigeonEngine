#include "../../../../../Entry/EngineMain.h"
#include "../Headers/CMeshRendererComponent.h"
#include "../Headers/CRenderDevice.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../../EngineGame/Headers/CGameObject.h"

CMeshRendererComponent::CMeshRendererComponent() : CRenderComponent(TRUE, FALSE, FALSE)
{
	this->m_GameObject.reset();
	this->m_RenderType			= RenderTypeEnum::RENDER_TYPE_OPAQUE;
	this->m_VertexShaderName	= ENGINE_SHADER_NONE;
	this->m_PixelShaderName		= ENGINE_SHADER_NONE;
	this->m_VertexShader.reset();
	this->m_PixelShader.reset();
	//this->m_InputLayoutDesc.clear();
	//::ZeroMemory(&(this->m_PerDrawInfo), sizeof(this->m_PerDrawInfo));
	//this->m_ConstantBufferDesc = CustomStruct::CRenderBufferDesc();
	this->m_ConstantBuffer		= nullptr;
}
CMeshRendererComponent::CMeshRendererComponent(const CMeshRendererComponent& renderer) : CRenderComponent(TRUE, FALSE, FALSE)
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
CMeshRendererComponent::~CMeshRendererComponent()
{
}
void CMeshRendererComponent::InitShadersAndInputLayout(const std::string& vertexShaderName, const std::string& pixelShaderName, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, RenderTypeEnum type)
{
	this->m_RenderType			= type;
	this->m_VertexShaderName	= vertexShaderName;
	this->m_PixelShaderName		= pixelShaderName;
	this->SetInputLayoutDesc(inputLayoutDesc, inputLayoutNum);
	CRenderDevice::CreateBuffer(this->m_PerDrawInfo.PerDrawBuffer, CustomStruct::CRenderBufferDesc(sizeof(CustomStruct::CShaderGlobalPerDraw), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	this->LoadShader();
}
void CMeshRendererComponent::SetMeshComponent(std::shared_ptr<CMeshComponent> meshComponent)
{
	this->m_MeshComponent = meshComponent;
}
void CMeshRendererComponent::SetPerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter)
{
	this->m_PerDrawInfo.PerDrawData.WorldMatrix				= worldMatrix.GetGPUUploadFloat4x4();
	this->m_PerDrawInfo.PerDrawData.WorldInvMatrix			= worldInvMatrix.GetGPUUploadFloat4x4();
	this->m_PerDrawInfo.PerDrawData.WorldInvTransposeMatrix	= worldInvMatrix.GetXMFLOAT4X4();
	this->m_PerDrawInfo.PerDrawData.CustomParameter			= customParameter.GetXMFLOAT4();
}
void CMeshRendererComponent::UploadConstantBuffer(const void* ptrData)
{
	CRenderDevice::UploadBuffer(this->m_ConstantBuffer, ptrData);
}
void CMeshRendererComponent::BindConstantBuffer(const UINT& startSlot)
{
	CRenderDevice::BindVSConstantBuffer(this->m_ConstantBuffer, startSlot);
	CRenderDevice::BindPSConstantBuffer(this->m_ConstantBuffer, startSlot);
}
void CMeshRendererComponent::Draw()const
{
	if (this->m_MeshComponent.expired() || !this->m_VertexShader || !this->m_PixelShader)
	{
		return;
	}
	this->Bind(TRUE);
	std::shared_ptr<CMeshComponent> meshSharedPtr(this->m_MeshComponent.lock());
	std::vector<CustomStruct::CSubMeshInfo> subMesh = meshSharedPtr->GetSubMeshInfo();
	if (subMesh.size() < 1)
	{
		CRenderDevice::DrawIndexed(meshSharedPtr->GetIndexCount());
	}
	else
	{
		for (UINT i = 0; i < static_cast<UINT>(subMesh.size()); i++)
		{
			const CustomStruct::CSubMeshInfo& subInfo = subMesh[i];
			CRenderDevice::DrawIndexed(subInfo.IndexCount, subInfo.IndexStart, subInfo.VertexStart);
		}
	}
}
void CMeshRendererComponent::DrawExtra()const
{
	if (this->m_MeshComponent.expired() || !this->m_VertexShader)
	{
		return;
	}
	this->Bind(FALSE);
	std::shared_ptr<CMeshComponent> meshSharedPtr(this->m_MeshComponent.lock());
	std::vector<CustomStruct::CSubMeshInfo> subMesh = meshSharedPtr->GetSubMeshInfo();
	if (subMesh.size() < 1)
	{
		CRenderDevice::DrawIndexed(meshSharedPtr->GetIndexCount());
	}
	else
	{
		for (UINT i = 0; i < static_cast<UINT>(subMesh.size()); i++)
		{
			const CustomStruct::CSubMeshInfo& subInfo = subMesh[i];
			CRenderDevice::DrawIndexed(subInfo.IndexCount, subInfo.IndexStart, subInfo.VertexStart);
		}
	}
}
void CMeshRendererComponent::SetInputLayoutDesc(const CustomStruct::CRenderInputLayoutDesc* layoutDesc, const UINT& layoutNum)
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
void CMeshRendererComponent::LoadShader()
{
	this->m_VertexShader = CShaderManager::LoadVertexShader(this->m_VertexShaderName, this->m_InputLayoutDesc.data(), static_cast<UINT>(this->m_InputLayoutDesc.size()));
	this->m_PixelShader = CShaderManager::LoadPixelShader(this->m_PixelShaderName);
}
BOOL CMeshRendererComponent::CreateConstantBuffer(const UINT& size)
{
	this->m_ConstantBufferDesc = CustomStruct::CRenderBufferDesc(size, CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT));
	return (CRenderDevice::CreateBuffer(m_ConstantBuffer, this->m_ConstantBufferDesc));
}
void CMeshRendererComponent::UploadPerDrawConstantBuffer()const
{
	CRenderDevice::UploadBuffer(this->m_PerDrawInfo.PerDrawBuffer, static_cast<const void*>(&(this->m_PerDrawInfo.PerDrawData)));
}
void CMeshRendererComponent::Bind(const BOOL& needPixelShader)const
{
	if (this->m_MeshComponent.expired() || !this->m_VertexShader)
	{
		return;
	}
	std::shared_ptr<CMeshComponent> meshSharedPtr(this->m_MeshComponent);
	this->UploadPerDrawConstantBuffer();
	CRenderDevice::BindVSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	CRenderDevice::BindPSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	this->m_VertexShader->Bind();
	CRenderDevice::SetVertexBuffer(meshSharedPtr->GetVertexBuffer(), meshSharedPtr->GetVertexStride());
	CRenderDevice::SetIndexBuffer(meshSharedPtr->GetIndexBuffer());
	if (needPixelShader && this->m_PixelShader)
	{
		this->m_PixelShader->Bind();
	}
}