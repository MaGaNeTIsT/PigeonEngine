#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

    /*
    * Shared RHI-layer descriptors and enums. The pure-virtual IRRHIDevice
    * and IRCommandList speak in these types; concrete D3D11 / D3D12
    * back-ends translate them at the boundary.
    *
    * The RHI layer reuses existing engine descriptors from RenderCommon.h
    * (RBufferDesc, RTextureDesc, RSamplerState, RRasterizerState,
    *  RBlendState, RDepthState, RStencilState, RInputLayoutDesc,
    *  RFormatType, RPrimitiveTopologyType) and only introduces new types
    * for concepts that did not exist in the DX11-only abstraction:
    * resource states, command-list-level render passes, monolithic
    * pipeline descriptors, and bindless indices.
    */

    // Forward declarations (defined under RHI/IRRHIResource.h)
    class IRRHIResource;
    class IRRHIBuffer;
    class IRRHITexture;
    class IRRHIShader;
    class IRRHIPipelineState;
    class IRRHISampler;

    enum ERHIBackendType : UINT8
    {
        RHI_BACKEND_NONE    = 0,
        RHI_BACKEND_D3D11   = 1,
        RHI_BACKEND_D3D12   = 2,
        RHI_BACKEND_COUNT
    };

    enum ERHIShaderStage : UINT8
    {
        RHI_SHADER_STAGE_VERTEX     = 0,
        RHI_SHADER_STAGE_PIXEL      = 1,
        RHI_SHADER_STAGE_COMPUTE    = 2,
        RHI_SHADER_STAGE_COUNT
    };

    enum ERHIIndexFormat : UINT8
    {
        RHI_INDEX_FORMAT_UINT16 = 0,
        RHI_INDEX_FORMAT_UINT32 = 1,
        RHI_INDEX_FORMAT_COUNT
    };

    /*
    * Logical resource state used by ResourceBarrier on D3D12. D3D11
    * back-end ignores these (state tracking is implicit on D3D11).
    * The set covers the states we actually transition between in the
    * existing forward + deferred passes.
    */
    enum ERHIResourceState : UINT16
    {
        RHI_RESOURCE_STATE_COMMON               = 0,
        RHI_RESOURCE_STATE_VERTEX_BUFFER        = (1u << 0),
        RHI_RESOURCE_STATE_INDEX_BUFFER         = (1u << 1),
        RHI_RESOURCE_STATE_CONSTANT_BUFFER      = (1u << 2),
        RHI_RESOURCE_STATE_RENDER_TARGET        = (1u << 3),
        RHI_RESOURCE_STATE_DEPTH_WRITE          = (1u << 4),
        RHI_RESOURCE_STATE_DEPTH_READ           = (1u << 5),
        RHI_RESOURCE_STATE_SHADER_RESOURCE      = (1u << 6),
        RHI_RESOURCE_STATE_UNORDERED_ACCESS     = (1u << 7),
        RHI_RESOURCE_STATE_COPY_SOURCE          = (1u << 8),
        RHI_RESOURCE_STATE_COPY_DEST            = (1u << 9),
        RHI_RESOURCE_STATE_PRESENT              = (1u << 10),
        RHI_RESOURCE_STATE_INDIRECT_ARGUMENT    = (1u << 11)
    };

    enum ERHILoadOp : UINT8
    {
        RHI_LOAD_OP_LOAD        = 0,
        RHI_LOAD_OP_CLEAR       = 1,
        RHI_LOAD_OP_DONT_CARE   = 2
    };

    enum ERHIStoreOp : UINT8
    {
        RHI_STORE_OP_STORE      = 0,
        RHI_STORE_OP_DONT_CARE  = 1
    };

    PE_CONSTEXPR_STATIC UINT32 RHI_MAX_RENDER_TARGETS       = 8u;
    PE_CONSTEXPR_STATIC UINT32 RHI_MAX_VERTEX_BUFFERS       = 8u;
    PE_CONSTEXPR_STATIC UINT32 RHI_INVALID_BINDLESS_INDEX   = 0xffffffffu;

    struct RRHIDeviceInitDesc
    {
        RRHIDeviceInitDesc() noexcept
            : WindowHandle(nullptr)
            , BackBufferWidth(0u)
            , BackBufferHeight(0u)
            , BackBufferCount(2u)
            , RefreshRateHz(60u)
            , BackBufferFormat(RFormatType::FORMAT_R8G8B8A8_UNORM)
            , DepthStencilFormat(RFormatType::FORMAT_R24G8_TYPELESS)
            , Backend(ERHIBackendType::RHI_BACKEND_D3D12)
            , bIsWindowed(TRUE)
            , bEnableDebugLayer(FALSE)
            , bEnableGPUValidation(FALSE)
        {
        }

        HWND                WindowHandle;
        UINT32              BackBufferWidth;
        UINT32              BackBufferHeight;
        UINT32              BackBufferCount;        // Swapchain back-buffer count (2 = double, 3 = triple)
        UINT32              RefreshRateHz;          // Swapchain refresh rate hint
        RFormatType         BackBufferFormat;
        RFormatType         DepthStencilFormat;
        ERHIBackendType     Backend;
        BOOL8               bIsWindowed;
        BOOL8               bEnableDebugLayer;
        BOOL8               bEnableGPUValidation;
    };

    struct RRHIViewport
    {
        RRHIViewport() noexcept
            : TopLeftX(0.f), TopLeftY(0.f), Width(0.f), Height(0.f)
            , MinDepth(RCommonSettings::RENDER_DEPTH_MIN)
            , MaxDepth(RCommonSettings::RENDER_DEPTH_MAX)
        {
        }

        constexpr RRHIViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight,
            FLOAT InMinDepth = RCommonSettings::RENDER_DEPTH_MIN,
            FLOAT InMaxDepth = RCommonSettings::RENDER_DEPTH_MAX) noexcept
            : TopLeftX(InTopLeftX), TopLeftY(InTopLeftY), Width(InWidth), Height(InHeight)
            , MinDepth(InMinDepth), MaxDepth(InMaxDepth)
        {
        }

        FLOAT TopLeftX;
        FLOAT TopLeftY;
        FLOAT Width;
        FLOAT Height;
        FLOAT MinDepth;
        FLOAT MaxDepth;
    };

    struct RRHIRect
    {
        RRHIRect() noexcept
            : Left(0), Top(0), Right(0), Bottom(0)
        {
        }

        constexpr RRHIRect(INT32 InLeft, INT32 InTop, INT32 InRight, INT32 InBottom) noexcept
            : Left(InLeft), Top(InTop), Right(InRight), Bottom(InBottom)
        {
        }

        INT32 Left;
        INT32 Top;
        INT32 Right;
        INT32 Bottom;
    };

    struct RRHIRenderTargetBinding
    {
        RRHIRenderTargetBinding() noexcept
            : Target(nullptr)
            , LoadOp(ERHILoadOp::RHI_LOAD_OP_LOAD)
            , StoreOp(ERHIStoreOp::RHI_STORE_OP_STORE)
            , ClearColor(Color4(0.f, 0.f, 0.f, 1.f))
        {
        }

        IRRHITexture*   Target;
        ERHILoadOp      LoadOp;
        ERHIStoreOp     StoreOp;
        Color4          ClearColor;
    };

    struct RRHIDepthStencilBinding
    {
        RRHIDepthStencilBinding() noexcept
            : Target(nullptr)
            , DepthLoadOp(ERHILoadOp::RHI_LOAD_OP_LOAD)
            , DepthStoreOp(ERHIStoreOp::RHI_STORE_OP_STORE)
            , StencilLoadOp(ERHILoadOp::RHI_LOAD_OP_LOAD)
            , StencilStoreOp(ERHIStoreOp::RHI_STORE_OP_STORE)
            , ClearDepth(RCommonSettings::RENDER_DEPTH_MAX)
            , ClearStencil(0u)
            , bIsReadOnly(FALSE)
        {
        }

        IRRHITexture*   Target;
        ERHILoadOp      DepthLoadOp;
        ERHIStoreOp     DepthStoreOp;
        ERHILoadOp      StencilLoadOp;
        ERHIStoreOp     StencilStoreOp;
        FLOAT           ClearDepth;
        UINT8           ClearStencil;
        BOOL8           bIsReadOnly;
    };

    struct RRHIRenderPassDesc
    {
        RRHIRenderPassDesc() noexcept
            : RenderTargetCount(0u)
        {
        }

        RRHIRenderTargetBinding RenderTargets[RHI_MAX_RENDER_TARGETS];
        UINT32                  RenderTargetCount;
        RRHIDepthStencilBinding DepthStencil;
    };

    struct RRHIShaderDesc
    {
        RRHIShaderDesc() noexcept
            : ByteCode(nullptr), ByteCodeSize(0u), Stage(ERHIShaderStage::RHI_SHADER_STAGE_COUNT)
        {
        }

        constexpr RRHIShaderDesc(const void* InByteCode, SIZE_T InByteCodeSize, ERHIShaderStage InStage) noexcept
            : ByteCode(InByteCode), ByteCodeSize(InByteCodeSize), Stage(InStage)
        {
        }

        const void*     ByteCode;
        SIZE_T          ByteCodeSize;
        ERHIShaderStage Stage;
    };

    struct RRHIGraphicsPipelineDesc
    {
        RRHIGraphicsPipelineDesc() noexcept
            : VertexShader(nullptr), PixelShader(nullptr)
            , InputLayouts(nullptr), InputLayoutCount(0u)
            , Rasterizer(), Blend(), Depth(), Stencil()
            , PrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST)
            , RenderTargetCount(0u)
            , DepthStencilFormat(RFormatType::FORMAT_UNKNOWN)
            , SampleCount(1u), SampleQuality(0u)
        {
            for (UINT32 i = 0u; i < RHI_MAX_RENDER_TARGETS; i++)
            {
                RenderTargetFormats[i] = RFormatType::FORMAT_UNKNOWN;
            }
        }

        IRRHIShader*                VertexShader;
        IRRHIShader*                PixelShader;
        const RInputLayoutDesc*     InputLayouts;
        UINT32                      InputLayoutCount;
        RRasterizerState            Rasterizer;
        RBlendState                 Blend;
        RDepthState                 Depth;
        RStencilState               Stencil;
        RPrimitiveTopologyType      PrimitiveTopology;
        UINT32                      RenderTargetCount;
        RFormatType                 RenderTargetFormats[RHI_MAX_RENDER_TARGETS];
        RFormatType                 DepthStencilFormat;
        UINT32                      SampleCount;
        UINT32                      SampleQuality;
    };

    struct RRHIComputePipelineDesc
    {
        RRHIComputePipelineDesc() noexcept
            : ComputeShader(nullptr)
        {
        }

        IRRHIShader* ComputeShader;
    };

    /*
    * CPU-side description of a subresource payload. Mirrors the legacy
    * RSubresourceDataDesc but lives here so RHI users do not need to drag
    * in RenderCommon directly when they only deal with uploads.
    */
    struct RRHISubresourceData
    {
        RRHISubresourceData() noexcept
            : Data(nullptr), RowPitch(0u), SlicePitch(0u)
        {
        }

        constexpr RRHISubresourceData(const void* InData, UINT32 InRowPitch, UINT32 InSlicePitch) noexcept
            : Data(InData), RowPitch(InRowPitch), SlicePitch(InSlicePitch)
        {
        }

        const void* Data;
        UINT32      RowPitch;
        UINT32      SlicePitch;
    };

};
