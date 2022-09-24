#include "../../../../Entry/EngineMain.h"
#include "../Headers/CTestModel.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTexture2D.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMesh.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CCamera.h"
#include "../../EngineGame/Headers/CScene.h"

CTestModel::CTestModel()
{
	this->m_AlbedoTexture = NULL;
	this->m_NormalTexture = NULL;
	this->m_PropertyTexture = NULL;
}
CTestModel::~CTestModel()
{
}
void CTestModel::Init()
{
	this->m_MeshRenderer = new CMeshRenderer(this, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS);
	this->m_Mesh = CMeshManager::LoadMeshFromFile("./Engine/Assets/Robot/Object/Robot.obj", TRUE);
	this->m_MeshRenderer->LoadShader();

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Engine/Assets/Robot/Textures/HX_DJ_Robot_BaseColor.tga", TRUE);
	this->m_NormalTexture = CTextureManager::LoadTexture2D("./Engine/Assets/Robot/Textures/HX_DJ_Robot_Normal.tga", FALSE);
	this->m_PropertyTexture = CTextureManager::LoadTexture2D("./Engine/Assets/Robot/Textures/HX_DJ_Robot_MR.tga", FALSE);

	{
		std::vector<FLOAT> minmax(6u, 0.f);

		{
			auto compare = [&minmax](const CustomStruct::CVertex3D& input)
			{
				minmax[0 * 3 + 0] = CustomType::CMath::Min(input.Position.x, minmax[0 * 3 + 0]);
				minmax[0 * 3 + 1] = CustomType::CMath::Min(input.Position.y, minmax[0 * 3 + 1]);
				minmax[0 * 3 + 2] = CustomType::CMath::Min(input.Position.z, minmax[0 * 3 + 2]);
				minmax[1 * 3 + 0] = CustomType::CMath::Max(input.Position.x, minmax[1 * 3 + 0]);
				minmax[1 * 3 + 1] = CustomType::CMath::Max(input.Position.y, minmax[1 * 3 + 1]);
				minmax[1 * 3 + 2] = CustomType::CMath::Max(input.Position.z, minmax[1 * 3 + 2]);
			};
			for (UINT i = 0u; i < this->m_Mesh->GetVertexData().size(); i++)
			{
				const CustomStruct::CVertex3D& vertex = (this->m_Mesh->GetVertexData())[i];
				compare(vertex);
			}
			this->SetBoundingBox(CustomType::Vector3(minmax[0], minmax[1], minmax[2]), CustomType::Vector3(minmax[3] - minmax[0], minmax[4] - minmax[1], minmax[5] - minmax[2]));
		}

		{
			auto boundingSphere = [&minmax](CustomType::Vector3& anchor, FLOAT& radius)
			{
				CustomType::Vector3 tempAnchor(minmax[0], minmax[1], minmax[2]);
				CustomType::Vector3 tempVec(minmax[3], minmax[4], minmax[5]);
				tempVec = (tempVec - tempAnchor) * 0.5f;

				anchor = tempVec + tempAnchor;
				radius = tempVec.Length();
			};
			FLOAT radius;
			CustomType::Vector3 anchor;
			boundingSphere(anchor, radius);
			this->SetBoundingSphere(anchor, radius);
		}
	}
}
void CTestModel::Uninit()
{
	if (this->m_MeshRenderer != NULL)
	{
		delete (this->m_MeshRenderer);
		this->m_MeshRenderer = NULL;
	}
}
void CTestModel::Update()
{

}
void CTestModel::FixedUpdate()
{

}
void CTestModel::PrepareDraw()
{
	CustomType::Matrix4x4 tempWorldMatrix(this->GetLocalToWorldMatrix());
	CustomType::Matrix4x4 tempWorldInverseMatrix(tempWorldMatrix.Inverse());
	this->m_ConstantBuffer.WorldMatrix = tempWorldMatrix.GetGPUUploadFloat4x4();
	this->m_ConstantBuffer.WorldInvMatrix = tempWorldInverseMatrix.GetGPUUploadFloat4x4();
	this->m_ConstantBuffer.WorldInvTransposeMatrix = tempWorldInverseMatrix.GetXMFLOAT4X4();

	this->m_MeshRenderer->UploadPerDrawConstantBuffer(this->m_ConstantBuffer);
}
void CTestModel::Draw()
{
	this->PrepareDraw();
	CRenderDevice::BindPSShaderResourceView(this->m_AlbedoTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
	CRenderDevice::BindPSShaderResourceView(this->m_NormalTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_NORMAL_START_SLOT);
	CRenderDevice::BindPSShaderResourceView(this->m_PropertyTexture->GetShaderResourceView(), ENGINE_TEXTURE2D_PROPERTY_START_SLOT);
	this->m_MeshRenderer->Draw();
}
void CTestModel::DrawExtra()
{
	this->PrepareDraw();
	CRenderPipeline::GetDefaultEmptyPS()->Bind();
	this->m_MeshRenderer->Draw(FALSE);
}