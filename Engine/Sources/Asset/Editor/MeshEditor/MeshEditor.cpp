#include "MeshEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <IO/FileHelper.h>
#include <BaseAsset.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <Base/DataStructure/Text/Path.h>

namespace PigeonEngine
{
	namespace
	{
		const CHAR* MeshTypeAsString(const EMeshType InMeshType)
		{
			switch (InMeshType)
			{
			case EMeshType::MESH_TYPE_STATIC:
				return "Static";
			case EMeshType::MESH_TYPE_SKIN:
				return "Skinned";
			default:
				return "Unknown";
			}
		}

		const CHAR* VertexLayoutTypeAsString(const UINT32 InPartType)
		{
			switch (static_cast<EVertexLayoutType>(InPartType))
			{
			case EVertexLayoutType::MESH_INDEX_FULL:
				return "Index32";
			case EVertexLayoutType::MESH_INDEX_HALF:
				return "Index16";
			case EVertexLayoutType::MESH_VERTEX:
				return "Position";
			case EVertexLayoutType::MESH_TEXTURECOORD:
				return "TexCoord";
			case EVertexLayoutType::MESH_NORMAL:
				return "Normal";
			case EVertexLayoutType::MESH_TANGENT:
				return "Tangent";
			case EVertexLayoutType::MESH_COLOR:
				return "Color";
			case EVertexLayoutType::MESH_BITANGENT:
				return "Bitangent";
			case EVertexLayoutType::MESH_SKIN:
				return "Skin";
			default:
				return "Unknown";
			}
		}

		EString NormalizeAssetDirectory(const EString& InAssetDirectory)
		{
			EString Result = InAssetDirectory.Replace("\\", "/");
			if ((Result.Length() > 0u) && (Result[Result.LastIndex()] != '/'))
			{
				Result += "/";
			}
			return Result;
		}

		BOOL8 ReadMeshTypeFromAssetFile(const EString& InAssetFullPath, EMeshType& OutMeshType)
		{
			void* HeaderData = nullptr;
			if (!(EFileHelper::ReadFirstNumberOfBytesInFile(InAssetFullPath, HeaderData, sizeof(UINT32) * 2u)) || (!HeaderData))
			{
				return FALSE;
			}

			const UINT32* HeaderWords = static_cast<const UINT32*>(HeaderData);
			const EAssetType AssetType = static_cast<EAssetType>(HeaderWords[0]);
			OutMeshType = static_cast<EMeshType>(HeaderWords[1]);
			delete[] static_cast<CHAR*>(HeaderData);
			HeaderData = nullptr;

			return (AssetType == EAssetType::ASSET_TYPE_MESH) && (OutMeshType < EMeshType::MESH_TYPE_COUNT);
		}

		BOOL8 ReadAssetTypeFromAssetFile(const EString& InAssetFullPath, EAssetType& OutAssetType)
		{
			void* HeaderData = nullptr;
			if (!(EFileHelper::ReadFirstNumberOfBytesInFile(InAssetFullPath, HeaderData, sizeof(UINT32))) || (!HeaderData))
			{
				return FALSE;
			}

			OutAssetType = static_cast<EAssetType>(*static_cast<const UINT32*>(HeaderData));
			delete[] static_cast<CHAR*>(HeaderData);
			return TRUE;
		}

