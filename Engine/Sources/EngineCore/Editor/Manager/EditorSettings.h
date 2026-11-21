#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	/*
	* Editor preferences that outlive a session. They live in
	* Engine/Configs/EditorSettings.json next to the ImGui layout file; a missing or invalid
	* file keeps the compiled defaults and is rewritten, so deleting it resets the editor.
	*
	* Only user-facing editor state belongs here: optional panel visibility and the Display
	* Settings switches. Project data (levels, assets, navmesh profiles) never does.
	*/
	namespace EEditorSettings
	{
		// Loads the file and creates it with defaults when it is missing or invalid.
		void Initialize();
		void Save();

		// Optional panels. A missing file keeps both closed on a fresh checkout.
		PE_NODISCARD BOOL32 IsLevelPanelVisible();
		void SetLevelPanelVisible(const BOOL32& bInVisible);
		PE_NODISCARD BOOL32 IsNavigationPanelVisible();
		void SetNavigationPanelVisible(const BOOL32& bInVisible);

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
