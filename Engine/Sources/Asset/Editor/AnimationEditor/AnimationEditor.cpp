#include "AnimationEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationAsset/AnimationAsset.h>
#include <BaseAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <IO/FileHelper.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{
	namespace
	{
		const CHAR* AnimationBehaviourTypeAsString(const EAnimationBehaviourType InType)
		{
			switch (InType)
			{
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT:
				return "Default";
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_CONSTANT:
				return "Constant";
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_LINEAR:
				return "Linear";
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_REPEAT:
				return "Repeat";
			default:
				return "Unknown";
			}
		}

		INT32 ClampSelectionIndex(const INT32 InIndex, const INT32 InCount)
		{
			if (InCount <= 0)
			{
				return 0;
			}
			if (InIndex < 0)
			{
				return 0;
			}
			if (InIndex >= InCount)
			{
				return InCount - 1;
			}
			return InIndex;
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

		UINT32 GetCurveKeyCount(const EBoneAnimationCurve& InCurve)
		{
			return InCurve.PositionKeys.Num<UINT32>() + InCurve.RotationKeys.Num<UINT32>() + InCurve.ScalingKeys.Num<UINT32>();
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
			return (AssetType == EAssetType::ASSET_TYPE_MESH) && (OutMeshType == EMeshType::MESH_TYPE_SKIN);
		}

		BOOL8 LoadSkinnedMeshAssetFromFullPath(const EString& InAssetFullPath, const ESkinnedMeshAsset*& OutMeshAsset)
		{
			OutMeshAsset = nullptr;
			const EString AssetName = EPath::GetFileNameWithoutExtension(InAssetFullPath);
			const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(InAssetFullPath));
			return (AssetName.Length() > 0u) && (AssetDirectory.Length() > 0u) && EMeshAssetManager::GetManagerSingleton()->LoadSkinnedMeshAsset(AssetDirectory, AssetName, OutMeshAsset) && (!!OutMeshAsset);
		}

		BOOL8 LoadSkeletonAssetFromFullPath(const EString& InAssetFullPath, const ESkeletonAsset*& OutSkeletonAsset)
		{
			OutSkeletonAsset = nullptr;
			const EString AssetName = EPath::GetFileNameWithoutExtension(InAssetFullPath);
			const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(InAssetFullPath));
			return (AssetName.Length() > 0u) && (AssetDirectory.Length() > 0u) && ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(AssetDirectory, AssetName, OutSkeletonAsset) && (!!OutSkeletonAsset);
		}

		template<typename TKeyType>
		BOOL8 TryGetKeyTimeRange(const TArray<TKeyType>& InKeys, FLOAT& OutStartTime, FLOAT& OutEndTime)
		{
			const INT32 KeyNum = InKeys.Num();
			if (KeyNum <= 0)
			{
				return FALSE;
			}
			OutStartTime = static_cast<FLOAT>(InKeys[0].Time);
			OutEndTime = static_cast<FLOAT>(InKeys[KeyNum - 1].Time);
			return TRUE;
		}
	}

	void EAnimationEditor::EditorInit()
	{
		ResetSelection();
		m_AnimationAsset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastOpenError = EString();
		m_ShowEditor = FALSE;
		m_PreviewScene.Initialize("AnimationEditorPreview", Vector3(36000.0f, 32000.0f, 32000.0f));
	}

	void EAnimationEditor::OpenAnimationAsset(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		m_AnimationAsset = nullptr;
		m_LastOpenError = EString();

		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedAssetPath);
		const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(NormalizedAssetPath));
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			m_LastOpenError = EString("Animation editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const ESkeletonAnimationAsset* LoadedAsset = nullptr;
		if (!(EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			m_LastOpenError = EString("Animation editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		m_AnimationAsset = LoadedAsset;
		m_OpenedAssetFullPath = NormalizedAssetPath;
		ResetSelection();
		RefreshPreviewOptions();
	}

	void EAnimationEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		const ESkeletonAnimation* Animation = nullptr;
		if (m_AnimationAsset)
		{
			Animation = m_AnimationAsset->GetStoragedResource();
		}

		ImGui::SetNextWindowSize(ImVec2(960.0f, 640.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Editor", &m_ShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				OpenAnimationAsset(m_OpenedAssetFullPath);
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No animation asset selected.");

			if (m_LastOpenError.Length() > 0u)
			{
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastOpenError);
			}

			if (!m_AnimationAsset)
			{
				ImGui::Separator();
				ImGui::Text("No animation asset selected.");
			}
			else if (!Animation)
			{
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "The selected animation asset does not contain a loaded resource.");
			}
			else
			{
				ImGui::Separator();
				DrawAssetSummary(m_AnimationAsset, Animation);
			}
		}
		ImGui::End();

		if ((!m_ShowEditor) || (!Animation))
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(280.0f, 520.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 220.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Clip List", nullptr, ImGuiWindowFlags_None);
		{
			DrawClipList(Animation);
		}
		ImGui::End();

		const INT32 ClipNum = Animation->GetAnimationClipNum();
		if (ClipNum <= 0)
		{
			return;
		}

		m_SelectedClipIndex = ClampSelectionIndex(m_SelectedClipIndex, ClipNum);
		const ESkeletonAnimationClip* SelectedClip = Animation->GetAnimationClip(m_SelectedClipIndex);
		if (!SelectedClip)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(1360.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Preview", nullptr, ImGuiWindowFlags_None);
		{
			if (m_AvailablePreviewMeshPaths.Num() > 0)
			{
				const CHAR* CurrentMeshLabel = (m_SelectedPreviewMeshIndex >= 0) ? *EPath::GetFileNameWithoutExtension(m_AvailablePreviewMeshPaths[m_SelectedPreviewMeshIndex]) : "None";
				if (ImGui::BeginCombo("Preview Mesh", CurrentMeshLabel))
				{
					for (INT32 MeshIndex = 0, MeshNum = m_AvailablePreviewMeshPaths.Num(); MeshIndex < MeshNum; MeshIndex++)
					{
						const BOOL8 bSelected = (m_SelectedPreviewMeshIndex == MeshIndex);
						const EString AssetName = EPath::GetFileNameWithoutExtension(m_AvailablePreviewMeshPaths[MeshIndex]);
						if (ImGui::Selectable(*AssetName, bSelected))
						{
							m_SelectedPreviewMeshIndex = MeshIndex;
							UpdatePreviewScene(SelectedClip);
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
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No skinned mesh asset found in current folder.");
			}

			if (m_AvailablePreviewSkeletonPaths.Num() > 0)
			{
				const CHAR* CurrentSkeletonLabel = (m_SelectedPreviewSkeletonIndex >= 0) ? *EPath::GetFileNameWithoutExtension(m_AvailablePreviewSkeletonPaths[m_SelectedPreviewSkeletonIndex]) : "None";
				if (ImGui::BeginCombo("Preview Skeleton", CurrentSkeletonLabel))
				{
					for (INT32 SkeletonIndex = 0, SkeletonNum = m_AvailablePreviewSkeletonPaths.Num(); SkeletonIndex < SkeletonNum; SkeletonIndex++)
					{
						const BOOL8 bSelected = (m_SelectedPreviewSkeletonIndex == SkeletonIndex);
						const EString AssetName = EPath::GetFileNameWithoutExtension(m_AvailablePreviewSkeletonPaths[SkeletonIndex]);
						if (ImGui::Selectable(*AssetName, bSelected))
						{
							m_SelectedPreviewSkeletonIndex = SkeletonIndex;
							UpdatePreviewScene(SelectedClip);
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

			ImGui::Checkbox("Auto Rotate", reinterpret_cast<bool*>(&m_PreviewAutoRotate));
			ImGui::SameLine();
			ImGui::Checkbox("Play", reinterpret_cast<bool*>(&m_PreviewPlayAnimation));
			ImGui::SliderFloat("Play Rate", &m_PreviewPlaybackRate, 0.0f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

			UpdatePreviewScene(SelectedClip);
			m_PreviewScene.SetAutoRotate(m_PreviewAutoRotate);
			m_PreviewScene.SetPlayAnimation(m_PreviewPlayAnimation);
			m_PreviewScene.SetPlaybackRate(m_PreviewPlaybackRate);

			ImGui::Separator();
			ImVec2 AvailableRegion = ImGui::GetContentRegionAvail();
			const FLOAT PreviewWidth = EMath::Max(AvailableRegion.x, 64.0f);
			const FLOAT PreviewHeight = EMath::Max(AvailableRegion.y, 64.0f);
			m_PreviewScene.UpdateViewportSize(static_cast<UINT32>(PreviewWidth), static_cast<UINT32>(PreviewHeight));
			m_PreviewScene.Tick(ImGui::GetIO().DeltaTime);
			if (ImTextureID PreviewTexture = m_PreviewScene.GetTextureId())
			{
				ImGui::Image(PreviewTexture, ImVec2(PreviewWidth, PreviewHeight));
			}
			else
			{
				ImGui::TextWrapped("Preview texture hookup is temporarily skipped. Mesh, skeleton and clip binding stay available for the next step.");
			}
		}
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(720.0f, 220.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Clip Details", nullptr, ImGuiWindowFlags_None);
		{
			DrawClipDetails(*SelectedClip);
		}
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(280.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(1148.0f, 220.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Curve List", nullptr, ImGuiWindowFlags_None);
		{
			DrawCurveList(*SelectedClip);
		}
		ImGui::End();

		const INT32 CurveNum = SelectedClip->AnimationCurves.Num();
		if (CurveNum <= 0)
		{
			return;
		}

		m_SelectedCurveIndex = ClampSelectionIndex(m_SelectedCurveIndex, CurveNum);
		ImGui::SetNextWindowSize(ImVec2(420.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(1148.0f, 540.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Curve Details", nullptr, ImGuiWindowFlags_None);
		{
			DrawCurveDetails(SelectedClip->AnimationCurves[m_SelectedCurveIndex]);
		}
		ImGui::End();
	}

	void EAnimationEditor::ResetSelection()
	{
		m_SelectedClipIndex = 0;
		m_SelectedCurveIndex = 0;
	}

	void EAnimationEditor::RefreshPreviewOptions()
	{
		m_AvailablePreviewMeshPaths.Empty();
		m_AvailablePreviewSkeletonPaths.Empty();
		m_PreviewMeshAsset = nullptr;
		m_PreviewSkeletonAsset = nullptr;
		m_SelectedPreviewMeshIndex = -1;
		m_SelectedPreviewSkeletonIndex = -1;

		if (m_OpenedAssetFullPath.Length() <= 0u)
		{
			m_PreviewScene.ClearPreview();
			return;
		}

		TArray<EString> FolderPaths;
		TArray<EString> FilePaths;
		if (!(EFileHelper::ScanDirectory(NormalizeAssetDirectory(EPath::GetFileFolderPath(m_OpenedAssetFullPath)), FolderPaths, FilePaths, TRUE)))
		{
			m_PreviewScene.ClearPreview();
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
			if (!(ReadAssetTypeFromAssetFile(AssetPath, AssetType)))
			{
				continue;
			}

			if (AssetType == EAssetType::ASSET_TYPE_SKELETON)
			{
				m_AvailablePreviewSkeletonPaths.Add(AssetPath);
			}
			else if (AssetType == EAssetType::ASSET_TYPE_MESH)
			{
				EMeshType MeshType = EMeshType::MESH_TYPE_COUNT;
				if (ReadMeshTypeFromAssetFile(AssetPath, MeshType) && (MeshType == EMeshType::MESH_TYPE_SKIN))
				{
					m_AvailablePreviewMeshPaths.Add(AssetPath);
				}
			}
		}

		if (m_AvailablePreviewMeshPaths.Num() > 0)
		{
			m_SelectedPreviewMeshIndex = 0;
		}
		if (m_AvailablePreviewSkeletonPaths.Num() > 0)
		{
			m_SelectedPreviewSkeletonIndex = 0;
		}
	}

	void EAnimationEditor::UpdatePreviewScene(const ESkeletonAnimationClip* InSelectedClip)
	{
		m_PreviewMeshAsset = nullptr;
		m_PreviewSkeletonAsset = nullptr;
		if ((m_SelectedPreviewMeshIndex >= 0) && (m_SelectedPreviewMeshIndex < m_AvailablePreviewMeshPaths.Num()))
		{
			LoadSkinnedMeshAssetFromFullPath(m_AvailablePreviewMeshPaths[m_SelectedPreviewMeshIndex], m_PreviewMeshAsset);
		}
		if ((m_SelectedPreviewSkeletonIndex >= 0) && (m_SelectedPreviewSkeletonIndex < m_AvailablePreviewSkeletonPaths.Num()))
		{
			LoadSkeletonAssetFromFullPath(m_AvailablePreviewSkeletonPaths[m_SelectedPreviewSkeletonIndex], m_PreviewSkeletonAsset);
		}

		m_PreviewScene.SetAutoRotate(m_PreviewAutoRotate);
		m_PreviewScene.SetPlayAnimation(m_PreviewPlayAnimation);
		m_PreviewScene.SetPlaybackRate(m_PreviewPlaybackRate);
		m_PreviewScene.SetAnimationClip(InSelectedClip);

		if ((m_PreviewMeshAsset) && (m_PreviewSkeletonAsset))
		{
			m_PreviewScene.SetSkinnedMesh(m_PreviewMeshAsset, m_PreviewSkeletonAsset);
			m_PreviewScene.SetAnimationClip(InSelectedClip);
		}
		else
		{
			m_PreviewScene.ClearPreview();
		}
	}

	void EAnimationEditor::DrawAssetSummary(const ESkeletonAnimationAsset* InAsset, const ESkeletonAnimation* InAnimation) const
	{
		UINT32 TotalCurveNum = 0u;
		UINT32 TotalKeyNum = 0u;
		const TArray<ESkeletonAnimationClip>& AnimationClips = InAnimation->GetAnimationClips();
		for (INT32 ClipIndex = 0, ClipNum = AnimationClips.Num(); ClipIndex < ClipNum; ClipIndex++)
		{
			const ESkeletonAnimationClip& Clip = AnimationClips[ClipIndex];
			TotalCurveNum += Clip.AnimationCurves.Num<UINT32>();
			for (INT32 CurveIndex = 0, CurveNum = Clip.AnimationCurves.Num(); CurveIndex < CurveNum; CurveIndex++)
			{
				TotalKeyNum += GetCurveKeyCount(Clip.AnimationCurves[CurveIndex]);
			}
		}

		ImGui::Text("Asset Name: %s", *InAsset->GetAssetName());
		ImGui::Text("Asset Path: %s", *InAsset->GetAssetPath());
		ImGui::Text("Animation Name: %s", *InAnimation->GetAnimationName());
		ImGui::Text("Clip Count: %d", InAnimation->GetAnimationClipNum());
		ImGui::Text("Curve Count: %u", TotalCurveNum);
		ImGui::Text("Key Count: %u", TotalKeyNum);
		ImGui::Text("Resource Valid: %s", InAnimation->IsResourceValid() ? "True" : "False");
	}

	void EAnimationEditor::DrawClipList(const ESkeletonAnimation* InAnimation)
	{
		const TArray<ESkeletonAnimationClip>& AnimationClips = InAnimation->GetAnimationClips();
		const INT32 ClipNum = AnimationClips.Num();
		m_SelectedClipIndex = ClampSelectionIndex(m_SelectedClipIndex, ClipNum);

		ImGui::Text("Clips");
		ImGui::Separator();
		for (INT32 ClipIndex = 0; ClipIndex < ClipNum; ClipIndex++)
		{
			const ESkeletonAnimationClip& Clip = AnimationClips[ClipIndex];
			const BOOL8 bIsSelected = (m_SelectedClipIndex == ClipIndex);
			ImGui::PushID(ClipIndex);
			if (ImGui::Selectable(*Clip.ClipName, bIsSelected))
			{
				m_SelectedClipIndex = ClipIndex;
				m_SelectedCurveIndex = 0;
			}
			ImGui::PopID();
		}
	}

	void EAnimationEditor::DrawClipDetails(const ESkeletonAnimationClip& InClip)
	{
		UINT32 TotalKeyNum = 0u;
		for (INT32 CurveIndex = 0, CurveNum = InClip.AnimationCurves.Num(); CurveIndex < CurveNum; CurveIndex++)
		{
			TotalKeyNum += GetCurveKeyCount(InClip.AnimationCurves[CurveIndex]);
		}

		ImGui::Text("Clip Name: %s", *InClip.ClipName);
		ImGui::Text("Duration: %.3f", InClip.Duration);
		ImGui::Text("Ticks Per Second: %.3f", InClip.TicksPerSecond);
		ImGui::Text("Length (Seconds): %.3f", InClip.TicksPerSecond > 0.0f ? (InClip.Duration / InClip.TicksPerSecond) : 0.0f);
		ImGui::Text("Curve Count: %d", InClip.AnimationCurves.Num());
		ImGui::Text("Key Count: %u", TotalKeyNum);
	}

	void EAnimationEditor::DrawCurveList(const ESkeletonAnimationClip& InClip)
	{
		const INT32 CurveNum = InClip.AnimationCurves.Num();
		m_SelectedCurveIndex = ClampSelectionIndex(m_SelectedCurveIndex, CurveNum);

		ImGui::Text("Bone Curves");
		ImGui::Separator();
		for (INT32 CurveIndex = 0; CurveIndex < CurveNum; CurveIndex++)
		{
			const EBoneAnimationCurve& Curve = InClip.AnimationCurves[CurveIndex];
			const BOOL8 bIsSelected = (m_SelectedCurveIndex == CurveIndex);
			ImGui::PushID(CurveIndex);
			if (ImGui::Selectable(*Curve.BoneName, bIsSelected))
			{
				m_SelectedCurveIndex = CurveIndex;
			}
			ImGui::PopID();
		}
	}

	void EAnimationEditor::DrawCurveDetails(const EBoneAnimationCurve& InCurve) const
	{
		FLOAT TimeStart = 0.0f;
		FLOAT TimeEnd = 0.0f;

		ImGui::Text("Bone Name: %s", *InCurve.BoneName);
		ImGui::Text("Pre State: %s", AnimationBehaviourTypeAsString(InCurve.PreState));
		ImGui::Text("Post State: %s", AnimationBehaviourTypeAsString(InCurve.PostState));
		ImGui::Text("Position Keys: %d", InCurve.PositionKeys.Num());
		if (TryGetKeyTimeRange(InCurve.PositionKeys, TimeStart, TimeEnd))
		{
			ImGui::Text("Position Range: %.3f -> %.3f", TimeStart, TimeEnd);
		}
		ImGui::Text("Rotation Keys: %d", InCurve.RotationKeys.Num());
		if (TryGetKeyTimeRange(InCurve.RotationKeys, TimeStart, TimeEnd))
		{
			ImGui::Text("Rotation Range: %.3f -> %.3f", TimeStart, TimeEnd);
		}
		ImGui::Text("Scaling Keys: %d", InCurve.ScalingKeys.Num());
		if (TryGetKeyTimeRange(InCurve.ScalingKeys, TimeStart, TimeEnd))
		{
			ImGui::Text("Scaling Range: %.3f -> %.3f", TimeStart, TimeEnd);
		}
		ImGui::Text("Total Keys: %u", GetCurveKeyCount(InCurve));
	}
}

#endif