#include "EditorGlobalPanel.h"

#if _EDITOR_ONLY

#include <Base/RTTI/RTTIManager.h>
#include <imgui.h>
#include "../Main/MainManager.h"

namespace PigeonEngine
{
	static void RegisterEditorGlobalPanelClassTypes()
	{
		RegisterClassType<EEditorGlobalPanel, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterEditorGlobalPanelClassTypes);

	EEditorGlobalPanel::EEditorGlobalPanel()
	{
	}

	EEditorGlobalPanel::~EEditorGlobalPanel()
	{
	}

	void EEditorGlobalPanel::Initialize()
	{
	}

	void EEditorGlobalPanel::ShutDown()
	{
	}

	void EEditorGlobalPanel::DrawResolutionPanel()
	{
		EMainManager* MainManager = EMainManager::GetManagerSingleton();
		if (!MainManager)
		{
			return;
		}

		const TArray<RResolutionItem>& Resolutions = MainManager->GetSupportedResolutions();
		const INT32 ResolutionCount = static_cast<INT32>(Resolutions.Num());

		ImGui::Begin("Display Settings");

		if (ResolutionCount > 0)
		{
			// Clamp index in case resolution list changed
			if (m_SelectedResolutionIndex >= ResolutionCount)
			{
				m_SelectedResolutionIndex = ResolutionCount - 1;
			}

			const RResolutionItem& CurrentItem = Resolutions[m_SelectedResolutionIndex];
			char PreviewBuffer[32];
			::sprintf_s(PreviewBuffer, sizeof(PreviewBuffer), "%ux%u", CurrentItem.Width, CurrentItem.Height);

			if (ImGui::BeginCombo("Resolution", PreviewBuffer))
			{
				for (INT32 Index = 0; Index < ResolutionCount; ++Index)
				{
					const RResolutionItem& Item = Resolutions[Index];
					char ItemBuffer[32];
					::sprintf_s(ItemBuffer, sizeof(ItemBuffer), "%ux%u", Item.Width, Item.Height);

					const BOOL32 bSelected = (Index == m_SelectedResolutionIndex);
					if (ImGui::Selectable(ItemBuffer, bSelected != FALSE))
					{
						m_SelectedResolutionIndex = Index;
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
			ImGui::TextDisabled("No resolutions available");
		}

		bool bFullscreen = (m_bIsFullscreen != FALSE);
		ImGui::Checkbox("Fullscreen (Borderless)", &bFullscreen);
		m_bIsFullscreen = bFullscreen ? TRUE : FALSE;

		if (ImGui::Button("Apply") && ResolutionCount > 0)
		{
			EMainManager* Manager = EMainManager::GetManagerSingleton();
			if (Manager)
			{
				const RResolutionItem& Selected = Resolutions[m_SelectedResolutionIndex];
				Manager->ApplyResolution(Selected.Width, Selected.Height, m_bIsFullscreen);
			}
		}

		ImGui::End();
	}
}

#endif
