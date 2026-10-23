#include "AnimationStateMachineEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <Base/DataStructure/Text/Path.h>

namespace PigeonEngine
{

	namespace
	{
		// The editor gives the author room to rewrite the JSON without reallocating the
		// ImGui buffer every frame.
		constexpr INT32 StateMachineEditorBufferHeadroom = 16 * 1024;
	}

	void EAnimStateMachineEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastError = EString();
		m_JsonBuffer.Empty();
	}

	void EAnimStateMachineEditor::OpenStateMachineAsset(const EString& InAssetFullPath)
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
			m_LastError = EString("State machine editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const EAnimStateMachineAsset* LoadedAsset = nullptr;
		if (!(EAnimStateMachineManager::GetManagerSingleton()->LoadStateMachineAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			m_LastError = EString("State machine editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		m_Asset = LoadedAsset;
		m_OpenedAssetFullPath = NormalizedAssetPath;
		LoadJsonBufferFromAsset();
	}

	void EAnimStateMachineEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(900.0f, 720.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("State Machine Editor", &m_ShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				OpenStateMachineAsset(m_OpenedAssetFullPath);
			}
			ImGui::SameLine();
			if (ImGui::Button("Apply"))
			{
				if (m_Asset)
				{
					EString Error;
					const EString JsonText(m_JsonBuffer.GetData());
					if (EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, JsonText, Error))
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
					if (EAnimStateMachineManager::GetManagerSingleton()->SaveStateMachineAsset(m_Asset, JsonText, Error))
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
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No state machine selected."));

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
			}

			ImGui::Separator();
			if (m_Asset)
			{
				const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
				if (Machine)
				{
					ImGui::Text("Name: %s", *Machine->Name);
					ImGui::Text("Skeleton: %s", *Machine->SkeletonName);
					ImGui::Text("Animation: %s", *Machine->AnimationAssetName);
					ImGui::Text("States: %d  Parameters: %d  Transitions: %d",
						Machine->States.Num(), Machine->Parameters.Num(), Machine->Transitions.Num());
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

	void EAnimStateMachineEditor::LoadJsonBufferFromAsset()
	{
		m_JsonBuffer.Empty();
		if (!m_Asset)
		{
			return;
		}
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if (!Machine)
		{
			return;
		}

		const EString& SourceJson = Machine->SourceJson;
		const INT32 BufferLength = static_cast<INT32>(SourceJson.Length()) + StateMachineEditorBufferHeadroom + 1;
		m_JsonBuffer.SetNum(BufferLength);
		::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
		if (SourceJson.Length() > 0u)
		{
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *SourceJson, SourceJson.Length());
		}
	}

}

#endif
