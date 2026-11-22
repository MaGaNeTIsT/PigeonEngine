#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	/*
	* Editor preferences that outlive a session. They live in
	* Engine/Configs/EditorSettings.default.json (tracked template) and
	* Engine/Configs/EditorSettings.user.json (local sparse overrides). A missing file keeps
	* the compiled schema defaults, so deleting the user file resets the editor.
	*
	* Only user-facing editor state belongs here: optional panel visibility and the Display
	* Settings switches. Project data (levels, assets, navmesh profiles) never does.
	*/
	namespace EEditorSettings
	{
		// Loads the default and user files; the user file is written only when a value changes.
		void Initialize();

		// Optional panels. Level and Navigation start closed; the Content Browser and
		// Output Log start visible because they used to be drawn unconditionally.
		PE_NODISCARD BOOL32 IsLevelPanelVisible();
		void SetLevelPanelVisible(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 IsNavigationPanelVisible();
		void SetNavigationPanelVisible(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 IsContentBrowserPanelVisible();
		void SetContentBrowserPanelVisible(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 IsOutputLogPanelVisible();
		void SetOutputLogPanelVisible(const BOOL32& bInVisible);

		// Display Settings switches. Defaults match the engine defaults, so a missing file
		// leaves the first run exactly as before.
		PE_NODISCARD BOOL32 GetShowCollision();
		void SetShowCollision(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 GetShowDebugWireframe();
		void SetShowDebugWireframe(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 GetShowNavigation();
		void SetShowNavigation(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 GetFullNavMeshPolygons();
		void SetFullNavMeshPolygons(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 GetShowNavMeshTriangulation();
		void SetShowNavMeshTriangulation(const BOOL32& bInVisible);
	}
}

#endif
