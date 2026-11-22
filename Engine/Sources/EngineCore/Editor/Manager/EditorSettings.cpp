#include <Editor/Manager/EditorSettings.h>

#if ENGINE_EDITOR_ONLY

#include <Development/EditorLogManager.h>
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

		BOOL32 bInitialized = FALSE;

		EString MakePanelKey(const EString& InId)
		{
			EString Key(PE_TEXT("Panels."));
			Key += InId;
			return Key;
		}
	}

	namespace EEditorSettings
	{
		void Initialize()
		{
			EJsonConfigFile& Settings = Config();
			// Panel keys are declared by EEditorWindowRegistry::RegisterWindow before this
			// call, so their defaults also participate in the default/user layering.
			Settings.AddBool(PE_TEXT("Display.ShowCollision"), TRUE);
			Settings.AddBool(PE_TEXT("Display.ShowDebugWireframe"), FALSE);
			Settings.AddBool(PE_TEXT("Display.ShowNavigation"), TRUE);
			Settings.AddBool(PE_TEXT("Display.FullNavMeshPolygons"), FALSE);
			Settings.AddBool(PE_TEXT("Display.ShowNavMeshTriangulation"), TRUE);
			Settings.Load();
			bInitialized = TRUE;
		}

		void RegisterPanelVisibility(const EString& InId, const BOOL32& bInDefault)
		{
			if (bInitialized != FALSE)
			{
				// EJsonConfigFile only copies declared keys while loading. Registering after
				// the load would silently ignore the default and user files for this key.
				PE_LOG_ERROR(EString("EditorSettings: panel registered after Initialize: ") + InId);
				return;
			}
			Config().AddBool(MakePanelKey(InId), bInDefault);
		}

		BOOL32 IsPanelVisible(const EString& InId)
		{
			return Config().GetBool(MakePanelKey(InId));
		}

		void SetPanelVisible(const EString& InId, const BOOL32& bInVisible)
		{
			Config().SetBool(MakePanelKey(InId), bInVisible);
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
