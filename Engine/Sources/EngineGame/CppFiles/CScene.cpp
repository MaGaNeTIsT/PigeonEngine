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
#include "../../Development/Headers/CReadWriteFile.h"
#include "../../../EngineThirdParty/Cassimp/Headers/CassimpManager.h"

CScene::CScene()
{
	this->m_MainCamera = nullptr;
#ifdef _DEVELOPMENT_EDITOR
	this->m_ShowSceneInfo = FALSE;
	this->m_ShowCameraInfo = FALSE;
	this->m_ShowLightInfo = TRUE;
	this->m_ShowSelectObjectInfo = TRUE;
	this->m_SelectedObject = nullptr;
#endif
}
CScene::~CScene()
{
	if (this->m_MainCamera)
	{
		this->m_MainCamera->Uninit();
		delete (this->m_MainCamera);
		this->m_MainCamera = nullptr;
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
		CCamera* mainCamera = this->AddMainCamera<CCamera>();
		CLightDirectional* mainLight = this->AddLight<CLightDirectional>();
		CPlane* terrainPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_TERRAIN);

		mainCamera->SetWorldPosition(CustomType::Vector3(0.f, 600.f, -950.f));
		terrainPlane->SetMeshInfo(2000.f, 8, 4.f);
	}

	{
		CassimpManager::ReadSkeletonBoneFile("E:/Projects/Model/Misaki/Misaki_SchoolUniform.fbx");
	}
#ifdef _DEVELOPMENT_EDITOR
	{
		this->m_ShowSceneInfo = FALSE;
		this->m_ShowCameraInfo = FALSE;
		this->m_ShowLightInfo = TRUE;
		this->m_ShowSelectObjectInfo = TRUE;
	}

	{
		CSceneGameObject* sceneGameObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
		sceneGameObject->SetWorldPosition(CustomType::Vector3(0.f, 600.f, -600.f));
		sceneGameObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
		this->m_SelectedObject = sceneGameObject;
	}

	{
		std::string materialConfigPath = "./Engine/Assets/Development/MaterialConfigs/";
		std::string materialTypeName = "mat_tex_cfg";
		BOOL showDebugFabric = TRUE;
		BOOL useModelFromFile = FALSE;
		FLOAT modelY = 200.f;
		FLOAT modelOffsetX = 300.f;
		FLOAT modelOffsetZ = 300.f;
		FLOAT modelScale = 100.f;

		if (showDebugFabric)
		{
			std::string modelFilePath = "./Engine/Assets/EngineModels/SceneModels/ClothOnly/Cloth.obj";
			CMeshManager::CEngineBaseModelType defaultModelType = CMeshManager::CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE;

			const static CustomStruct::CRenderInputLayoutDesc testMeshInputLayout[4u] = {
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

			std::vector<std::string> fileNames;
			INT fileNum = CTempFileHelper::CountFileNumberInFolder(materialConfigPath, materialTypeName, fileNames);
			if (fileNum > 0)
			{
				CReadWriteMaterialParamsFile materialFileReadWrite;

				INT objNum = (CustomType::CMath::Log2Ceil(fileNum));
				FLOAT maxX = ((objNum - 1.f) * 0.5f) * modelOffsetX;
				FLOAT minX = -maxX;
				FLOAT maxZ = ((objNum - 1.f) * 0.5f) * modelOffsetZ;
				FLOAT minZ = -maxZ;

				for (INT i = 0; i < fileNum; i++)
				{
					FLOAT z = static_cast<FLOAT>(i / (objNum));
					FLOAT x = static_cast<FLOAT>(i % (objNum));
					if (objNum > 1)
					{
						z /= static_cast<FLOAT>(objNum - 1);
						x /= static_cast<FLOAT>(objNum - 1);
					}

					CSceneGameObject* testObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);
					testObject->SetWorldPosition(CustomType::Vector3(x * maxX + (1.f - x) * minX, modelY, z * maxZ + (1.f - z) * minZ));
					testObject->SetWorldScale(CustomType::Vector3(modelScale, modelScale, modelScale));
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

					materialFileReadWrite.LoadMaterialParams(materialConfigPath, fileNames[i], material);
				}
			}
		}
	}
