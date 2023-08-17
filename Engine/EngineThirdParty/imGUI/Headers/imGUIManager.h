#pragma once

#include <CoreMinimal.h>
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{
#ifdef _EDITOR_ONLY

    class CImGUIManager : public EManagerBase
    {
    public:
        struct WndData
        {
            WndData() { ::ZeroMemory(this, sizeof(*this)); }
            HWND                    hWnd;
            HWND                    MouseHwnd;
            BOOL                    MouseTracked;
            INT                     MouseButtonsDown;
            ImGuiMouseCursor        LastMouseCursor;
            BOOL                    HasGamepad;
            BOOL                    WantUpdateHasGamepad;
        };
        struct D3DData
        {
            D3DData() { ::ZeroMemory(this, sizeof(*this)); this->VertexBufferSize = 5000; this->IndexBufferSize = 10000; }
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
        virtual void            Initialize()override;
        virtual void            ShutDown()override;
        void                    Update();
        void                    Draw();
        IMGUI_IMPL_API LRESULT  WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    private:
        void                    InitWnd();
        BOOL                    WndUpdateMouseCursor();
        BOOL                    WndIsVkDown(INT vk);
        void                    WndAddKeyEvent(ImGuiKey key, BOOL down, INT native_keycode, INT native_scancode = -1);
        void                    WndProcessKeyEventsWorkarounds();
        void                    WndUpdateKeyModifiers();
        ImGuiKey                WndVirtualKeyToImGuiKey(WPARAM wParam);
    private:
        void                    InitD3D();
        void                    D3DSetupRenderState(ImDrawData* drawData);
        void                    D3DRenderDrawData(ImDrawData* drawData);
        void                    D3DCreateFontsTexture();
        BOOL                    D3DCreateDeviceObjects();
        void                    D3DInvalidateDeviceObjects();
    private:
        WndData                 m_WndData;
        D3DData                 m_D3DData;

        CLASS_MANAGER_SINGLETON_BODY(CImGUIManager)

    };

#endif
};