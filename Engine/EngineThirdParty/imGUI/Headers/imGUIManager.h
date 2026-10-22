#pragma once

#include <CoreMinimal.h>
#include <imgui.h>
#include "EImGuiRenderer.h"

namespace PigeonEngine
{
#if _EDITOR_ONLY

	/*
	* Backend-agnostic ImGui manager. Owns:
	*   * The Win32 platform binding (HWND, mouse/keyboard event pump).
	*   * The per-frame ImGui lifecycle (Update / PrepareDrawData / RenderDrawData).
	*
	* GPU rendering is delegated entirely to EImGuiRenderer, which talks to
	* IRRHIDevice + IRCommandList - no D3D11/D3D12 types appear here. To
	* swap backends just relink the engine; the manager carries no
	* backend-specific state.
	*
	* Multi-viewport: a panel dragged out of the main window becomes an OS window. The
	* platform half (Win32 window class, monitor list, mouse routing) and the renderer half
	* (one IRRHISwapChain per window) are both implemented here; the old D3D11-only backend
	* was removed during the Main merge and has been restored on top of IRRHIDevice /
	* IRCommandList. Docking itself is unaffected: this only adds "drag a panel out into its
	* own window".
	*/
	class IRRHISwapChain;

	class CImGUIManager : public EManagerBase
	{
	public:
		struct WndData
		{
			WndData() { ::ZeroMemory(this, sizeof(*this)); }
			HWND                    hWnd;
			HWND                    MouseHwnd;
			BOOL32                  MouseTracked;
			INT32                   MouseButtonsDown;
			ImGuiMouseCursor        LastMouseCursor;
			BOOL32                  HasGamepad;
			BOOL32                  WantUpdateHasGamepad;
			BOOL32                  WantUpdateMonitors;
			UINT32                  Tick;
			UINT32                  LastMouseMoveTick;
			// Physical pixels per ImGui unit, taken from the monitor the main window is on.
			// ImGui's layout/style are global, so the whole application uses this single scale;
			// the physical resolution is reached through DisplayFramebufferScale.
			FLOAT                   DpiScale;
		};
	private:
		// State of one platform window this backend owns (or borrowed, for the main window).
		struct PlatformViewportData
		{
			HWND	Hwnd;
			HWND	HwndParent;
			BOOL32	HwndOwned;
			DWORD	Style;
			DWORD	ExStyle;

			PlatformViewportData()
				: Hwnd(nullptr), HwndParent(nullptr), HwndOwned(FALSE), Style(0u), ExStyle(0u)
			{
			}
		};
		// Render state of one platform window: the chain its draw data is recorded into.
		struct RendererViewportData
		{
			IRRHISwapChain*	SwapChain;

			RendererViewportData()
				: SwapChain(nullptr)
			{
			}
		};
	public:
		virtual void            Initialize()override;
		virtual void            ShutDown()override;
		void                    Update();
		// Backwards-compatible single-call form (NewFrame must already be done).
		// Internally just chains PrepareDrawData + RenderDrawData. Prefer the
		// split form when you want main-thread/render-thread overlap.
		void                    Draw();
		// Main-thread step: finalize ImGui frame and produce ImDrawData. Must
		// be called between NewFrame (in Update()) and the next Update().
		void                    PrepareDrawData();
		// Render-thread step: replay the prepared ImDrawData through the
		// active IRCommandList. Caller (SceneRenderer::ImGuiPass) is
		// responsible for BeginRenderPass / EndRenderPass.
		void                    RenderDrawData();
		// Render-thread step: draw and present the platform windows (windows a panel was
		// dragged out into). The main window's overlay is part of SceneRenderer::ImGuiPass;
		// this covers the extra ones, each into its own swap chain.
		void                    RenderPlatformWindows();
		// Game thread, after the main window changes size: keeps the floating windows that
		// fit in the old size inside the new one, so a resize does not detach them into
		// platform windows of their own.
		void                    ClampWindowsToMainViewport(UINT32 InNewWidth, UINT32 InNewHeight);

		// Render thread: a shader the renderer draws with was replaced. It records a game thread command
		// that tears the renderer down, and the lazy initialization in Update() rebuilds it from the
		// current asset - see the comment there for why the rebuild happens on that thread.
		void                    OnShaderAssetReplaced();
		IMGUI_IMPL_API LRESULT  WndProcHandler(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);

