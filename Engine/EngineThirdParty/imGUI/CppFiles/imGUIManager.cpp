#include "../Headers/imGUIManager.h"
#include "../../../Sources/EngineCore/Main/MainManager.h"
#include "../../../Sources/EngineCore/Main/GameThreadCommandQueue.h"
#include <Base/Timer/Timer.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <Config/EngineConfig.h>
#include <TextureAsset/TextureAsset.h>

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
	void CImGUIManager::Update()
	{
		ImGuiIO& io = ImGui::GetIO();
		const EGameTimer* GameTimer = EMainManager::GetManagerSingleton()->GetGameTimer();
		io.DeltaTime = static_cast<FLOAT>(GameTimer->GetDeltaTime());

		{
			const Vector2Int& WindowSize = EMainManager::GetManagerSingleton()->GetWindowSize();
			io.DisplaySize = ImVec2(static_cast<FLOAT>(WindowSize.x), static_cast<FLOAT>(WindowSize.y));
		}

		WndProcessKeyEventsWorkarounds();

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
	}
	void CImGUIManager::PrepareDrawData()
	{
		// Finalize ImGui frame on the main thread. After this returns,
		// ImGui::GetDrawData() is valid and stable until the next ImGui::NewFrame.
		ImGui::Render();
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
	void CImGUIManager::InitWnd()
	{
		ImGuiIO& io = ImGui::GetIO();

		//PE_CHECK((ENGINE_RENDER_CORE_ERROR), PE_TEXT("Already initialized a platform backend!"), (io.BackendPlatformUserData == nullptr));
		IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

		io.BackendPlatformUserData = (void*)(&m_WndData);
		io.BackendPlatformName = "imgui_impl_win32";
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		// dock 分支：必须打开 DockingEnable，否则 EditorManager 里的
		// DockSpaceOverViewport() 不会生成任何 dock node，编辑器面板无法停靠。
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		HWND hWnd = EMainManager::GetManagerSingleton()->GetWindowHandle();
		m_WndData.hWnd = hWnd;
		m_WndData.WantUpdateHasGamepad = FALSE;
		m_WndData.LastMouseCursor = ImGuiMouseCursor_COUNT;

		ImGui::GetMainViewport()->PlatformHandleRaw = (void*)hWnd;
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
			if (!m_WndData.MouseTracked)
			{
				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
				::TrackMouseEvent(&tme);
				m_WndData.MouseTracked = TRUE;
			}
			io.AddMousePosEvent((FLOAT)GET_X_LPARAM(lParam), (FLOAT)GET_Y_LPARAM(lParam));
			break;
		case WM_MOUSELEAVE:
			if (m_WndData.MouseHwnd == hWnd)
			{
				m_WndData.MouseHwnd = nullptr;
			}
			m_WndData.MouseTracked = FALSE;
			io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
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
			if (m_WndData.MouseButtonsDown == 0 && ::GetCapture() == nullptr)
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
