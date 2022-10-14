#include "../../../../Entry/EngineMain.h"
#include "../Headers/CTestModel.h"
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
	CustomStruct::CRenderInputLayoutDesc desc[4u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

	this->m_Mesh = CMeshManager::LoadMeshFromFile("./Engine/Assets/Robot/Object/Robot.obj", desc, 4u, TRUE);
	this->m_MeshRenderer = new CMeshRenderer();
	this->m_MeshRenderer->Init(this, ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS, desc, 4u, CMeshRenderer::RenderTypeEnum::RENDER_TYPE_OPAQUE);

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Engine/Assets/Robot/Textures/HX_DJ_Robot_BaseColor.tga");
	this->m_NormalTexture = CTextureManager::LoadTexture2D("./Engine/Assets/Robot/Textures/HX_DJ_Robot_Normal.tga", FALSE);
	this->m_PropertyTexture = CTextureManager::LoadTexture2D("./Engine/Assets/Robot/Textures/HX_DJ_Robot_MR.tga", FALSE);

	{
		std::vector<FLOAT> minmax(6u, 0.f);

		BOOL havePosition = FALSE;
		UINT offsetPosition = 0u;
		if (this->m_Mesh->IsHaveVertexData())
		{
			UINT vertexStride = 0u;
			for (UINT i = 0u; i < this->m_Mesh->GetInputLayoutDesc().size(); i++)
			{
				if (this->m_Mesh->GetInputLayoutDesc()[i].SemanticName == CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION)
				{
					havePosition = TRUE;
					offsetPosition = vertexStride;
				}
				vertexStride += this->m_Mesh->GetInputLayoutDesc()[i].GetSemanticSizeByByte();
			}
		}

		{
			auto compare = [&](const FLOAT* input)
			{
				minmax[0 * 3 + 0] = CustomType::CMath::Min(input[0u], minmax[0 * 3 + 0]);
				minmax[0 * 3 + 1] = CustomType::CMath::Min(input[1u], minmax[0 * 3 + 1]);
				minmax[0 * 3 + 2] = CustomType::CMath::Min(input[2u], minmax[0 * 3 + 2]);
				minmax[1 * 3 + 0] = CustomType::CMath::Max(input[0u], minmax[1 * 3 + 0]);
				minmax[1 * 3 + 1] = CustomType::CMath::Max(input[1u], minmax[1 * 3 + 1]);
				minmax[1 * 3 + 2] = CustomType::CMath::Max(input[2u], minmax[1 * 3 + 2]);
			};

			if (havePosition)
			{
				for (UINT i = 0u; i < this->m_Mesh->GetVertexCount(); i++)
				{
					const CHAR* tempVertex = &(((const CHAR*)(this->m_Mesh->GetVertexData()))[i * this->m_Mesh->GetVertexStride()]);
					const FLOAT* tempPosition = (const FLOAT*)(&(tempVertex[offsetPosition]));
					compare(tempPosition);
				}
				this->SetBoundingBox(CustomType::Vector3(minmax[0], minmax[1], minmax[2]), CustomType::Vector3(minmax[3] - minmax[0], minmax[4] - minmax[1], minmax[5] - minmax[2]));
			}
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

			if (havePosition)
			{
				FLOAT radius;
				CustomType::Vector3 anchor;
				boundingSphere(anchor, radius);
				this->SetBoundingSphere(anchor, radius);
			}
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
	this->m_MeshRenderer->SetPerDrawInfo(tempWorldMatrix, tempWorldInverseMatrix, CustomType::Vector4::Zero());
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