		// Accessor for code that wants to map an engine texture asset's
		// bindless index into ImTextureID (see EngineTextureToImgui).
		const EImGuiRenderer&   GetRenderer()const { return Renderer; }
		EImGuiRenderer&         GetRenderer()      { return Renderer; }

	private:
		void                    InitWnd();
		BOOL32                  WndUpdateMouseCursor();
		BOOL32                  WndIsVkDown(INT32 vk);
		void                    WndAddKeyEvent(ImGuiKey key, BOOL32 down, INT32 native_keycode, INT32 native_scancode = -1);
		void                    WndProcessKeyEventsWorkarounds();
		void                    WndUpdateKeyModifiers();
		ImGuiKey                WndVirtualKeyToImGuiKey(WPARAM wParam);
	private:
		// Platform-window backend (ImGui multi-viewport). A panel dragged out of the main
		// window becomes an OS window of this class, with its own swap chain: these are the
		// Platform_* / Renderer_* callbacks ImGui::GetPlatformIO() drives.
		void                    InitWndViewportSupport();
		void                    ShutDownWndViewportSupport();
		void                    WndUpdateMouseData();
		void                    WndPollMouseWhileButtonDown();
		void                    WndUpdateMonitors();
		static BOOL CALLBACK    WndUpdateMonitorsEnumProc(HMONITOR monitor, HDC, LPRECT, LPARAM);
		static ImGuiViewport*   WndFindViewportByWindowHandle(HWND hWnd);
		static LRESULT CALLBACK WndProcHandlerPlatformWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static void             PlatformCreateWindow(ImGuiViewport* viewport);
		static void             PlatformDestroyWindow(ImGuiViewport* viewport);
		static void             PlatformShowWindow(ImGuiViewport* viewport);
		static void             PlatformSetWindowPos(ImGuiViewport* viewport, ImVec2 pos);
		static ImVec2           PlatformGetWindowPos(ImGuiViewport* viewport);
		static void             PlatformSetWindowSize(ImGuiViewport* viewport, ImVec2 size);
		static ImVec2           PlatformGetWindowSize(ImGuiViewport* viewport);
		static void             PlatformSetWindowFocus(ImGuiViewport* viewport);
		static bool             PlatformGetWindowFocus(ImGuiViewport* viewport);
		static bool             PlatformGetWindowMinimized(ImGuiViewport* viewport);
		static void             PlatformUpdateWindow(ImGuiViewport* viewport);
		static void             PlatformSetWindowTitle(ImGuiViewport* viewport, const char* title);
		static ImVec2           PlatformGetWindowFramebufferScale(ImGuiViewport* viewport);
		static FLOAT            PlatformGetWindowDpiScale(ImGuiViewport* viewport);
		// Physical pixels per ImGui unit. Always positive so callers can divide without a guard.
		static FLOAT            GetDpiScale();
		static void             RendererCreateWindow(ImGuiViewport* viewport);
		static void             RendererDestroyWindow(ImGuiViewport* viewport);
		static void             RendererSetWindowSize(ImGuiViewport* viewport, ImVec2 size);
		static void             RendererRenderWindow(ImGuiViewport* viewport, void* render_arg);
		static void             RendererSwapBuffers(ImGuiViewport* viewport, void* render_arg);
	private:
		WndData                 m_WndData;
		EImGuiRenderer          Renderer;

		CLASS_MANAGER_SINGLETON_BODY(CImGUIManager)

	};

	// Convert an engine texture asset to an ImTextureID suitable for ImGui::Image() etc.
	// Returns ImTextureID_Invalid if the asset has no valid bindless SRV.
	extern PE_INLINE ImTextureID EngineTextureToImgui(const class ETexture2DAsset* InAsset);

	// Convert a raw IRRHITexture* (render textures, manually created textures, etc.)
	// to an ImTextureID. Registers a bindless SRV if one doesn't exist yet.
	// Returns ImTextureID_Invalid if InTexture is null or registration fails.
	extern PE_INLINE ImTextureID EngineTextureToImgui(class IRRHITexture* InTexture);

#endif
};
