#include "AnimationNotifyEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

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
		m_Asset = nullptr;
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
	}

	void EAnimNotifyEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(760.0f, 620.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(480.0f, 60.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Notify Editor", &m_ShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				OpenNotifyAsset(m_OpenedAssetFullPath);
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
					}
					else
					{
						m_LastError = Error;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					EString Error;
					const EString JsonText(m_JsonBuffer.GetData());
					if (EAnimNotifyManager::GetManagerSingleton()->SaveNotifyAsset(m_Asset, JsonText, Error))
					{
						m_LastError = EString();
					}
					else
					{
						m_LastError = Error;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No notify selected."));

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
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
				ImGui::InputTextMultiline("JSON", m_JsonBuffer.GetData(), static_cast<int>(m_JsonBuffer.Num())
					, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
			}
		}
		ImGui::End();
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
