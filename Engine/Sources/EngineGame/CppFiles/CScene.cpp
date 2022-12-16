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
#include "../Headers/CGameBoundComponent.h"
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
#ifdef _DEVELOPMENT_EDITOR
void CScene::ReSelectSceneObject(const INT& mouseX, const INT& mouseY)
{
	if (this->m_MainCamera == NULL)
	{
		return;
	}
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_::ImGuiFocusedFlags_AnyWindow) || !CInput::Controller.IsLeftMouseButtonDown())
	{
		return;
	}

	struct Screen2DObject
	{
		Screen2DObject(const CustomType::Vector2& coord, const FLOAT& depth) { ScreenCoord = coord; ViewDepth = depth; }
		CustomType::Vector2	ScreenCoord;
		FLOAT				ViewDepth;
	};

	std::vector<std::pair<CGameObject*, Screen2DObject>> selectedObjects;
	CustomType::Vector2 mousePos(mouseX, mouseY);
	CCamera* camera = this->GetMainCamera<CCamera>();
	CustomStruct::CCullingFrustumInfo frustumInfo(camera->PrepareTempFrustumInfo());

	for (const auto& obj : this->m_AllObjectList)
	{
		if (obj.second != NULL && obj.second->HasGameBoundComponent())
		{
			CGameBoundBaseComponent* boundComponent = obj.second->GetGameBoundComponent<CGameBoundBaseComponent>();
			if (boundComponent == NULL)
			{
				continue;
			}
			CustomType::Vector2	tempScreenCoord; FLOAT tempViewDepth;
			if (!boundComponent->SelectedInEditorScreen(mousePos, camera, frustumInfo, tempScreenCoord, tempViewDepth))
			{
				continue;
			}
			selectedObjects.push_back(std::pair<CGameObject*, Screen2DObject>(obj.second, Screen2DObject(tempScreenCoord, tempViewDepth)));
		}
	}

	if (selectedObjects.size() > 0)
	{
		const static FLOAT _ErrorSq = 100.f;
		std::vector<std::pair<FLOAT, CGameObject*>> depthList;
		this->m_SelectedObject = selectedObjects[0].first;
		FLOAT depth = selectedObjects[0].second.ViewDepth;
		FLOAT minDisSq = CustomType::Vector2::DistanceSquare(mousePos, selectedObjects[0].second.ScreenCoord);
		for (size_t i = 1; i < selectedObjects.size(); i++)
		{
			CGameObject*& obj = selectedObjects[i].first;
			Screen2DObject& screen = selectedObjects[i].second;

			FLOAT tempMinDisSq = CustomType::Vector2::DistanceSquare(mousePos, screen.ScreenCoord);
			if (tempMinDisSq < (minDisSq - _ErrorSq))
			{
				this->m_SelectedObject = obj;
				depth = screen.ViewDepth;
				minDisSq = tempMinDisSq;
				if (depthList.size() > 0)
				{
					depthList.clear();
				}
			}
			else if (CustomType::CMath::Abs(tempMinDisSq - minDisSq) < _ErrorSq)
			{
				depthList.push_back(std::pair<FLOAT, CGameObject*>(depth, this->m_SelectedObject));
				depthList.push_back(std::pair<FLOAT, CGameObject*>(screen.ViewDepth, obj));
				if (tempMinDisSq < tempMinDisSq)
				{
					this->m_SelectedObject = obj;
					depth = screen.ViewDepth;
					minDisSq = tempMinDisSq;
				}
			}
		}
		if (depthList.size() > 1)
		{
			this->m_SelectedObject = depthList[0].second;
			depth = depthList[0].first;
			for (size_t i = 1; i < depthList.size(); i++)
			{
				if (depthList[i].first < depth)
				{
					this->m_SelectedObject = depthList[i].second;
				}
			}
		}
	}
}
#endif
void CScene::Init()
{
	{
		CReadMaterialParamsFile readMaterialFile;
		readMaterialFile.ReadFile("./Engine/Assets/Development/MaterialConfigs/test.mat_tex_cfg");
		int a = 0;
	}

	{
		CCamera* mainCamera = this->AddCamera<CCamera>();
		CLightDirectional* mainLight = this->AddLight<CLightDirectional>();
		CPlane* terrainPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_TERRAIN);

		mainCamera->SetWorldPosition(CustomType::Vector3(0.f, 600.f, -950.f));
		terrainPlane->SetMeshInfo(2000.f, 8, 4.f);
	}

#ifdef _DEVELOPMENT_EDITOR
	{
		BOOL showDebugFabric = TRUE;

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
		ImGui::Text("Mouse focus : any window = %d.", (ImGui::IsWindowFocused(ImGuiFocusedFlags_::ImGuiFocusedFlags_AnyWindow) ? 1 : 0));
		ImGui::Text("Mouse press : left = %d right = %d.", CInput::Controller.IsLeftMouseButtonDown(), CInput::Controller.IsRightMouseButtonDown());
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

#ifdef _DEVELOPMENT_EDITOR
	std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
	this->ReSelectSceneObject(mousePos.first, mousePos.second);
#endif
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