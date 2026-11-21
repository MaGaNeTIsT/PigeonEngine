#include <Editor/Manager/EditorSettings.h>

#if ENGINE_EDITOR_ONLY

#include <IO/FileHelper.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{
	namespace
	{
		const CHAR* EditorSettingsPath = PE_TEXT("./Engine/Configs/EditorSettings.json");

		struct FEditorSettingsData
		{
			// Optional panels; both are closed on a fresh checkout.
			BOOL32	bShowLevelPanel				= FALSE;
			BOOL32	bShowNavigationPanel		= FALSE;

			// Display Settings; the defaults mirror the engine's own startup values.
			BOOL32	bShowCollision				= TRUE;
			BOOL32	bShowDebugWireframe			= FALSE;
			BOOL32	bShowNavigation				= TRUE;
			BOOL32	bFullNavMeshPolygons		= FALSE;
			BOOL32	bShowNavMeshTriangulation	= TRUE;
		};

		FEditorSettingsData& Settings()
		{
			static FEditorSettingsData Instance;
			return Instance;
		}

		BOOL32 ReadBool(CJsonObject& InObject, const CHAR* InName, BOOL32& InOutValue)
		{
			BOOL8 Value = FALSE;
			if (InObject.GetBoolField(EString(InName), Value))
			{
				InOutValue = (Value != 0) ? TRUE : FALSE;
				return TRUE;
			}
			return FALSE;
		}
	}

	namespace EEditorSettings
	{
		void Initialize()
		{
			FEditorSettingsData& Data = Settings();
			Data = FEditorSettingsData();

			EString Text;
			if (!EFileHelper::ReadFileAsString(EString(EditorSettingsPath), Text) || (Text.Length() == 0u))
			{
				Save();
				return;
			}

			CJsonObject Root(*Text);
			if (!Root.IsValid())
			{
				// A hand-edited or truncated file must not assert; fall back to defaults and
				// rewrite a valid file so the next launch starts clean.
				Save();
				return;
			}

			CJsonObject Panels;
			if (Root.GetObjectField(EString(PE_TEXT("Panels")), Panels))
			{
				ReadBool(Panels, PE_TEXT("Level"), Data.bShowLevelPanel);
				ReadBool(Panels, PE_TEXT("Navigation"), Data.bShowNavigationPanel);
			}
			CJsonObject Display;
			if (Root.GetObjectField(EString(PE_TEXT("Display")), Display))
			{
				ReadBool(Display, PE_TEXT("ShowCollision"), Data.bShowCollision);
				ReadBool(Display, PE_TEXT("ShowDebugWireframe"), Data.bShowDebugWireframe);
				ReadBool(Display, PE_TEXT("ShowNavigation"), Data.bShowNavigation);
				ReadBool(Display, PE_TEXT("FullNavMeshPolygons"), Data.bFullNavMeshPolygons);
				ReadBool(Display, PE_TEXT("ShowNavMeshTriangulation"), Data.bShowNavMeshTriangulation);
			}
		}

		void Save()
		{
			const FEditorSettingsData& Data = Settings();
			CJsonObject Root;

			CJsonObject Panels;
			Panels.SetBoolField(EString(PE_TEXT("Level")), Data.bShowLevelPanel != FALSE);
			Panels.SetBoolField(EString(PE_TEXT("Navigation")), Data.bShowNavigationPanel != FALSE);
			Root.SetObjectField(EString(PE_TEXT("Panels")), &Panels);

			CJsonObject Display;
			Display.SetBoolField(EString(PE_TEXT("ShowCollision")), Data.bShowCollision != FALSE);
			Display.SetBoolField(EString(PE_TEXT("ShowDebugWireframe")), Data.bShowDebugWireframe != FALSE);
			Display.SetBoolField(EString(PE_TEXT("ShowNavigation")), Data.bShowNavigation != FALSE);
			Display.SetBoolField(EString(PE_TEXT("FullNavMeshPolygons")), Data.bFullNavMeshPolygons != FALSE);
			Display.SetBoolField(EString(PE_TEXT("ShowNavMeshTriangulation")), Data.bShowNavMeshTriangulation != FALSE);
			Root.SetObjectField(EString(PE_TEXT("Display")), &Display);

			EFileHelper::SaveStringToFile(EString(EditorSettingsPath), Root.ToString());
		}

		BOOL32 IsLevelPanelVisible()
		{
			return Settings().bShowLevelPanel;
		}

		void SetLevelPanelVisible(const BOOL32& bInVisible)
		{
			Settings().bShowLevelPanel = bInVisible;
			Save();
		}

		BOOL32 IsNavigationPanelVisible()
		{
			return Settings().bShowNavigationPanel;
		}

		void SetNavigationPanelVisible(const BOOL32& bInVisible)
		{
			Settings().bShowNavigationPanel = bInVisible;
			Save();
		}

		BOOL32 GetShowCollision()
		{
			return Settings().bShowCollision;
		}

		void SetShowCollision(const BOOL32& bInVisible)
		{
			Settings().bShowCollision = bInVisible;
			Save();
		}

		BOOL32 GetShowDebugWireframe()
		{
			return Settings().bShowDebugWireframe;
		}

		void SetShowDebugWireframe(const BOOL32& bInVisible)
		{
			Settings().bShowDebugWireframe = bInVisible;
			Save();
		}

		BOOL32 GetShowNavigation()
		{
			return Settings().bShowNavigation;
		}

		void SetShowNavigation(const BOOL32& bInVisible)
		{
			Settings().bShowNavigation = bInVisible;
			Save();
		}

		BOOL32 GetFullNavMeshPolygons()
		{
			return Settings().bFullNavMeshPolygons;
		}

		void SetFullNavMeshPolygons(const BOOL32& bInVisible)
		{
			Settings().bFullNavMeshPolygons = bInVisible;
			Save();
		}

		BOOL32 GetShowNavMeshTriangulation()
		{
			return Settings().bShowNavMeshTriangulation;
		}

		void SetShowNavMeshTriangulation(const BOOL32& bInVisible)
		{
			Settings().bShowNavMeshTriangulation = bInVisible;
			Save();
		}
	}
}

#endif
