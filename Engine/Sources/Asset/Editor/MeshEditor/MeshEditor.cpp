#include "MeshEditor.h"

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <IO/FileHelper.h>
#include <BaseAsset.h>
#include <Config/EngineConfig.h>
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

		UINT32 CommonPrefixLength(const EString& InValue0, const EString& InValue1)
		{
			const UINT32 Count = EMath::Min(InValue0.Length(), InValue1.Length());
			UINT32 Index = 0u;
			while ((Index < Count) && (InValue0[Index] == InValue1[Index]))
			{
				Index++;
			}
			return Index;
		}

		// A mesh asset does not record the skeleton it was imported against, so the preview
		// picks the skeleton whose asset name shares the longest prefix with the mesh name.
		// This pairs "Foo_Mesh" with "Foo_Skeleton" without naming any specific asset; the
		// author can still choose another skeleton in the combo.
		INT32 FindClosestSkeletonIndex(const TArray<EString>& InSkeletonPaths, const EString& InMeshName)
		{
			if (InSkeletonPaths.Num() <= 0)
			{
				return -1;
			}

			INT32 BestIndex = 0;
			UINT32 BestPrefixLength = 0u;
			for (INT32 SkeletonIndex = 0; SkeletonIndex < InSkeletonPaths.Num(); SkeletonIndex++)
			{
				const EString SkeletonName = EPath::GetFileNameWithoutExtension(InSkeletonPaths[SkeletonIndex]);
				const UINT32 PrefixLength = CommonPrefixLength(SkeletonName, InMeshName);
				if (PrefixLength > BestPrefixLength)
				{
					BestPrefixLength = PrefixLength;
					BestIndex = SkeletonIndex;
				}
			}
			return BestIndex;
		}
	}

	void EMeshEditor::EditorInit()
	{
		ResetLoadedAsset();
		OpenedAssetFullPath = EString();
		LastOpenError = EString();
		bShowEditor = FALSE;
		PreviewScene.Initialize("MeshEditorPreview", Vector3(32000.0f, 32000.0f, 32000.0f));
	}

	void EMeshEditor::EditorUpdate()
	{
		if (!bShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(1320.0f, 820.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Mesh Editor", &bShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (OpenedAssetFullPath.Length() > 0u))
			{
				OpenMeshAsset(OpenedAssetFullPath);
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", OpenedAssetFullPath.Length() > 0u ? *OpenedAssetFullPath : "No mesh asset selected.");

			if (LastOpenError.Length() > 0u)
			{
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *LastOpenError);
			}

			if (!MeshResource)
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

				ImGui::BeginChild("MeshEditorInfoPanel", ImVec2(LeftPanelWidth, TopPanelHeight), TRUE, ImGuiWindowFlags_None);
				{
					DrawMeshSummary();

					if (MeshType == static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN))
					{
						ImGui::Separator();
						ImGui::Text("Preview Settings");

						if (AvailableSkeletonAssetPaths.Num() > 0)
						{
							// Keep the label string alive through BeginCombo; a pointer into a
							// temporary EString is freed before ImGui draws the preview text.
							const EString CurrentSkeletonName = (SelectedPreviewSkeletonIndex >= 0)
								? EPath::GetFileNameWithoutExtension(AvailableSkeletonAssetPaths[SelectedPreviewSkeletonIndex])
								: EString("None");
							if (ImGui::BeginCombo("Preview Skeleton", *CurrentSkeletonName))
							{
								for (INT32 SkeletonIndex = 0, SkeletonNum = AvailableSkeletonAssetPaths.Num(); SkeletonIndex < SkeletonNum; SkeletonIndex++)
								{
									const BOOL8 bSelected = (SelectedPreviewSkeletonIndex == SkeletonIndex);
									const EString AssetName = EPath::GetFileNameWithoutExtension(AvailableSkeletonAssetPaths[SkeletonIndex]);
									if (ImGui::Selectable(*AssetName, bSelected))
									{
										SelectedPreviewSkeletonIndex = SkeletonIndex;
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
							ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No skeleton asset found in the skeleton asset folder.");
						}
					}

					ImGui::Checkbox("Auto Rotate", reinterpret_cast<bool*>(&bPreviewAutoRotate));
					PreviewScene.SetAutoRotate(bPreviewAutoRotate);
				}
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("MeshEditorPreviewPanel", ImVec2(0.0f, TopPanelHeight), TRUE, ImGuiWindowFlags_None);
				{
					ImGui::Text("Preview Scene");
					ImGui::Separator();

					ImVec2 PreviewRegion = ImGui::GetContentRegionAvail();
					const FLOAT PreviewWidth = EMath::Max(PreviewRegion.x, 64.0f);
					const FLOAT PreviewHeight = EMath::Max(PreviewRegion.y, 64.0f);
					PreviewScene.UpdateViewportSize(static_cast<UINT32>(PreviewWidth), static_cast<UINT32>(PreviewHeight));
					PreviewScene.Tick(ImGui::GetIO().DeltaTime);
					if (ImTextureID PreviewTexture = PreviewScene.GetTextureId())
					{
						ImGui::Image(PreviewTexture, ImVec2(PreviewWidth, PreviewHeight));
					}
					else
					{
						ImGui::TextWrapped("No preview to show yet.");
					}
				}
				ImGui::EndChild();

				ImGui::BeginChild("MeshEditorDataPanel", ImVec2(0.0f, 0.0f), TRUE, ImGuiWindowFlags_None);
				{
					DrawIndexSection(*MeshResource);
					DrawVertexSection(*MeshResource);
					DrawSubmeshSection(*MeshResource);
					if (MeshType == static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN))
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
		bShowEditor = TRUE;
		ResetLoadedAsset();
		LastOpenError = EString();

		OpenedAssetFullPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(OpenedAssetFullPath);
		const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(OpenedAssetFullPath));
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			LastOpenError = EString("Mesh editor cannot parse asset path: ") + OpenedAssetFullPath;
			return;
		}

		EMeshType AssetMeshType = EMeshType::MESH_TYPE_COUNT;
		if (!(ReadMeshTypeFromAssetFile(OpenedAssetFullPath, AssetMeshType)))
		{
			LastOpenError = EString("Mesh editor failed to read mesh type from asset: ") + OpenedAssetFullPath;
			return;
		}

		switch (AssetMeshType)
		{
		case EMeshType::MESH_TYPE_STATIC:
			{
				const EStaticMeshAsset* LoadedAsset = nullptr;
				if (!(EMeshAssetManager::GetManagerSingleton()->LoadStaticMeshAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
				{
					LastOpenError = EString("Mesh editor failed to load static mesh asset: ") + OpenedAssetFullPath;
					return;
				}
				StaticMeshAsset = LoadedAsset;
				MeshResource = LoadedAsset->GetStoragedResource();
				MeshType = static_cast<UINT32>(EMeshType::MESH_TYPE_STATIC);
			}
			break;
		case EMeshType::MESH_TYPE_SKIN:
			{
				const ESkinnedMeshAsset* LoadedAsset = nullptr;
				if (!(EMeshAssetManager::GetManagerSingleton()->LoadSkinnedMeshAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
				{
					LastOpenError = EString("Mesh editor failed to load skinned mesh asset: ") + OpenedAssetFullPath;
					return;
				}
				SkinnedMeshAsset = LoadedAsset;
				MeshResource = LoadedAsset->GetStoragedResource();
				MeshType = static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN);
			}
			break;
		default:
			LastOpenError = EString("Mesh editor does not support this mesh type: ") + OpenedAssetFullPath;
			return;
		}

		if (!MeshResource)
		{
			LastOpenError = EString("Mesh editor loaded an asset without mesh resource: ") + OpenedAssetFullPath;
			ResetLoadedAsset();
			PreviewScene.ClearPreview();
			return;
		}

		RefreshPreviewSkeletonOptions();
		UpdatePreviewScene();
	}

	void EMeshEditor::ResetLoadedAsset()
	{
		StaticMeshAsset = nullptr;
		SkinnedMeshAsset = nullptr;
		MeshResource = nullptr;
		MeshType = static_cast<UINT32>(EMeshType::MESH_TYPE_COUNT);
		AvailableSkeletonAssetPaths.Empty();
		PreviewSkeletonAsset = nullptr;
		SelectedPreviewSkeletonIndex = -1;
	}

	void EMeshEditor::RefreshPreviewSkeletonOptions()
	{
		AvailableSkeletonAssetPaths.Empty();
		PreviewSkeletonAsset = nullptr;
		SelectedPreviewSkeletonIndex = -1;

		if (MeshType != static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN))
		{
			return;
		}

		// Skeleton assets live in the engine's skeleton folder, not next to the mesh. Scanning
		// the mesh folder only ever found meshes, which is why skinned previews reported no
		// skeleton at all.
		TArray<EString> FolderPaths;
		TArray<EString> FilePaths;
		if (!(EFileHelper::ScanDirectory(NormalizeAssetDirectory(EBaseSettings::ENGINE_SKELETON_PATH), FolderPaths, FilePaths, TRUE)))
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
				AvailableSkeletonAssetPaths.Add(AssetPath);
			}
		}

		if (AvailableSkeletonAssetPaths.Num() > 0)
		{
			const EString MeshName = EPath::GetFileNameWithoutExtension(OpenedAssetFullPath);
			SelectedPreviewSkeletonIndex = FindClosestSkeletonIndex(AvailableSkeletonAssetPaths, MeshName);
		}
	}

	void EMeshEditor::UpdatePreviewScene()
	{
		PreviewSkeletonAsset = nullptr;
		if (MeshType == static_cast<UINT32>(EMeshType::MESH_TYPE_STATIC))
		{
			PreviewScene.SetStaticMesh(StaticMeshAsset);
			return;
		}

		if ((MeshType != static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN)) || (!SkinnedMeshAsset))
		{
			PreviewScene.ClearPreview();
			return;
		}

		if ((SelectedPreviewSkeletonIndex >= 0) && (SelectedPreviewSkeletonIndex < AvailableSkeletonAssetPaths.Num()))
		{
			LoadSkeletonAssetFromFullPath(AvailableSkeletonAssetPaths[SelectedPreviewSkeletonIndex], PreviewSkeletonAsset);
		}

		if (PreviewSkeletonAsset)
		{
			PreviewScene.SetSkinnedMesh(SkinnedMeshAsset, PreviewSkeletonAsset);
		}
		else
		{
			PreviewScene.ClearPreview();
		}
	}

	void EMeshEditor::DrawMeshSummary() const
	{
		const EBoundAABB& Bound = MeshResource->GetBoundAABB();
		const EMesh::EIndexPart& IndexPart = MeshResource->GetIndexPart();
		const EMesh::EVertexPart& VertexPart = MeshResource->GetVertexPart();
		const EMesh::ESubmeshPart& SubmeshPart = MeshResource->GetSubmeshPart();

		BOOL32 bRenderResourceValid = FALSE;
		BOOL32 bAssetValid = FALSE;
		BOOL32 bHoldResource = FALSE;
		if (StaticMeshAsset)
		{
			bRenderResourceValid = StaticMeshAsset->IsRenderResourceValid();
			bAssetValid = StaticMeshAsset->IsValid();
			// The asset lifecycle no longer carries a "holds cold data" flag: the stored
			// resource is the cold data, so its presence is the answer.
			bHoldResource = (StaticMeshAsset->GetStoragedResource() != nullptr);
		}
		else if (SkinnedMeshAsset)
		{
			bRenderResourceValid = SkinnedMeshAsset->IsRenderResourceValid();
			bAssetValid = SkinnedMeshAsset->IsValid();
			bHoldResource = (SkinnedMeshAsset->GetStoragedResource() != nullptr);
		}

		ImGui::Text("Mesh Name: %s", *MeshResource->GetMeshName());
		ImGui::Text("Mesh Type: %s", MeshTypeAsString(static_cast<EMeshType>(MeshType)));
		ImGui::Text("Asset Valid: %s", bAssetValid ? "True" : "False");
		ImGui::Text("Resource Valid: %s", MeshResource->IsResourceValid() ? "True" : "False");
		ImGui::Text("Render Resource Valid: %s", bRenderResourceValid ? "True" : "False");
		ImGui::Text("Hold CPU Resource: %s", bHoldResource ? "True" : "False");
		ImGui::Text("Vertex Layout Mask: 0x%08X", MeshResource->GetVertexLayout());
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
		if ((!SkinnedMeshAsset) || (!MeshResource) || (MeshType != static_cast<UINT32>(EMeshType::MESH_TYPE_SKIN)))
		{
			return;
		}

		if (!ImGui::CollapsingHeader("Skin Data", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		const ESkinnedMesh* SkinnedMesh = MeshResource->AsType<ESkinnedMesh>();
		if (!SkinnedMesh)
		{
			ImGui::Text("Skinned mesh resource is unavailable.");
			return;
		}

		const ESkinnedMesh::EBindPoseValue& BindPoseValues = SkinnedMesh->GetBindPoseValue();
		const ESkinnedMesh::ESkinPart& SkinPart = SkinnedMesh->GetSkinPart();
		ImGui::Text("Linked Skeleton: %s", SkinnedMeshAsset->GetSkeleton() ? "True" : "False");
		ImGui::Text("Bind Pose Count: %d", BindPoseValues.Num());
		ImGui::Text("Effect Bone Num: %u", static_cast<UINT32>(SkinnedMesh->GetEffectBoneNum()));
		ImGui::Text("Skin Stream Count: %d", SkinPart.Num());

		const ESkinnedMeshRenderResource* RenderResource = SkinnedMeshAsset->GetRenderResource();
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