#include "../Headers/CScene.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureType.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/RenderBase/Headers/CMaterialBase.h"
#include "../../EngineRender/RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../EngineRender/RenderMaterials/Headers/CClearCoatMaterial.h"
#include "../../EngineRender/RenderMaterials/Headers/CClothMaterial.h"
#include "../Headers/CCamera.h"
#include "../Headers/CLightType.h"
#include "../Headers/CScreenPolygon2D.h"
#include "../Headers/CPlane.h"
#include "../Headers/CCube.h"

#include "../../Development/Headers/CSceneGameObject.h"

CScene::CScene()
{
	this->m_MainCamera = NULL;
#ifdef _DEVELOPMENT_EDITOR
	this->m_SelectedObject = NULL;
#endif
}
CScene::~CScene()
{
	if (this->m_MainCamera)
	{
		this->m_MainCamera->Uninit();
		delete (this->m_MainCamera);
		this->m_MainCamera = NULL;
	}
	if (this->m_Lights.size() > 0u)
	{
		for (const auto& object : this->m_Lights)
		{
			if (object.second)
			{
				object.second->Uninit();
				delete (object.second);
			}
		}
		this->m_Lights.clear();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		if (this->m_GameObjects[i].size() > 0u)
		{
			for (const auto& object : this->m_GameObjects[i])
			{
				if (object.second)
				{
					object.second->Uninit();
					delete (object.second);
				}
			}
			this->m_GameObjects[i].clear();
		}
	}
}
void CScene::Init()
{
	{
		CCamera* mainCamera = this->AddCamera<CCamera>();
		CLightDirectional* mainLight = this->AddLight<CLightDirectional>();
		CPlane* terrainPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_TERRAIN);

		mainCamera->SetWorldPosition(CustomType::Vector3(0.f, 600.f, -950.f));
		terrainPlane->SetMeshInfo(2000.f, 8, 4.f);
	}

