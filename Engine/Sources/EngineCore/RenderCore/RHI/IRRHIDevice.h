#pragma once

#include <CoreMinimal.h>
#include <RHI/RHITypes.h>
#include <RHI/IRRHIResource.h>
#include <RHI/IRCommandList.h>

namespace PigeonEngine
{

    /*
    * Pure-virtual rendering device. Implemented by RDeviceD3D11 and
    * RDeviceD3D12; selected at startup via EEngineSettings::GraphicsAPI.
    *
    * Threading model (see plan §3):
    *   - Initialize / Shutdown / ResizeSwapChain / Present run on the
    *     RenderThread (main thread for D3D11 backend during transition).
    *   - BeginFrame / EndFrame frame the render-side of the lockstep
    *     handshake; they are called once per frame from the RenderThread.
    *   - AcquireCommandList / SubmitCommandList are thread-safe and may
    *     be called from worker recording threads.
    *   - Resource creation (CreateBuffer / CreateTexture / ...) is
    *     thread-safe; back-ends serialize internally if needed.
    *
    * Bindless model:
    *   - SRV / UAV access is via UINT32 indices into a global descriptor
    *     heap. RegisterBindless* returns an index; the index lives until
    *     ReleaseBindlessIndex is called.
    *   - For the D3D11 back-end the index is a virtual handle the
    *     command list resolves back to ID3D11ShaderResourceView* before
    *     Draw, so the same shader source path can target both back-ends
    *     once each is rebuilt for its API.
    */
    class IRRHIDevice
    {
    public:
        IRRHIDevice() = default;
        virtual ~IRRHIDevice() = default;

        // ---------- Lifecycle ----------
        virtual BOOL8   Initialize(const RRHIDeviceInitDesc& InDesc) = 0;
        virtual void    Shutdown() = 0;

        virtual ERHIBackendType GetBackendType() const = 0;

        // ---------- Frame fencing ----------
        // Wait for the GPU to be done with frame (Current - FRAME_IN_FLIGHT)
        // before recording new commands; reset per-frame command allocators.
        virtual void BeginFrame() = 0;

        // Submit any pending work, signal the frame fence, advance the
        // current frame index. Present is called separately so the caller
        // can choose VSync interval.
        virtual void EndFrame() = 0;

        virtual void    Present(UINT32 InSyncInterval) = 0;
        virtual BOOL8   ResizeSwapChain(UINT32 InWidth, UINT32 InHeight) = 0;

        // Block on GPU until everything submitted so far has retired.
        // Use sparingly (shutdown, resize, hot-reload).
        virtual void WaitForGPUIdle() = 0;

        // ---------- Resource creation ----------
        virtual BOOL8 CreateBuffer(const RBufferDesc& InDesc, const RRHISubresourceData* InInitialData, IRRHIBuffer** OutBuffer) = 0;
        virtual BOOL8 CreateTexture(const RTextureDesc& InDesc, const RRHISubresourceData* InInitialSubresources, UINT32 InSubresourceCount, IRRHITexture** OutTexture) = 0;
        virtual BOOL8 CreateShader(const RRHIShaderDesc& InDesc, IRRHIShader** OutShader) = 0;
        virtual BOOL8 CreateGraphicsPipelineState(const RRHIGraphicsPipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) = 0;
        virtual BOOL8 CreateComputePipelineState(const RRHIComputePipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) = 0;
        virtual BOOL8 CreateSampler(const RSamplerState& InDesc, IRRHISampler** OutSampler) = 0;

        // Destruction is queued through the device so back-ends can defer
        // release until the GPU is no longer using the resource.
        virtual void DestroyResource(IRRHIResource* InResource) = 0;

        // ---------- Bindless descriptor management ----------
        virtual UINT32 RegisterBindlessSRV(IRRHITexture* InTexture) = 0;
        virtual UINT32 RegisterBindlessSRV(IRRHIBuffer* InBuffer) = 0;
        virtual UINT32 RegisterBindlessUAV(IRRHITexture* InTexture) = 0;
        virtual UINT32 RegisterBindlessUAV(IRRHIBuffer* InBuffer) = 0;
        virtual void   ReleaseBindlessIndex(UINT32 InIndex) = 0;

        // ---------- Data upload (CPU -> GPU) ----------
        // Routes through a back-end-managed upload heap / staging buffer.
        virtual void UploadBuffer(IRRHIBuffer* InDest, const void* InData, SIZE_T InSize, SIZE_T InDestOffset = 0) = 0;
        virtual void UploadTexture(IRRHITexture* InDest, const RRHISubresourceData* InSubresources, UINT32 InSubresourceCount, UINT32 InFirstSubresource = 0u) = 0;

        // ---------- Command lists ----------
        // Acquired lists are returned to the device's pool when submitted.
        virtual IRCommandList*  AcquireCommandList() = 0;
        virtual void            SubmitCommandList(IRCommandList* InCommandList) = 0;
        virtual void            SubmitCommandLists(IRCommandList* const* InCommandLists, UINT32 InCount) = 0;

        // ---------- Backbuffer access ----------
        virtual IRRHITexture*   GetCurrentBackBuffer() const = 0;
        virtual UINT32          GetCurrentFrameIndex() const = 0;
        virtual UINT32          GetFrameInFlightCount() const = 0;

    public:
        IRRHIDevice(const IRRHIDevice&) = delete;
        IRRHIDevice(IRRHIDevice&&) = delete;
        IRRHIDevice& operator=(const IRRHIDevice&) = delete;
        IRRHIDevice& operator=(IRRHIDevice&&) = delete;
    };

};
