#include "../Headers/CSceneSkeletonMeshObject.h"
#include "../../EngineGame/Headers/CComponent.h"
#include "../../EngineGame/Headers/CGameBoundComponent.h"
#include "../../EngineGame/Headers/CMeshComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CSkeletonComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CSkeletonAnimationManager.h"
#include "../../EngineGame/Headers/CSkeletonAnimationComponent.h"
#include "../../EngineRender/RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../EngineRender/RenderMaterials/Headers/CClearCoatMaterial.h"
#include "../../EngineRender/RenderMaterials/Headers/CClothMaterial.h"

CSceneSkeletonMeshObject::CSceneSkeletonMeshObject(const BOOL& active, const CScene* scene) : CGameObject(active, scene)
{
	this->AddNewTransform();
	{
		std::string skeletonMeshPath = "./Engine/Assets/EngineModels/SceneModels/Misaki/Misaki_SchoolUniform.fbx";
		
		std::string skeletonAnimationPath = "./Engine/Assets/EngineModels/SceneModels/Misaki/Misaki_SchoolUniform.fbx";

		//CGameBoundSphereComponent* boundSphereComponent = new CGameBoundSphereComponent();
		CGameBoundBoxComponent* boundBoxComponent = new CGameBoundBoxComponent();
		CMeshRendererComponent* meshRendererComponent = new CMeshRendererComponent();
		CSkeletonMeshComponent* meshComponent = new CSkeletonMeshComponent();
		CSkeletonComponent* skeletonComponent = new CSkeletonComponent(skeletonMeshPath);
		CSkeletonAnimationComponent* skeletonAnimationComponent = new CSkeletonAnimationComponent();
		//this->AddComponent(boundSphereComponent);
		this->AddComponent(boundBoxComponent);
		this->AddComponent(meshRendererComponent);
		this->AddComponent(meshComponent);
		this->AddComponent(skeletonComponent);
		this->AddComponent(skeletonAnimationComponent);

		skeletonComponent->SetGameObject(this);
		skeletonAnimationComponent->SetGameObject(this);

		meshRendererComponent->SetMeshComponent(meshComponent);
		meshComponent->SetSkeletonComponent(skeletonComponent);
		skeletonAnimationComponent->SetSkeletonComponent(skeletonComponent);

		const CustomStruct::CRenderInputLayoutDesc* desc; UINT descNum;
		CustomStruct::CRenderInputLayoutDesc::GetEngineSkeletonMeshInputLayouts(desc, descNum);

		BOOL isOutputSkeleton = FALSE; UINT rootNode = 0u;
		std::vector<CustomStruct::CGameBoneNodeInfo> skeleton; std::vector<UINT> boneList;
		const CBaseMesh<UINT>* tempMesh = CMeshManager::LoadSkeletonMeshAsset(skeletonMeshPath, isOutputSkeleton, skeleton, boneList, rootNode, FALSE);
		meshComponent->SetMesh(tempMesh);
		skeletonComponent->SetSkeleton(skeleton, boneList, rootNode);

		CDefaultLitSkeletonMeshMaterial* material = meshRendererComponent->AddMaterial<CDefaultLitSkeletonMeshMaterial>();

		CustomType::Vector3 boundMin, boundMax;
		meshComponent->GetMinMaxBounding(boundMin, boundMax);

		skeletonAnimationComponent->AddAnimation("TestAnimation", CSkeletonAnimationManager::LoadSkeletonAnimationAsset(skeletonAnimationPath, nullptr));
		skeletonAnimationComponent->SetCurrentAnimation("TestAnimation");

		auto errorMinMax = [](CustomType::Vector3& v0, CustomType::Vector3& v1, const FLOAT& error) {
			FLOAT errorV[3] = { v1.X() - v0.X(), v1.Y() - v0.Y(), v1.Z() - v0.Z() };
			for (UINT i = 0u; i < 3u; i++)
			{
				errorV[i] = (errorV[i] < error) ? error : 0.f;
			}
			v0 = CustomType::Vector3(v0.X() - errorV[0], v0.Y() - errorV[1], v0.Z() - errorV[2]);
			v1 = CustomType::Vector3(v1.X() + errorV[0], v1.Y() + errorV[1], v1.Z() + errorV[2]); };

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
			boundBoxComponent->SetLocalAnchorExtent(anchor, extent * 0.5f);
			//boundSphereComponent->SetLocalAnchorRadius(anchor, extent.Length() * 0.5f);
		}

#ifdef _DEVELOPMENT_EDITOR
		this->m_MeshRendererComponent		= meshRendererComponent;
		this->m_MeshComponent				= meshComponent;
		this->m_SkeletonComponent			= skeletonComponent;
		this->m_SkeletonAnimationComponent	= skeletonAnimationComponent;
		strncpy_s(this->m_SaveMaterialPath, 512, "./Engine/Assets/Development/MaterialConfigs/", 45);
		strncpy_s(this->m_LoadMaterialPath, 512, "./Engine/Assets/Development/MaterialConfigs/", 45);
#endif
	}
}
CSceneSkeletonMeshObject::~CSceneSkeletonMeshObject()
{
}
void CSceneSkeletonMeshObject::Init()
{
	CGameObject::Init();
}
void CSceneSkeletonMeshObject::Uninit()
{
	CGameObject::Uninit();
}
void CSceneSkeletonMeshObject::Update()
{
	CGameObject::Update();
}
void CSceneSkeletonMeshObject::FixedUpdate()
{
	CGameObject::FixedUpdate();
}
#ifdef _DEVELOPMENT_EDITOR
CMeshRendererComponent* CSceneSkeletonMeshObject::GetMeshRendererComponentNotConst()
{
	return (this->m_MeshRendererComponent);
}
CMeshComponent* CSceneSkeletonMeshObject::GetMeshComponentNotConst()
{
	return (this->m_MeshComponent);
}
CSkeletonComponent* CSceneSkeletonMeshObject::GetSkeletonComponentNotConst()
{
	return (this->m_SkeletonComponent);
}
CSkeletonAnimationComponent* CSceneSkeletonMeshObject::GetSkeletonAnimationComponentNotConst()
{
	return (this->m_SkeletonAnimationComponent);
}
void CSceneSkeletonMeshObject::SelectedEditorUpdate()
{
	//Editor body
	{
		ImGui::Begin("SceneSkeletonMeshObject", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		CGameObject::SelectedEditorUpdate();
		ImGui::End();
	}
}
#endif