		BOOL8 LoadSkeletonAssetFromFullPath(const EString& InAssetFullPath, const ESkeletonAsset*& OutSkeletonAsset)
		{
			OutSkeletonAsset = nullptr;
			const EString AssetName = EPath::GetFileNameWithoutExtension(InAssetFullPath);
			const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(InAssetFullPath));
			return (AssetName.Length() > 0u) && (AssetDirectory.Length() > 0u) && ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(AssetDirectory, AssetName, OutSkeletonAsset) && (!!OutSkeletonAsset);
		}
	}

	void EMeshEditor::EditorInit()
	{
		ResetLoadedAsset();
		m_OpenedAssetFullPath = EString();
		m_LastOpenError = EString();
		m_ShowEditor = FALSE;
		m_PreviewScene.Initialize("MeshEditorPreview", Vector3(32000.0f, 32000.0f, 32000.0f));
	}

	void EMeshEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(1320.0f, 820.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Mesh Editor", &m_ShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				OpenMeshAsset(m_OpenedAssetFullPath);
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No mesh asset selected.");

			if (m_LastOpenError.Length() > 0u)
			{
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastOpenError);
			}

			if (!m_MeshResource)
			{
				ImGui::Separator();
				ImGui::Text("No mesh asset selected.");
			}
			else
			{
				ImGui::Separator();

				const ImVec2 AvailableRegion = ImGui::GetContentRegionAvail();
				const FLOAT TopPanelHeight = EMath::Max(AvailableRegion.y * 0.54f, 320.0f);
				const FLOAT LeftPanelWidth = EMath::Max(AvailableRegion.x * 0.36f, 340.0f);

				ImGui::BeginChild("MeshEditorInfoPanel", ImVec2(LeftPanelWidth, TopPanelHeight), true, ImGuiWindowFlags_None);
				{
					DrawMeshSummary();

					if (m_MeshType == static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN))
					{
						ImGui::Separator();
						ImGui::Text("Preview Settings");

						if (m_AvailableSkeletonAssetPaths.Num() > 0)
						{
							const CHAR* CurrentSkeletonLabel = (m_SelectedPreviewSkeletonIndex >= 0) ? *EPath::GetFileNameWithoutExtension(m_AvailableSkeletonAssetPaths[m_SelectedPreviewSkeletonIndex]) : "None";
							if (ImGui::BeginCombo("Preview Skeleton", CurrentSkeletonLabel))
							{
								for (INT32 SkeletonIndex = 0, SkeletonNum = m_AvailableSkeletonAssetPaths.Num(); SkeletonIndex < SkeletonNum; SkeletonIndex++)
								{
									const BOOL8 bSelected = (m_SelectedPreviewSkeletonIndex == SkeletonIndex);
									const EString AssetName = EPath::GetFileNameWithoutExtension(m_AvailableSkeletonAssetPaths[SkeletonIndex]);
									if (ImGui::Selectable(*AssetName, bSelected))
									{
										m_SelectedPreviewSkeletonIndex = SkeletonIndex;
										UpdatePreviewScene();
									}
									if (bSelected)
									{
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}
						}
						else
						{
							ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No skeleton asset found in current folder.");
						}
					}

					ImGui::Checkbox("Auto Rotate", reinterpret_cast<bool*>(&m_PreviewAutoRotate));
					m_PreviewScene.SetAutoRotate(m_PreviewAutoRotate);
				}
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("MeshEditorPreviewPanel", ImVec2(0.0f, TopPanelHeight), true, ImGuiWindowFlags_None);
				{
					ImGui::Text("Preview Scene");
					ImGui::Separator();

					ImVec2 PreviewRegion = ImGui::GetContentRegionAvail();
					const FLOAT PreviewWidth = EMath::Max(PreviewRegion.x, 64.0f);
					const FLOAT PreviewHeight = EMath::Max(PreviewRegion.y, 64.0f);
					m_PreviewScene.UpdateViewportSize(static_cast<UINT32>(PreviewWidth), static_cast<UINT32>(PreviewHeight));
					m_PreviewScene.Tick(ImGui::GetIO().DeltaTime);
					if (ImTextureID PreviewTexture = m_PreviewScene.GetTextureId())
					{
						ImGui::Image(PreviewTexture, ImVec2(PreviewWidth, PreviewHeight));
					}
					else
					{
						ImGui::TextWrapped("Preview texture hookup is temporarily skipped. Preview scene state and skeleton selection remain in place.");
					}
				}
				ImGui::EndChild();

				ImGui::BeginChild("MeshEditorDataPanel", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_None);
				{
					DrawIndexSection(*m_MeshResource);
					DrawVertexSection(*m_MeshResource);
					DrawSubmeshSection(*m_MeshResource);
					if (m_MeshType == static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN))
					{
						DrawSkinnedMeshSection();
					}
				}
				ImGui::EndChild();
			}
		}
		ImGui::End();
	}

	void EMeshEditor::OpenMeshAsset(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		ResetLoadedAsset();
		m_LastOpenError = EString();

		m_OpenedAssetFullPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(m_OpenedAssetFullPath);
		const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(m_OpenedAssetFullPath));
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			m_LastOpenError = EString("Mesh editor cannot parse asset path: ") + m_OpenedAssetFullPath;
			return;
		}

		EMeshType AssetMeshType = EMeshType::MESH_TYPE_COUNT;
		if (!(ReadMeshTypeFromAssetFile(m_OpenedAssetFullPath, AssetMeshType)))
		{
			m_LastOpenError = EString("Mesh editor failed to read mesh type from asset: ") + m_OpenedAssetFullPath;
			return;
		}

		switch (AssetMeshType)
		{
		case EMeshType::MESH_TYPE_STATIC:
			{
				const EStaticMeshAsset* LoadedAsset = nullptr;
				if (!(EMeshAssetManager::GetManagerSingleton()->LoadStaticMeshAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
				{
					m_LastOpenError = EString("Mesh editor failed to load static mesh asset: ") + m_OpenedAssetFullPath;
					return;
				}
				m_StaticMeshAsset = LoadedAsset;
				m_MeshResource = LoadedAsset->GetStoragedResource();
				m_MeshType = static_cast<UINT32>(EMeshType::MESH_TYPE_STATIC);
			}
			break;
		case EMeshType::MESH_TYPE_SKIN:
			{
				const ESkinnedMeshAsset* LoadedAsset = nullptr;
				if (!(EMeshAssetManager::GetManagerSingleton()->LoadSkinnedMeshAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
				{
					m_LastOpenError = EString("Mesh editor failed to load skinned mesh asset: ") + m_OpenedAssetFullPath;
					return;
				}
				m_SkinnedMeshAsset = LoadedAsset;
				m_MeshResource = LoadedAsset->GetStoragedResource();
				m_MeshType = static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN);
			}
			break;
		default:
			m_LastOpenError = EString("Mesh editor does not support this mesh type: ") + m_OpenedAssetFullPath;
			return;
		}

		if (!m_MeshResource)
		{
			m_LastOpenError = EString("Mesh editor loaded an asset without mesh resource: ") + m_OpenedAssetFullPath;
			ResetLoadedAsset();
			m_PreviewScene.ClearPreview();
			return;
		}

		RefreshPreviewSkeletonOptions();
		UpdatePreviewScene();
	}

	void EMeshEditor::ResetLoadedAsset()
	{
		m_StaticMeshAsset = nullptr;
		m_SkinnedMeshAsset = nullptr;
		m_MeshResource = nullptr;
		m_MeshType = static_cast<UINT32>(EMeshType::MESH_TYPE_COUNT);
		m_AvailableSkeletonAssetPaths.Empty();
		m_PreviewSkeletonAsset = nullptr;
		m_SelectedPreviewSkeletonIndex = -1;
	}

	void EMeshEditor::RefreshPreviewSkeletonOptions()
	{
		m_AvailableSkeletonAssetPaths.Empty();
		m_PreviewSkeletonAsset = nullptr;
		m_SelectedPreviewSkeletonIndex = -1;

		if (m_MeshType != static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN))
		{
			return;
		}

		TArray<EString> FolderPaths;
		TArray<EString> FilePaths;
		if (!(EFileHelper::ScanDirectory(NormalizeAssetDirectory(EPath::GetFileFolderPath(m_OpenedAssetFullPath)), FolderPaths, FilePaths, TRUE)))
		{
			return;
		}

		for (INT32 FileIndex = 0, FileNum = FilePaths.Num(); FileIndex < FileNum; FileIndex++)
		{
			const EString AssetPath = FilePaths[FileIndex].Replace("\\", "/");
			if (!(AssetPath.Contains(EEngineSettings::ENGINE_ASSET_NAME_TYPE)))
			{
				continue;
			}

			EAssetType AssetType = EAssetType::ASSET_TYPE_UNKNOWN;
			if (ReadAssetTypeFromAssetFile(AssetPath, AssetType) && (AssetType == EAssetType::ASSET_TYPE_SKELETON))
			{
				m_AvailableSkeletonAssetPaths.Add(AssetPath);
			}
		}

		if (m_AvailableSkeletonAssetPaths.Num() > 0)
		{
			m_SelectedPreviewSkeletonIndex = 0;
		}
	}

	void EMeshEditor::UpdatePreviewScene()
	{
		m_PreviewSkeletonAsset = nullptr;
		if (m_MeshType == static_cast<UINT32>(EMeshType::MESH_TYPE_STATIC))
		{
			m_PreviewScene.SetStaticMesh(m_StaticMeshAsset);
			return;
		}

		if ((m_MeshType != static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN)) || (!m_SkinnedMeshAsset))
		{
			m_PreviewScene.ClearPreview();
			return;
		}

		if ((m_SelectedPreviewSkeletonIndex >= 0) && (m_SelectedPreviewSkeletonIndex < m_AvailableSkeletonAssetPaths.Num()))
		{
			LoadSkeletonAssetFromFullPath(m_AvailableSkeletonAssetPaths[m_SelectedPreviewSkeletonIndex], m_PreviewSkeletonAsset);
		}

		if (m_PreviewSkeletonAsset)
		{
			m_PreviewScene.SetSkinnedMesh(m_SkinnedMeshAsset, m_PreviewSkeletonAsset);
		}
		else
		{
			m_PreviewScene.ClearPreview();
		}
	}

	void EMeshEditor::DrawMeshSummary() const
	{
		const EBoundAABB& Bound = m_MeshResource->GetBoundAABB();
		const EMesh::EIndexPart& IndexPart = m_MeshResource->GetIndexPart();
		const EMesh::EVertexPart& VertexPart = m_MeshResource->GetVertexPart();
		const EMesh::ESubmeshPart& SubmeshPart = m_MeshResource->GetSubmeshPart();

		BOOL32 bRenderResourceValid = FALSE;
		BOOL32 bAssetValid = FALSE;
		BOOL32 bHoldResource = FALSE;
		if (m_StaticMeshAsset)
		{
			bRenderResourceValid = m_StaticMeshAsset->IsRenderResourceValid();
			bAssetValid = m_StaticMeshAsset->IsValid();
			bHoldResource = m_StaticMeshAsset->IsHoldResource();
		}
		else if (m_SkinnedMeshAsset)
		{
			bRenderResourceValid = m_SkinnedMeshAsset->IsRenderResourceValid();
			bAssetValid = m_SkinnedMeshAsset->IsValid();
			bHoldResource = m_SkinnedMeshAsset->IsHoldResource();
		}

		ImGui::Text("Mesh Name: %s", *m_MeshResource->GetMeshName());
		ImGui::Text("Mesh Type: %s", MeshTypeAsString(static_cast<EMeshType>(m_MeshType)));
		ImGui::Text("Asset Valid: %s", bAssetValid ? "True" : "False");
		ImGui::Text("Resource Valid: %s", m_MeshResource->IsResourceValid() ? "True" : "False");
		ImGui::Text("Render Resource Valid: %s", bRenderResourceValid ? "True" : "False");
		ImGui::Text("Hold CPU Resource: %s", bHoldResource ? "True" : "False");
		ImGui::Text("Vertex Layout Mask: 0x%08X", m_MeshResource->GetVertexLayout());
		ImGui::Text("Index Count: %u", IndexPart.ElementNum);
		ImGui::Text("Vertex Stream Count: %d", VertexPart.Num());
		ImGui::Text("Submesh Count: %d", SubmeshPart.Num());
		ImGui::Text("Bounds Min: (%.3f, %.3f, %.3f)", Bound.AABBMin.x, Bound.AABBMin.y, Bound.AABBMin.z);
		ImGui::Text("Bounds Max: (%.3f, %.3f, %.3f)", Bound.AABBMax.x, Bound.AABBMax.y, Bound.AABBMax.z);
	}

	void EMeshEditor::DrawIndexSection(const EMesh& InMesh) const
	{
		if (!ImGui::CollapsingHeader("Index Buffer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		const EMesh::EIndexPart& IndexPart = InMesh.GetIndexPart();
		ImGui::Text("Type: %s", VertexLayoutTypeAsString(IndexPart.PartType));
		ImGui::Text("Stride: %u bytes", IndexPart.Stride);
		ImGui::Text("Element Count: %u", IndexPart.ElementNum);
		ImGui::Text("Buffer Bytes: %u", IndexPart.Stride * IndexPart.ElementNum);
	}

	void EMeshEditor::DrawVertexSection(const EMesh& InMesh) const
	{
		if (!ImGui::CollapsingHeader("Vertex Streams", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		const EMesh::EVertexPart& VertexPart = InMesh.GetVertexPart();
		if (VertexPart.Num() <= 0)
		{
			ImGui::Text("No vertex streams.");
			return;
		}

		for (INT32 VertexIndex = 0, VertexNum = VertexPart.Num(); VertexIndex < VertexNum; VertexIndex++)
		{
			const EVertexData& VertexData = VertexPart[VertexIndex];
			ImGui::PushID(VertexIndex);
			if (ImGui::TreeNodeEx("stream", ImGuiTreeNodeFlags_DefaultOpen, "Stream %d: %s", VertexIndex, VertexLayoutTypeAsString(VertexData.PartType)))
			{
				ImGui::Text("Part Type: %u", VertexData.PartType);
				ImGui::Text("Stride: %u bytes", VertexData.Stride);
				ImGui::Text("Element Count: %u", VertexData.ElementNum);
				ImGui::Text("Buffer Bytes: %u", VertexData.Stride * VertexData.ElementNum);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	void EMeshEditor::DrawSubmeshSection(const EMesh& InMesh) const
	{
		if (!ImGui::CollapsingHeader("Submeshes", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		const EMesh::ESubmeshPart& SubmeshPart = InMesh.GetSubmeshPart();
		if (SubmeshPart.Num() <= 0)
		{
			ImGui::Text("No submeshes.");
			return;
		}

		for (INT32 SubmeshIndex = 0, SubmeshNum = SubmeshPart.Num(); SubmeshIndex < SubmeshNum; SubmeshIndex++)
		{
			const ESubmeshData& Submesh = SubmeshPart[SubmeshIndex];
			ImGui::PushID(SubmeshIndex);
			if (ImGui::TreeNodeEx("submesh", ImGuiTreeNodeFlags_DefaultOpen, "Submesh %d", SubmeshIndex))
			{
				ImGui::Text("Start Vertex: %u", Submesh.StartVertex);
				ImGui::Text("Vertex Count: %u", Submesh.VertexNum);
				ImGui::Text("Start Index: %u", Submesh.StartIndex);
				ImGui::Text("Index Count: %u", Submesh.IndexNum);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	void EMeshEditor::DrawSkinnedMeshSection() const
	{
		if ((!m_SkinnedMeshAsset) || (!m_MeshResource) || (m_MeshType != static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN)))
		{
			return;
		}

		if (!ImGui::CollapsingHeader("Skin Data", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		const ESkinnedMesh* SkinnedMesh = m_MeshResource->AsType<ESkinnedMesh>();
		if (!SkinnedMesh)
		{
			ImGui::Text("Skinned mesh resource is unavailable.");
			return;
		}

		const ESkinnedMesh::EBindPoseValue& BindPoseValues = SkinnedMesh->GetBindPoseValue();
		const ESkinnedMesh::ESkinPart& SkinPart = SkinnedMesh->GetSkinPart();
		ImGui::Text("Linked Skeleton: %s", m_SkinnedMeshAsset->GetSkeleton() ? "True" : "False");
		ImGui::Text("Bind Pose Count: %d", BindPoseValues.Num());
		ImGui::Text("Effect Bone Num: %u", static_cast<UINT32>(SkinnedMesh->GetEffectBoneNum()));
		ImGui::Text("Skin Stream Count: %d", SkinPart.Num());

		const ESkinnedMeshRenderResource* RenderResource = m_SkinnedMeshAsset->GetRenderResource();
		if (RenderResource)
		{
			ImGui::Text("Skin Render Resource Count: %d", RenderResource->GetSkinRenderResource().Num());
		}

		for (INT32 SkinIndex = 0, SkinNum = SkinPart.Num(); SkinIndex < SkinNum; SkinIndex++)
		{
			const ESkinData& SkinData = SkinPart[SkinIndex];
			ImGui::PushID(SkinIndex);
			if (ImGui::TreeNodeEx("skin", ImGuiTreeNodeFlags_DefaultOpen, "Skin Stream %d", SkinIndex))
			{
				ImGui::Text("Stride: %u bytes", SkinData.Stride);
				ImGui::Text("Element Count: %u", SkinData.ElementNum);
				ImGui::Text("Influence Count Per Vertex: %u", SkinData.Stride / sizeof(FLOAT));
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
}

#endif