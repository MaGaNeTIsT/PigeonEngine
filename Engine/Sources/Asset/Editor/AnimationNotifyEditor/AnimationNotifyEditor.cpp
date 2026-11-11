#include "AnimationNotifyEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationAsset/AnimationAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <Base/DataStructure/Text/Path.h>

namespace PigeonEngine
{

	namespace
	{
		constexpr INT32 NotifyEditorBufferHeadroom = 8 * 1024;
	}

	void EAnimNotifyEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastError = EString();
		m_JsonBuffer.Empty();
	}

	void EAnimNotifyEditor::OpenNotifyAsset(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		if (m_Document.bDirty)
		{
			if (NormalizedAssetPath == m_OpenedAssetFullPath)
			{
				m_Document.bReloadPending = TRUE;
			}
			else
			{
				m_Document.bOpenPending = TRUE;
				m_Document.PendingOpenPath = InAssetFullPath;
			}
			return;
		}
		OpenNotifyAssetNow(InAssetFullPath);
	}

	void EAnimNotifyEditor::OpenNotifyAssetNow(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		m_LastError = EString();

		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedAssetPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedAssetPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			m_LastError = EString("Notify editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const EAnimNotifyAsset* LoadedAsset = nullptr;
		if (!(EAnimNotifyManager::GetManagerSingleton()->LoadNotifyAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			m_LastError = EString("Notify editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		m_Asset = LoadedAsset;
		m_OpenedAssetFullPath = NormalizedAssetPath;
		LoadJsonBufferFromAsset();
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Loaded.");
		m_bStatusSucceeded = TRUE;
	}

	void EAnimNotifyEditor::CaptureBaseline()
	{
		m_BaselineJson = EString();
		const EAnimNotifyDefinition* Definition = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (Definition)
		{
			m_BaselineJson = Definition->SourceJson;
		}
	}

	BOOL32 EAnimNotifyEditor::DiscardChanges()
	{
		if (m_Asset && (m_BaselineJson.Length() > 0u))
		{
			EString Error;
			if (!EAnimNotifyManager::GetManagerSingleton()->RebuildNotifyAsset(m_Asset, m_BaselineJson, Error))
			{
				m_LastError = Error;
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
				return FALSE;
			}
			EAnimationManager::GetManagerSingleton()->RefreshNotifyReferences();
			EAnimMontageManager::GetManagerSingleton()->RefreshNotifyReferences();
		}
		m_JsonBuffer.Empty();
		const EString Baseline = m_BaselineJson;
		if (Baseline.Length() > 0u)
		{
			const INT32 BufferLength = static_cast<INT32>(Baseline.Length()) + NotifyEditorBufferHeadroom + 1;
			m_JsonBuffer.SetNum(BufferLength);
			::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *Baseline, Baseline.Length());
		}
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Discarded unsaved changes.");
		m_bStatusSucceeded = TRUE;
		return TRUE;
	}

	BOOL32 EAnimNotifyEditor::SaveNotifyAsset(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No notify asset is open.");
			return FALSE;
		}
		const EString JsonText(m_JsonBuffer.GetData());
		if (!(EAnimNotifyManager::GetManagerSingleton()->SaveNotifyAsset(m_Asset, JsonText, OutError)))
		{
			return FALSE;
		}
		EAnimationManager::GetManagerSingleton()->RefreshNotifyReferences();
		EAnimMontageManager::GetManagerSingleton()->RefreshNotifyReferences();
		m_BaselineJson = JsonText;
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	BOOL32 EAnimNotifyEditor::ReloadNotifyAssetNow(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No notify asset is open.");
			return FALSE;
		}

		const EAnimNotifyAsset* FreshAsset = nullptr;
		if (!(EAnimNotifyManager::GetManagerSingleton()->ReloadNotifyAsset(
			m_Asset->GetAssetPath(), m_Asset->GetAssetName(), FreshAsset, OutError)) || (!FreshAsset))
		{
			return FALSE;
		}

		m_Asset = FreshAsset;
		LoadJsonBufferFromAsset();
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	void EAnimNotifyEditor::HandleDocumentAction()
	{
		const EEditorUnsavedChoiceType Choice = DrawEditorUnsavedChangesPopup("##notify_editor_unsaved"
			, (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : nullptr), m_Document);
		if (Choice == EEditorUnsavedChoiceType::NONE)
		{
			return;
		}

		const BOOL8 bWasClose = m_Document.bClosePending;
		const BOOL8 bWasOpen = m_Document.bOpenPending;
		const BOOL8 bWasReload = m_Document.bReloadPending;
		const EString PendingPath = m_Document.PendingOpenPath;

		if (Choice == EEditorUnsavedChoiceType::CANCEL)
		{
			m_Document.ClearPendingAction();
			return;
		}
		if (Choice == EEditorUnsavedChoiceType::SAVE)
		{
			EString SaveError;
			if (!SaveNotifyAsset(SaveError))
			{
				m_LastError = SaveError;
				m_StatusMessage = SaveError;
				m_bStatusSucceeded = FALSE;
				return;
			}
		}
		else
		{
			if (!DiscardChanges())
			{
				return;
			}
		}

		m_Document.ClearPendingAction();
		if (bWasClose)
		{
			m_ShowEditor = FALSE;
			return;
		}
		if (bWasReload)
		{
			EString ReloadError;
			if (!ReloadNotifyAssetNow(ReloadError))
			{
				m_LastError = ReloadError;
				m_StatusMessage = ReloadError;
				m_bStatusSucceeded = FALSE;
			}
			else
			{
				m_StatusMessage = EString("Reloaded from disk.");
				m_bStatusSucceeded = TRUE;
			}
			return;
		}
		if (bWasOpen)
		{
			OpenNotifyAssetNow(PendingPath);
		}
	}

	void EAnimNotifyEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		HandleDocumentAction();
		if (!m_ShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(760.0f, 620.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(480.0f, 60.0f), ImGuiCond_FirstUseEver);
		bool bWindowOpen = true;
		ImGui::Begin("Notify Editor", &bWindowOpen, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				if (m_Document.bDirty)
				{
					m_Document.bReloadPending = TRUE;
				}
				else
				{
					EString ReloadError;
					if (!ReloadNotifyAssetNow(ReloadError))
					{
						m_LastError = ReloadError;
						m_StatusMessage = ReloadError;
						m_bStatusSucceeded = FALSE;
					}
					else
					{
						m_StatusMessage = EString("Reloaded from disk.");
						m_bStatusSucceeded = TRUE;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Apply"))
			{
				if (m_Asset)
				{
					EString Error;
					const EString JsonText(m_JsonBuffer.GetData());
					if (EAnimNotifyManager::GetManagerSingleton()->RebuildNotifyAsset(m_Asset, JsonText, Error))
					{
						m_LastError = EString();
						m_Document.bDirty = TRUE;
						m_StatusMessage = EString("Applied to the live asset. Save to write it to disk.");
						m_bStatusSucceeded = TRUE;
						// Re-apply the definition to loaded clips/montages in place.
						EAnimationManager::GetManagerSingleton()->RefreshNotifyReferences();
						EAnimMontageManager::GetManagerSingleton()->RefreshNotifyReferences();
					}
					else
					{
						m_LastError = Error;
						m_StatusMessage = Error;
						m_bStatusSucceeded = FALSE;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					EString Error;
					if (SaveNotifyAsset(Error))
					{
						m_LastError = EString();
						m_StatusMessage = EString("Saved.");
						m_bStatusSucceeded = TRUE;
					}
					else
					{
						m_LastError = Error;
						m_StatusMessage = Error;
						m_bStatusSucceeded = FALSE;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No notify selected."));
			if (m_Document.bDirty)
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "UNSAVED");
			}

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
			}
			if (m_StatusMessage.Length() > 0u)
			{
				const ImVec4 StatusColor = (m_bStatusSucceeded != FALSE)
					? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
					: ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
				ImGui::TextColored(StatusColor, "%s", *m_StatusMessage);
			}

			ImGui::Separator();
			if (m_Asset)
			{
				const EAnimNotifyDefinition* Definition = m_Asset->GetStoragedResource();
				if (Definition)
				{
					ImGui::Text("Name: %s", *Definition->Name);
					if (Definition->Description.Length() > 0u)
					{
						ImGui::TextWrapped("Description: %s", *Definition->Description);
					}
					ImGui::Text("Class: %s", *Definition->NotifyClassName);
					ImGui::Text("Kind: %s", (Definition->bIsState ? "State" : "Instant"));
					if (Definition->bIsState)
					{
						ImGui::Text("Default duration: %.3fs", Definition->DefaultDuration);
					}
					ImGui::Text("Payload: %s", *Definition->PayloadJson);
				}
			}

			if (m_JsonBuffer.Num() > 0)
			{
				if (ImGui::InputTextMultiline("JSON", m_JsonBuffer.GetData(), static_cast<int>(m_JsonBuffer.Num())
					, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput))
				{
					m_Document.bDirty = TRUE;
				}
			}
		}
		ImGui::End();

		if (!bWindowOpen)
		{
			if (m_Document.bDirty)
			{
				m_Document.bClosePending = TRUE;
			}
			else
			{
				m_ShowEditor = FALSE;
			}
		}
	}

	void EAnimNotifyEditor::LoadJsonBufferFromAsset()
	{
		m_JsonBuffer.Empty();
		if (!m_Asset)
		{
			return;
		}
		const EAnimNotifyDefinition* Definition = m_Asset->GetStoragedResource();
		if (!Definition)
		{
			return;
		}

		const EString& SourceJson = Definition->SourceJson;
		const INT32 BufferLength = static_cast<INT32>(SourceJson.Length()) + NotifyEditorBufferHeadroom + 1;
		m_JsonBuffer.SetNum(BufferLength);
		::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
		if (SourceJson.Length() > 0u)
		{
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *SourceJson, SourceJson.Length());
		}
	}

}

#endif
