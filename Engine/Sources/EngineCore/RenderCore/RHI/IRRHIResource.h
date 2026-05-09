#pragma once

#include <CoreMinimal.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

    /*
    * RHI resource handle hierarchy.
    *
    * IRRHIResource is the polymorphic base every GPU object inherits from.
    * Concrete back-ends (RDeviceD3D11 / RDeviceD3D12) sub-class the leaf
    * interfaces (IRRHIBuffer, IRRHITexture, ...) and stash their native
    * COM objects there. The engine code only ever holds IRRHIBuffer*
    * etc.; ownership is transferred via TUniquePtr or release explicitly.
    *
    * The base class is intentionally narrow: queries needed by the
    * renderer-facing layer (debug name, current resource state for the
    * D3D12 barrier tracker, the underlying type tag). Anything D3D-
    * specific lives behind back-end private state.
    */

    enum ERHIResourceTypeTag : UINT8
    {
        RHI_RESOURCE_TYPE_BUFFER            = 0,
        RHI_RESOURCE_TYPE_TEXTURE           = 1,
        RHI_RESOURCE_TYPE_SHADER            = 2,
        RHI_RESOURCE_TYPE_PIPELINE_STATE    = 3,
        RHI_RESOURCE_TYPE_SAMPLER           = 4,
        RHI_RESOURCE_TYPE_COUNT
    };

    class IRRHIResource
    {
    public:
        IRRHIResource() = default;
        virtual ~IRRHIResource() = default;

        virtual ERHIResourceTypeTag GetResourceType() const = 0;

        // Track resource state for D3D12 barrier insertion. D3D11 ignores this.
        ERHIResourceState GetCurrentState() const
        {
            return CurrentState;
        }

        void SetCurrentState(ERHIResourceState InState)
        {
            CurrentState = InState;
        }

#if _EDITOR_ONLY
        const EString& GetDebugName() const
        {
            return DebugName;
        }

        virtual void SetDebugName(const EString& InDebugName)
        {
            DebugName = InDebugName;
        }
#else
        virtual void SetDebugName(const EString& /*InDebugName*/) {}
#endif

    public:
        IRRHIResource(const IRRHIResource&) = delete;
        IRRHIResource(IRRHIResource&&) = delete;
        IRRHIResource& operator=(const IRRHIResource&) = delete;
        IRRHIResource& operator=(IRRHIResource&&) = delete;

    protected:
        ERHIResourceState   CurrentState = ERHIResourceState::RHI_RESOURCE_STATE_COMMON;
#if _EDITOR_ONLY
        EString             DebugName;
#endif
    };

    /*
    * Linear GPU buffer (vertex, index, constant, structured, indirect-args).
    * The bind-flag mix is recorded at create time via RBufferDesc.
    */
    class IRRHIBuffer : public IRRHIResource
    {
    public:
        virtual ERHIResourceTypeTag GetResourceType() const override
        {
            return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_BUFFER;
        }

        virtual UINT32  GetByteSize() const = 0;
        virtual UINT32  GetStructureByteStride() const = 0;
        virtual UINT8   GetBindFlags() const = 0;

        // Returns RHI_INVALID_BINDLESS_INDEX if not registered as bindless.
        virtual UINT32  GetBindlessSRVIndex() const = 0;
        virtual UINT32  GetBindlessUAVIndex() const = 0;
    };

    class IRRHITexture : public IRRHIResource
    {
    public:
        virtual ERHIResourceTypeTag GetResourceType() const override
        {
            return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE;
        }

        virtual UINT32      GetWidth() const = 0;
        virtual UINT32      GetHeight() const = 0;
        virtual UINT32      GetDepth() const = 0;
        virtual UINT32      GetMipLevels() const = 0;
        virtual UINT32      GetArraySize() const = 0;
        virtual RFormatType GetFormat() const = 0;
        virtual UINT8       GetBindFlags() const = 0;

        virtual UINT32      GetBindlessSRVIndex() const = 0;
        virtual UINT32      GetBindlessUAVIndex() const = 0;
    };

    class IRRHIShader : public IRRHIResource
    {
    public:
        virtual ERHIResourceTypeTag GetResourceType() const override
        {
            return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_SHADER;
        }

        virtual ERHIShaderStage GetShaderStage() const = 0;
    };

    class IRRHIPipelineState : public IRRHIResource
    {
    public:
        virtual ERHIResourceTypeTag GetResourceType() const override
        {
            return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_PIPELINE_STATE;
        }

        virtual BOOL8 IsCompute() const = 0;
    };

    class IRRHISampler : public IRRHIResource
    {
    public:
        virtual ERHIResourceTypeTag GetResourceType() const override
        {
            return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_SAMPLER;
        }
    };

};
