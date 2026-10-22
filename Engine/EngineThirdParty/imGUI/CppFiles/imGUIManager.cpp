#include "../Headers/imGUIManager.h"
#include "../../../Sources/EngineCore/Main/MainManager.h"
#include "../../../Sources/EngineCore/Main/GameThreadCommandQueue.h"
#include <Base/Timer/Timer.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/IRRHISwapChain.h>
#include <Config/EngineConfig.h>
#include <Development/GPUProfiler.h>
#include <TextureAsset/TextureAsset.h>
#include <imgui_internal.h>

namespace PigeonEngine
{
#if _EDITOR_ONLY

#define IM_VK_KEYPAD_ENTER          (VK_RETURN + 256)

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL              0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED        0x0007
#endif

	static void RegisterClassTypes()
	{
		RegisterClassType<CImGUIManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	// Windows reports DPI on a 96-DPI baseline; 1.0f means no extra scale.
	static constexpr FLOAT GDpiBase = 96.0f;

	CImGUIManager::CImGUIManager()
	{
#if _EDITOR_ONLY
		DebugName = ENGINE_IMGUI_MANAGER_NAME;
#endif
	}
	CImGUIManager::~CImGUIManager()
	{
	}
	void CImGUIManager::Initialize()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();

		io.IniFilename = "./Engine/Configs/ImGUIEngine.ini";
		io.LogFilename = "./Engine/Configs/ImGUIEngineLog.txt";
		io.IniSavingRate = 30.f;

		InitWnd();

		// Mark the renderer backend so ImGui's IO knows a GPU pipeline is
		// attached. Actual GPU init (font atlas + PSO + bindless slot) is
		// deferred to Update() once MainManager's RHI device is live.
		io.BackendRendererName     = "PigeonEngine_RHI";
		io.BackendRendererUserData = (void*)(&Renderer);
		io.BackendFlags           |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.BackendFlags           |= ImGuiBackendFlags_RendererHasTextures;

		RECT rect = { 0, 0, 0, 0 };
		::GetClientRect(m_WndData.hWnd, &rect);
		io.DisplaySize = ImVec2((FLOAT)(rect.right - rect.left), (FLOAT)(rect.bottom - rect.top));
	}
	void CImGUIManager::ShutDown()
	{
		// Platform windows first: destroying them runs Renderer_DestroyWindow, which releases
		// the swap chains, and those have to go before the device does.
		ShutDownWndViewportSupport();

		// The font atlas is session scoped, so it is released here rather than by ShutDown, which the
		// shader replacement path also calls: this is the one point where nothing will be drawn again,
		// and the renderer answers for the order (it owns the device object).
		Renderer.ReleaseFontTexture();

		Renderer.ShutDown();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererName = nullptr;
		io.BackendRendererUserData = nullptr;
		io.BackendPlatformName = nullptr;
		io.BackendPlatformUserData = nullptr;
	}
	FLOAT CImGUIManager::GetDpiScale()
	{
		const FLOAT Scale = GetManagerSingleton()->m_WndData.DpiScale;
		return (Scale > 0.0f) ? Scale : 1.0f;
	}

