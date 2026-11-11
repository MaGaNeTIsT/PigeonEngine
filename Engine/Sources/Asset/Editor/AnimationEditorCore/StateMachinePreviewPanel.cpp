#include "StateMachinePreviewPanel.h"

#if _EDITOR_ONLY

#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <IO/FileHelper.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <imgui.h>

namespace PigeonEngine
{

	namespace
	{
		EString NormalizeAssetDirectory(const EString& InPath)
		{
			EString Result = InPath.Replace("\\", "/");
			if ((Result.Length() > 0u) && (Result[Result.LastIndex()] != '/'))
			{
				Result += "/";
			}
			return Result;
		}

		BOOL32 ReadAssetTypeFromAssetFile(const EString& InAssetFullPath, EAssetType& OutAssetType)
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

		BOOL32 ReadSkinnedMeshTypeFromAssetFile(const EString& InAssetFullPath)
		{
			void* HeaderData = nullptr;
			if (!(EFileHelper::ReadFirstNumberOfBytesInFile(InAssetFullPath, HeaderData
				, sizeof(UINT32) * 2u)) || (!HeaderData))
			{
				return FALSE;
			}
			const UINT32* HeaderWords = static_cast<const UINT32*>(HeaderData);
			const BOOL32 bSkin = ((HeaderWords[0] == EAssetType::ASSET_TYPE_MESH)
				&& (HeaderWords[1] == EMeshType::MESH_TYPE_SKIN)) ? TRUE : FALSE;
			delete[] static_cast<CHAR*>(HeaderData);
			return bSkin;
		}
	}

	FStateMachinePreviewPanel::FStateMachinePreviewPanel()
	{
		// Register the preview world/renderer during editor construction, before the frame
		// loop starts. Lazy initialization from a panel's Draw registered a renderer while
		// the current frame was already being recorded.
		m_PreviewScene.Initialize("StateMachinePreview", Vector3::Zero());
		m_bInitialized = TRUE;
	}

	FStateMachinePreviewPanel::~FStateMachinePreviewPanel()
	{
		if (m_bInitialized)
		{
			m_PreviewScene.Shutdown();
		}
	}

	void FStateMachinePreviewPanel::EnsureInitialized()
	{
		if (m_bInitialized != FALSE)
		{
			return;
		}
		m_PreviewScene.Initialize("StateMachinePreview", Vector3::Zero());
		m_bInitialized = TRUE;
	}

	void FStateMachinePreviewPanel::RefreshAssetOptions(const EString& InPreferredSkeleton)
	{
		m_MeshPaths.Empty();
		m_SkeletonPaths.Empty();

		TArray<EString> Folders;
		TArray<EString> Files;
		EFileHelper::ScanDirectory(NormalizeAssetDirectory(EBaseSettings::ENGINE_MESH_PATH)
			, Folders, Files, TRUE);
		for (INT32 FileIndex = 0; FileIndex < Files.Num(); FileIndex++)
		{
			const EString Path = Files[FileIndex].Replace("\\", "/");
			if ((!Path.Contains(EEngineSettings::ENGINE_ASSET_NAME_TYPE)) || (!ReadSkinnedMeshTypeFromAssetFile(Path)))
			{
				continue;
			}
			m_MeshPaths.Add(Path);
		}
		Files.Empty();
		EFileHelper::ScanDirectory(NormalizeAssetDirectory(EBaseSettings::ENGINE_SKELETON_PATH)
			, Folders, Files, TRUE);
		for (INT32 FileIndex = 0; FileIndex < Files.Num(); FileIndex++)
		{
			const EString Path = Files[FileIndex].Replace("\\", "/");
			if (!Path.Contains(EEngineSettings::ENGINE_ASSET_NAME_TYPE))
			{
				continue;
			}
			EAssetType AssetType = EAssetType::ASSET_TYPE_UNKNOWN;
			if ((ReadAssetTypeFromAssetFile(Path, AssetType)) && (AssetType == EAssetType::ASSET_TYPE_SKELETON))
			{
				m_SkeletonPaths.Add(Path);
			}
		}

		m_SelectedMeshIndex = ((m_MeshPaths.Num() > 0) ? 0 : -1);
		m_SelectedSkeletonIndex = ((m_SkeletonPaths.Num() > 0) ? 0 : -1);
		if ((InPreferredSkeleton.Length() > 0u) && (m_SkeletonPaths.Num() > 0))
		{
			for (INT32 SkeletonIndex = 0; SkeletonIndex < m_SkeletonPaths.Num(); SkeletonIndex++)
			{
				if (EPath::GetFileNameWithoutExtension(m_SkeletonPaths[SkeletonIndex]) == InPreferredSkeleton)
				{
					m_SelectedSkeletonIndex = SkeletonIndex;
					break;
				}
			}
		}
		LoadPreviewAssets();
	}

