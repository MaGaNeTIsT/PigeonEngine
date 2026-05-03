#include "../Headers/imGUIManager.h"
#include "../../../Sources/EngineCore/Main/MainManager.h"
#include <Base/Timer/Timer.h>
#include <RenderDevice/DeviceD3D11.h>
#include <Config/EngineConfig.h>
#include <TextureAsset/TextureAsset.h>
#include <cstddef>

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
#ifndef WM_DPICHANGED
#define WM_DPICHANGED               0x02E0
#endif

#define IMGUI_VS_FILE_NAME_PATH     ("imGUI_PEVS.cso")
#define IMGUI_PS_FILE_NAME_PATH     ("imGUI_PEPS.cso")

    static const WCHAR* GImGuiPlatformWindowClassName = L"PigeonEngineImGuiPlatform";

    struct ImGuiPlatformViewportData
    {
        HWND    Hwnd;
        HWND    HwndParent;
        BOOL32  HwndOwned;
        DWORD   Style;
        DWORD   ExStyle;

        ImGuiPlatformViewportData()
            : Hwnd(nullptr)
            , HwndParent(nullptr)
            , HwndOwned(FALSE)
            , Style(0u)
            , ExStyle(0u)
        {
        }
    };

    struct ImGuiRendererViewportData
    {
        IDXGISwapChain*          SwapChain;
        ID3D11RenderTargetView*  RenderTargetView;

        ImGuiRendererViewportData()
            : SwapChain(nullptr)
            , RenderTargetView(nullptr)
        {
        }
    };

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
        return Viewport->PlatformHandleRaw ? static_cast<HWND>(Viewport->PlatformHandleRaw) : static_cast<HWND>(Viewport->PlatformHandle);
    }

    static void AdjustViewportWindowRect(const ImGuiViewport* Viewport, RECT& InOutRect, DWORD Style, DWORD ExStyle)
    {
        IM_UNUSED(Viewport);
        ::AdjustWindowRectEx(&InOutRect, Style, FALSE, ExStyle);
    }

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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        io.IniFilename = "./Engine/Configs/ImGUIEngine.ini";
        io.LogFilename = "./Engine/Configs/ImGUIEngineLog.txt";
        io.IniSavingRate = 30.f;

        InitWnd();
        InitD3D();

        RECT rect = { 0, 0, 0, 0 };
        ::GetClientRect(m_WndData.hWnd, &rect);
        io.DisplaySize = ImVec2((FLOAT)(rect.right - rect.left), (FLOAT)(rect.bottom - rect.top));
    }
    void CImGUIManager::ShutDown()
    {
        ImGuiIO& io = ImGui::GetIO();

        ShutDownWndViewportSupport();
        ShutDownD3DViewportSupport();

        D3DInvalidateDeviceObjects();
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

        if (m_WndData.WantUpdateMonitors)
        {
            WndUpdateMonitors();
        }

        WndUpdateMouseData();

        WndProcessKeyEventsWorkarounds();

        ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
        if (m_WndData.LastMouseCursor != mouse_cursor)
        {
            m_WndData.LastMouseCursor = mouse_cursor;
            WndUpdateMouseCursor();
        }

        if (!m_D3DData.FontSampler)
        {
            D3DCreateDeviceObjects();
        }

        ImGui::NewFrame();
    }
    void CImGUIManager::Draw()
    {
        ImGui::Render();
        D3DRenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
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
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
        io.BackendFlags |= ImGuiBackendFlags_HasParentViewport;

        HWND hWnd = EMainManager::GetManagerSingleton()->GetWindowHandle();
        m_WndData.hWnd = hWnd;
        m_WndData.WantUpdateHasGamepad = FALSE;
        m_WndData.WantUpdateMonitors = TRUE;
        m_WndData.LastMouseCursor = ImGuiMouseCursor_COUNT;

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
            if (!m_WndData.MouseTracked)
            {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                ::TrackMouseEvent(&tme);
                m_WndData.MouseTracked = TRUE;
            }
            {
                POINT mouse_pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    ::ClientToScreen(hWnd, &mouse_pos);
                }
                io.AddMousePosEvent((FLOAT)mouse_pos.x, (FLOAT)mouse_pos.y);
            }
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
        case WM_DISPLAYCHANGE:
            m_WndData.WantUpdateMonitors = TRUE;
            return 0;
        case WM_SETTINGCHANGE:
            m_WndData.WantUpdateMonitors = TRUE;
            return 0;
        case WM_DPICHANGED:
            m_WndData.WantUpdateMonitors = TRUE;
            if (io.ConfigDpiScaleViewports)
            {
                const RECT* suggested_rect = (const RECT*)lParam;
                ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
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
        io.AddKeyEvent(ImGuiMod_Ctrl, CImGUIManager::WndIsVkDown(VK_CONTROL));
        io.AddKeyEvent(ImGuiMod_Shift, CImGUIManager::WndIsVkDown(VK_SHIFT));
        io.AddKeyEvent(ImGuiMod_Alt, CImGUIManager::WndIsVkDown(VK_MENU));
        io.AddKeyEvent(ImGuiMod_Super, CImGUIManager::WndIsVkDown(VK_LWIN) || CImGUIManager::WndIsVkDown(VK_RWIN));
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
    ImGuiViewport* CImGUIManager::WndFindViewportByWindowHandle(HWND hWnd)
    {
        ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
        for (INT32 Index = 0; Index < PlatformIO.Viewports.Size; ++Index)
        {
            ImGuiViewport* Viewport = PlatformIO.Viewports[Index];
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

        POINT ScreenPos;
        if (::GetCursorPos(&ScreenPos))
        {
            io.AddMouseViewportEvent(0);
            if (HWND HoveredWindow = ::WindowFromPoint(ScreenPos))
            {
                if (ImGuiViewport* HoveredViewport = WndFindViewportByWindowHandle(HoveredWindow))
                {
                    io.AddMouseViewportEvent(HoveredViewport->ID);
                }
            }
        }

        if (io.WantSetMousePos)
        {
            ::SetCursorPos((INT32)io.MousePos.x, (INT32)io.MousePos.y);
        }
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
        ImGuiMonitor.MainPos = ImVec2((FLOAT)MonitorInfo.rcMonitor.left, (FLOAT)MonitorInfo.rcMonitor.top);
        ImGuiMonitor.MainSize = ImVec2((FLOAT)(MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left), (FLOAT)(MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top));
        ImGuiMonitor.WorkPos = ImVec2((FLOAT)MonitorInfo.rcWork.left, (FLOAT)MonitorInfo.rcWork.top);
        ImGuiMonitor.WorkSize = ImVec2((FLOAT)(MonitorInfo.rcWork.right - MonitorInfo.rcWork.left), (FLOAT)(MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top));
        ImGuiMonitor.DpiScale = 1.0f;
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
    void CImGUIManager::InitWndViewportSupport()
    {
        WNDCLASSEXW WindowClass = {};
        WindowClass.cbSize = sizeof(WNDCLASSEXW);
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        WindowClass.lpfnWndProc = WndProcHandlerPlatformWindow;
        WindowClass.cbClsExtra = 0;
        WindowClass.cbWndExtra = 0;
        WindowClass.hInstance = ::GetModuleHandle(nullptr);
        WindowClass.hIcon = nullptr;
        WindowClass.hCursor = nullptr;
        WindowClass.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
        WindowClass.lpszMenuName = nullptr;
        WindowClass.lpszClassName = GImGuiPlatformWindowClassName;
        WindowClass.hIconSm = nullptr;
        ::RegisterClassExW(&WindowClass);

        ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
        PlatformIO.Platform_CreateWindow = PlatformCreateWindow;
        PlatformIO.Platform_DestroyWindow = PlatformDestroyWindow;
        PlatformIO.Platform_ShowWindow = PlatformShowWindow;
        PlatformIO.Platform_SetWindowPos = PlatformSetWindowPos;
        PlatformIO.Platform_GetWindowPos = PlatformGetWindowPos;
        PlatformIO.Platform_SetWindowSize = PlatformSetWindowSize;
        PlatformIO.Platform_GetWindowSize = PlatformGetWindowSize;
        PlatformIO.Platform_SetWindowFocus = PlatformSetWindowFocus;
        PlatformIO.Platform_GetWindowFocus = PlatformGetWindowFocus;
        PlatformIO.Platform_GetWindowMinimized = PlatformGetWindowMinimized;
        PlatformIO.Platform_SetWindowTitle = PlatformSetWindowTitle;

        ImGuiViewport* MainViewport = ImGui::GetMainViewport();
        ImGuiPlatformViewportData* ViewportData = IM_NEW(ImGuiPlatformViewportData)();
        ViewportData->Hwnd = GetManagerSingleton()->m_WndData.hWnd;
        ViewportData->HwndOwned = FALSE;
        MainViewport->PlatformUserData = ViewportData;

        WndUpdateMonitors();
    }
    void CImGUIManager::ShutDownWndViewportSupport()
    {
        if (ImGui::GetCurrentContext())
        {
            ImGui::DestroyPlatformWindows();
        }

        if (GetManagerSingleton()->m_WndData.hWnd)
        {
            ::SetPropA(GetManagerSingleton()->m_WndData.hWnd, "IMGUI_CONTEXT", nullptr);
        }

        ::UnregisterClassW(GImGuiPlatformWindowClassName, ::GetModuleHandle(nullptr));
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
        ImGuiPlatformViewportData* ViewportData = IM_NEW(ImGuiPlatformViewportData)();
        viewport->PlatformUserData = ViewportData;

        GetWin32WindowStyleFromViewportFlags(viewport->Flags, ViewportData->Style, ViewportData->ExStyle);
        ViewportData->HwndParent = GetViewportWindowHandle(viewport->ParentViewport);

        RECT WindowRect = { (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)(viewport->Pos.x + viewport->Size.x), (LONG)(viewport->Pos.y + viewport->Size.y) };
        AdjustViewportWindowRect(viewport, WindowRect, ViewportData->Style, ViewportData->ExStyle);

        ViewportData->Hwnd = ::CreateWindowExW(
            ViewportData->ExStyle,
            GImGuiPlatformWindowClassName,
            L"PigeonEngine ImGui Viewport",
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
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (ViewportData)
        {
            if (ViewportData->Hwnd)
            {
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
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (ViewportData && ViewportData->Hwnd)
        {
            ::ShowWindow(ViewportData->Hwnd, (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing) ? SW_SHOWNA : SW_SHOW);
        }
    }
    void CImGUIManager::PlatformSetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (!ViewportData || !ViewportData->Hwnd)
        {
            return;
        }
        RECT WindowRect = { (LONG)pos.x, (LONG)pos.y, (LONG)pos.x, (LONG)pos.y };
        AdjustViewportWindowRect(viewport, WindowRect, ViewportData->Style, ViewportData->ExStyle);
        ::SetWindowPos(ViewportData->Hwnd, nullptr, WindowRect.left, WindowRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    ImVec2 CImGUIManager::PlatformGetWindowPos(ImGuiViewport* viewport)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (!ViewportData || !ViewportData->Hwnd)
        {
            return ImVec2(0.0f, 0.0f);
        }
        POINT Position = { 0, 0 };
        ::ClientToScreen(ViewportData->Hwnd, &Position);
        return ImVec2((FLOAT)Position.x, (FLOAT)Position.y);
    }
    void CImGUIManager::PlatformSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (!ViewportData || !ViewportData->Hwnd)
        {
            return;
        }
        RECT WindowRect = { 0, 0, (LONG)size.x, (LONG)size.y };
        AdjustViewportWindowRect(viewport, WindowRect, ViewportData->Style, ViewportData->ExStyle);
        ::SetWindowPos(ViewportData->Hwnd, nullptr, 0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
    }
    ImVec2 CImGUIManager::PlatformGetWindowSize(ImGuiViewport* viewport)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (!ViewportData || !ViewportData->Hwnd)
        {
            return ImVec2(0.0f, 0.0f);
        }
        RECT ClientRect = {};
        ::GetClientRect(ViewportData->Hwnd, &ClientRect);
        return ImVec2((FLOAT)(ClientRect.right - ClientRect.left), (FLOAT)(ClientRect.bottom - ClientRect.top));
    }
    void CImGUIManager::PlatformSetWindowFocus(ImGuiViewport* viewport)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (ViewportData && ViewportData->Hwnd)
        {
            ::BringWindowToTop(ViewportData->Hwnd);
            ::SetForegroundWindow(ViewportData->Hwnd);
            ::SetFocus(ViewportData->Hwnd);
        }
    }
    bool CImGUIManager::PlatformGetWindowFocus(ImGuiViewport* viewport)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        return (ViewportData && ViewportData->Hwnd && ::GetForegroundWindow() == ViewportData->Hwnd);
    }
    bool CImGUIManager::PlatformGetWindowMinimized(ImGuiViewport* viewport)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        return (ViewportData && ViewportData->Hwnd && ::IsIconic(ViewportData->Hwnd) != 0);
    }
    void CImGUIManager::PlatformSetWindowTitle(ImGuiViewport* viewport, const char* title)
    {
        ImGuiPlatformViewportData* ViewportData = (ImGuiPlatformViewportData*)viewport->PlatformUserData;
        if (ViewportData && ViewportData->Hwnd)
        {
            ::SetWindowTextA(ViewportData->Hwnd, title);
        }
    }
    void CImGUIManager::InitD3D()
    {
        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

        io.BackendRendererUserData = (void*)(&m_D3DData);
        io.BackendRendererName = "imgui_impl_dx11";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

        InitD3DViewportSupport();
    }
    void CImGUIManager::D3DSetupRenderState(ImDrawData* drawData)
    {
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> ctx = RDeviceD3D11::GetDeviceSingleton()->GetRenderDeviceContext();

        D3D11_VIEWPORT vp;
        ::ZeroMemory(&vp, sizeof(vp));
        vp.Width = drawData->DisplaySize.x;
        vp.Height = drawData->DisplaySize.y;
        vp.MinDepth = 0.f;
        vp.MaxDepth = 1.f;
        vp.TopLeftX = vp.TopLeftY = 0.f;
        ctx->RSSetViewports(1u, &vp);

        UINT32 stride = sizeof(ImDrawVert);
        UINT32 offset = 0u;
        ctx->IASetInputLayout(m_D3DData.InputLayout);
        ctx->IASetVertexBuffers(0u, 1u, &m_D3DData.VertexBuffer, &stride, &offset);
        ctx->IASetIndexBuffer(m_D3DData.IndexBuffer, sizeof(ImDrawIdx) == 2u ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0u);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(m_D3DData.VertexShader, nullptr, 0u);
        ctx->VSSetConstantBuffers(0u, 1u, &m_D3DData.ConstantBuffer);
        ctx->PSSetShader(m_D3DData.PixelShader, nullptr, 0u);
        ctx->PSSetSamplers(0u, 1u, &m_D3DData.FontSampler);
        ctx->GSSetShader(nullptr, nullptr, 0u);
        ctx->HSSetShader(nullptr, nullptr, 0u);
        ctx->DSSetShader(nullptr, nullptr, 0u);
        ctx->CSSetShader(nullptr, nullptr, 0u);

        const FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
        ctx->OMSetBlendState(m_D3DData.BlendState, blendFactor, 0xffffffff);
        ctx->OMSetDepthStencilState(m_D3DData.DepthStencilState, 0u);
        ctx->RSSetState(m_D3DData.RasterizerState);
    }
    void CImGUIManager::D3DRenderDrawData(ImDrawData* drawData)
    {
        if (drawData->DisplaySize.x <= 0.f || drawData->DisplaySize.y <= 0.f)
            return;

        Microsoft::WRL::ComPtr<ID3D11DeviceContext> ctx = RDeviceD3D11::GetDeviceSingleton()->GetRenderDeviceContext();
        Microsoft::WRL::ComPtr<ID3D11Device> dvc = RDeviceD3D11::GetDeviceSingleton()->GetRenderDevice();

        if (!m_D3DData.VertexBuffer || m_D3DData.VertexBufferSize < drawData->TotalVtxCount)
        {
            if (m_D3DData.VertexBuffer) { m_D3DData.VertexBuffer->Release(); m_D3DData.VertexBuffer = nullptr; }
            m_D3DData.VertexBufferSize = drawData->TotalVtxCount + 5000;
            D3D11_BUFFER_DESC desc;
            ::ZeroMemory(&desc, sizeof(desc));
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = m_D3DData.VertexBufferSize * sizeof(ImDrawVert);
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0u;
            HRESULT hr = dvc->CreateBuffer(&desc, nullptr, &m_D3DData.VertexBuffer);
            if (FAILED(hr))
            {
                return;
            }
        }
        if (!m_D3DData.IndexBuffer || m_D3DData.IndexBufferSize < drawData->TotalIdxCount)
        {
            if (m_D3DData.IndexBuffer) { m_D3DData.IndexBuffer->Release(); m_D3DData.IndexBuffer = nullptr; }
            m_D3DData.IndexBufferSize = drawData->TotalIdxCount + 10000;
            D3D11_BUFFER_DESC desc;
            ::ZeroMemory(&desc, sizeof(desc));
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = m_D3DData.IndexBufferSize * sizeof(ImDrawIdx);
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            HRESULT hr = dvc->CreateBuffer(&desc, nullptr, &m_D3DData.IndexBuffer);
            if (FAILED(hr))
            {
                return;
            }
        }

        D3D11_MAPPED_SUBRESOURCE vtxResource, idxResource;
        if (ctx->Map(m_D3DData.VertexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &vtxResource) != S_OK)
        {
            return;
        }
        if (ctx->Map(m_D3DData.IndexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &idxResource) != S_OK)
        {
            return;
        }
        ImDrawVert* vtxDst = (ImDrawVert*)vtxResource.pData;
        ImDrawIdx* idxDst = (ImDrawIdx*)idxResource.pData;
        for (INT32 n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            ::memcpy_s(vtxDst, m_D3DData.VertexBufferSize * sizeof(ImDrawVert), cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            ::memcpy_s(idxDst, m_D3DData.IndexBufferSize * sizeof(ImDrawIdx), cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmdList->VtxBuffer.Size;
            idxDst += cmdList->IdxBuffer.Size;
        }
        ctx->Unmap(m_D3DData.VertexBuffer, 0u);
        ctx->Unmap(m_D3DData.IndexBuffer, 0u);

        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (ctx->Map(m_D3DData.ConstantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mappedResource) != S_OK)
            {
                return;
            }
            D3DConstantBuffer* constantBuffer = (D3DConstantBuffer*)mappedResource.pData;
            FLOAT L = drawData->DisplayPos.x;
            FLOAT R = drawData->DisplayPos.x + drawData->DisplaySize.x;
            FLOAT T = drawData->DisplayPos.y;
            FLOAT B = drawData->DisplayPos.y + drawData->DisplaySize.y;
            FLOAT mvp[4][4] =
            {
                { 2.0f / (R - L),       0.0f,               0.0f,   0.0f },
                { 0.0f,                 2.0f / (T - B),     0.0f,   0.0f },
                { 0.0f,                 0.0f,               0.5f,   0.0f },
                { (R + L) / (L - R),    (T + B) / (B - T),  0.5f,   1.0f },
            };
            ::memcpy_s(&constantBuffer->MVP, sizeof(constantBuffer->MVP), mvp, sizeof(mvp));
            ctx->Unmap(m_D3DData.ConstantBuffer, 0u);
        }

        struct BACKUP_DX11_STATE
        {
            UINT32                        ScissorRectsCount, ViewportsCount;
            D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
            D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
            ID3D11RasterizerState*      RS;
            ID3D11BlendState*           BlendState;
            FLOAT                       BlendFactor[4];
            UINT32                        SampleMask;
            UINT32                        StencilRef;
            ID3D11DepthStencilState*    DepthStencilState;
            ID3D11ShaderResourceView*   PSShaderResource;
            ID3D11SamplerState*         PSSampler;
            ID3D11PixelShader*          PS;
            ID3D11VertexShader*         VS;
            ID3D11GeometryShader*       GS;
            UINT32                        PSInstancesCount, VSInstancesCount, GSInstancesCount;
            ID3D11ClassInstance*        PSInstances[256], * VSInstances[256], * GSInstances[256];
            D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
            ID3D11Buffer                * IndexBuffer, * VertexBuffer, * VSConstantBuffer;
            UINT32                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
            DXGI_FORMAT                 IndexBufferFormat;
            ID3D11InputLayout*          InputLayout;
        };
        BACKUP_DX11_STATE old = {};
        old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
        ctx->RSGetViewports(&old.ViewportsCount, old.Viewports);
        ctx->RSGetState(&old.RS);
        ctx->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
        ctx->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
        ctx->PSGetShaderResources(0u, 1u, &old.PSShaderResource);
        ctx->PSGetSamplers(0u, 1u, &old.PSSampler);
        old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256u;
        ctx->PSGetShader(&old.PS, old.PSInstances, &old.PSInstancesCount);
        ctx->VSGetShader(&old.VS, old.VSInstances, &old.VSInstancesCount);
        ctx->VSGetConstantBuffers(0u, 1u, &old.VSConstantBuffer);
        ctx->GSGetShader(&old.GS, old.GSInstances, &old.GSInstancesCount);

        ctx->IAGetPrimitiveTopology(&old.PrimitiveTopology);
        ctx->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
        ctx->IAGetVertexBuffers(0u, 1u, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
        ctx->IAGetInputLayout(&old.InputLayout);

        CImGUIManager::D3DSetupRenderState(drawData);

        INT32 globalIdxOffset = 0;
        INT32 globalVtxOffset = 0;
        ImVec2 clipOff = drawData->DisplayPos;
        for (INT32 n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            for (INT32 iCmd = 0; iCmd < cmdList->CmdBuffer.Size; iCmd++)
            {
                const ImDrawCmd* pCmd = &cmdList->CmdBuffer[iCmd];
                if (pCmd->UserCallback != nullptr)
                {
                    if (pCmd->UserCallback == ImDrawCallback_ResetRenderState)
                    {
                        CImGUIManager::D3DSetupRenderState(drawData);
                    }
                    else
                    {
                        pCmd->UserCallback(cmdList, pCmd);
                    }
                }
                else
                {
                    ImVec2 clipMin(pCmd->ClipRect.x - clipOff.x, pCmd->ClipRect.y - clipOff.y);
                    ImVec2 clipMax(pCmd->ClipRect.z - clipOff.x, pCmd->ClipRect.w - clipOff.y);
                    if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                    {
                        continue;
                    }

                    const D3D11_RECT r = { (LONG)clipMin.x, (LONG)clipMin.y, (LONG)clipMax.x, (LONG)clipMax.y };
                    ctx->RSSetScissorRects(1u, &r);

                    ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pCmd->GetTexID();
                    ctx->PSSetShaderResources(0u, 1u, &texture_srv);
                    ctx->DrawIndexed(pCmd->ElemCount, pCmd->IdxOffset + globalIdxOffset, pCmd->VtxOffset + globalVtxOffset);
                }
            }
            globalIdxOffset += cmdList->IdxBuffer.Size;
            globalVtxOffset += cmdList->VtxBuffer.Size;
        }

        ctx->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
        ctx->RSSetViewports(old.ViewportsCount, old.Viewports);
        ctx->RSSetState(old.RS); if (old.RS) old.RS->Release();
        ctx->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
        ctx->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
        ctx->PSSetShaderResources(0u, 1u, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
        ctx->PSSetSamplers(0u, 1u, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
        ctx->PSSetShader(old.PS, old.PSInstances, old.PSInstancesCount); if (old.PS) old.PS->Release();
        for (UINT32 i = 0u; i < old.PSInstancesCount; i++) if (old.PSInstances[i]) old.PSInstances[i]->Release();
        ctx->VSSetShader(old.VS, old.VSInstances, old.VSInstancesCount); if (old.VS) old.VS->Release();
        ctx->VSSetConstantBuffers(0u, 1u, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
        ctx->GSSetShader(old.GS, old.GSInstances, old.GSInstancesCount); if (old.GS) old.GS->Release();
        for (UINT32 i = 0u; i < old.VSInstancesCount; i++) if (old.VSInstances[i]) old.VSInstances[i]->Release();
        ctx->IASetPrimitiveTopology(old.PrimitiveTopology);
        ctx->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
        ctx->IASetVertexBuffers(0u, 1u, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
        ctx->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
    }
    void CImGUIManager::D3DCreateFontsTexture()
    {
        Microsoft::WRL::ComPtr<ID3D11Device> dvc = RDeviceD3D11::GetDeviceSingleton()->GetRenderDevice();
        ImGuiIO& io = ImGui::GetIO();
        UCHAR* pixels;
        INT32 width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        {
            D3D11_TEXTURE2D_DESC desc;
            ::ZeroMemory(&desc, sizeof(desc));
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1u;
            desc.ArraySize = 1u;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1u;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0u;

            ID3D11Texture2D* texture2D = nullptr;
            D3D11_SUBRESOURCE_DATA subResource;
            subResource.pSysMem = pixels;
            subResource.SysMemPitch = desc.Width * 4u;
            subResource.SysMemSlicePitch = 0u;
            HRESULT hr = dvc->CreateTexture2D(&desc, &subResource, &texture2D);
            IM_ASSERT(texture2D != nullptr);
            if (FAILED(hr))
            {
                return;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ::ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0u;
            hr = dvc->CreateShaderResourceView(texture2D, &srvDesc, &m_D3DData.FontTextureView);
            texture2D->Release();
            if (FAILED(hr))
            {
                return;
            }
        }

        io.Fonts->SetTexID((ImTextureID)m_D3DData.FontTextureView);

        {
            D3D11_SAMPLER_DESC desc;
            ::ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.MipLODBias = 0.f;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.MinLOD = 0.f;
            desc.MaxLOD = 0.f;
            HRESULT hr = dvc->CreateSamplerState(&desc, &m_D3DData.FontSampler);
            if (FAILED(hr))
            {
                return;
            }
        }
    }
    BOOL32 CImGUIManager::D3DCreateDeviceObjects()
    {
        Microsoft::WRL::ComPtr<ID3D11Device> dvc = RDeviceD3D11::GetDeviceSingleton()->GetRenderDevice();
        if (!dvc)
        {
            return FALSE;
        }
        if (m_D3DData.FontSampler)
        {
            CImGUIManager::D3DInvalidateDeviceObjects();
        }

        {
            EString vsName(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
            vsName += IMGUI_VS_FILE_NAME_PATH;
            FILE* file;
            LONG fsize;
            BYTE* buffer;
            {
                fopen_s(&file, *vsName, "rb");
                if (file == nullptr)
                {
                    return FALSE;
                }
                fsize = _filelength(_fileno(file));
                buffer = new BYTE[fsize];
                fread_s(buffer, fsize, fsize, 1u, file);
                fclose(file);
            }

            {
                HRESULT hr = dvc->CreateVertexShader(static_cast<void*>(buffer), fsize, nullptr, &m_D3DData.VertexShader);
                if (FAILED(hr))
                {
                    delete[]buffer;
                    return FALSE;
                }
            }

            {
                D3D11_INPUT_ELEMENT_DESC tempLayout[3] =
                {
                    { "POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT,   0u, (UINT32)offsetof(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0u },
                    { "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT,   0u, (UINT32)offsetof(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0u },
                    { "COLOR",    0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, (UINT32)offsetof(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0u }
                };
                HRESULT hr = dvc->CreateInputLayout(tempLayout, 3u, static_cast<void*>(buffer), fsize, &m_D3DData.InputLayout);
                if (FAILED(hr))
                {
                    delete[]buffer;
                    return FALSE;
                }
            }
            delete[]buffer;
            {
                D3D11_BUFFER_DESC desc;
                ::ZeroMemory(&desc, sizeof(desc));
                desc.ByteWidth = sizeof(D3DConstantBuffer);
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                desc.MiscFlags = 0u;
                HRESULT hr = dvc->CreateBuffer(&desc, nullptr, &m_D3DData.ConstantBuffer);
                if (FAILED(hr))
                {
                    return FALSE;
                }
            }
        }

        {
            EString psName(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
            psName += IMGUI_PS_FILE_NAME_PATH;
            FILE* file;
            LONG fsize;
            BYTE* buffer;
            {
                fopen_s(&file, *psName, "rb");
                if (file == nullptr)
                {
                    return FALSE;
                }
                fsize = _filelength(_fileno(file));
                buffer = new BYTE[fsize];
                fread_s(buffer, fsize, fsize, 1u, file);
                fclose(file);
            }

            {
                HRESULT hr = dvc->CreatePixelShader(static_cast<void*>(buffer), fsize, nullptr, &m_D3DData.PixelShader);
                if (FAILED(hr))
                {
                    delete[]buffer;
                    return FALSE;
                }
            }
            delete[]buffer;
        }

        {
            D3D11_BLEND_DESC desc;
            ::ZeroMemory(&desc, sizeof(desc));
            desc.AlphaToCoverageEnable = FALSE;
            desc.RenderTarget[0].BlendEnable = TRUE;
            desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            dvc->CreateBlendState(&desc, &m_D3DData.BlendState);
        }

        {
            D3D11_RASTERIZER_DESC desc;
            ::ZeroMemory(&desc, sizeof(desc));
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            desc.ScissorEnable = TRUE;
            desc.DepthClipEnable = TRUE;
            dvc->CreateRasterizerState(&desc, &m_D3DData.RasterizerState);
        }

        {
            D3D11_DEPTH_STENCIL_DESC desc;
            ::ZeroMemory(&desc, sizeof(desc));
            desc.DepthEnable = FALSE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
            desc.StencilEnable = FALSE;
            desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            desc.BackFace = desc.FrontFace;
            dvc->CreateDepthStencilState(&desc, &m_D3DData.DepthStencilState);
        }
        CImGUIManager::D3DCreateFontsTexture();
        return TRUE;
    }
    void CImGUIManager::D3DInvalidateDeviceObjects()
    {
        if (m_D3DData.FontSampler) { m_D3DData.FontSampler->Release(); m_D3DData.FontSampler = nullptr; }
        if (m_D3DData.FontTextureView) { m_D3DData.FontTextureView->Release(); m_D3DData.FontTextureView = nullptr; ImGui::GetIO().Fonts->SetTexID(nullptr); }
        if (m_D3DData.IndexBuffer) { m_D3DData.IndexBuffer->Release(); m_D3DData.IndexBuffer = nullptr; }
        if (m_D3DData.VertexBuffer) { m_D3DData.VertexBuffer->Release(); m_D3DData.VertexBuffer = nullptr; }
        if (m_D3DData.BlendState) { m_D3DData.BlendState->Release(); m_D3DData.BlendState = nullptr; }
        if (m_D3DData.DepthStencilState) { m_D3DData.DepthStencilState->Release(); m_D3DData.DepthStencilState = nullptr; }
        if (m_D3DData.RasterizerState) { m_D3DData.RasterizerState->Release(); m_D3DData.RasterizerState = nullptr; }
        if (m_D3DData.PixelShader) { m_D3DData.PixelShader->Release(); m_D3DData.PixelShader = nullptr; }
        if (m_D3DData.ConstantBuffer) { m_D3DData.ConstantBuffer->Release(); m_D3DData.ConstantBuffer = nullptr; }
        if (m_D3DData.InputLayout) { m_D3DData.InputLayout->Release(); m_D3DData.InputLayout = nullptr; }
        if (m_D3DData.VertexShader) { m_D3DData.VertexShader->Release(); m_D3DData.VertexShader = nullptr; }
    }
    void CImGUIManager::InitD3DViewportSupport()
    {
        ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
        PlatformIO.Renderer_CreateWindow = RendererCreateWindow;
        PlatformIO.Renderer_DestroyWindow = RendererDestroyWindow;
        PlatformIO.Renderer_SetWindowSize = RendererSetWindowSize;
        PlatformIO.Renderer_RenderWindow = RendererRenderWindow;
        PlatformIO.Renderer_SwapBuffers = RendererSwapBuffers;
    }
    void CImGUIManager::ShutDownD3DViewportSupport()
    {
        ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
        PlatformIO.Renderer_CreateWindow = nullptr;
        PlatformIO.Renderer_DestroyWindow = nullptr;
        PlatformIO.Renderer_SetWindowSize = nullptr;
        PlatformIO.Renderer_RenderWindow = nullptr;
        PlatformIO.Renderer_SwapBuffers = nullptr;
    }
    void CImGUIManager::CreateViewportRenderTarget(ImGuiViewport* viewport)
    {
        ImGuiRendererViewportData* ViewportData = (ImGuiRendererViewportData*)viewport->RendererUserData;
        if (!ViewportData || !ViewportData->SwapChain)
        {
            return;
        }

        ID3D11Texture2D* BackBuffer = nullptr;
        if (FAILED(ViewportData->SwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), (void**)(&BackBuffer))) || !BackBuffer)
        {
            return;
        }

        Microsoft::WRL::ComPtr<ID3D11Device> Device = RDeviceD3D11::GetDeviceSingleton()->GetRenderDevice();
        Device->CreateRenderTargetView(BackBuffer, nullptr, &ViewportData->RenderTargetView);
        BackBuffer->Release();
    }
    void CImGUIManager::DestroyViewportRenderTarget(ImGuiViewport* viewport)
    {
        ImGuiRendererViewportData* ViewportData = (ImGuiRendererViewportData*)viewport->RendererUserData;
        if (ViewportData && ViewportData->RenderTargetView)
        {
            ViewportData->RenderTargetView->Release();
            ViewportData->RenderTargetView = nullptr;
        }
    }
    void CImGUIManager::RendererCreateWindow(ImGuiViewport* viewport)
    {
        ImGuiRendererViewportData* ViewportData = IM_NEW(ImGuiRendererViewportData)();
        viewport->RendererUserData = ViewportData;

        HWND hWnd = GetViewportWindowHandle(viewport);
        IM_ASSERT(hWnd != nullptr);

        Microsoft::WRL::ComPtr<ID3D11Device> Device = RDeviceD3D11::GetDeviceSingleton()->GetRenderDevice();
        Microsoft::WRL::ComPtr<IDXGIDevice> DxgiDevice;
        Microsoft::WRL::ComPtr<IDXGIAdapter> DxgiAdapter;
        Microsoft::WRL::ComPtr<IDXGIFactory> DxgiFactory;
        if (FAILED(Device->QueryInterface(IID_PPV_ARGS(DxgiDevice.ReleaseAndGetAddressOf()))))
        {
            return;
        }
        if (FAILED(DxgiDevice->GetParent(IID_PPV_ARGS(DxgiAdapter.ReleaseAndGetAddressOf()))))
        {
            return;
        }
        if (FAILED(DxgiAdapter->GetParent(IID_PPV_ARGS(DxgiFactory.ReleaseAndGetAddressOf()))))
        {
            return;
        }

        DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
        SwapChainDesc.BufferDesc.Width = (UINT)viewport->Size.x;
        SwapChainDesc.BufferDesc.Height = (UINT)viewport->Size.y;
        SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
        SwapChainDesc.SampleDesc.Count = 1u;
        SwapChainDesc.SampleDesc.Quality = 0u;
        SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.BufferCount = 1u;
        SwapChainDesc.OutputWindow = hWnd;
        SwapChainDesc.Windowed = TRUE;
        SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        SwapChainDesc.Flags = 0u;

        if (FAILED(DxgiFactory->CreateSwapChain(Device.Get(), &SwapChainDesc, &ViewportData->SwapChain)))
        {
            return;
        }

        DxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
        CreateViewportRenderTarget(viewport);
    }
    void CImGUIManager::RendererDestroyWindow(ImGuiViewport* viewport)
    {
        ImGuiRendererViewportData* ViewportData = (ImGuiRendererViewportData*)viewport->RendererUserData;
        if (ViewportData)
        {
            DestroyViewportRenderTarget(viewport);
            if (ViewportData->SwapChain)
            {
                ViewportData->SwapChain->Release();
                ViewportData->SwapChain = nullptr;
            }
            IM_DELETE(ViewportData);
        }
        viewport->RendererUserData = nullptr;
    }
    void CImGUIManager::RendererSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
    {
        ImGuiRendererViewportData* ViewportData = (ImGuiRendererViewportData*)viewport->RendererUserData;
        if (!ViewportData || !ViewportData->SwapChain)
        {
            return;
        }
        DestroyViewportRenderTarget(viewport);
        ViewportData->SwapChain->ResizeBuffers(0u, (UINT)size.x, (UINT)size.y, DXGI_FORMAT_UNKNOWN, 0u);
        CreateViewportRenderTarget(viewport);
    }
    void CImGUIManager::RendererRenderWindow(ImGuiViewport* viewport, void*)
    {
        ImGuiRendererViewportData* ViewportData = (ImGuiRendererViewportData*)viewport->RendererUserData;
        if (!ViewportData || !ViewportData->RenderTargetView)
        {
            return;
        }

        Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context = RDeviceD3D11::GetDeviceSingleton()->GetRenderDeviceContext();
        ID3D11RenderTargetView* RenderTarget = ViewportData->RenderTargetView;
        Context->OMSetRenderTargets(1u, &RenderTarget, nullptr);
        if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
        {
            const FLOAT ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            Context->ClearRenderTargetView(RenderTarget, ClearColor);
        }
        GetManagerSingleton()->D3DRenderDrawData(viewport->DrawData);
    }
    void CImGUIManager::RendererSwapBuffers(ImGuiViewport* viewport, void*)
    {
        ImGuiRendererViewportData* ViewportData = (ImGuiRendererViewportData*)viewport->RendererUserData;
        if (ViewportData && ViewportData->SwapChain)
        {
            ViewportData->SwapChain->Present(0u, 0u);
        }
    }

    PE_INLINE ImTextureID EngineTextureToImgui(const ETexture2DAsset* InAsset)
    {
        if (!!InAsset)
        {
            if (InAsset->IsRenderResourceValid())
            {
                const RTexture2DResource* RenderResource = InAsset->GetRenderResource();
                return ((ImTextureID)(RenderResource->ShaderResourceView.Get()));
            }
        }
        return ImTextureID_Invalid;
    }

#endif
};