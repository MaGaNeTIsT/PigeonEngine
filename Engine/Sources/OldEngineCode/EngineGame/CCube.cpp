#include "../Headers/CCube.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureType.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../Headers/CComponent.h"
#include "../Headers/CGameBoundComponent.h"
#include "../Headers/CMeshComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../EngineGame/Headers/CCamera.h"
#include "../../EngineGame/Headers/CScene.h"

CCube::CCube(const BOOL& active, const class CScene* scene) : CGameObject(active, scene)
{
	this->AddNewTransform();
	{
		//CGameBoundSphereComponent* boundSphereComponent = new CGameBoundSphereComponent();
		//this->AddComponent(boundSphereComponent);
		CGameBoundBoxComponent* boundBoxComponent = new CGameBoundBoxComponent();
		this->AddComponent(boundBoxComponent);

		CMeshRendererComponent* meshRendererComponent = new CMeshRendererComponent();
		CMeshComponent* meshComponent = new CMeshComponent();
		this->AddComponent(meshRendererComponent);
		this->AddComponent(meshComponent);

		CustomStruct::CRenderInputLayoutDesc desc[4u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		meshComponent->SetMesh(CMeshManager::LoadCubeMesh(desc, 4u));
		meshRendererComponent->SetMeshComponent(meshComponent);
		meshRendererComponent->AddMaterial<CDefaultLitMaterial>();

		CustomType::Vector3 boundMin, boundMax;
		meshComponent->GetMinMaxBounding(boundMin, boundMax);

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
	}
}
CCube::~CCube()
{
}
void CCube::Update()
{
	CGameObject::Update();
}