	void CImGUIManager::Update()
	{
		ImGuiIO& io = ImGui::GetIO();
		m_WndData.Tick++;
		const EGameTimer* GameTimer = EMainManager::GetManagerSingleton()->GetGameTimer();
		io.DeltaTime = static_cast<FLOAT>(GameTimer->GetDeltaTime());

		{
			const Vector2Int& WindowSize = EMainManager::GetManagerSingleton()->GetWindowSize();
			const FLOAT DpiScale = GetDpiScale();
			io.DisplaySize = ImVec2(static_cast<FLOAT>(WindowSize.x) / DpiScale, static_cast<FLOAT>(WindowSize.y) / DpiScale);
			io.DisplayFramebufferScale = ImVec2(DpiScale, DpiScale);
		}

		WndProcessKeyEventsWorkarounds();

		if (m_WndData.WantUpdateMonitors)
		{
			WndUpdateMonitors();
		}
		// Tells ImGui which platform window the OS cursor is over; without it, the backend
		// cannot route input to the window a panel was dragged into.
		WndUpdateMouseData();
		WndPollMouseWhileButtonDown();

		ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
		if (m_WndData.LastMouseCursor != mouse_cursor)
		{
			m_WndData.LastMouseCursor = mouse_cursor;
			WndUpdateMouseCursor();
		}

		// Lazy GPU init: first frame after MainManager has wired the render
		// device. Failure here leaves the renderer uninitialized - UI won't
		// draw but the engine keeps running.
		//
		// It doubles as the rebuild path for a replaced shader: that arrives as a command on the game
		// thread queue which shuts the renderer down, and this line then brings it back up from the
		// current asset. Same thread as the command, so the order is the frame's, not a race.
		if (!Renderer.IsInitialized())
		{
			Renderer.Initialize();
		}

		ImGui::NewFrame();
	}
	void CImGUIManager::OnShaderAssetReplaced()
	{
		/*
		* Recorded for this thread rather than done here: this runs on the render thread, and the
		* rebuild belongs where the renderer's lazy initialization lives, because it creates device
		* objects outside the frame's command list.
		*
		* The command captures this manager, which is a singleton and therefore outlives it - the
		* lifetime rule EGameThreadCommandQueue states. What the renderer derived from the replaced
		* program (pipeline state, two blocks, font slot) is rebuilt by the next Initialize, and the
		* instance itself is already current: the process wide cache re-pointed it.
		*/
		EGameThreadCommandQueue::GetManagerSingleton()->EnqueueCommand([this]() -> void
		{
			if (Renderer.IsInitialized())
			{
				Renderer.ShutDown();
			}
		});
	}
	void CImGUIManager::Draw()
	{
		PrepareDrawData();
		RenderDrawData();
		RenderPlatformWindows();
	}
	void CImGUIManager::PrepareDrawData()
	{
		// Finalize ImGui frame on the main thread. After this returns,
		// ImGui::GetDrawData() is valid and stable until the next ImGui::NewFrame.
		ImGui::Render();

		// Multi-viewport: turn the docking state into platform windows now, on this thread.
		// A window that was just dragged out gets its HWND and its swap chain here; the
		// recording of its contents is RenderPlatformWindows, on the render thread.
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
		}
	}
	void CImGUIManager::RenderDrawData()
	{
		// Replay through the RHI-abstracted renderer. The caller (SceneRenderer::
		// ImGuiPass) has already opened the appropriate render pass on the
		// active IRCommandList.
		IRCommandList* Cmd = EMainManager::GetManagerSingleton()->GetActiveCommandList();
		if (!Cmd) { return; }
		Renderer.Render(Cmd, ImGui::GetDrawData());
	}
	void CImGUIManager::ClampWindowsToMainViewport(UINT32 InNewWidth, UINT32 InNewHeight)
	{
		ImGuiContext* Context = ImGui::GetCurrentContext();
		if ((!Context) || (InNewWidth == 0u) || (InNewHeight == 0u))
		{
			return;
		}

		// A floating window that fits in the old viewport is expected to stay in it. When the
		// main window shrinks (leaving fullscreen is the common case) ImGui would otherwise see
		// those windows outside the new viewport and hand each of them its own platform window,
		// which is how a fullscreen toggle scatters the editor panels. Windows that were already
		// outside the old viewport are left alone: those are the ones the user detached on
		// purpose, possibly onto another monitor.
		ImGuiContext& G = *Context;
		if (G.Viewports.Size <= 0)
		{
			return;
		}
		ImGuiViewportP* MainViewport = G.Viewports[0];
		const ImVec2 MainPos = MainViewport->Pos;
		// The caller reports the OS client size (physical); the viewport is in logical units.
		const FLOAT DpiScale = GetDpiScale();
		const FLOAT NewWidth = static_cast<FLOAT>(InNewWidth) / DpiScale;
		const FLOAT NewHeight = static_cast<FLOAT>(InNewHeight) / DpiScale;
		const ImVec2 OldMax(MainPos.x + MainViewport->Size.x, MainPos.y + MainViewport->Size.y);
		const ImVec2 NewMax(MainPos.x + NewWidth, MainPos.y + NewHeight);
		constexpr FLOAT EdgeEpsilon = 1.0f;

		for (INT32 WindowIndex = 0; WindowIndex < G.Windows.Size; WindowIndex++)
		{
			ImGuiWindow* Window = G.Windows[WindowIndex];
			if ((!Window) || (!Window->Active) || (Window->Hidden != FALSE))
			{
				continue;
			}
			// Only windows that belong to the main viewport and are not dock-managed: a dock
			// node lays its windows out itself, and moving those would fight the layout.
			if ((Window->Viewport != MainViewport) || (Window->DockNode != nullptr))
			{
				continue;
			}
			const ImVec2 WindowSize = Window->Size;
			if (((Window->Pos.x + WindowSize.x) > (OldMax.x + EdgeEpsilon))
				|| ((Window->Pos.y + WindowSize.y) > (OldMax.y + EdgeEpsilon)))
			{
				// It was already outside: this one was detached deliberately.
				continue;
			}

			const FLOAT MaxX = NewMax.x - WindowSize.x;
			const FLOAT MaxY = NewMax.y - WindowSize.y;
			if (Window->Pos.x > MaxX)
			{
				Window->Pos.x = (MaxX > MainPos.x) ? MaxX : MainPos.x;
			}
			if (Window->Pos.y > MaxY)
			{
				Window->Pos.y = (MaxY > MainPos.y) ? MaxY : MainPos.y;
			}
			if (Window->Pos.x < MainPos.x)
			{
				Window->Pos.x = MainPos.x;
			}
			if (Window->Pos.y < MainPos.y)
			{
				Window->Pos.y = MainPos.y;
			}
		}
	}

	// While a mouse button is held, the OS cursor is polled as a fallback for when no move
	// message arrives. Dragging a panel out of the main window creates a platform window in
	// the middle of the drag, and the cursor can then be over that window, over the desktop
	// between two displays, or over another application. WM_MOUSEMOVE is the authoritative
	// source whenever it arrives (its coordinates are the same space as the window positions),
	// so polling is only used after a few frames without a move message; mixing both sources
	// would feed two different coordinate spaces into the same drag on mixed-DPI desktops.
	// A release can be delivered to another window and lost the same way, so the physical
	// button state is always checked.
	void CImGUIManager::WndPollMouseWhileButtonDown()
	{
		ImGuiIO& io = ImGui::GetIO();
		if ((!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) || (m_WndData.MouseButtonsDown == 0))
		{
			return;
		}

		constexpr UINT32 MouseMoveTimeoutTicks = 3;
		if ((m_WndData.Tick - m_WndData.LastMouseMoveTick) >= MouseMoveTimeoutTicks)
		{
			POINT CursorPos = {};
			if (::GetCursorPos(&CursorPos) != FALSE)
			{
				const FLOAT DpiScale = GetDpiScale();
				io.AddMousePosEvent(static_cast<FLOAT>(CursorPos.x) / DpiScale, static_cast<FLOAT>(CursorPos.y) / DpiScale);
			}
		}

		static constexpr INT32 ButtonVirtualKeys[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
		for (INT32 ButtonIndex = 0; ButtonIndex < IM_ARRAYSIZE(ButtonVirtualKeys); ButtonIndex++)
		{
			if ((m_WndData.MouseButtonsDown & (1 << ButtonIndex)) == 0)
			{
				continue;
			}
			if ((::GetAsyncKeyState(ButtonVirtualKeys[ButtonIndex]) & 0x8000) != 0)
			{
				continue;
			}

			m_WndData.MouseButtonsDown &= ~(1 << ButtonIndex);
			io.AddMouseButtonEvent(ButtonIndex, FALSE);
			if ((m_WndData.MouseButtonsDown == 0) && (::GetCapture() != nullptr))
			{
				::ReleaseCapture();
			}
		}
	}

	void CImGUIManager::RenderPlatformWindows()
	{
		// Render-thread half of ImGui's multi-viewport support: for every window a panel was
		// dragged out into, ImGui calls the Renderer_RenderWindow / Renderer_SwapBuffers hooks
		// below. The draw data those hooks consume was built by UpdatePlatformWindows on the
		// game thread (PrepareDrawData), so nothing is being built here.
		if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
		{
			return;
		}
		if (!(ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasViewports))
		{
			return;
		}
		ImGui::RenderPlatformWindowsDefault();
	}
	void CImGUIManager::InitWnd()
	{
		ImGuiIO& io = ImGui::GetIO();

		//PE_CHECK((ENGINE_RENDER_CORE_ERROR), PE_TEXT("Already initialized a platform backend!"), (io.BackendPlatformUserData == nullptr));
		IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

		io.BackendPlatformUserData = (void*)(&m_WndData);
		io.BackendPlatformName = "imgui_impl_win32";
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		// dock 分支：必须打开 DockingEnable，否则 EditorManager 里的
		// DockSpaceOverViewport() 不会生成任何 dock node，编辑器面板无法停靠。
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		// Multi-viewport: a panel dragged out of the main window becomes an OS window. The
		// platform and renderer halves are both implemented here (Win32 window + RHI swap
		// chain), so both capability bits are claimed.
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
		io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
		io.BackendFlags |= ImGuiBackendFlags_HasParentViewport;
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGuiStyle& Style = ImGui::GetStyle();
			Style.WindowRounding = 0.0f;
			Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		HWND hWnd = EMainManager::GetManagerSingleton()->GetWindowHandle();
		m_WndData.hWnd = hWnd;
		m_WndData.WantUpdateHasGamepad = FALSE;
		m_WndData.WantUpdateMonitors = TRUE;
		m_WndData.LastMouseCursor = ImGuiMouseCursor_COUNT;
		// One UI scale for the whole application, taken from the monitor the main window is on.
		// ImGui's style and layout are global, so a per-monitor UI scale cannot be expressed by
		// the docking branch; a single scale keeps the saved logical layout valid on every
		// monitor. The process is per-monitor DPI aware (Main.cpp), so window/mouse coordinates
		// are physical and continuous across displays; DisplayFramebufferScale turns those
		// logical units into physical pixels and is also what ImGui 1.92 uses to rasterize
		// fonts at the right density. The scale is fixed for the lifetime of the context:
		// moving the main window to another display keeps the same logical layout.
		m_WndData.DpiScale = static_cast<FLOAT>(::GetDpiForWindow(hWnd)) / GDpiBase;
		if (m_WndData.DpiScale <= 0.0f)
		{
			m_WndData.DpiScale = 1.0f;
		}
		io.DisplayFramebufferScale = ImVec2(m_WndData.DpiScale, m_WndData.DpiScale);

		ImGuiViewport* MainViewport = ImGui::GetMainViewport();
		MainViewport->PlatformHandle = (void*)hWnd;
		MainViewport->PlatformHandleRaw = (void*)hWnd;
		::SetPropA(hWnd, "IMGUI_CONTEXT", ImGui::GetCurrentContext());

		InitWndViewportSupport();
	}
	IMGUI_IMPL_API LRESULT CImGUIManager::WndProcHandler(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui::GetCurrentContext() == nullptr)
			return 0;

		ImGuiIO& io = ImGui::GetIO();

		switch (msg)
		{
		case WM_MOUSEMOVE:
			m_WndData.MouseHwnd = hWnd;
			m_WndData.LastMouseMoveTick = m_WndData.Tick;
			if (!m_WndData.MouseTracked)
			{
				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
				::TrackMouseEvent(&tme);
				m_WndData.MouseTracked = TRUE;
			}
			{
				// With multi-viewport enabled, ImGui works in absolute (screen) coordinates:
				// every window position and the mouse position share that space, and the
				// renderer subtracts the viewport origin through ImDrawData::DisplayPos. A
				// client-local position here would be off by the window's screen origin for
				// every hit test - hovered windows, docking targets, buttons and sliders.
				// The OS speaks physical pixels (the process is per-monitor DPI aware) while
				// ImGui's layout is logical, so the position is divided by the UI scale.
				POINT MousePos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					::ClientToScreen(hWnd, &MousePos);
				}
				const FLOAT DpiScale = GetDpiScale();
				io.AddMousePosEvent((FLOAT)MousePos.x / DpiScale, (FLOAT)MousePos.y / DpiScale);
			}
			break;
		case WM_MOUSELEAVE:
			if (m_WndData.MouseHwnd == hWnd)
			{
				m_WndData.MouseHwnd = nullptr;
			}
			m_WndData.MouseTracked = FALSE;
			{
				// Entering another one of our windows also sends WM_MOUSELEAVE to the one that
				// was left. Reporting "no mouse" unconditionally would drop the position in the
				// frame the cursor crosses into a platform window, which is exactly the frame a
				// drag out of the main window is resolved in.
				POINT MousePos = { 0, 0 };
				ImGuiViewport* OtherViewport = nullptr;
				if (::GetCursorPos(&MousePos))
				{
					OtherViewport = WndFindViewportByWindowHandle(::WindowFromPoint(MousePos));
				}
				if (!OtherViewport)
				{
					io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
				}
			}
			break;
		case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
		{
			INT32 button = 0;
			if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
			if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
			if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
			if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
			// A release can be missed when the button came down in the main window and the
			// window it was dragging out becomes its own OS window mid-drag: the up edge may
			// then never reach this bookkeeping. Without this reset the button stays down
			// forever in ImGui's view, and whatever is active (a splitter, a dock node, a
			// slider) keeps following the mouse. Capture being owned by another window is the
			// signal that the up edge was lost.
			HWND HwndWithCapture = ::GetCapture();
			if ((m_WndData.MouseButtonsDown != 0) && (HwndWithCapture != hWnd))
			{
				m_WndData.MouseButtonsDown = 0;
			}
			if ((m_WndData.MouseButtonsDown == 0) && (HwndWithCapture == nullptr))
			{
				::SetCapture(hWnd);
			}
			m_WndData.MouseButtonsDown |= 1 << button;
			io.AddMouseButtonEvent(button, TRUE);
			return 0;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			INT32 button = 0;
			if (msg == WM_LBUTTONUP) { button = 0; }
			if (msg == WM_RBUTTONUP) { button = 1; }
			if (msg == WM_MBUTTONUP) { button = 2; }
			if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
			m_WndData.MouseButtonsDown &= ~(1 << button);
			if (m_WndData.MouseButtonsDown == 0 && ::GetCapture() == hWnd)
			{
				::ReleaseCapture();
			}
			io.AddMouseButtonEvent(button, FALSE);
			return 0;
		}
		case WM_MOUSEWHEEL:
			io.AddMouseWheelEvent(0.0f, (FLOAT)GET_WHEEL_DELTA_WPARAM(wParam) / (FLOAT)WHEEL_DELTA);
			return 0;
		case WM_MOUSEHWHEEL:
			io.AddMouseWheelEvent((FLOAT)GET_WHEEL_DELTA_WPARAM(wParam) / (FLOAT)WHEEL_DELTA, 0.0f);
			return 0;
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		{
			const BOOL32 is_key_down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
			if (wParam < 256)
			{
				CImGUIManager::WndUpdateKeyModifiers();

				INT32 vk = (INT32)wParam;
				if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
				{
					vk = IM_VK_KEYPAD_ENTER;
				}

				const ImGuiKey key = CImGUIManager::WndVirtualKeyToImGuiKey(vk);
				const INT32 scancode = (INT32)LOBYTE(HIWORD(lParam));
				if (key != ImGuiKey_None)
				{
					CImGUIManager::WndAddKeyEvent(key, is_key_down, vk, scancode);
				}

				if (vk == VK_SHIFT)
				{
					if (CImGUIManager::WndIsVkDown(VK_LSHIFT) == is_key_down) { CImGUIManager::WndAddKeyEvent(ImGuiKey_LeftShift, is_key_down, VK_LSHIFT, scancode); }
					if (CImGUIManager::WndIsVkDown(VK_RSHIFT) == is_key_down) { CImGUIManager::WndAddKeyEvent(ImGuiKey_RightShift, is_key_down, VK_RSHIFT, scancode); }
				}
				else if (vk == VK_CONTROL)
				{
					if (CImGUIManager::WndIsVkDown(VK_LCONTROL) == is_key_down) { CImGUIManager::WndAddKeyEvent(ImGuiKey_LeftCtrl, is_key_down, VK_LCONTROL, scancode); }
					if (CImGUIManager::WndIsVkDown(VK_RCONTROL) == is_key_down) { CImGUIManager::WndAddKeyEvent(ImGuiKey_RightCtrl, is_key_down, VK_RCONTROL, scancode); }
				}
				else if (vk == VK_MENU)
				{
					if (CImGUIManager::WndIsVkDown(VK_LMENU) == is_key_down) { CImGUIManager::WndAddKeyEvent(ImGuiKey_LeftAlt, is_key_down, VK_LMENU, scancode); }
					if (CImGUIManager::WndIsVkDown(VK_RMENU) == is_key_down) { CImGUIManager::WndAddKeyEvent(ImGuiKey_RightAlt, is_key_down, VK_RMENU, scancode); }
				}
			}
			return 0;
		}
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			io.AddFocusEvent(msg == WM_SETFOCUS);
			return 0;
		case WM_CHAR:
			if (wParam > 0 && wParam < 0x10000)
			{
				io.AddInputCharacterUTF16((USHORT)wParam);
			}
			return 0;
		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT && CImGUIManager::WndUpdateMouseCursor())
			{
				return 1;
			}
			return 0;
		case WM_DEVICECHANGE:
			if ((UINT32)wParam == DBT_DEVNODES_CHANGED)
			{
				m_WndData.WantUpdateHasGamepad = TRUE;
			}
			return 0;
		case WM_DISPLAYCHANGE:
			// A monitor was added, removed or resized; the monitor list the platform backend
			// published to ImGui has to be rebuilt before the next frame positions a window.
			m_WndData.WantUpdateMonitors = TRUE;
			return 0;
		}
		return 0;
	}
	BOOL32 CImGUIManager::WndUpdateMouseCursor()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		{
			return FALSE;
		}

		ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
		if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
		{
			::SetCursor(nullptr);
		}
		else
		{
			LPTSTR win32_cursor = IDC_ARROW;
			switch (imgui_cursor)
			{
			case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
			case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
			case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
			case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
			case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
			case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
			case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
			case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
			case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
			}
			::SetCursor(::LoadCursor(nullptr, win32_cursor));
		}
		return TRUE;
	}
	BOOL32 CImGUIManager::WndIsVkDown(INT32 vk)
	{
		return (::GetKeyState(vk) & 0x8000) != 0;
	}
	void CImGUIManager::WndAddKeyEvent(ImGuiKey key, BOOL32 down, INT32 native_keycode, INT32 native_scancode)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.AddKeyEvent(key, down);
		io.SetKeyEventNativeData(key, native_keycode, native_scancode);
		IM_UNUSED(native_scancode);
	}
	void CImGUIManager::WndProcessKeyEventsWorkarounds()
	{
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && !CImGUIManager::WndIsVkDown(VK_LSHIFT))
		{
			CImGUIManager::WndAddKeyEvent(ImGuiKey_LeftShift, FALSE, VK_LSHIFT);
		}
		if (ImGui::IsKeyDown(ImGuiKey_RightShift) && !CImGUIManager::WndIsVkDown(VK_RSHIFT))
		{
			CImGUIManager::WndAddKeyEvent(ImGuiKey_RightShift, FALSE, VK_RSHIFT);
		}
		if (ImGui::IsKeyDown(ImGuiKey_LeftSuper) && !CImGUIManager::WndIsVkDown(VK_LWIN))
		{
			CImGUIManager::WndAddKeyEvent(ImGuiKey_LeftSuper, FALSE, VK_LWIN);
		}
		if (ImGui::IsKeyDown(ImGuiKey_RightSuper) && !CImGUIManager::WndIsVkDown(VK_RWIN))
		{
			CImGUIManager::WndAddKeyEvent(ImGuiKey_RightSuper, FALSE, VK_RWIN);
		}
	}
	void CImGUIManager::WndUpdateKeyModifiers()
	{
		ImGuiIO& io = ImGui::GetIO();
		// imgui 1.91+ renamed ImGuiKey_Mod* -> ImGuiMod_* (the Mod prefix moved).
		io.AddKeyEvent(ImGuiMod_Ctrl,  CImGUIManager::WndIsVkDown(VK_CONTROL));
		io.AddKeyEvent(ImGuiMod_Shift, CImGUIManager::WndIsVkDown(VK_SHIFT));
		io.AddKeyEvent(ImGuiMod_Alt,   CImGUIManager::WndIsVkDown(VK_MENU));
		io.AddKeyEvent(ImGuiMod_Super, CImGUIManager::WndIsVkDown(VK_APPS));
	}
	ImGuiKey CImGUIManager::WndVirtualKeyToImGuiKey(WPARAM wParam)
	{
		switch (wParam)
		{
		case VK_TAB: return ImGuiKey_Tab;
		case VK_LEFT: return ImGuiKey_LeftArrow;
		case VK_RIGHT: return ImGuiKey_RightArrow;
		case VK_UP: return ImGuiKey_UpArrow;
		case VK_DOWN: return ImGuiKey_DownArrow;
		case VK_PRIOR: return ImGuiKey_PageUp;
		case VK_NEXT: return ImGuiKey_PageDown;
		case VK_HOME: return ImGuiKey_Home;
		case VK_END: return ImGuiKey_End;
		case VK_INSERT: return ImGuiKey_Insert;
		case VK_DELETE: return ImGuiKey_Delete;
		case VK_BACK: return ImGuiKey_Backspace;
		case VK_SPACE: return ImGuiKey_Space;
		case VK_RETURN: return ImGuiKey_Enter;
		case VK_ESCAPE: return ImGuiKey_Escape;
		case VK_OEM_7: return ImGuiKey_Apostrophe;
		case VK_OEM_COMMA: return ImGuiKey_Comma;
		case VK_OEM_MINUS: return ImGuiKey_Minus;
		case VK_OEM_PERIOD: return ImGuiKey_Period;
		case VK_OEM_2: return ImGuiKey_Slash;
		case VK_OEM_1: return ImGuiKey_Semicolon;
		case VK_OEM_PLUS: return ImGuiKey_Equal;
		case VK_OEM_4: return ImGuiKey_LeftBracket;
		case VK_OEM_5: return ImGuiKey_Backslash;
		case VK_OEM_6: return ImGuiKey_RightBracket;
		case VK_OEM_3: return ImGuiKey_GraveAccent;
		case VK_CAPITAL: return ImGuiKey_CapsLock;
		case VK_SCROLL: return ImGuiKey_ScrollLock;
		case VK_NUMLOCK: return ImGuiKey_NumLock;
		case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
		case VK_PAUSE: return ImGuiKey_Pause;
		case VK_NUMPAD0: return ImGuiKey_Keypad0;
		case VK_NUMPAD1: return ImGuiKey_Keypad1;
		case VK_NUMPAD2: return ImGuiKey_Keypad2;
		case VK_NUMPAD3: return ImGuiKey_Keypad3;
		case VK_NUMPAD4: return ImGuiKey_Keypad4;
		case VK_NUMPAD5: return ImGuiKey_Keypad5;
		case VK_NUMPAD6: return ImGuiKey_Keypad6;
		case VK_NUMPAD7: return ImGuiKey_Keypad7;
		case VK_NUMPAD8: return ImGuiKey_Keypad8;
		case VK_NUMPAD9: return ImGuiKey_Keypad9;
		case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
		case VK_DIVIDE: return ImGuiKey_KeypadDivide;
		case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
		case VK_ADD: return ImGuiKey_KeypadAdd;
		case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
		case VK_LSHIFT: return ImGuiKey_LeftShift;
		case VK_LCONTROL: return ImGuiKey_LeftCtrl;
		case VK_LMENU: return ImGuiKey_LeftAlt;
		case VK_LWIN: return ImGuiKey_LeftSuper;
		case VK_RSHIFT: return ImGuiKey_RightShift;
		case VK_RCONTROL: return ImGuiKey_RightCtrl;
		case VK_RMENU: return ImGuiKey_RightAlt;
		case VK_RWIN: return ImGuiKey_RightSuper;
		case VK_APPS: return ImGuiKey_Menu;
		case '0': return ImGuiKey_0;
		case '1': return ImGuiKey_1;
		case '2': return ImGuiKey_2;
		case '3': return ImGuiKey_3;
		case '4': return ImGuiKey_4;
		case '5': return ImGuiKey_5;
		case '6': return ImGuiKey_6;
		case '7': return ImGuiKey_7;
		case '8': return ImGuiKey_8;
		case '9': return ImGuiKey_9;
		case 'A': return ImGuiKey_A;
		case 'B': return ImGuiKey_B;
		case 'C': return ImGuiKey_C;
		case 'D': return ImGuiKey_D;
		case 'E': return ImGuiKey_E;
		case 'F': return ImGuiKey_F;
		case 'G': return ImGuiKey_G;
		case 'H': return ImGuiKey_H;
		case 'I': return ImGuiKey_I;
		case 'J': return ImGuiKey_J;
		case 'K': return ImGuiKey_K;
		case 'L': return ImGuiKey_L;
		case 'M': return ImGuiKey_M;
		case 'N': return ImGuiKey_N;
		case 'O': return ImGuiKey_O;
		case 'P': return ImGuiKey_P;
		case 'Q': return ImGuiKey_Q;
		case 'R': return ImGuiKey_R;
		case 'S': return ImGuiKey_S;
		case 'T': return ImGuiKey_T;
		case 'U': return ImGuiKey_U;
		case 'V': return ImGuiKey_V;
		case 'W': return ImGuiKey_W;
		case 'X': return ImGuiKey_X;
		case 'Y': return ImGuiKey_Y;
		case 'Z': return ImGuiKey_Z;
		case VK_F1: return ImGuiKey_F1;
		case VK_F2: return ImGuiKey_F2;
		case VK_F3: return ImGuiKey_F3;
		case VK_F4: return ImGuiKey_F4;
		case VK_F5: return ImGuiKey_F5;
		case VK_F6: return ImGuiKey_F6;
		case VK_F7: return ImGuiKey_F7;
		case VK_F8: return ImGuiKey_F8;
		case VK_F9: return ImGuiKey_F9;
		case VK_F10: return ImGuiKey_F10;
		case VK_F11: return ImGuiKey_F11;
		case VK_F12: return ImGuiKey_F12;
		default: return ImGuiKey_None;
		}
	}

	// -----------------------------------------------------------------------
	// ImGui multi-viewport: Win32 platform windows + RHI swap chains.
	//
	// Restored from the pre-merge D3D11 backend, with the renderer half rewritten to go
	// through IRRHIDevice / IRCommandList. The platform half (window class, monitor list,
	// mouse routing) was backend agnostic then and still is; what changed is that a window's
	// frame is now recorded into an IRRHISwapChain instead of a hand-made D3D11 swap chain.
	// -----------------------------------------------------------------------

	static const WCHAR* GImGuiPlatformWindowClassName = L"PigeonEngineImGuiPlatformWindow";

	static void GetWin32WindowStyleFromViewportFlags(ImGuiViewportFlags Flags, DWORD& OutStyle, DWORD& OutExStyle)
	{
		OutStyle = (Flags & ImGuiViewportFlags_NoDecoration) ? WS_POPUP : WS_OVERLAPPEDWINDOW;
		OutExStyle = (Flags & ImGuiViewportFlags_NoTaskBarIcon) ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW;
		if (Flags & ImGuiViewportFlags_TopMost)
		{
			OutExStyle |= WS_EX_TOPMOST;
		}
	}

	static HWND GetViewportWindowHandle(ImGuiViewport* Viewport)
	{
		if (!Viewport)
		{
			return nullptr;
		}
		return Viewport->PlatformHandleRaw
			? static_cast<HWND>(Viewport->PlatformHandleRaw)
			: static_cast<HWND>(Viewport->PlatformHandle);
	}

	static void AdjustViewportWindowRect(const ImGuiViewport* Viewport, RECT& InOutRect, DWORD Style, DWORD ExStyle)
	{
		IM_UNUSED(Viewport);
		::AdjustWindowRectEx(&InOutRect, Style, FALSE, ExStyle);
	}

	// A platform window restored from the layout file can name a position that no longer
	// exists: a monitor was unplugged or rearranged, the display scale changed, or the entry
	// was written for a larger window. Windows creates it off-screen and it is then a window
	// the user cannot get back to. Bringing the rectangle into the work area of the monitor
	// it is nearest to is what makes a restored window always reachable.
	static void ClampViewportWindowRectToWorkArea(RECT& InOutRect)
	{
		const HMONITOR Monitor = ::MonitorFromRect(&InOutRect, MONITOR_DEFAULTTONEAREST);
		MONITORINFO MonitorInfo = {};
		MonitorInfo.cbSize = sizeof(MONITORINFO);
		if ((!Monitor) || (!::GetMonitorInfo(Monitor, &MonitorInfo)))
		{
			return;
		}

		const RECT& WorkArea = MonitorInfo.rcWork;
		const LONG Width = InOutRect.right - InOutRect.left;
		const LONG Height = InOutRect.bottom - InOutRect.top;
		const LONG WorkWidth = WorkArea.right - WorkArea.left;
		const LONG WorkHeight = WorkArea.bottom - WorkArea.top;

		// Keep the whole window inside the work area when it fits, and at least its top-left
		// corner when it does not (a window larger than the monitor still has to be reachable).
		LONG Left = InOutRect.left;
		if (Width <= WorkWidth)
		{
			if (Left > (WorkArea.right - Width))
			{
				Left = WorkArea.right - Width;
			}
		}
		else
		{
			Left = WorkArea.left;
		}
		if (Left < WorkArea.left)
		{
			Left = WorkArea.left;
		}
		InOutRect.left = Left;
		InOutRect.right = Left + Width;

		LONG Top = InOutRect.top;
		if (Height <= WorkHeight)
		{
			if (Top > (WorkArea.bottom - Height))
			{
				Top = WorkArea.bottom - Height;
			}
		}
		else
		{
			Top = WorkArea.top;
		}
		if (Top < WorkArea.top)
		{
			Top = WorkArea.top;
		}
		InOutRect.top = Top;
		InOutRect.bottom = Top + Height;
	}

	void CImGUIManager::InitWndViewportSupport()
	{
		WNDCLASSEXW WindowClass = {};
		WindowClass.cbSize			= sizeof(WNDCLASSEXW);
		WindowClass.style			= CS_HREDRAW | CS_VREDRAW;
		WindowClass.lpfnWndProc		= WndProcHandlerPlatformWindow;
		WindowClass.cbClsExtra		= 0;
		WindowClass.cbWndExtra		= 0;
		WindowClass.hInstance		= ::GetModuleHandle(nullptr);
		WindowClass.hIcon			= nullptr;
		WindowClass.hCursor			= nullptr;
		WindowClass.hbrBackground	= (HBRUSH)(COLOR_BACKGROUND + 1);
		WindowClass.lpszMenuName	= nullptr;
		WindowClass.lpszClassName	= GImGuiPlatformWindowClassName;
		WindowClass.hIconSm			= nullptr;
		::RegisterClassExW(&WindowClass);

		ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
		PlatformIO.Platform_CreateWindow		= PlatformCreateWindow;
		PlatformIO.Platform_DestroyWindow		= PlatformDestroyWindow;
		PlatformIO.Platform_ShowWindow			= PlatformShowWindow;
		PlatformIO.Platform_SetWindowPos		= PlatformSetWindowPos;
		PlatformIO.Platform_GetWindowPos		= PlatformGetWindowPos;
		PlatformIO.Platform_SetWindowSize		= PlatformSetWindowSize;
		PlatformIO.Platform_GetWindowSize		= PlatformGetWindowSize;
		PlatformIO.Platform_SetWindowFocus		= PlatformSetWindowFocus;
		PlatformIO.Platform_GetWindowFocus		= PlatformGetWindowFocus;
		PlatformIO.Platform_GetWindowMinimized	= PlatformGetWindowMinimized;
		PlatformIO.Platform_SetWindowTitle		= PlatformSetWindowTitle;
		PlatformIO.Platform_UpdateWindow		= PlatformUpdateWindow;
		PlatformIO.Platform_GetWindowFramebufferScale = PlatformGetWindowFramebufferScale;
		PlatformIO.Platform_GetWindowDpiScale	= PlatformGetWindowDpiScale;

		PlatformIO.Renderer_CreateWindow		= RendererCreateWindow;
		PlatformIO.Renderer_DestroyWindow		= RendererDestroyWindow;
		PlatformIO.Renderer_SetWindowSize		= RendererSetWindowSize;
		PlatformIO.Renderer_RenderWindow		= RendererRenderWindow;
		PlatformIO.Renderer_SwapBuffers			= RendererSwapBuffers;

		// The main viewport has no CreateWindow call: it is the window the engine already
		// owns, so its platform data is attached here. HwndOwned stays false, which is what
		// keeps DestroyWindow off it.
		ImGuiViewport* MainViewport = ImGui::GetMainViewport();
		if (MainViewport && (!MainViewport->PlatformUserData))
		{
			PlatformViewportData* MainViewportData = IM_NEW(PlatformViewportData)();
			MainViewportData->Hwnd		= GetManagerSingleton()->m_WndData.hWnd;
			MainViewportData->HwndOwned	= FALSE;
			MainViewport->PlatformUserData = MainViewportData;
		}

		WndUpdateMonitors();
	}

	void CImGUIManager::ShutDownWndViewportSupport()
	{
		if (ImGui::GetCurrentContext())
		{
			ImGui::DestroyPlatformWindows();
		}

		if (ImGuiViewport* MainViewport = ImGui::GetMainViewport())
		{
			PlatformViewportData* MainViewportData = static_cast<PlatformViewportData*>(MainViewport->PlatformUserData);
			if (MainViewportData)
			{
				IM_DELETE(MainViewportData);
				MainViewport->PlatformUserData = nullptr;
			}
		}

		if (GetManagerSingleton()->m_WndData.hWnd)
		{
			::SetPropA(GetManagerSingleton()->m_WndData.hWnd, "IMGUI_CONTEXT", nullptr);
		}

		ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
		PlatformIO.Platform_CreateWindow		= nullptr;
		PlatformIO.Platform_DestroyWindow		= nullptr;
		PlatformIO.Platform_ShowWindow			= nullptr;
		PlatformIO.Platform_SetWindowPos		= nullptr;
		PlatformIO.Platform_GetWindowPos		= nullptr;
		PlatformIO.Platform_SetWindowSize		= nullptr;
		PlatformIO.Platform_GetWindowSize		= nullptr;
		PlatformIO.Platform_SetWindowFocus		= nullptr;
		PlatformIO.Platform_GetWindowFocus		= nullptr;
		PlatformIO.Platform_GetWindowMinimized	= nullptr;
		PlatformIO.Platform_SetWindowTitle		= nullptr;
		PlatformIO.Platform_GetWindowFramebufferScale = nullptr;
		PlatformIO.Platform_GetWindowDpiScale	= nullptr;
		PlatformIO.Renderer_CreateWindow		= nullptr;
		PlatformIO.Renderer_DestroyWindow		= nullptr;
		PlatformIO.Renderer_SetWindowSize		= nullptr;
		PlatformIO.Renderer_RenderWindow		= nullptr;
		PlatformIO.Renderer_SwapBuffers			= nullptr;

		::UnregisterClassW(GImGuiPlatformWindowClassName, ::GetModuleHandle(nullptr));
	}

	ImGuiViewport* CImGUIManager::WndFindViewportByWindowHandle(HWND hWnd)
	{
		ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
		for (INT32 ViewportIndex = 0; ViewportIndex < PlatformIO.Viewports.Size; ViewportIndex++)
		{
			ImGuiViewport* Viewport = PlatformIO.Viewports[ViewportIndex];
			if (GetViewportWindowHandle(Viewport) == hWnd)
			{
				return Viewport;
			}
		}
		return nullptr;
	}

	void CImGUIManager::WndUpdateMouseData()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
		{
			return;
		}

		POINT ScreenPos = { 0, 0 };
		const BOOL32 bHasScreenPos = (::GetCursorPos(&ScreenPos) != FALSE) ? TRUE : FALSE;
		const FLOAT DpiScale = GetDpiScale();

		// Focus on any of our windows counts as the application being focused: a platform
		// window a panel was dragged into is one of ours, not a foreign application.
		const HWND ForegroundWindow = ::GetForegroundWindow();
		const BOOL32 bAppFocused = ((ForegroundWindow != nullptr)
			&& ((ForegroundWindow == m_WndData.hWnd)
				|| (::IsChild(ForegroundWindow, m_WndData.hWnd) != FALSE)
				|| (WndFindViewportByWindowHandle(ForegroundWindow) != nullptr))) ? TRUE : FALSE;

		if (bAppFocused != FALSE)
		{
			// Never warp the cursor while a button is held: the position ImGui asks for is
			// derived from the window it is dragging, so honouring it moves the cursor under
			// the window and the two chase each other - the panel drifts away from the pointer
			// instead of following it.
			if (io.WantSetMousePos && (m_WndData.MouseButtonsDown == 0))
			{
				// ImGui's MousePos is in logical units; Win32's SetCursorPos takes physical pixels.
				::SetCursorPos((INT32)(io.MousePos.x * DpiScale), (INT32)(io.MousePos.y * DpiScale));
			}
			else if ((m_WndData.MouseTracked == FALSE) && (bHasScreenPos != FALSE))
			{
				// Not inside any client area (over a frame, between windows, or before the
				// first WM_MOUSEMOVE): publish the OS position so ImGui always has one while
				// the application is focused. Same rule as the reference Win32 backend.
				io.AddMousePosEvent((FLOAT)ScreenPos.x / DpiScale, (FLOAT)ScreenPos.y / DpiScale);
			}
		}

		// Which of our windows is under the cursor, 0 when it is not over the application.
		ImGuiID HoveredViewportId = 0;
		if (bHasScreenPos != FALSE)
		{
			if (HWND HoveredWindow = ::WindowFromPoint(ScreenPos))
			{
				if (ImGuiViewport* HoveredViewport = WndFindViewportByWindowHandle(HoveredWindow))
				{
					HoveredViewportId = HoveredViewport->ID;
				}
			}
		}
		io.AddMouseViewportEvent(HoveredViewportId);
	}

	BOOL CALLBACK CImGUIManager::WndUpdateMonitorsEnumProc(HMONITOR monitor, HDC, LPRECT, LPARAM)
	{
		MONITORINFO MonitorInfo = {};
		MonitorInfo.cbSize = sizeof(MONITORINFO);
		if (!::GetMonitorInfo(monitor, &MonitorInfo))
		{
			return TRUE;
		}

		ImGuiPlatformMonitor ImGuiMonitor;
		// The logical space is shared by every window and matches the mouse/ImGui units, so
		// monitor rects are converted with the same UI scale. DpiScale stays 1.0f because the
		// physical density is carried per viewport by Platform_GetWindowFramebufferScale.
		const FLOAT DpiScale = GetDpiScale();
		ImGuiMonitor.MainPos	= ImVec2((FLOAT)MonitorInfo.rcMonitor.left / DpiScale, (FLOAT)MonitorInfo.rcMonitor.top / DpiScale);
		ImGuiMonitor.MainSize	= ImVec2((FLOAT)(MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left) / DpiScale, (FLOAT)(MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top) / DpiScale);
		ImGuiMonitor.WorkPos	= ImVec2((FLOAT)MonitorInfo.rcWork.left / DpiScale, (FLOAT)MonitorInfo.rcWork.top / DpiScale);
		ImGuiMonitor.WorkSize	= ImVec2((FLOAT)(MonitorInfo.rcWork.right - MonitorInfo.rcWork.left) / DpiScale, (FLOAT)(MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top) / DpiScale);
		ImGuiMonitor.DpiScale	= 1.0f;
		ImGuiMonitor.PlatformHandle = (void*)monitor;

		ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
		if (MonitorInfo.dwFlags & MONITORINFOF_PRIMARY)
		{
			PlatformIO.Monitors.push_front(ImGuiMonitor);
		}
		else
		{
			PlatformIO.Monitors.push_back(ImGuiMonitor);
		}
		return TRUE;
	}

	void CImGUIManager::WndUpdateMonitors()
	{
		ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
		PlatformIO.Monitors.resize(0);
		::EnumDisplayMonitors(nullptr, nullptr, WndUpdateMonitorsEnumProc, 0);
		GetManagerSingleton()->m_WndData.WantUpdateMonitors = FALSE;
	}

	LRESULT CALLBACK CImGUIManager::WndProcHandlerPlatformWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		ImGuiContext* WindowContext = (ImGuiContext*)::GetPropA(hWnd, "IMGUI_CONTEXT");
		if (!WindowContext)
		{
			return ::DefWindowProcW(hWnd, msg, wParam, lParam);
		}

		ImGuiContext* PreviousContext = ImGui::GetCurrentContext();
		if (PreviousContext != WindowContext)
		{
			ImGui::SetCurrentContext(WindowContext);
		}

		LRESULT Result = 0;
		if (GetManagerSingleton()->WndProcHandler(hWnd, msg, wParam, lParam))
		{
			Result = 1;
		}
		else if (ImGuiViewport* Viewport = WndFindViewportByWindowHandle(hWnd))
		{
			switch (msg)
			{
			case WM_CLOSE:
				Viewport->PlatformRequestClose = true;
				Result = 0;
				break;
			case WM_MOVE:
				Viewport->PlatformRequestMove = true;
				break;
			case WM_SIZE:
				Viewport->PlatformRequestResize = true;
				break;
			case WM_MOUSEACTIVATE:
				if (Viewport->Flags & ImGuiViewportFlags_NoFocusOnClick)
				{
					Result = MA_NOACTIVATE;
				}
				break;
			case WM_NCHITTEST:
				if (Viewport->Flags & ImGuiViewportFlags_NoInputs)
				{
					Result = HTTRANSPARENT;
				}
				break;
			}
		}

		if (PreviousContext != WindowContext)
		{
			ImGui::SetCurrentContext(PreviousContext);
		}

		if (Result != 0)
		{
			return Result;
		}
		return ::DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	void CImGUIManager::PlatformCreateWindow(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = IM_NEW(PlatformViewportData)();
		viewport->PlatformUserData = ViewportData;

		GetWin32WindowStyleFromViewportFlags(viewport->Flags, ViewportData->Style, ViewportData->ExStyle);
		ViewportData->HwndParent = GetViewportWindowHandle(viewport->ParentViewport);

		const FLOAT DpiScale = GetDpiScale();
		RECT WindowRect = { (LONG)(viewport->Pos.x * DpiScale), (LONG)(viewport->Pos.y * DpiScale), (LONG)((viewport->Pos.x + viewport->Size.x) * DpiScale), (LONG)((viewport->Pos.y + viewport->Size.y) * DpiScale) };
		AdjustViewportWindowRect(viewport, WindowRect, ViewportData->Style, ViewportData->ExStyle);
		// Only rescue a position that no monitor contains at all (a layout entry written on a
		// monitor that is gone). A window that is on any monitor - including a secondary one
		// to the left of the primary, with negative coordinates - must be created exactly where
		// it was asked to be, or dragging a panel onto another display would move it back.
		if (::MonitorFromRect(&WindowRect, MONITOR_DEFAULTTONULL) == nullptr)
		{
			ClampViewportWindowRectToWorkArea(WindowRect);
		}

		ViewportData->Hwnd = ::CreateWindowExW(
			ViewportData->ExStyle,
			GImGuiPlatformWindowClassName,
			L"PigeonEngine",
			ViewportData->Style,
			WindowRect.left,
			WindowRect.top,
			WindowRect.right - WindowRect.left,
			WindowRect.bottom - WindowRect.top,
			ViewportData->HwndParent,
			nullptr,
			::GetModuleHandle(nullptr),
			nullptr);
		ViewportData->HwndOwned = TRUE;
		viewport->PlatformHandle = (void*)ViewportData->Hwnd;
		viewport->PlatformHandleRaw = (void*)ViewportData->Hwnd;

		::SetPropA(ViewportData->Hwnd, "IMGUI_CONTEXT", ImGui::GetCurrentContext());
	}

	void CImGUIManager::PlatformDestroyWindow(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (ViewportData)
		{
			if (ViewportData->Hwnd)
			{
				// A platform window can disappear while it owns the mouse capture (the payload
				// is merged back into the main viewport when the drag is released). Hand the
				// capture to the main window instead of dropping it, or the MOUSEUP edge is
				// lost and ImGui keeps the drag alive with a button that is no longer down.
				if (::GetCapture() == ViewportData->Hwnd)
				{
					::ReleaseCapture();
					if (GetManagerSingleton()->m_WndData.hWnd)
					{
						::SetCapture(GetManagerSingleton()->m_WndData.hWnd);
					}
				}
				::SetPropA(ViewportData->Hwnd, "IMGUI_CONTEXT", nullptr);
				if (ViewportData->HwndOwned)
				{
					::DestroyWindow(ViewportData->Hwnd);
				}
			}
			IM_DELETE(ViewportData);
		}

		viewport->PlatformUserData = nullptr;
		viewport->PlatformHandle = nullptr;
		viewport->PlatformHandleRaw = nullptr;
	}

	void CImGUIManager::PlatformShowWindow(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (ViewportData && ViewportData->Hwnd)
		{
			// Showing an owned window with SW_SHOWNA can bring its owner (the main window) to
			// the front. During a drag that reorders the two windows under the cursor, which is
			// what makes a window that was just dragged out appear to jump behind the editor.
			// The owner is detached for the call and restored right after.
			const BOOL32 bAvoidBringingParentToFront = ((ViewportData->HwndParent != nullptr)
				&& ((viewport->Flags & (ImGuiViewportFlags_NoFocusOnAppearing | ImGuiViewportFlags_NoTaskBarIcon)) != 0)) ? TRUE : FALSE;
			if (bAvoidBringingParentToFront != FALSE)
			{
				::SetWindowLongPtr(ViewportData->Hwnd, GWLP_HWNDPARENT, (LONG_PTR)nullptr);
			}
			::ShowWindow(ViewportData->Hwnd, (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing) ? SW_SHOWNA : SW_SHOW);
			if (bAvoidBringingParentToFront != FALSE)
			{
				::SetWindowLongPtr(ViewportData->Hwnd, GWLP_HWNDPARENT, (LONG_PTR)ViewportData->HwndParent);
			}
		}
	}

	void CImGUIManager::PlatformUpdateWindow(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if ((!ViewportData) || (!ViewportData->Hwnd))
		{
			return;
		}

		// The owner can change after creation (ImGui reparents a floating window when it is
		// moved between viewports). Win32 calls this an owner, not a parent: the window has no
		// WS_CHILD, so CreateWindowEx()/GWLP_HWNDPARENT set an owner, which keeps the window
		// above the editor without changing its coordinate space or clipping.
		HWND NewParent = GetViewportWindowHandle(viewport->ParentViewport);
		if (NewParent != ViewportData->HwndParent)
		{
			ViewportData->HwndParent = NewParent;
			::SetWindowLongPtr(ViewportData->Hwnd, GWLP_HWNDPARENT, (LONG_PTR)NewParent);
		}
	}

	void CImGUIManager::PlatformSetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (!ViewportData || !ViewportData->Hwnd)
		{
			return;
		}
		const FLOAT DpiScale = GetDpiScale();
		RECT WindowRect = { (LONG)(pos.x * DpiScale), (LONG)(pos.y * DpiScale), (LONG)((pos.x + viewport->Size.x) * DpiScale), (LONG)((pos.y + viewport->Size.y) * DpiScale) };
		AdjustViewportWindowRect(viewport, WindowRect, ViewportData->Style, ViewportData->ExStyle);
		// No clamp while ImGui is moving the window: a window being dragged across displays
		// legitimately passes through positions that are on no monitor (the gap some
		// mixed-DPI layouts leave between two outputs). Correcting the position here would
		// snap it back every frame and the panel would stop following the cursor. The position
		// picked at creation is the only one that is validated.
		::SetWindowPos(ViewportData->Hwnd, nullptr, WindowRect.left, WindowRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	ImVec2 CImGUIManager::PlatformGetWindowPos(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (!ViewportData || !ViewportData->Hwnd)
		{
			return ImVec2(0.0f, 0.0f);
		}
		POINT Position = { 0, 0 };
		::ClientToScreen(ViewportData->Hwnd, &Position);
		const FLOAT DpiScale = GetDpiScale();
		return ImVec2((FLOAT)Position.x / DpiScale, (FLOAT)Position.y / DpiScale);
	}

	void CImGUIManager::PlatformSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (!ViewportData || !ViewportData->Hwnd)
		{
			return;
		}
		const FLOAT DpiScale = GetDpiScale();
		RECT WindowRect = { 0, 0, (LONG)(size.x * DpiScale), (LONG)(size.y * DpiScale) };
		AdjustViewportWindowRect(viewport, WindowRect, ViewportData->Style, ViewportData->ExStyle);
		::SetWindowPos(ViewportData->Hwnd, nullptr, 0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}

	ImVec2 CImGUIManager::PlatformGetWindowSize(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (!ViewportData || !ViewportData->Hwnd)
		{
			return ImVec2(0.0f, 0.0f);
		}
		RECT ClientRect = {};
		::GetClientRect(ViewportData->Hwnd, &ClientRect);
		const FLOAT DpiScale = GetDpiScale();
		return ImVec2((FLOAT)(ClientRect.right - ClientRect.left) / DpiScale, (FLOAT)(ClientRect.bottom - ClientRect.top) / DpiScale);
	}

	ImVec2 CImGUIManager::PlatformGetWindowFramebufferScale(ImGuiViewport*)
	{
		// ImGui's layout is logical at 96 DPI; every window is rendered at the same physical
		// density, which is what DisplayFramebufferScale carries. ImGui 1.92 uses this value
		// to rasterize the font atlas at the matching density, so text stays crisp.
		const FLOAT DpiScale = GetDpiScale();
		return ImVec2(DpiScale, DpiScale);
	}

	FLOAT CImGUIManager::PlatformGetWindowDpiScale(ImGuiViewport*)
	{
		// The logical space is 96 DPI for all windows (see PlatformGetWindowFramebufferScale);
		// returning a per-monitor scale here would make ImGui scale the layout a second time.
		return 1.0f;
	}

	void CImGUIManager::PlatformSetWindowFocus(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (ViewportData && ViewportData->Hwnd)
		{
			::BringWindowToTop(ViewportData->Hwnd);
			::SetForegroundWindow(ViewportData->Hwnd);
			::SetFocus(ViewportData->Hwnd);
		}
	}

	bool CImGUIManager::PlatformGetWindowFocus(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		return (ViewportData && ViewportData->Hwnd && ::GetForegroundWindow() == ViewportData->Hwnd);
	}

	bool CImGUIManager::PlatformGetWindowMinimized(ImGuiViewport* viewport)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		return (ViewportData && ViewportData->Hwnd && ::IsIconic(ViewportData->Hwnd) != 0);
	}

	void CImGUIManager::PlatformSetWindowTitle(ImGuiViewport* viewport, const char* title)
	{
		PlatformViewportData* ViewportData = static_cast<PlatformViewportData*>(viewport->PlatformUserData);
		if (ViewportData && ViewportData->Hwnd)
		{
			::SetWindowTextA(ViewportData->Hwnd, title);
		}
	}

	void CImGUIManager::RendererCreateWindow(ImGuiViewport* viewport)
	{
		RendererViewportData* ViewportData = IM_NEW(RendererViewportData)();
		viewport->RendererUserData = ViewportData;

		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		HWND hWnd = GetViewportWindowHandle(viewport);
		if ((!Device) || (!hWnd))
		{
			return;
		}

		// R8G8B8A8_UNORM: the format the ImGui pass's pipeline state declares, and therefore
		// the one a target this backend records into has to carry. The swap chain is physical
		// (the window's client size); ImGui's viewport size is logical.
		const FLOAT DpiScale = GetDpiScale();
		ViewportData->SwapChain = Device->CreateSwapChainForWindow(hWnd
			, static_cast<UINT32>(viewport->Size.x * DpiScale), static_cast<UINT32>(viewport->Size.y * DpiScale)
			, RFormatType::FORMAT_R8G8B8A8_UNORM);
	}

	void CImGUIManager::RendererDestroyWindow(ImGuiViewport* viewport)
	{
		RendererViewportData* ViewportData = static_cast<RendererViewportData*>(viewport->RendererUserData);
		if (ViewportData)
		{
			delete ViewportData->SwapChain;
			ViewportData->SwapChain = nullptr;
			IM_DELETE(ViewportData);
		}
		viewport->RendererUserData = nullptr;
	}

	void CImGUIManager::RendererSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
	{
		RendererViewportData* ViewportData = static_cast<RendererViewportData*>(viewport->RendererUserData);
		if ((!ViewportData) || (!ViewportData->SwapChain))
		{
			return;
		}
		const FLOAT DpiScale = GetDpiScale();
		ViewportData->SwapChain->Resize(static_cast<UINT32>(size.x * DpiScale), static_cast<UINT32>(size.y * DpiScale));
	}

	void CImGUIManager::RendererRenderWindow(ImGuiViewport* viewport, void*)
	{
		if (!viewport)
		{
			return;
		}
		RendererViewportData* ViewportData = static_cast<RendererViewportData*>(viewport->RendererUserData);
		if ((!ViewportData) || (!ViewportData->SwapChain))
		{
			return;
		}
		IRRHITexture* Target = ViewportData->SwapChain->GetBackBuffer();
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!Target) || (!Device))
		{
			return;
		}

		IRCommandList* Cmd = Device->AcquireCommandList();
		if (!Cmd)
		{
			return;
		}
		EMainManager::GetManagerSingleton()->SetActiveCommandList(Cmd);

		// These passes are recorded after the main renderer closed the frame's profiling
		// slice, so they are dropped rather than charged to it - the same reason the asset
		// preview renderer drops its own.
		EGPUProfiler::GetManagerSingleton()->SetPassRecordingEnabled(FALSE);

		const BOOL8 bClear = ((viewport->Flags & ImGuiViewportFlags_NoRendererClear) == 0) ? TRUE : FALSE;
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount			= 1u;
		Pass.RenderTargets[0].Target	= Target;
		Pass.RenderTargets[0].LoadOp	= bClear ? ERHILoadOp::RHI_LOAD_OP_CLEAR : ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.RenderTargets[0].StoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.RenderTargets[0].ClearColor= Color4(0.f, 0.f, 0.f, 1.f);
		Pass.DepthStencil.Target		= nullptr;
		Pass.PassName					= PE_TEXT("ImGuiViewport");
		Cmd->BeginRenderPass(Pass);

		// The renderer sets its own viewport from the draw data's display size, which is the
		// window's client size here - the same call the main window's overlay makes.
		GetManagerSingleton()->Renderer.Render(Cmd, viewport->DrawData);

		Cmd->EndRenderPass();
		// A swap chain buffer is a presentation source after the pass; the next frame's pass
		// brings it back to a render target through its own LoadOp.
		Cmd->ResourceBarrier(Target, ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET, ERHIResourceState::RHI_RESOURCE_STATE_PRESENT);

		EGPUProfiler::GetManagerSingleton()->SetPassRecordingEnabled(TRUE);

		Device->SubmitCommandList(Cmd);
		EMainManager::GetManagerSingleton()->SetActiveCommandList(nullptr);
	}

	void CImGUIManager::RendererSwapBuffers(ImGuiViewport* viewport, void*)
	{
		RendererViewportData* ViewportData = static_cast<RendererViewportData*>(viewport->RendererUserData);
		if ((!ViewportData) || (!ViewportData->SwapChain))
		{
			return;
		}
		ViewportData->SwapChain->Present(0u);
	}

	PE_INLINE ImTextureID EngineTextureToImgui(const ETexture2DAsset* InAsset)
	{
		// ImTextureID_Invalid == 0, but our bindless indices start at 0 too.
		// We store (RealBindlessIndex + 1) so index 0 is never confused with
		// Invalid. EImGuiRenderer subtracts 1 before passing to the RHI.
		if (InAsset && InAsset->IsRenderResourceValid())
		{
			const RTexture2DResource* RenderResource = InAsset->GetRenderResource();
			if (RenderResource)
			{
				const UINT32 Idx = RenderResource->GetBindlessSRVIndex();
				if (Idx != RHI_INVALID_BINDLESS_INDEX)
				{
					return static_cast<ImTextureID>(Idx + 1u);
				}
			}
		}
		return ImTextureID_Invalid;
	}

	PE_INLINE ImTextureID EngineTextureToImgui(IRRHITexture* InTexture)
	{
		// Overload for raw IRRHITexture* (e.g. render textures, font atlas,
		// any RHI-level texture not wrapped in an asset). Registers the
		// texture as a bindless SRV if it hasn't been registered yet, then
		// returns the ImGui-offset ID (RealIndex + 1).
		if (!InTexture) { return ImTextureID_Invalid; }
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!Device) { return ImTextureID_Invalid; }
		UINT32 Idx = InTexture->GetBindlessSRVIndex();
		if (Idx == RHI_INVALID_BINDLESS_INDEX)
		{
			Idx = Device->RegisterBindlessSRV(InTexture);
		}
		if (Idx == RHI_INVALID_BINDLESS_INDEX) { return ImTextureID_Invalid; }
		return static_cast<ImTextureID>(Idx + 1u);
	}

#endif
};
