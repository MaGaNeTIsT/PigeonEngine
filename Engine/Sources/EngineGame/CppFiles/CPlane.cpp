#include "../../../../Entry/EngineMain.h"
#include "../Headers/CPlane.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureType.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CCamera.h"
#include "../../EngineGame/Headers/CScene.h"

CPlane::CPlane()
{
	this->AddNewTransform();
	this->m_AlbedoTexture	= NULL;
	this->m_NormalTexture	= NULL;
	this->m_PropertyTexture	= NULL;
	this->m_PlaneMeshInfo.Length		= 1.f;
	this->m_PlaneMeshInfo.UV			= 1.f;
	this->m_PlaneMeshInfo.VertexCount	= 2;
}
CPlane::CPlane(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	this->AddNewTransform();
	this->m_AlbedoTexture	= NULL;
	this->m_NormalTexture	= NULL;
	this->m_PropertyTexture = NULL;
	this->m_PlaneMeshInfo.Length		= length;
	this->m_PlaneMeshInfo.UV			= uv;
	this->m_PlaneMeshInfo.VertexCount	= vertexCount;
}
CPlane::~CPlane()
{
}
void CPlane::Init()
{
	{
		std::shared_ptr<CMeshRendererComponent> meshRendererComponent(std::shared_ptr<CMeshRendererComponent>(new CMeshRendererComponent()));
		std::shared_ptr<CMeshComponent> meshComponent(std::shared_ptr<CMeshComponent>(new CMeshComponent()));
		this->AddComponent(meshRendererComponent);
		this->AddComponent(meshComponent);

		this->m_PlaneMesh = std::weak_ptr<CMeshComponent>(meshComponent);

		CustomStruct::CRenderInputLayoutDesc desc[4u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		meshRendererComponent->InitShadersAndInputLayout(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS, desc, 4u, CMeshRendererComponent::RenderTypeEnum::RENDER_TYPE_OPAQUE);
		meshRendererComponent->SetMeshComponent(meshComponent);
	}
	this->SetMeshInfo(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV);

	this->m_AlbedoTexture = CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/WetChess/WetChess_Albedo.tga");
	this->m_NormalTexture = CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/WetChess/WetChess_Normal.tga", FALSE);
	this->m_PropertyTexture = CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/WetChess/WetChess_Property.tga", FALSE);

	CGameObject::Init();
}
void CPlane::Update()
{
}
void CPlane::SetMeshInfo(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	this->m_PlaneMeshInfo.Length = length;
	this->m_PlaneMeshInfo.UV = uv;
	this->m_PlaneMeshInfo.VertexCount = vertexCount;

	std::shared_ptr<CMeshComponent> mesh(this->m_PlaneMesh.lock());

	CustomStruct::CRenderInputLayoutDesc desc[4u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	mesh->SetMesh(CMeshManager::LoadPlaneMesh(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV, desc, 4u));
	{
		CustomType::Vector3 boundMin, boundMax;
		mesh->GetMinMaxBounding(boundMin, boundMax);
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
		this->SetRenderLocalBoundingBox(boundMin, boundMax - boundMin);
		{
			CustomType::Vector3 anchor; FLOAT radius;
			boundingSphere(anchor, radius);
			this->SetRenderLocalBoundingSphere(anchor, radius);
		}
	}
}