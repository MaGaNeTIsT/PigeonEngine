#include "../../../../Entry/EngineMain.h"
#include "../Headers/CCube.h"
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
#include "../../EngineRender/RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../EngineGame/Headers/CCamera.h"
#include "../../EngineGame/Headers/CScene.h"

CCube::CCube()
{
	this->AddNewTransform();
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
		meshComponent->SetMesh(CMeshManager::LoadCubeMesh(desc, 4u));
		meshRendererComponent->SetMeshComponent(meshComponent);
		meshRendererComponent->AddMaterial<CDefaultLitMaterial>();
		{
			CustomType::Vector3 boundMin, boundMax;
			meshComponent->GetMinMaxBounding(boundMin, boundMax);
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
	//CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Cloud001.tga");
}
CCube::~CCube()
{
}
void CCube::Update()
{
	CGameObject::Update();
	//FLOAT pos[3] = { this->GetPosition().X(), this->GetPosition().Y(), this->GetPosition().Z() };
	//FLOAT scale[3] = { this->GetScale().X(), this->GetScale().Y(), this->GetScale().Z() };
	//ImGui::Begin("Cube properties");
	//ImGui::InputFloat3(" Position", pos);
	//ImGui::InputFloat3(" Scale", scale);
	//ImGui::End();
	//this->SetPosition(CustomType::Vector3(pos[0], pos[1], pos[2]));
	//this->SetScale(CustomType::Vector3(scale[0], scale[1], scale[2]));
}