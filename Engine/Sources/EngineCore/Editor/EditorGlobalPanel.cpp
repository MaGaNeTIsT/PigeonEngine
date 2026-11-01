#include "EditorGlobalPanel.h"

#if _EDITOR_ONLY

#include <Base/RTTI/RTTIManager.h>
#include <imgui.h>
#include <PhysicsConfig/PhysicsConfig.h>
#include <RenderProxy/RenderSingletonObject.h>
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

	void EEditorGlobalPanel::DrawDisplaySettings()
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
			if (SelectedResolutionIndex >= ResolutionCount)
			{
				SelectedResolutionIndex = ResolutionCount - 1;
			}

			const RResolutionItem& CurrentItem = Resolutions[SelectedResolutionIndex];
			char PreviewBuffer[32];
			::sprintf_s(PreviewBuffer, sizeof(PreviewBuffer), "%ux%u", CurrentItem.Width, CurrentItem.Height);

			if (ImGui::BeginCombo("Resolution", PreviewBuffer))
			{
				for (INT32 Index = 0; Index < ResolutionCount; ++Index)
				{
					const RResolutionItem& Item = Resolutions[Index];
					char ItemBuffer[32];
					::sprintf_s(ItemBuffer, sizeof(ItemBuffer), "%ux%u", Item.Width, Item.Height);

					const BOOL32 bSelected = (Index == SelectedResolutionIndex);
					if (ImGui::Selectable(ItemBuffer, bSelected != FALSE))
					{
						SelectedResolutionIndex = Index;
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

		bool bFullscreen = (bIsFullscreen != FALSE);
		ImGui::Checkbox("Fullscreen (Borderless)", &bFullscreen);
		bIsFullscreen = bFullscreen ? TRUE : FALSE;

		ImGui::Separator();

		// The collision shapes a physics body draws are queued as debug wireframe primitives of
		// their own, so this decides whether they are queued at all. Off by default: a scene full
		// of bodies otherwise fills the view with boxes.
		FPhysicsCommonSettings* PhysicsSettings = FPhysicsCommonSettings::GetSingleton();
		bool bShowCollision = (PhysicsSettings->bDrawCollision != FALSE);
		if (ImGui::Checkbox("Show Collision", &bShowCollision))
		{
			PhysicsSettings->bDrawCollision = bShowCollision ? TRUE : FALSE;
		}

		// Everything the debug wireframe manager owns - the built in primitive preview, the
		// selection gizmo, object bounds and the collision shapes when enabled - is one overlay,
		// so one checkbox hides all of it.
		RDebugWireframePrimitiveManager* DebugWireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		bool bShowDebugWireframe = (DebugWireframeManager->IsDebugWireframeVisible() != FALSE);
		if (ImGui::Checkbox("Show Debug Wireframe", &bShowDebugWireframe))
		{
			DebugWireframeManager->SetDebugWireframeVisible(bShowDebugWireframe ? TRUE : FALSE);
		}

		if (ImGui::Button("Apply") && ResolutionCount > 0)
		{
			EMainManager* Manager = EMainManager::GetManagerSingleton();
			if (Manager)
			{
				const RResolutionItem& Selected = Resolutions[SelectedResolutionIndex];
				Manager->ApplyResolution(Selected.Width, Selected.Height, bIsFullscreen);
			}
		}

		ImGui::End();
	}
}

#endif
