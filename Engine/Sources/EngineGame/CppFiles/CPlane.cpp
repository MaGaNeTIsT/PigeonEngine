#include "../../../../Entry/EngineMain.h"
#include "../Headers/CPlane.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureType.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../Headers/CComponent.h"
#include "../Headers/CGameBoundComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../EngineGame/Headers/CCamera.h"
#include "../../EngineGame/Headers/CScene.h"
#include "../../EnginePhysics/Headers/CRigidBodyComponent.h"

CPlane::CPlane()
{
	this->AddNewTransform();

	//CGameBoundSphereComponent* boundSphereComponent = new CGameBoundSphereComponent();
	//this->AddComponent(boundSphereComponent);
	CGameBoundBoxComponent* boundBoxComponent = new CGameBoundBoxComponent();
	this->AddComponent(boundBoxComponent);

	this->m_MeshComponent				= NULL;
	this->m_PlaneMeshInfo.Length		= 1.f;
	this->m_PlaneMeshInfo.UV			= 1.f;
	this->m_PlaneMeshInfo.VertexCount	= 2;
}
CPlane::CPlane(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	this->AddNewTransform();

	//CGameBoundSphereComponent* boundSphereComponent = new CGameBoundSphereComponent();
	//this->AddComponent(boundSphereComponent);
	CGameBoundBoxComponent* boundBoxComponent = new CGameBoundBoxComponent();
	this->AddComponent(boundBoxComponent);

	this->m_MeshComponent				= NULL;
	this->m_PlaneMeshInfo.Length		= length;
	this->m_PlaneMeshInfo.UV			= uv;
	this->m_PlaneMeshInfo.VertexCount	= vertexCount;
}
CPlane::~CPlane()
{
}
void CPlane::Init()
{
	CDefaultLitMaterial* material = NULL;
	if (!this->HasMeshComponent() && !this->HasMeshRendererComponent())
	{
		CMeshRendererComponent* meshRendererComponent = new CMeshRendererComponent();
		CMeshComponent* meshComponent = new CMeshComponent();
		this->AddComponent(meshRendererComponent);
		this->AddComponent(meshComponent);

		CustomStruct::CRenderInputLayoutDesc desc[4u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		meshRendererComponent->SetMeshComponent(meshComponent);
		material = meshRendererComponent->AddMaterial<CDefaultLitMaterial>();

		this->m_MeshComponent = meshComponent;
	}
	this->SetMeshInfo(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV);

	CGameObject::Init();

	material->SetAlbedoTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/WetChess/WetChess_Albedo.tga"));
	material->SetNormalTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/WetChess/WetChess_Normal.tga", FALSE));
	material->SetRoughnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/WetChess/WetChess_R.tga", FALSE));
	material->SetMetallicnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/WetChess/WetChess_M.tga", FALSE));

	CRigidBodyComponent* RigidBodyComponent = new CRigidBodyComponent();
	this->AddComponent(RigidBodyComponent);
	this->m_RigidBodyComponent = RigidBodyComponent;
}
void CPlane::Update()
{
	CGameObject::Update();
}
void CPlane::SetMeshInfo(const CustomType::Vector2& length, const CustomType::Vector2Int& vertexCount, const CustomType::Vector2& uv)
{
	this->m_PlaneMeshInfo.Length = length;
	this->m_PlaneMeshInfo.UV = uv;
	this->m_PlaneMeshInfo.VertexCount = vertexCount;

	CustomStruct::CRenderInputLayoutDesc desc[4u] = {
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
		CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
	this->m_MeshComponent->SetMesh(CMeshManager::LoadPlaneMesh(this->m_PlaneMeshInfo.Length, this->m_PlaneMeshInfo.VertexCount, this->m_PlaneMeshInfo.UV, desc, 4u));
	{
		auto errorMinMax = [](CustomType::Vector3& v0, CustomType::Vector3& v1, const FLOAT& error) {
			FLOAT errorV[3] = { v1.X() - v0.X(), v1.Y() - v0.Y(), v1.Z() - v0.Z() };
			for (UINT i = 0u; i < 3u; i++)
			{
				errorV[i] = (errorV[i] < error) ? error : 0.f;
			}
			v0 = CustomType::Vector3(v0.X() - errorV[0], v0.Y() - errorV[1], v0.Z() - errorV[2]);
			v1 = CustomType::Vector3(v1.X() + errorV[0], v1.Y() + errorV[1], v1.Z() + errorV[2]); };

		CustomType::Vector3 boundMin, boundMax;
		this->m_MeshComponent->GetMinMaxBounding(boundMin, boundMax);

		{
			CustomType::Vector3 renderBoundMin(boundMin), renderBoundMax(boundMax);
			errorMinMax(renderBoundMin, renderBoundMax, CustomStruct::CRenderBaseSetting::RenderBoundHalfMinimum);
			auto boundingSphere = [&](CustomType::Vector3& anchor, FLOAT& radius) {
				CustomType::Vector3 tempVec(renderBoundMax);
				tempVec = (tempVec - renderBoundMin) * 0.5f;
				anchor = tempVec + renderBoundMin;
				radius = tempVec.Length(); };
			this->SetRenderLocalBoundingBox(renderBoundMin, renderBoundMax - renderBoundMin);
			{
				CustomType::Vector3 anchor; FLOAT radius;
				boundingSphere(anchor, radius);
				this->SetRenderLocalBoundingSphere(anchor, radius);
			}
		}

		{
			CustomType::Vector3 gameBoundMin(boundMin), gameBoundMax(boundMax);
			errorMinMax(gameBoundMin, gameBoundMax, CustomStruct::CGameBaseSetting::GameBoundHalfMinimum);
			CustomType::Vector3 anchor(((gameBoundMin + gameBoundMax) * 0.5f));
			CustomType::Vector3 extent((gameBoundMax - gameBoundMin));
			CGameBoundBoxComponent* boxComponent = this->GetGameBoundComponent<CGameBoundBoxComponent>();
			if (boxComponent != NULL)
			{
				boxComponent->SetLocalAnchorExtent(anchor, extent * 0.5f);
			}
			CGameBoundSphereComponent* sphereComponent = this->GetGameBoundComponent<CGameBoundSphereComponent>();
			if (sphereComponent != NULL)
			{
				sphereComponent->SetLocalAnchorRadius(anchor, extent.Length() * 0.5f);
			}
		}
	}
}