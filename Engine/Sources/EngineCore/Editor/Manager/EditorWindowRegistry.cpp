#include <Editor/Manager/EditorWindowRegistry.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

namespace PigeonEngine
{
	namespace
	{
		constexpr UINT32 EditorWindowCount = static_cast<UINT32>(EEditorWindowType::Count);

		struct FEditorWindowEntry
		{
			// Panel title, which is also the ImGui window and docking layout key.
			const CHAR*	Name;
			BOOL32		bDefaultVisible;
			BOOL32		bVisible;
		};

		// Indexed by EEditorWindowType. Both are authoring panels that are not needed every
		// session, so they start hidden; the Window menu brings them back.
		FEditorWindowEntry WindowEntries[EditorWindowCount] =
		{
			{ PE_TEXT("Level"),			FALSE,	FALSE	},
			{ PE_TEXT("Navigation"),	FALSE,	FALSE	},
		};

		BOOL32 IsValidWindow(const EEditorWindowType& InWindow)
		{
			return (static_cast<UINT32>(InWindow) < EditorWindowCount) ? TRUE : FALSE;
		}
	}

	namespace EEditorWindowRegistry
	{
		void Initialize()
		{
			for (UINT32 Index = 0u; Index < EditorWindowCount; Index++)
			{
				WindowEntries[Index].bVisible = WindowEntries[Index].bDefaultVisible;
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
				WindowEntries[static_cast<UINT32>(InWindow)].bVisible = bInVisible;
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
					Entry.bVisible = bVisible ? TRUE : FALSE;
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
