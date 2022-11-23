#include "../Headers/CSceneGameObject.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshComponent.h"

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
		meshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(CMeshManager::CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE, desc, 4u, FALSE));
		//meshRendererComponent->InitShadersAndInputLayout(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS, desc, 4u, CMeshRendererComponent::RenderTypeEnum::RENDER_TYPE_OPAQUE);
		meshRendererComponent->InitShadersAndInputLayout(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_DEFAULT_PS, desc, 4u, CMeshRendererComponent::RenderTypeEnum::RENDER_TYPE_OPAQUE_FORWARD);
		meshRendererComponent->SetMaterialTextures(NULL, NULL, NULL);
		meshRendererComponent->SetMeshComponent(meshComponent);
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
#if _DEVELOPMENT_EDITOR
		this->m_MeshRendererComponent = meshRendererComponent;
		this->m_MeshComponent = meshComponent;
		this->m_CurrentMeshType = CMeshManager::CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE;
		this->m_PreviousMeshType = CMeshManager::CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE;
		this->m_LoadBaseModel = TRUE;
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
#if _DEVELOPMENT_EDITOR
void CSceneGameObject::SelectedEditorUpdate()
{
	bool loadBaseModel = this->m_LoadBaseModel;
	BOOL needLoadMesh = FALSE;

	//Editor body
	{
		ImGui::Begin("SceneGameObject");
		CGameObject::SelectedEditorUpdate();
		if (ImGui::TreeNode("EditorMenu"))
		{
			if (this->m_MeshComponent != NULL)
			{
				ImGui::Checkbox("IsLoadBase", &loadBaseModel);
				if (ImGui::TreeNode("EngineBaseModel"))
				{
					if (loadBaseModel)
					{
						ImGui::RadioButton("Normal cube", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_NORMAL_CUBE);
						ImGui::RadioButton("Rounded cube", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_ROUNDED_CUBE);
						ImGui::RadioButton("Smooth sphere", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_SMOOTH_SPHERE);
						ImGui::RadioButton("UV sphere", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_UV_SPHERE);
						ImGui::RadioButton("Torus", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_TORUS);
						ImGui::RadioButton("Prism", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_PRISM);
						ImGui::RadioButton("Material sphere", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_MATERIAL_SPHERE);
						ImGui::RadioButton("Monkey", &(this->m_CurrentMeshType), CMeshManager::CEngineBaseModelType::ENGINE_BASE_MONKEY);
						if (ImGui::Button("Apply"))
						{
							needLoadMesh = TRUE;
						}
					}
					else
					{
						if (ImGui::Button("Apply"))
						{

						}
					}
					ImGui::TreePop();
				}
			}
			if (this->m_MeshRendererComponent != NULL)
			{

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
			if (loadBaseModel)
			{
				if (this->m_CurrentMeshType > 0 && this->m_CurrentMeshType < CMeshManager::CEngineBaseModelType::ENGINE_BASE_COUNT && this->m_PreviousMeshType != this->m_CurrentMeshType && needLoadMesh)
				{
					this->m_PreviousMeshType = this->m_CurrentMeshType;
					CustomStruct::CRenderInputLayoutDesc desc[4u] = {
						CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
						CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
						CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
						CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
					this->m_MeshComponent->SetMesh(CMeshManager::LoadEngineBaseModel(static_cast<CMeshManager::CEngineBaseModelType>(this->m_CurrentMeshType), desc, 4u, FALSE));
					needResetBound = TRUE;
				}
			}
			else
			{

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
	}
}
#endif