	BOOL32 FStateMachinePreviewPanel::LoadPreviewAssets()
	{
		m_MeshAsset = nullptr;
		m_SkeletonAsset = nullptr;
		if ((m_SelectedMeshIndex >= 0) && (m_SelectedMeshIndex < m_MeshPaths.Num()))
		{
			const EString& Path = m_MeshPaths[m_SelectedMeshIndex];
			const EString Name = EPath::GetFileNameWithoutExtension(Path);
			const EString Directory = NormalizeAssetDirectory(EPath::GetFileFolderPath(Path));
			EMeshAssetManager::GetManagerSingleton()->LoadSkinnedMeshAsset(Directory, Name, m_MeshAsset);
		}
		if ((m_SelectedSkeletonIndex >= 0) && (m_SelectedSkeletonIndex < m_SkeletonPaths.Num()))
		{
			const EString& Path = m_SkeletonPaths[m_SelectedSkeletonIndex];
			const EString Name = EPath::GetFileNameWithoutExtension(Path);
			const EString Directory = NormalizeAssetDirectory(EPath::GetFileFolderPath(Path));
			ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(Directory, Name, m_SkeletonAsset);
		}
		m_BoundMeshAsset = nullptr;
		m_BoundSkeletonAsset = nullptr;
		m_BoundAnimationAsset = nullptr;
		m_BoundMachine = nullptr;
		return ((m_MeshAsset && m_SkeletonAsset) ? TRUE : FALSE);
	}

