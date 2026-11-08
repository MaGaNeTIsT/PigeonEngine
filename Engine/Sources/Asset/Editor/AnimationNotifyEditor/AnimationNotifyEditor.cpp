#include "AnimationNotifyEditor.h"

#if ENGINE_EDITOR_ONLY

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
		bShowEditor = FALSE;
		Asset = nullptr;
		OpenedAssetFullPath = EString();
		LastError = EString();
		JsonBuffer.Empty();
	}

	void EAnimNotifyEditor::OpenNotifyAsset(const EString& InAssetFullPath)
	{
		bShowEditor = TRUE;
		Asset = nullptr;
		LastError = EString();

		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedAssetPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedAssetPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			LastError = EString("Notify editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const EAnimNotifyAsset* LoadedAsset = nullptr;
		if (!(EAnimNotifyManager::GetManagerSingleton()->LoadNotifyAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			LastError = EString("Notify editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		Asset = LoadedAsset;
		OpenedAssetFullPath = NormalizedAssetPath;
		LoadJsonBufferFromAsset();
	}

	void EAnimNotifyEditor::EditorUpdate()
	{
		if (!bShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(760.0f, 620.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(480.0f, 60.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Notify Editor", &bShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (OpenedAssetFullPath.Length() > 0u))
			{
				OpenNotifyAsset(OpenedAssetFullPath);
			}
			ImGui::SameLine();
			if (ImGui::Button("Apply"))
			{
				if (Asset)
				{
					EString Error;
					const EString JsonText(JsonBuffer.GetData());
					if (EAnimNotifyManager::GetManagerSingleton()->RebuildNotifyAsset(Asset, JsonText, Error))
					{
						LastError = EString();
						// Re-apply the definition to loaded clips/montages in place.
						EAnimationManager::GetManagerSingleton()->RefreshNotifyReferences();
						EAnimMontageManager::GetManagerSingleton()->RefreshNotifyReferences();
					}
					else
					{
						LastError = Error;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (Asset)
				{
					EString Error;
					const EString JsonText(JsonBuffer.GetData());
					if (EAnimNotifyManager::GetManagerSingleton()->SaveNotifyAsset(Asset, JsonText, Error))
					{
						LastError = EString();
						EAnimationManager::GetManagerSingleton()->RefreshNotifyReferences();
						EAnimMontageManager::GetManagerSingleton()->RefreshNotifyReferences();
					}
					else
					{
						LastError = Error;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (OpenedAssetFullPath.Length() > 0u ? *OpenedAssetFullPath : "No notify selected."));

			if (LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *LastError);
			}

			ImGui::Separator();
			if (Asset)
			{
				const EAnimNotifyDefinition* Definition = Asset->GetStoragedResource();
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

			if (JsonBuffer.Num() > 0)
			{
				ImGui::InputTextMultiline("JSON", JsonBuffer.GetData(), static_cast<int>(JsonBuffer.Num())
					, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
			}
		}
		ImGui::End();
	}

	void EAnimNotifyEditor::LoadJsonBufferFromAsset()
	{
		JsonBuffer.Empty();
		if (!Asset)
		{
			return;
		}
		const EAnimNotifyDefinition* Definition = Asset->GetStoragedResource();
		if (!Definition)
		{
			return;
		}

		const EString& SourceJson = Definition->SourceJson;
		const INT32 BufferLength = static_cast<INT32>(SourceJson.Length()) + NotifyEditorBufferHeadroom + 1;
		JsonBuffer.SetNum(BufferLength);
		::memset(JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
		if (SourceJson.Length() > 0u)
		{
			::memcpy_s(JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *SourceJson, SourceJson.Length());
		}
	}

}

#endif
