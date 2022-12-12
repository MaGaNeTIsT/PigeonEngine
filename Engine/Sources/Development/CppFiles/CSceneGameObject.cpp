#include "../Headers/CSceneGameObject.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"
#include "../../EngineRender/RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../EngineRender/RenderMaterials/Headers/CClearCoatMaterial.h"
#include "../../EngineRender/RenderMaterials/Headers/CClothMaterial.h"

CSceneGameObject::CSceneGameObject()
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
		meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(CMeshManager::CEngineBaseModelType::ENGINE_BASE_MATERIAL_SPHERE, desc, 4u, FALSE));
		meshRendererComponent->SetMeshComponent(meshComponent);
		CDefaultLitMaterial* material = meshRendererComponent->AddMaterial<CDefaultLitMaterial>();
		{
			CustomType::Vector3 boundMin, boundMax;
			meshComponent->GetMinMaxBounding(boundMin, boundMax);
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
#ifdef _DEVELOPMENT_EDITOR
		this->m_MeshRendererComponent	= meshRendererComponent;
		this->m_MeshComponent			= meshComponent;
		this->m_CurrentMeshType			= CMeshManager::CEngineBaseModelType::ENGINE_BASE_MATERIAL_SPHERE;
		this->m_PreviousMeshType		= CMeshManager::CEngineBaseModelType::ENGINE_BASE_MATERIAL_SPHERE;
		this->m_CurrentMaterialType		= DefaultMaterialType::DefaultMaterialType_DefaultLit;
		this->m_PreviousMaterialType	= DefaultMaterialType::DefaultMaterialType_DefaultLit;
		this->m_LoadBaseModel			= TRUE;
		this->m_LoadCustomModelPath[0] = 'N'; this->m_LoadCustomModelPath[1] = 'U'; this->m_LoadCustomModelPath[2] = 'L'; this->m_LoadCustomModelPath[3] = 'L';
		this->m_LoadCustomModelPath[4] = '\0';
#endif
	}
}
CSceneGameObject::~CSceneGameObject()
{
}
void CSceneGameObject::Init()
{
	CGameObject::Init();
}
void CSceneGameObject::Uninit()
{
	CGameObject::Uninit();
}
void CSceneGameObject::Update()
{
	CGameObject::Update();
}
void CSceneGameObject::FixedUpdate()
{
	CGameObject::FixedUpdate();
}
#ifdef _DEVELOPMENT_EDITOR
CMeshRendererComponent* CSceneGameObject::GetMeshRendererComponentNotConst()
{
	return (this->m_MeshRendererComponent);
}
CMeshComponent* CSceneGameObject::GetMeshComponentNotConst()
{
	return (this->m_MeshComponent);
}
void CSceneGameObject::SelectedEditorUpdate()
{
	bool loadBaseModel = this->m_LoadBaseModel;
	BOOL needLoadMesh = FALSE;
	BOOL needLoadMaterial = FALSE;

	//Editor body
	{
		ImGui::Begin("SceneGameObject", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		CGameObject::SelectedEditorUpdate();
		ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
		if (ImGui::TreeNode("EditorMenu"))
		{
			if (this->m_MeshComponent != NULL)
			{
				ImGui::Checkbox("IsLoadBase", &loadBaseModel);
				ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
				if (ImGui::TreeNode("EditMeshComponent"))
				{
					if (loadBaseModel)
					{
						std::map<INT, std::string> baseEngineMeshItems = {
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_NORMAL_CUBE, "Normal cube" },
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_ROUNDED_CUBE, "Rounded cube" },
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE, "Smooth sphere" },
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_UV_SPHERE, "UV sphere" },
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_TORUS, "Torus" },
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_PRISM, "Prism" },
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_MATERIAL_SPHERE, "Material sphere" },
							{ CMeshManager::CEngineBaseModelType::ENGINE_BASE_MONKEY, "Monkey" } };
						auto meshCombo = [&baseEngineMeshItems](const std::string& name, INT& select) {
							if (ImGui::BeginCombo(name.c_str(), baseEngineMeshItems[select].c_str()))
							{
								for (const auto& textureItem : baseEngineMeshItems)
								{
									BOOL selected = (select == textureItem.first);
									if (ImGui::Selectable(textureItem.second.c_str(), &selected))
									{
										select = textureItem.first;
									}
									if (selected)
									{
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}};
						meshCombo("MeshSelect", this->m_CurrentMeshType);
						if (ImGui::Button("ApplyMeshSelect"))
						{
							needLoadMesh = TRUE;
						}
					}
					else
					{
						ImGui::InputText("InputMeshPath", this->m_LoadCustomModelPath, 512, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue);
						if (ImGui::Button("ApplyMeshInput"))
						{
							needLoadMesh = TRUE;
						}
					}
					ImGui::TreePop();
				}
			}
			if (this->m_MeshRendererComponent != NULL)
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
				if (ImGui::TreeNode("EditMeshRendererComponent"))
				{
					std::string materialName = "NULL";
					if (this->m_MeshRendererComponent->HasMaterial())
					{
						materialName = this->m_MeshRendererComponent->GetMaterial()->GetName();
					}
					ImGui::Text("CurrentMaterial : %s", materialName.c_str());
					std::map<INT, std::string> baseEngineMaterialItems = {
						{ DefaultMaterialType::DefaultMaterialType_DefaultLit, "DefaultLitMaterial" },
						{ DefaultMaterialType::DefaultMaterialType_Anisotropic, "AnisotropicMaterial" },
						{ DefaultMaterialType::DefaultMaterialType_ClearCoat, "ClearCoatMaterial" },
						{ DefaultMaterialType::DefaultMaterialType_Cloth, "ClothMaterial" },
						{ DefaultMaterialType::DefaultMaterialType_ClothAnisotropic, "ClothAnisotropicMaterial" } };
					auto materialCombo = [&baseEngineMaterialItems](const std::string& name, INT& select) {
						if (ImGui::BeginCombo(name.c_str(), baseEngineMaterialItems[select].c_str()))
						{
							for (const auto& materialItem : baseEngineMaterialItems)
							{
								BOOL selected = (select == materialItem.first);
								if (ImGui::Selectable(materialItem.second.c_str(), &selected))
								{
									select = materialItem.first;
								}
								if (selected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}};
					materialCombo("MaterialSelect", this->m_CurrentMaterialType);
					if (ImGui::Button("ApplyMaterial"))
					{
						needLoadMaterial = TRUE;
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		ImGui::End();
	}

	//Run result for editor
	{
		this->m_LoadBaseModel = loadBaseModel;
		if (this->m_MeshComponent != NULL)
		{
			BOOL needResetBound = FALSE;
			CustomStruct::CRenderInputLayoutDesc desc[4u] = {
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
				CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
			if (loadBaseModel)
			{
				if (this->m_CurrentMeshType > 0 && this->m_CurrentMeshType < CMeshManager::CEngineBaseModelType::ENGINE_BASE_COUNT && this->m_PreviousMeshType != this->m_CurrentMeshType && needLoadMesh)
				{
					this->m_PreviousMeshType = this->m_CurrentMeshType;
					this->m_MeshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(static_cast<CMeshManager::CEngineBaseModelType>(this->m_CurrentMeshType), desc, 4u, FALSE));
					needResetBound = TRUE;
				}
			}
			else
			{
				if (needLoadMesh)
				{
					std::string loadCustomModelPath(this->m_LoadCustomModelPath);
					//this->m_MeshComponent->SetMesh(CMeshManager::LoadMeshFromFile(loadCustomModelPath, desc, 4u, FALSE));
					this->m_MeshComponent->SetMesh(CMeshManager::LoadDefaultMeshAsset(loadCustomModelPath, FALSE));
					needResetBound = TRUE;
				}
			}
			if (needResetBound)
			{
				CustomType::Vector3 boundMin, boundMax;
				this->m_MeshComponent->GetMinMaxBounding(boundMin, boundMax);
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
		if (this->m_MeshRendererComponent != NULL)
		{
			if (needLoadMaterial && this->m_CurrentMaterialType >= 0 && this->m_CurrentMaterialType < DefaultMaterialType_Count && this->m_CurrentMaterialType != this->m_PreviousMaterialType)
			{
				this->m_PreviousMaterialType = this->m_CurrentMaterialType;
				switch (this->m_CurrentMaterialType)
				{
				case DefaultMaterialType::DefaultMaterialType_DefaultLit:
					this->m_MeshRendererComponent->AddMaterial<CDefaultLitMaterial>(TRUE);
					break;
				case DefaultMaterialType::DefaultMaterialType_Anisotropic:
					this->m_MeshRendererComponent->AddMaterial<CAnisotropicMaterial>(TRUE);
					break;
				case DefaultMaterialType::DefaultMaterialType_ClearCoat:
					this->m_MeshRendererComponent->AddMaterial<CClearCoatMaterial>(TRUE);
					break;
				case DefaultMaterialType::DefaultMaterialType_Cloth:
					this->m_MeshRendererComponent->AddMaterial<CClothMaterial>(TRUE);
					break;
				case DefaultMaterialType::DefaultMaterialType_ClothAnisotropic:
					this->m_MeshRendererComponent->AddMaterial<CClothAnisotropicMaterial>(TRUE);
					break;
				}
			}
		}
	}
}
#endif