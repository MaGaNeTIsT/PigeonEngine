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
		CustomType::Vector3 boundMin, boundMax;
		this->m_Mesh->GetMinMaxBounding(boundMin, boundMax);
		{
			auto errorMinMax = [](CustomType::Vector3& v0, CustomType::Vector3& v1) {
				FLOAT errorV[3] = { v1.X() - v0.X(), v1.Y() - v0.Y(), v1.Z() - v0.Z() };
				for (UINT i = 0u; i < 3u; i++)
				{
					errorV[i] = (errorV[i] < 2.5f) ? 2.5f : 0.f;
				}
				v0 = CustomType::Vector3(v0.X() - errorV[0], v0.Y() - errorV[1], v0.Z() - errorV[2]);
				v1 = CustomType::Vector3(v1.X() + errorV[0], v1.Y() + errorV[1], v1.Z() + errorV[2]);
			};
			errorMinMax(boundMin, boundMax);
		}
		auto boundingSphere = [&](CustomType::Vector3& anchor, FLOAT& radius) {
			CustomType::Vector3 tempVec(boundMax);
			tempVec = (tempVec - boundMin) * 0.5f;
			anchor = tempVec + boundMin;
			radius = tempVec.Length(); };
		this->SetBoundingBox(boundMin, boundMax - boundMin);
		{
			CustomType::Vector3 anchor; FLOAT radius;
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