#endif
}
void CScene::Uninit()
{
	if (this->m_MainCamera)
	{
		this->m_MainCamera->Uninit();
		delete (this->m_MainCamera);
		this->m_MainCamera = nullptr;
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
#ifdef _DEVELOPMENT_EDITOR
void CScene::EditorUpdate()
{
	bool showSceneInfo = this->m_ShowSceneInfo;
	bool showCameraInfo = this->m_ShowCameraInfo;
	bool showLightInfo = this->m_ShowLightInfo;
	bool showSelectObjectInfo = this->m_ShowSelectObjectInfo;

	ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
	if (ImGui::TreeNode("CurrentSceneInfo"))
	{
		ImGui::Checkbox("ShowSelectObjectInfo", &showSelectObjectInfo);
		ImGui::Checkbox("ShowSceneInfo", &showSceneInfo);
		ImGui::Checkbox("ShowCameraInfo", &showCameraInfo);
		ImGui::Checkbox("ShowLightInfo", &showLightInfo);
		ImGui::TreePop();
	}
	if (showSceneInfo)
	{
		ImGui::Begin("CurrentSceneInfo");

		{
			std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
			ImGui::Text("Mouse position : x = %d, y = %d.", mousePos.first, mousePos.second);
			ImGui::Text("Mouse focus : any window = %d.", (ImGui::IsWindowFocused(ImGuiFocusedFlags_::ImGuiFocusedFlags_AnyWindow) ? 1 : 0));
			ImGui::Text("Mouse press : left = %d right = %d.", CInput::Controller.IsLeftMouseButtonDown(), CInput::Controller.IsRightMouseButtonDown());
		}

		{
			CCamera* camera = this->GetMainCamera<CCamera>();
			if (camera != nullptr)
			{
				CustomType::Vector3 cameraPos(camera->GetWorldPosition());
				CustomType::Vector3 cameraDir(camera->GetForwardVector());
				ImGui::Text("Camera position :\nx = %f\ny = %f\nz = %f", cameraPos.X(), cameraPos.Y(), cameraPos.Z());
				ImGui::Text("Camera direction :\nx = %f\ny = %f\nz = %f", cameraDir.X(), cameraDir.Y(), cameraDir.Z());
			}
		}

		{
			UINT index = 0u;
			for (const auto& object : this->m_Lights)
			{
				CGameObjectTransformBase* obj = reinterpret_cast<CGameObjectTransformBase*>(object.second);
				if (obj != nullptr)
				{
					CustomType::Vector3 lightPos(obj->GetWorldPosition());
					CustomType::Vector3 lightDir(obj->GetForwardVector());
					ImGui::Text("Light index = %d", index);
					ImGui::Text("Light position :\nx = %f\ny = %f\nz = %f", lightPos.X(), lightPos.Y(), lightPos.Z());
					ImGui::Text("Light direction :\nx = %f\ny = %f\nz = %f", lightDir.X(), lightDir.Y(), lightDir.Z());
					index++;
				}
			}
		}

		ImGui::End();
	}
	if (showCameraInfo && this->m_MainCamera)
	{
		this->m_MainCamera->SelectedEditorUpdate();
	}
	if (showLightInfo && this->m_Lights.size() > 0u)
	{
		for (const auto& object : this->m_Lights)
		{
			if (object.second)
			{
				object.second->SelectedEditorUpdate();
			}
		}
	}
	if (showSelectObjectInfo)
	{
		std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
		this->OnClickReselectSceneObjectWithBound(mousePos.first, mousePos.second);

		if (this->m_SelectedObject != nullptr)
		{
			this->m_SelectedObject->SelectedEditorUpdate();
		}
	}

	this->m_ShowSceneInfo = showSceneInfo;
	this->m_ShowCameraInfo = showCameraInfo;
	this->m_ShowLightInfo = showLightInfo;
	this->m_ShowSelectObjectInfo = showSelectObjectInfo;
}
void CScene::OnClickReselectSceneObjectWithBound(const INT& mouseX, const INT& mouseY)
{
	if (this->m_MainCamera == nullptr)
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
		if (obj.second != nullptr)
		{
			CGameObject* gameObjectPtr = reinterpret_cast<CGameObject*>(obj.second);
			if (gameObjectPtr != nullptr && gameObjectPtr->HasGameBoundComponent())
			{
				CGameBoundBaseComponent* boundComponent = gameObjectPtr->GetGameBoundComponent<CGameBoundBaseComponent>();
				if (boundComponent == nullptr)
				{
					continue;
				}
				CustomType::Vector2	tempScreenCoord; FLOAT tempViewDepth;
				if (!boundComponent->SelectedInEditorScreen(mousePos, camera, frustumInfo, tempScreenCoord, tempViewDepth))
				{
					continue;
				}
				selectedObjects.push_back(std::pair<CGameObject*, Screen2DObject>(gameObjectPtr, Screen2DObject(tempScreenCoord, tempViewDepth)));
			}
		}
	}

	if (selectedObjects.size() > 0)
	{
		const static FLOAT _ErrorSq = 100.f;
		std::vector<std::pair<FLOAT, CGameObject*>> depthList;
		CGameObject* tempSelectedObject = selectedObjects[0].first;
		FLOAT depth = selectedObjects[0].second.ViewDepth;
		FLOAT minDisSq = CustomType::Vector2::DistanceSquare(mousePos, selectedObjects[0].second.ScreenCoord);
		for (size_t i = 1; i < selectedObjects.size(); i++)
		{
			CGameObject*& obj = selectedObjects[i].first;
			Screen2DObject& screen = selectedObjects[i].second;

			FLOAT tempMinDisSq = CustomType::Vector2::DistanceSquare(mousePos, screen.ScreenCoord);
			if (tempMinDisSq < (minDisSq - _ErrorSq))
			{
				tempSelectedObject = obj;
				depth = screen.ViewDepth;
				minDisSq = tempMinDisSq;
				if (depthList.size() > 0)
				{
					depthList.clear();
				}
			}
			else if (CustomType::CMath::Abs(tempMinDisSq - minDisSq) < _ErrorSq)
			{
				depthList.push_back(std::pair<FLOAT, CGameObject*>(depth, tempSelectedObject));
				depthList.push_back(std::pair<FLOAT, CGameObject*>(screen.ViewDepth, obj));
				if (tempMinDisSq < tempMinDisSq)
				{
					tempSelectedObject = obj;
					depth = screen.ViewDepth;
					minDisSq = tempMinDisSq;
				}
			}
		}
		if (depthList.size() > 1)
		{
			tempSelectedObject = depthList[0].second;
			depth = depthList[0].first;
			for (size_t i = 1; i < depthList.size(); i++)
			{
				if (depthList[i].first < depth)
				{
					tempSelectedObject = depthList[i].second;
				}
			}
		}
		this->m_SelectedObject = tempSelectedObject;
	}
}
#endif