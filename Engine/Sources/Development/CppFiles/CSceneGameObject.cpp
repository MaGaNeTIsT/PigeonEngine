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
		meshRendererComponent->InitShadersAndInputLayout(ENGINE_SHADER_DEFAULT_VS, ENGINE_SHADER_GBUFFER_WRITE_PS, desc, 4u, CMeshRendererComponent::RenderTypeEnum::RENDER_TYPE_OPAQUE);
		meshRendererComponent->SetMaterialTextures(NULL, NULL, NULL);
		meshRendererComponent->SetMeshComponent(meshComponent);
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
				}
			}
			else
			{

			}
		}
	}
}
#endif