#ifdef _DEVELOPMENT_EDITOR
	{
		BOOL showDebugFabric = FALSE;

		if (showDebugFabric)
		{
			BOOL useModelFromFile = FALSE;
			std::string modelFilePath = "./Engine/Assets/EngineModels/SceneModels/ClothOnly/Cloth.obj";
			CMeshManager::CEngineBaseModelType defaultModelType = CMeshManager::CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE;

			const static CustomStruct::CRenderInputLayoutDesc testMeshInputLayout[4u] = {
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

			{
				CSceneGameObject* testObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
				testObject->SetWorldPosition(CustomType::Vector3(-150.f, 200.f, -150.f));
				testObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
				CMeshComponent* meshComponent = testObject->GetMeshComponentNotConst();
				CMeshRendererComponent* meshRendererComponent = testObject->GetMeshRendererComponentNotConst();
				if (useModelFromFile)
				{
					meshComponent->SetMesh(CMeshManager::LoadMeshFromFile(modelFilePath, testMeshInputLayout, 4u, FALSE));
				}
				else
				{
					meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(defaultModelType, testMeshInputLayout, 4u, FALSE));
				}
				CDefaultLitMaterial* material = meshRendererComponent->AddMaterial<CDefaultLitMaterial>(TRUE);
				material->SetNormalTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric003_1K/Fabric003_NRM_1K.tga"));
				material->SetAlbedoTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric003_1K/Fabric003_COL_1K.tga"));
				material->SetRoughnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric003_1K/Fabric003_GLOSS_1K.tga"));
				material->SetIsGlossyRoughness(TRUE);
				material->SetRoughness(0.752f);
			}

			{
				CSceneGameObject* testObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
				testObject->SetWorldPosition(CustomType::Vector3(-150.f, 200.f, 150));
				testObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
				CMeshComponent* meshComponent = testObject->GetMeshComponentNotConst();
				CMeshRendererComponent* meshRendererComponent = testObject->GetMeshRendererComponentNotConst();
				if (useModelFromFile)
				{
					meshComponent->SetMesh(CMeshManager::LoadMeshFromFile(modelFilePath, testMeshInputLayout, 4u, FALSE));
				}
				else
				{
					meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(defaultModelType, testMeshInputLayout, 4u, FALSE));
				}
				CClothMaterial* material = meshRendererComponent->AddMaterial<CClothMaterial>(TRUE);
				material->SetNormalTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric003_1K/Fabric003_NRM_1K.tga"));
				material->SetAlbedoTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric003_1K/Fabric003_COL_1K.tga"));
				material->SetRoughnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric003_1K/Fabric003_GLOSS_1K.tga"));
				material->SetIsGlossyRoughness(TRUE);
				material->SetRoughness(0.752f);
				material->SetSheenColor(CustomStruct::CColor(154.f / 255.f, 146.f / 255.f, 127.f / 255.f));
				material->SetSubsurfaceColor(CustomStruct::CColor(81.f / 255.f, 62.f / 255.f, 17.f / 255.f));
			}

			{
				CSceneGameObject* testObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
				testObject->SetWorldPosition(CustomType::Vector3(150.f, 200.f, -150.f));
				testObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
				CMeshComponent* meshComponent = testObject->GetMeshComponentNotConst();
				CMeshRendererComponent* meshRendererComponent = testObject->GetMeshRendererComponentNotConst();
				if (useModelFromFile)
				{
					meshComponent->SetMesh(CMeshManager::LoadMeshFromFile(modelFilePath, testMeshInputLayout, 4u, FALSE));
				}
				else
				{
					meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(defaultModelType, testMeshInputLayout, 4u, FALSE));
				}
				CDefaultLitMaterial* material = meshRendererComponent->AddMaterial<CDefaultLitMaterial>(TRUE);
				material->SetNormalTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_NRM_1K.tga"));
				material->SetAlbedoTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_COL_VAR1_1K.tga"));
				material->SetRoughnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_GLOSS_1K.tga"));
				material->SetReflectanceTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_REFL_1K.tga"));
				material->SetIsGlossyRoughness(TRUE);
				material->SetRoughness(0.699f);
				material->SetReflectance(1.f);
				material->SetBaseColor(CustomStruct::CColor(73.f / 255.f, 73.f / 255.f, 73.f / 255.f));
			}

			{
				CSceneGameObject* testObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
				testObject->SetWorldPosition(CustomType::Vector3(150.f, 200.f, 150));
				testObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
				CMeshComponent* meshComponent = testObject->GetMeshComponentNotConst();
				CMeshRendererComponent* meshRendererComponent = testObject->GetMeshRendererComponentNotConst();
				if (useModelFromFile)
				{
					meshComponent->SetMesh(CMeshManager::LoadMeshFromFile(modelFilePath, testMeshInputLayout, 4u, FALSE));
				}
				else
				{
					meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(defaultModelType, testMeshInputLayout, 4u, FALSE));
				}
				CClothMaterial* material = meshRendererComponent->AddMaterial<CClothMaterial>(TRUE);
				material->SetNormalTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_NRM_1K.tga"));
				material->SetAlbedoTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_COL_VAR1_1K.tga"));
				material->SetRoughnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_GLOSS_1K.tga"));
				material->SetSheenColorTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_COL_VAR2_1K.tga"));
				material->SetSubsurfaceTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric001_1K/Fabric001_COL_VAR2_1K.tga"));
				material->SetIsGlossyRoughness(TRUE);
				material->SetRoughness(0.699f);
				material->SetBaseColor(CustomStruct::CColor(73.f / 255.f, 73.f / 255.f, 73.f / 255.f));
				material->SetSheenColor(CustomStruct::CColor(66.f / 255.f, 66.f / 255.f, 66.f / 255.f));
				material->SetSubsurfaceColor(CustomStruct::CColor(200.f / 255.f, 200.f / 255.f, 200.f / 255.f));
			}

			{
				CSceneGameObject* testObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
				testObject->SetWorldPosition(CustomType::Vector3(450.f, 200.f, -150.f));
				testObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
				CMeshComponent* meshComponent = testObject->GetMeshComponentNotConst();
				CMeshRendererComponent* meshRendererComponent = testObject->GetMeshRendererComponentNotConst();
				if (useModelFromFile)
				{
					meshComponent->SetMesh(CMeshManager::LoadMeshFromFile(modelFilePath, testMeshInputLayout, 4u, FALSE));
				}
				else
				{
					meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(defaultModelType, testMeshInputLayout, 4u, FALSE));
				}
				CDefaultLitMaterial* material = meshRendererComponent->AddMaterial<CDefaultLitMaterial>(TRUE);
				material->SetNormalTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric002_1K/Fabric002_1K_NormalDX.tga"));
				material->SetAlbedoTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric002_1K/Fabric002_1K_Color.tga"));
				material->SetRoughnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric002_1K/Fabric002_1K_Roughness.tga"));
				material->SetRoughness(0.329f);
			}

			{
				CSceneGameObject* testObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
				testObject->SetWorldPosition(CustomType::Vector3(450.f, 200.f, 150.f));
				testObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
				CMeshComponent* meshComponent = testObject->GetMeshComponentNotConst();
				CMeshRendererComponent* meshRendererComponent = testObject->GetMeshRendererComponentNotConst();
				if (useModelFromFile)
				{
					meshComponent->SetMesh(CMeshManager::LoadMeshFromFile(modelFilePath, testMeshInputLayout, 4u, FALSE));
				}
				else
				{
					meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(defaultModelType, testMeshInputLayout, 4u, FALSE));
				}
				CClothMaterial* material = meshRendererComponent->AddMaterial<CClothMaterial>(TRUE);
				material->SetNormalTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric002_1K/Fabric002_1K_NormalDX.tga"));
				material->SetAlbedoTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric002_1K/Fabric002_1K_Color.tga"));
				material->SetRoughnessTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric002_1K/Fabric002_1K_Roughness.tga"));
				material->SetSheenColorTexture(CTextureManager::LoadTexture2D("./Engine/Assets/EngineTextures/Resources/Fabric002_1K/Fabric002_1K_Color.tga"));
				material->SetRoughness(0.329f);
				material->SetSheenColor(CustomStruct::CColor(94.f / 255.f, 94.f / 255.f, 94.f / 255.f));
				material->SetSubsurfaceColor(CustomStruct::CColor(193.f / 255.f, 193.f / 255.f, 193.f / 255.f));
			}
		}
	}
