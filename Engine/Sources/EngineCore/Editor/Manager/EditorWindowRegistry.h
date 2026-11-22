#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	// Panels that can be hidden: the ones that are not part of the authoring layout most of
	// the time. Everything else keeps its existing always-on behavior.
	enum class EEditorWindowType : UINT8
	{
		Level,
		Navigation,
		ContentBrowser,
		OutputLog,

		Count
	};

	/*
	* Visibility state shared by the Window menu and the registry-managed panels.
	*
	* A panel draws through FScopedEditorWindow: the X button and the menu write the same
	* flag, so closing a panel and reopening it from Window can never disagree. Level and
	* Navigation start hidden; the Content Browser and Output Log start visible because
	* they replace windows that used to be drawn unconditionally.
	*/
	namespace EEditorWindowRegistry
	{
		// Restores the default visibility of every optional panel.
		void Initialize();
		PE_NODISCARD BOOL32 IsVisible(const EEditorWindowType& InWindow);
		void SetVisible(const EEditorWindowType& InWindow, const BOOL32& bInVisible);
		PE_NODISCARD const CHAR* GetWindowName(const EEditorWindowType& InWindow);
		// The contents of the main menu's "Window" menu.
		void DrawWindowMenu();
	}

	/*
	* Scoped panel frame: Begin is called only when the panel is visible and End is always
	* paired with it. IsOpen() is false when the panel is hidden or ImGui culled its content
	* (collapsed/closing), so the host can skip the body without extra checks.
	*/
	class FScopedEditorWindow final
	{
	public:
		explicit FScopedEditorWindow(const EEditorWindowType& InWindow);
		~FScopedEditorWindow();

		PE_NODISCARD BOOL32 IsOpen() const;

	private:
		EEditorWindowType	Window = EEditorWindowType::Count;
		BOOL32				bBegun = FALSE;
		BOOL32				bContentVisible = FALSE;
	};
}

#endif