	void FStateMachinePreviewPanel::Draw(FStateMachineDocument& InDocument)
	{
		EnsureInitialized();
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (!Machine)
		{
			ImGui::TextDisabled("Fix the draft before previewing it.");
			return;
		}
		if ((m_LastDocument != &InDocument) || (m_LastSkeletonName != Machine->SkeletonName))
		{
			m_LastDocument = &InDocument;
			m_LastSkeletonName = Machine->SkeletonName;
			RefreshAssetOptions(Machine->SkeletonName);
		}

		if (ImGui::BeginChild("##sm_preview_controls", ImVec2(0.0f, 190.0f), true))
		{
			DrawControls(InDocument);
			ImGui::Separator();
			DrawParameters(InDocument);
		}
		ImGui::EndChild();

		RebindPreview(InDocument);
		const ImVec2 Available = ImGui::GetContentRegionAvail();
		const ImVec2 ImageSize(EMath::Max(Available.x, 64.0f), EMath::Max(Available.y, 64.0f));
		m_PreviewScene.UpdateViewportSize(static_cast<UINT32>(ImageSize.x), static_cast<UINT32>(ImageSize.y));
		m_PreviewScene.Tick(ImGui::GetIO().DeltaTime);
		const ImTextureID TextureId = m_PreviewScene.GetTextureId();
		if (TextureId)
		{
			ImGui::Image(TextureId, ImageSize);
		}
		else
		{
			ImGui::Dummy(ImageSize);
			ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMin().x + 12.0f, ImGui::GetItemRectMin().y + 12.0f));
			ImGui::TextDisabled("Select a preview mesh and skeleton.");
		}
	}

	void FStateMachinePreviewPanel::RebindPreview(FStateMachineDocument& InDocument)
	{
		if ((m_BoundMeshAsset != m_MeshAsset) || (m_BoundSkeletonAsset != m_SkeletonAsset))
		{
			m_PreviewScene.SetSkinnedMesh(m_MeshAsset, m_SkeletonAsset);
			m_BoundMeshAsset = m_MeshAsset;
			m_BoundSkeletonAsset = m_SkeletonAsset;
			m_BoundAnimationAsset = nullptr;
			m_BoundMachine = nullptr;
		}
		const ESkeletonAnimationAsset* Animation = InDocument.GetAnimationAsset();
		if (m_BoundAnimationAsset != Animation)
		{
			m_PreviewScene.SetAnimationAsset(Animation, -1);
			m_BoundAnimationAsset = Animation;
			m_BoundMachine = nullptr;
		}
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (m_BoundMachine != Machine)
		{
			m_PreviewScene.SetStateMachine(Machine);
			m_BoundMachine = Machine;
		}
		m_PreviewScene.SetPlayAnimation(m_bPlay);
		m_PreviewScene.SetPlaybackRate(m_PlayRate);
		m_PreviewScene.SetAutoRotate(m_bAutoRotate);
	}

	void FStateMachinePreviewPanel::DrawControls(FStateMachineDocument& InDocument)
	{
		ImGui::TextDisabled("Preview");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##preview_mesh"
			, ((m_SelectedMeshIndex >= 0) && (m_SelectedMeshIndex < m_MeshPaths.Num()))
				? *EPath::GetFileNameWithoutExtension(m_MeshPaths[m_SelectedMeshIndex]) : "None"))
		{
			for (INT32 MeshIndex = 0; MeshIndex < m_MeshPaths.Num(); MeshIndex++)
			{
				const BOOL8 bSelected = (MeshIndex == m_SelectedMeshIndex);
				if (ImGui::Selectable(*EPath::GetFileNameWithoutExtension(m_MeshPaths[MeshIndex]), bSelected))
				{
					m_SelectedMeshIndex = MeshIndex;
					LoadPreviewAssets();
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##preview_skeleton"
			, ((m_SelectedSkeletonIndex >= 0) && (m_SelectedSkeletonIndex < m_SkeletonPaths.Num()))
				? *EPath::GetFileNameWithoutExtension(m_SkeletonPaths[m_SelectedSkeletonIndex]) : "None"))
		{
			for (INT32 SkeletonIndex = 0; SkeletonIndex < m_SkeletonPaths.Num(); SkeletonIndex++)
			{
				const BOOL8 bSelected = (SkeletonIndex == m_SelectedSkeletonIndex);
				if (ImGui::Selectable(*EPath::GetFileNameWithoutExtension(m_SkeletonPaths[SkeletonIndex]), bSelected))
				{
					m_SelectedSkeletonIndex = SkeletonIndex;
					LoadPreviewAssets();
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Checkbox("Play", &m_bPlay);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::SliderFloat("##preview_rate", &m_PlayRate, 0.0f, 4.0f, "Rate %.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SameLine();
		ImGui::Checkbox("Auto Rotate", &m_bAutoRotate);
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			m_PreviewScene.ResetPlayback();
		}

		const EAnimStateMachineInstance* Instance = m_PreviewScene.GetStateMachineInstance();
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if ((Instance) && (Machine) && (Instance->GetCurrentStateIndex() >= 0)
			&& (Instance->GetCurrentStateIndex() < Machine->States.Num()))
		{
			ImGui::TextDisabled("Current state: %s"
				, *Machine->States[Instance->GetCurrentStateIndex()].Name);
		}
		else
		{
			ImGui::TextDisabled("Current state: (not bound)");
		}
	}

	void FStateMachinePreviewPanel::DrawParameters(FStateMachineDocument& InDocument)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		EAnimStateMachineInstance* Instance = m_PreviewScene.GetStateMachineInstance();
		if ((!Machine) || (!Instance))
		{
			ImGui::TextDisabled("Bind the preview to inject parameters.");
			return;
		}
		const ImGuiTableFlags TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV
			| ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("##preview_parameters", 2, TableFlags))
		{
			ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableHeadersRow();
			for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
			{
				const EAnimStateMachineParameter& Parameter = Machine->Parameters[ParameterIndex];
				ImGui::PushID(ParameterIndex);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(*Parameter.Name);
				ImGui::TableSetColumnIndex(1);
				if (Parameter.Type == ANIM_PARAM_FLOAT)
				{
					FLOAT Value = Instance->GetFloat(Parameter.Name);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::InputFloat("##value", &Value, 0.0f, 0.0f, "%.3f"))
					{
						Instance->SetFloat(Parameter.Name, Value);
					}
				}
				else if (Parameter.Type == ANIM_PARAM_BOOL)
				{
					bool bValue = (Instance->GetBool(Parameter.Name) != FALSE);
					if (ImGui::Checkbox("##value", &bValue))
					{
						Instance->SetBool(Parameter.Name, (bValue ? TRUE : FALSE));
					}
				}
				else
				{
					if (ImGui::Button("Trigger"))
					{
						Instance->SetTrigger(Parameter.Name);
					}
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

}

#endif