#endif

#ifdef _DEVELOPMENT_EDITOR
	{
		CSceneGameObject* sceneGameObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
		sceneGameObject->SetWorldPosition(CustomType::Vector3(0.f, 600.f, -600.f));
		sceneGameObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
		this->m_SelectedObject = sceneGameObject;
	}
#endif
}
void CScene::Uninit()
{
	if (this->m_MainCamera)
	{
		this->m_MainCamera->Uninit();
		delete (this->m_MainCamera);
		this->m_MainCamera = NULL;
	}
	if (this->m_Lights.size() > 0u)
	{
		for (const auto& object : this->m_Lights)
		{
			if (object.second)
			{
				object.second->Uninit();
				delete (object.second);
			}
		}
		this->m_Lights.clear();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		if (this->m_GameObjects[i].size() > 0u)
		{
			for (const auto& object : this->m_GameObjects[i])
			{
				if (object.second)
				{
					object.second->Uninit();
					delete (object.second);
				}
			}
			this->m_GameObjects[i].clear();
		}
	}
}
void CScene::Update()
{
#ifdef _DEVELOPMENT_EDITOR
	{
		if (this->m_SelectedObject != NULL)
		{
			this->m_SelectedObject->SelectedEditorUpdate();
		}
		if (this->m_Lights.size() > 0u)
		{
			for (const auto& object : this->m_Lights)
			{
				if (object.second)
				{
					object.second->SelectedEditorUpdate();
				}
			}
		}
	}

	{
		CustomType::Vector3 cameraPos(this->m_MainCamera->GetWorldPosition());
		CustomType::Vector3 cameraDir(this->m_MainCamera->GetForwardVector());
		std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
		ImGui::Begin("Scene Manager");
		ImGui::Text("Mouse position : x = %d, y = %d.", mousePos.first, mousePos.second);
		ImGui::Text("Camera position :\nx = %f\ny = %f\nz = %f", cameraPos.X(), cameraPos.Y(), cameraPos.Z());
		ImGui::Text("Camera direction :\nx = %f\ny = %f\nz = %f", cameraDir.X(), cameraDir.Y(), cameraDir.Z());
		UINT index = 0u;
		for (const auto& object : this->m_Lights)
		{
			CustomType::Vector3 lightPos(object.second->GetWorldPosition());
			CustomType::Vector3 lightDir(object.second->GetForwardVector());
			ImGui::Text("Light index = %d", index);
			ImGui::Text("Light position :\nx = %f\ny = %f\nz = %f", lightPos.X(), lightPos.Y(), lightPos.Z());
			ImGui::Text("Light direction :\nx = %f\ny = %f\nz = %f", lightDir.X(), lightDir.Y(), lightDir.Z());
			index++;
		}
		ImGui::End();
	}
#endif

	this->m_MainCamera->Update();

	for (const auto& object : this->m_Lights)
	{
		object.second->Update();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObjects[i])
		{
			object.second->Update();
		}
	}
}
void CScene::FixedUpdate()
{
	this->m_MainCamera->FixedUpdate();

	for (const auto& object : this->m_Lights)
	{
		object.second->FixedUpdate();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObjects[i])
		{
			object.second->FixedUpdate();
		}
	}
}