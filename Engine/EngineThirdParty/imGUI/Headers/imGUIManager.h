#pragma once

#include <CoreMinimal.h>
#include <RenderDevice/DeviceD3D11.h>
#include <imgui.h>

namespace PigeonEngine
{
#if _EDITOR_ONLY

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
            INT32                       VertexBufferSize;
            INT32                       IndexBufferSize;
        };
        struct D3DConstantBuffer
        {
            FLOAT MVP[4][4];
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
        // Render-thread step: consume the ImDrawData produced above. Touches
        // the D3D11 immediate context, so the caller must own it (i.e. call
        // from the RenderThread between SceneRenderer.Render and Present).
        void                    RenderDrawData();
        IMGUI_IMPL_API LRESULT  WndProcHandler(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
    private:
        void                    InitWnd();
        BOOL32                  WndUpdateMouseCursor();
        BOOL32                  WndIsVkDown(INT32 vk);
        void                    WndAddKeyEvent(ImGuiKey key, BOOL32 down, INT32 native_keycode, INT32 native_scancode = -1);
        void                    WndProcessKeyEventsWorkarounds();
        void                    WndUpdateKeyModifiers();
        ImGuiKey                WndVirtualKeyToImGuiKey(WPARAM wParam);
    private:
        void                    InitD3D();
        void                    D3DSetupRenderState(ImDrawData* drawData);
        void                    D3DRenderDrawData(ImDrawData* drawData);
        void                    D3DCreateFontsTexture();
        BOOL32                  D3DCreateDeviceObjects();
        void                    D3DInvalidateDeviceObjects();
    private:
        WndData                 m_WndData;
        D3DData                 m_D3DData;

        CLASS_MANAGER_SINGLETON_BODY(CImGUIManager)

    };

    extern PE_INLINE ImTextureID EngineTextureToImgui(const class ETexture2DAsset* InAsset);

#endif
};