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
	*/
	// TODO(paidax): 本文件原带一套 Win32 多视口(multi-viewport)平台后端——
	// PlatformCreateWindow/PlatformSetWindowPos/WndUpdateMonitors/WndUpdateMouseData/
	// InitWndViewportSupport/RendererRenderWindow/RendererSwapBuffers/CreateViewportRenderTarget
	// 等 27 个函数（纯 D3D11）。2026-10 与 Main 合并时，Main 已把 imgui 渲染改到
	// EImGuiRenderer + IRRHIDevice/IRCommandList，这批回调未随之移植，暂时移除。
	// 恢复办法：旧实现见 origin/Animation 的 Engine/EngineThirdParty/imGUI/，
	// 需改写为走 RHI 后再重新注册到 ImGui::GetPlatformIO()。
	// 注意：停靠(docking)本身不受影响，只少了“把面板拖成独立 OS 窗口”。
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
