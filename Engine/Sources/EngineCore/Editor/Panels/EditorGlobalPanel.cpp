#include <Editor/Panels/EditorGlobalPanel.h>

#if ENGINE_EDITOR_ONLY

#include <Base/RTTI/RTTIManager.h>
#include <imgui.h>
#include <PhysicsConfig/PhysicsConfig.h>
#include <RenderProxy/RenderSingletonObject.h>
#include <Navigation/NavigationManager.h>
#include <Editor/Manager/EditorSettings.h>
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

	void EEditorGlobalPanel::ApplySettings()
	{
		FPhysicsCommonSettings::GetSingleton()->bDrawCollision = EEditorSettings::GetShowCollision();
		RDebugWireframePrimitiveManager::GetManagerSingleton()->SetDebugWireframeVisible(EEditorSettings::GetShowDebugWireframe());
		ENavigationManager* NavigationManager = ENavigationManager::GetManagerSingleton();
		NavigationManager->SetDebugDrawEnabled(EEditorSettings::GetShowNavigation());
		NavigationManager->SetDebugDrawMode(EEditorSettings::GetFullNavMeshPolygons()
			? ENavigationDebugModeType::NAVIGATION_DEBUG_FULL
			: ENavigationDebugModeType::NAVIGATION_DEBUG_BOUNDARY);
		NavigationManager->SetDebugTrianglesEnabled(EEditorSettings::GetShowNavMeshTriangulation());
	}

	void EEditorGlobalPanel::DrawDisplaySettingsContents()
	{
		EMainManager* MainManager = EMainManager::GetManagerSingleton();
		if (!MainManager)
		{
			return;
		}

		const TArray<RResolutionItem>& Resolutions = MainManager->GetSupportedResolutions();
		const INT32 ResolutionCount = static_cast<INT32>(Resolutions.Num());

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
		// their own, so this decides whether they are queued at all. The wireframe overlay has
		// its own master switch below; with that off, nothing reaches the render pass even when
		// this is on.
		FPhysicsCommonSettings* PhysicsSettings = FPhysicsCommonSettings::GetSingleton();
		bool bShowCollision = (PhysicsSettings->bDrawCollision != FALSE);
		if (ImGui::Checkbox("Show Collision", &bShowCollision))
		{
			PhysicsSettings->bDrawCollision = bShowCollision ? TRUE : FALSE;
			EEditorSettings::SetShowCollision(PhysicsSettings->bDrawCollision);
		}

		// Everything the debug wireframe manager owns - the built in primitive preview, the
		// selection gizmo, object bounds and the collision shapes when enabled - is one overlay,
		// so one checkbox hides all of it.
		RDebugWireframePrimitiveManager* DebugWireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		bool bShowDebugWireframe = (DebugWireframeManager->IsDebugWireframeVisible() != FALSE);
		if (ImGui::Checkbox("Show Debug Wireframe", &bShowDebugWireframe))
		{
			DebugWireframeManager->SetDebugWireframeVisible(bShowDebugWireframe ? TRUE : FALSE);
			EEditorSettings::SetShowDebugWireframe(bShowDebugWireframe ? TRUE : FALSE);
		}
		if ((PhysicsSettings->bDrawCollision != FALSE) && (DebugWireframeManager->IsDebugWireframeVisible() == FALSE))
		{
			ImGui::TextDisabled("Collision shapes are hidden while Show Debug Wireframe is off.");
		}

		// The navigation overlay is drawn from the manager's cached segment list, so it keeps a
		// switch of its own: the mesh is usually inspected without the bounds and collision
		// overlays the checkbox above controls.
		ENavigationManager* NavigationManager = ENavigationManager::GetManagerSingleton();
		bool bShowNavigation = (NavigationManager->IsDebugDrawEnabled() != FALSE);
		if (ImGui::Checkbox("Show Navigation", &bShowNavigation))
		{
			NavigationManager->SetDebugDrawEnabled(bShowNavigation ? TRUE : FALSE);
			EEditorSettings::SetShowNavigation(bShowNavigation ? TRUE : FALSE);
		}
		if (bShowNavigation)
		{
			bool bFullNavigation = (NavigationManager->GetDebugDrawMode() == ENavigationDebugModeType::NAVIGATION_DEBUG_FULL);
			if (ImGui::Checkbox("  Full NavMesh Polygons", &bFullNavigation))
			{
				NavigationManager->SetDebugDrawMode(bFullNavigation
					? ENavigationDebugModeType::NAVIGATION_DEBUG_FULL
					: ENavigationDebugModeType::NAVIGATION_DEBUG_BOUNDARY);
				EEditorSettings::SetFullNavMeshPolygons(bFullNavigation ? TRUE : FALSE);
			}

			bool bShowTriangles = (NavigationManager->IsDebugTrianglesEnabled() != FALSE);
			if (ImGui::Checkbox("  NavMesh Triangulation", &bShowTriangles))
			{
				NavigationManager->SetDebugTrianglesEnabled(bShowTriangles ? TRUE : FALSE);
				EEditorSettings::SetShowNavMeshTriangulation(bShowTriangles ? TRUE : FALSE);
			}
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
	}
}

#endif
