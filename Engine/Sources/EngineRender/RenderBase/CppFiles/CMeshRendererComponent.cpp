#include "../Headers/CMeshRendererComponent.h"
#include "../Headers/CRenderDevice.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderBase/Headers/CRenderPipeline.h"
#include "../../RenderBase/Headers/CMaterialBase.h"
#include "../../../EngineGame/Headers/CGameObject.h"

CMeshRendererComponent::CMeshRendererComponent() : CRenderComponent(TRUE, TRUE, FALSE)
{
	this->m_CurrentFramePerDrawUpload	= FALSE;
	this->m_CurrentFrameMaterialUpload	= FALSE;
	this->m_MeshComponent				= NULL;
	this->m_Material					= NULL;
	this->m_ConstantBuffer				= nullptr;
}
CMeshRendererComponent::~CMeshRendererComponent()
{
	if (this->HasMaterial())
	{
		delete (this->m_Material);
		this->m_Material = NULL;
	}
}
CMaterialBase::MaterialType	CMeshRendererComponent::GetMaterialType()const
{
	if (this->HasMaterial())
	{
		return (this->m_Material->GetMaterialType());
	}
	return CMaterialBase::MaterialType::MATERIAL_TYPE_NONE;
}
void CMeshRendererComponent::SetMeshComponent(const CMeshComponent* meshComponent)
{
	this->m_MeshComponent = meshComponent;
}
void CMeshRendererComponent::UpdatePerDrawInfo(CustomType::Matrix4x4& worldMatrix, CustomType::Matrix4x4& worldInvMatrix, const CustomType::Vector4& customParameter)
{
	this->m_PerDrawInfo.PerDrawData.WorldMatrix				= worldMatrix.GetGPUUploadFloat4x4();
	this->m_PerDrawInfo.PerDrawData.WorldInvMatrix			= worldInvMatrix.GetGPUUploadFloat4x4();
	this->m_PerDrawInfo.PerDrawData.WorldInvTransposeMatrix	= worldInvMatrix.GetXMFLOAT4X4();
	this->m_PerDrawInfo.PerDrawData.CustomParameter			= customParameter.GetXMFLOAT4();
}
void CMeshRendererComponent::Init()
{
	CRenderDevice::CreateBuffer(this->m_PerDrawInfo.PerDrawBuffer, CustomStruct::CRenderBufferDesc(sizeof(CustomStruct::CShaderGlobalPerDraw), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	if (this->HasMaterial())
	{
		this->m_Material->Init();
		if (this->m_Material->HasConstantData())
		{
			this->CreateConstantBuffer(this->m_Material->GetConstantSize());
		}
	}
}
void CMeshRendererComponent::Update()
{
	this->m_CurrentFramePerDrawUpload	= FALSE;
	this->m_CurrentFrameMaterialUpload	= FALSE;
	if (this->m_GameObject != NULL)
	{
		CustomType::Matrix4x4 locaToWorld(this->m_GameObject->GetLocalToWorldMatrix());
		this->UpdatePerDrawInfo(locaToWorld, locaToWorld.Inverse(), CustomType::Vector4(0.f));
	}
}
void CMeshRendererComponent::Draw()const
{
	if (!this->HasMesh() || !this->HasMaterial())
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
	if (!this->HasMesh() || !this->HasMaterial())
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
#if _DEVELOPMENT_EDITOR
void CMeshRendererComponent::SelectedEditorUpdate()
{
	ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
	if (ImGui::TreeNode("MeshRendererComponent"))
	{
		{
			std::string meshComponentName = "NULL";
			if (this->HasMesh())
			{
				meshComponentName = "Has, UniqueID : " + std::to_string((this->m_MeshComponent->GetUniqueID()));
			}
			ImGui::Text("Mesh component : %s", meshComponentName.c_str());
		}

		if (this->HasMaterial())
		{
			this->m_Material->SelectedEditorUpdate();
		}

		ImGui::TreePop();
	}
}
#endif
BOOL CMeshRendererComponent::CreateConstantBuffer(const UINT& size)
{
	this->m_ConstantBufferDesc = CustomStruct::CRenderBufferDesc(size, CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT));
	return (CRenderDevice::CreateBuffer(m_ConstantBuffer, this->m_ConstantBufferDesc));
}
void CMeshRendererComponent::UploadConstantBuffer(const void* ptrData)const
{
	CRenderDevice::UploadBuffer(this->m_ConstantBuffer, ptrData);
}
void CMeshRendererComponent::BindConstantBuffer(const UINT& startSlot)const
{
	CRenderDevice::BindVSConstantBuffer(this->m_ConstantBuffer, startSlot);
	CRenderDevice::BindPSConstantBuffer(this->m_ConstantBuffer, startSlot);
}
void CMeshRendererComponent::UploadPerDrawConstantBuffer()const
{
	CRenderDevice::UploadBuffer(this->m_PerDrawInfo.PerDrawBuffer, static_cast<const void*>(&(this->m_PerDrawInfo.PerDrawData)));
}
void CMeshRendererComponent::Bind(const BOOL& needPixelShader)const
{
	if (!this->HasMesh() || !this->HasMaterial())
	{
		return;
	}
	if (!this->m_CurrentFramePerDrawUpload)
	{
		this->UploadPerDrawConstantBuffer();
		this->m_CurrentFramePerDrawUpload = TRUE;
	}
	if (!m_CurrentFrameMaterialUpload && this->HasConstantBuffer() && this->m_Material->HasConstantData())
	{
		this->UploadConstantBuffer(this->m_Material->GetConstantData());
		this->m_CurrentFrameMaterialUpload = TRUE;
	}
	CRenderDevice::BindVSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	CRenderDevice::BindPSConstantBuffer(this->m_PerDrawInfo.PerDrawBuffer, ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT);
	if (this->HasConstantBuffer())
	{
		this->BindConstantBuffer(ENGINE_CONSTANT_BUFFER_MATERIAL_DATA_START_SLOT);
	}
	this->m_Material->Bind();
	CRenderDevice::SetVertexBuffer(this->m_MeshComponent->GetVertexBuffer(), this->m_MeshComponent->GetVertexStride());
	CRenderDevice::SetIndexBuffer(this->m_MeshComponent->GetIndexBuffer());
	if (this->m_Material->HasVertexShader())
	{
		this->m_Material->GetVertexShader()->Bind();
	}
	if (needPixelShader && this->m_Material->HasVertexShader())
	{
		this->m_Material->GetPixelShader()->Bind();
	}
}