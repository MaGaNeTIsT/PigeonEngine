#include <Editor/Manager/EditorSettings.h>

#if ENGINE_EDITOR_ONLY

#include <IO/JsonConfigFile.h>

namespace PigeonEngine
{
	namespace
	{
		EJsonConfigFile& Config()
		{
			static EJsonConfigFile Instance(PE_TEXT("EditorSettings"));
			return Instance;
		}
	}

	namespace EEditorSettings
	{
		void Initialize()
		{
			EJsonConfigFile& Settings = Config();
			// Schema defaults are the engine's startup values; EditorSettings.default.json can
			// change them without a rebuild, and EditorSettings.user.json keeps only overrides.
			Settings.AddBool(PE_TEXT("Panels.Level"), FALSE);
			Settings.AddBool(PE_TEXT("Panels.Navigation"), FALSE);
			Settings.AddBool(PE_TEXT("Panels.ContentBrowser"), TRUE);
			Settings.AddBool(PE_TEXT("Panels.OutputLog"), TRUE);
			Settings.AddBool(PE_TEXT("Display.ShowCollision"), TRUE);
			Settings.AddBool(PE_TEXT("Display.ShowDebugWireframe"), FALSE);
			Settings.AddBool(PE_TEXT("Display.ShowNavigation"), TRUE);
			Settings.AddBool(PE_TEXT("Display.FullNavMeshPolygons"), FALSE);
			Settings.AddBool(PE_TEXT("Display.ShowNavMeshTriangulation"), TRUE);
			Settings.Load();
		}

		BOOL32 IsLevelPanelVisible()
		{
			return Config().GetBool(PE_TEXT("Panels.Level"));
		}

		void SetLevelPanelVisible(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Panels.Level"), bInVisible);
		}

		BOOL32 IsNavigationPanelVisible()
		{
			return Config().GetBool(PE_TEXT("Panels.Navigation"));
		}

		void SetNavigationPanelVisible(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Panels.Navigation"), bInVisible);
		}

		BOOL32 IsContentBrowserPanelVisible()
		{
			return Config().GetBool(PE_TEXT("Panels.ContentBrowser"));
		}

		void SetContentBrowserPanelVisible(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Panels.ContentBrowser"), bInVisible);
		}

		BOOL32 IsOutputLogPanelVisible()
		{
			return Config().GetBool(PE_TEXT("Panels.OutputLog"));
		}

		void SetOutputLogPanelVisible(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Panels.OutputLog"), bInVisible);
		}

		BOOL32 GetShowCollision()
		{
			return Config().GetBool(PE_TEXT("Display.ShowCollision"));
		}

		void SetShowCollision(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Display.ShowCollision"), bInVisible);
		}

		BOOL32 GetShowDebugWireframe()
		{
			return Config().GetBool(PE_TEXT("Display.ShowDebugWireframe"));
		}

		void SetShowDebugWireframe(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Display.ShowDebugWireframe"), bInVisible);
		}

		BOOL32 GetShowNavigation()
		{
			return Config().GetBool(PE_TEXT("Display.ShowNavigation"));
		}

		void SetShowNavigation(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Display.ShowNavigation"), bInVisible);
		}

		BOOL32 GetFullNavMeshPolygons()
		{
			return Config().GetBool(PE_TEXT("Display.FullNavMeshPolygons"));
		}

		void SetFullNavMeshPolygons(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Display.FullNavMeshPolygons"), bInVisible);
		}

		BOOL32 GetShowNavMeshTriangulation()
		{
			return Config().GetBool(PE_TEXT("Display.ShowNavMeshTriangulation"));
		}

		void SetShowNavMeshTriangulation(const BOOL32& bInVisible)
		{
			Config().SetBool(PE_TEXT("Display.ShowNavMeshTriangulation"), bInVisible);
		}
	}
}

#endif
