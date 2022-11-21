#include "../../../../../Entry/EngineMain.h"
#include "../Headers/CMeshRendererComponent.h"
#include "../Headers/CRenderDevice.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderBase/Headers/CRenderPipeline.h"
#include "../../../EngineGame/Headers/CGameObject.h"

CMeshRendererComponent::CMeshRendererComponent() : CRenderComponent(TRUE, TRUE, FALSE)
{
	this->m_CurrentFrameUpload	= FALSE;
	this->m_AlbedoTexture		= NULL;
	this->m_NormalTexture		= NULL;
	this->m_PropertyTexture		= NULL;
	this->m_RenderType			= RenderTypeEnum::RENDER_TYPE_OPAQUE;
	this->m_VertexShaderName	= ENGINE_SHADER_NONE;
	this->m_PixelShaderName		= ENGINE_SHADER_NONE;
	this->m_VertexShader		= NULL;
	this->m_PixelShader			= NULL;
	this->m_ConstantBuffer		= nullptr;
}
CMeshRendererComponent::~CMeshRendererComponent()
{
	if (this->HasVertexShader())
	{
		delete (this->m_VertexShader);
		this->m_VertexShader = NULL;
	}
	if (this->HasPixelShader())
	{
		delete (this->m_PixelShader);
		this->m_PixelShader = NULL;
	}
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
void CMeshRendererComponent::SetMaterialTextures(CTexture2D* albedoTexture, CTexture2D* normalTexture, CTexture2D* propertyTexture)
{
	this->m_AlbedoTexture = (albedoTexture != NULL) ? albedoTexture : CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY);
	this->m_NormalTexture = (normalTexture != NULL) ? normalTexture : CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP);
	this->m_PropertyTexture = (propertyTexture != NULL) ? propertyTexture : CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY);
}
void CMeshRendererComponent::SetMeshComponent(const CMeshComponent* meshComponent)
{
	this->m_MeshComponent = meshComponent;
}
void CMeshRendererComponent::SetPerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter)const
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
void CMeshRendererComponent::Update()const
{
	this->m_CurrentFrameUpload = FALSE;
	if (this->m_GameObject != NULL)
	{
		CustomType::Matrix4x4 locaToWorld(this->m_GameObject->GetLocalToWorldMatrix());
		this->SetPerDrawInfo(locaToWorld, locaToWorld.Inverse(), CustomType::Vector4(0.f));
	}
}
void CMeshRendererComponent::Draw()const
{
	if (!this->HasMesh() || !this->HasVertexShader() || !this->HasPixelShader())
	{
		return;
	}
	this->Bind(TRUE);
	std::vector<CustomStruct::CSubMeshInfo> subMesh = this->m_MeshComponent->GetSubMeshInfo();
	if (subMesh.size() < 1)
	{
		CRenderDevice::DrawIndexed(this->m_MeshComponent->GetIndexCount());
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
	if (!this->HasMesh() || !this->HasVertexShader())
	{
		return;
	}
	this->Bind(FALSE);
	std::vector<CustomStruct::CSubMeshInfo> subMesh = this->m_MeshComponent->GetSubMeshInfo();
	if (subMesh.size() < 1)
	{
		CRenderDevice::DrawIndexed(this->m_MeshComponent->GetIndexCount());
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
	if (this->HasVertexShader())
	{
		delete (this->m_VertexShader);
		this->m_VertexShader = NULL;
	}
	if (this->HasPixelShader())
	{
		delete (this->m_PixelShader);
		this->m_PixelShader = NULL;
	}
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
	if (!this->HasMesh() || !this->HasVertexShader())
	{
		return;
	}
	if (!this->m_CurrentFrameUpload)
	{
		this->UploadPerDrawConstantBuffer();
		this->m_CurrentFrameUpload = TRUE;
	}
	CRenderDevice::BindVSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	CRenderDevice::BindPSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	this->m_VertexShader->Bind();
	CRenderDevice::SetVertexBuffer(this->m_MeshComponent->GetVertexBuffer(), this->m_MeshComponent->GetVertexStride());
	CRenderDevice::SetIndexBuffer(this->m_MeshComponent->GetIndexBuffer());
	if (needPixelShader && this->HasPixelShader())
	{
		if (this->m_AlbedoTexture != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(this->m_AlbedoTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
		}
		if (this->m_NormalTexture != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(this->m_NormalTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_NORMAL_START_SLOT);
		}
		if (this->m_PropertyTexture != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(this->m_PropertyTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_PROPERTY_START_SLOT);
		}
		this->m_PixelShader->Bind();
	}
}