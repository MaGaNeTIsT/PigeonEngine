#pragma once

#include "../../Entry/MyMain.h"

class CimGUIManager
{
public:
    struct WndData
    {
        WndData() { ZeroMemory(this, sizeof(*this)); }
        HWND                        hWnd;
        HWND                        MouseHwnd;
        BOOL                        MouseTracked;
        INT                         MouseButtonsDown;
        ImGuiMouseCursor            LastMouseCursor;
        BOOL                        HasGamepad;
        BOOL                        WantUpdateHasGamepad;
    };
    struct D3DData
    {
        D3DData() { ZeroMemory(this, sizeof(*this)); this->VertexBufferSize = 5000; this->IndexBufferSize = 10000; }
        ID3D11VertexShader*         VertexShader;
        ID3D11InputLayout*          InputLayout;
        ID3D11Buffer*               ConstantBuffer;
        ID3D11PixelShader*          PixelShader;
        ID3D11SamplerState*         FontSampler;
        ID3D11ShaderResourceView*   FontTextureView;
        ID3D11RasterizerState*      RasterizerState;
        ID3D11BlendState*           BlendState;
        ID3D11DepthStencilState*    DepthStencilState;
        ID3D11Buffer*               VertexBuffer;
        ID3D11Buffer*               IndexBuffer;
        INT                         VertexBufferSize;
        INT                         IndexBufferSize;
    };
    struct D3DConstantBuffer
    {
        FLOAT MVP[4][4];
    };
public:
    static const CimGUIManager*     GetGUIManager() { return m_imGUIManager; }
	static void                     Initialize();
	static void                     ShutDown();
public:
	static void                     Update();
	static void                     Draw();
    static IMGUI_IMPL_API LRESULT   WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
	static void                     InitWnd();
    static BOOL                     WndUpdateMouseCursor();
    static BOOL                     WndIsVkDown(INT vk);
    static void                     WndAddKeyEvent(ImGuiKey key, BOOL down, INT native_keycode, INT native_scancode = -1);
    static void                     WndProcessKeyEventsWorkarounds();
    static void                     WndUpdateKeyModifiers();
    static ImGuiKey                 WndVirtualKeyToImGuiKey(WPARAM wParam);
private:
    static void                     InitD3D();
    static void                     D3DSetupRenderState(ImDrawData* drawData);
    static void                     D3DRenderDrawData(ImDrawData* drawData);
    static void                     D3DCreateFontsTexture();
    static BOOL                     D3DCreateDeviceObjects();
    static void                     D3DInvalidateDeviceObjects();
private:
    WndData                 m_WndData;
    D3DData                 m_D3DData;
private:
	static CimGUIManager*   m_imGUIManager;
private:
	CimGUIManager() {}
	CimGUIManager(const CimGUIManager&) {}
	~CimGUIManager() {}
};