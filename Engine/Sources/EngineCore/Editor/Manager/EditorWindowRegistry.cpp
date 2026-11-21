#include <Editor/Manager/EditorWindowRegistry.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <Editor/Manager/EditorSettings.h>

namespace PigeonEngine
{
	namespace
	{
		constexpr UINT32 EditorWindowCount = static_cast<UINT32>(EEditorWindowType::Count);

		struct FEditorWindowEntry
		{
			// Panel title, which is also the ImGui window and docking layout key.
			const CHAR*	Name;
			BOOL32		bVisible;
		};

		// Indexed by EEditorWindowType.
		FEditorWindowEntry WindowEntries[EditorWindowCount] =
		{
			{ PE_TEXT("Level"),			FALSE	},
			{ PE_TEXT("Navigation"),	FALSE	},
		};

		BOOL32 IsValidWindow(const EEditorWindowType& InWindow)
		{
			return (static_cast<UINT32>(InWindow) < EditorWindowCount) ? TRUE : FALSE;
		}

		BOOL32 LoadVisibility(const EEditorWindowType& InWindow)
		{
			switch (InWindow)
			{
			case EEditorWindowType::Level:
				return EEditorSettings::IsLevelPanelVisible();
			case EEditorWindowType::Navigation:
				return EEditorSettings::IsNavigationPanelVisible();
			default:
				return FALSE;
			}
		}

		void StoreVisibility(const EEditorWindowType& InWindow, const BOOL32& bInVisible)
		{
			switch (InWindow)
			{
			case EEditorWindowType::Level:
				EEditorSettings::SetLevelPanelVisible(bInVisible);
				break;
			case EEditorWindowType::Navigation:
				EEditorSettings::SetNavigationPanelVisible(bInVisible);
				break;
			default:
				break;
			}
		}
	}

	namespace EEditorWindowRegistry
	{
		void Initialize()
		{
			for (UINT32 Index = 0u; Index < EditorWindowCount; Index++)
			{
				const EEditorWindowType Window = static_cast<EEditorWindowType>(Index);
				WindowEntries[Index].bVisible = LoadVisibility(Window);
			}
		}

		BOOL32 IsVisible(const EEditorWindowType& InWindow)
		{
			return (IsValidWindow(InWindow) && (WindowEntries[static_cast<UINT32>(InWindow)].bVisible != FALSE)) ? TRUE : FALSE;
		}

		void SetVisible(const EEditorWindowType& InWindow, const BOOL32& bInVisible)
		{
			if (IsValidWindow(InWindow))
			{
				FEditorWindowEntry& Entry = WindowEntries[static_cast<UINT32>(InWindow)];
				if (Entry.bVisible == bInVisible)
				{
					return;
				}
				Entry.bVisible = bInVisible;
				StoreVisibility(InWindow, bInVisible);
			}
		}

		const CHAR* GetWindowName(const EEditorWindowType& InWindow)
		{
			return IsValidWindow(InWindow) ? WindowEntries[static_cast<UINT32>(InWindow)].Name : "";
		}

		void DrawWindowMenu()
		{
			for (UINT32 Index = 0u; Index < EditorWindowCount; Index++)
			{
				FEditorWindowEntry& Entry = WindowEntries[Index];
				bool bVisible = (Entry.bVisible != FALSE);
				if (ImGui::MenuItem(Entry.Name, nullptr, &bVisible))
				{
					SetVisible(static_cast<EEditorWindowType>(Index), bVisible ? TRUE : FALSE);
				}
			}
		}
	}

	FScopedEditorWindow::FScopedEditorWindow(const EEditorWindowType& InWindow)
		: Window(InWindow)
	{
		if (!EEditorWindowRegistry::IsVisible(InWindow))
		{
			return;
		}
		BOOL32 bOpen = TRUE;
		bContentVisible = ImGui::Begin(EEditorWindowRegistry::GetWindowName(InWindow), &bOpen);
		bBegun = TRUE;
		// The X button and the menu share this flag; a reopened panel starts from the menu.
		EEditorWindowRegistry::SetVisible(InWindow, bOpen);
	}

	FScopedEditorWindow::~FScopedEditorWindow()
	{
		if (bBegun != FALSE)
		{
			ImGui::End();
		}
	}

	BOOL32 FScopedEditorWindow::IsOpen() const
	{
		return bContentVisible;
	}
}

#endif
