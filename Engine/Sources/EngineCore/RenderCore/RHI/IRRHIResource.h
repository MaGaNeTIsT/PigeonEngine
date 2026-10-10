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
		RHI_RESOURCE_TYPE_BUFFER				= 0,
		RHI_RESOURCE_TYPE_TEXTURE				= 1,
		RHI_RESOURCE_TYPE_SHADER				= 2,
		RHI_RESOURCE_TYPE_PIPELINE_STATE		= 3,
		RHI_RESOURCE_TYPE_SAMPLER				= 4,
		RHI_RESOURCE_TYPE_COUNT
	};

	class IRRHIResource
	{
	public:
		IRRHIResource() noexcept = default;
		virtual ~IRRHIResource() noexcept = default;

		virtual ERHIResourceTypeTag GetResourceType() const noexcept = 0;

		// Track resource state for D3D12 barrier insertion. D3D11 ignores this.
		ERHIResourceState GetCurrentState() const noexcept
		{
			return CurrentState;
		}

		void SetCurrentState(ERHIResourceState InState) noexcept
		{
			CurrentState = InState;
		}

#if _EDITOR_ONLY
		const EString& GetDebugName() const noexcept
		{
			return DebugName;
		}

		virtual void SetDebugName(const EString& InDebugName) noexcept
		{
			DebugName = InDebugName;
		}
#endif	// _EDITOR_ONLY

	public:
		IRRHIResource(const IRRHIResource&) = delete;
		IRRHIResource(IRRHIResource&&) = delete;
		IRRHIResource& operator=(const IRRHIResource&) = delete;
		IRRHIResource& operator=(IRRHIResource&&) = delete;

	protected:
		ERHIResourceState		CurrentState		= ERHIResourceState::RHI_RESOURCE_STATE_COMMON;
#if _EDITOR_ONLY
		EString					DebugName;
#endif	// _EDITOR_ONLY
	};

	/*
	* Linear GPU buffer (vertex, index, constant, structured, indirect-args).
	* The bind-flag mix is recorded at create time via RBufferDesc.
	*/
	class IRRHIBuffer : public IRRHIResource
	{
	public:
		virtual ERHIResourceTypeTag GetResourceType() const noexcept override
		{
			return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_BUFFER;
		}

		virtual UINT64		GetByteSize() const noexcept				= 0;
		virtual UINT32		GetStructureByteStride() const noexcept		= 0;
		virtual UINT8		GetBindFlags() const noexcept				= 0;

		// Returns RHI_INVALID_BINDLESS_INDEX if not registered as bindless.
		virtual UINT32		GetBindlessSRVIndex() const noexcept		= 0;
		virtual UINT32		GetBindlessUAVIndex() const noexcept		= 0;
	};

	class IRRHITexture : public IRRHIResource
	{
	public:
		virtual ERHIResourceTypeTag GetResourceType() const noexcept override
		{
			return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE;
		}

		virtual UINT32			GetWidth() const noexcept					= 0;
		virtual UINT32			GetHeight() const noexcept					= 0;
		virtual UINT32			GetDepth() const noexcept					= 0;
		virtual UINT32			GetMipLevels() const noexcept				= 0;
		virtual UINT32			GetArraySize() const noexcept				= 0;
		// The resource's content format: what the data is, which is the format the desc
		// asked for - an sRGB content format stays sRGB here even though the resource is
		// created with the typeless member of that family. Where the storage format itself
		// matters, ask GetViewFormat.
		virtual RFormatType		GetFormat() const noexcept					= 0;
		virtual UINT8			GetBindFlags() const noexcept				= 0;

		virtual UINT32			GetBindlessSRVIndex() const noexcept		= 0;
		virtual UINT32			GetBindlessUAVIndex() const noexcept		= 0;

		// The format one of this resource's views was created with, or FORMAT_UNKNOWN when
		// the resource has no such view. D3D12 requires a bound render target's format to
		// equal the pipeline state's declared one, so the render layer compares these
		// against the state it is about to draw with; a back-end that does not track its
		// view formats answers FORMAT_UNKNOWN and the comparison is skipped rather than
		// guessed at.
		virtual RFormatType		GetViewFormat(ERHIViewFormatType InView) const noexcept
		{
			(void)(InView);
			return RFormatType::FORMAT_UNKNOWN;
		}
	};

	class IRRHIShader : public IRRHIResource
	{
	public:
		virtual ERHIResourceTypeTag GetResourceType() const noexcept override
		{
			return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_SHADER;
		}

		// The stage this object was created for. A back-end object always knows its
		// own stage, so the engine side never has to carry a parallel copy of it.
		virtual ERHIShaderStage GetShaderStage() const noexcept = 0;
	};

	class IRRHIPipelineState : public IRRHIResource
	{
	public:
		virtual ERHIResourceTypeTag GetResourceType() const noexcept override
		{
			return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_PIPELINE_STATE;
		}

		virtual BOOL8 IsCompute() const noexcept		= 0;

		// Root signature this PSO was built against and the binding layout that came
		// with it. Both are always present on a successfully created PSO: when the desc
		// carries no signature the device substitutes its default one, so the command list
		// never has to fall back to hard coded root indices.
		// "Always present" is about identity, not about content: a back-end without root
		// signatures (D3D11) returns its no-root-signature layout, whose zero parameter
		// count and invalid table roots are the documented meaning of "bind straight to
		// register slots" (see RRHIPipelineLayoutInfo in RHITypes.h).
		virtual IRRHIRootSignature*				GetRootSignature() const noexcept	= 0;
		virtual const RRHIPipelineLayoutInfo&	GetLayoutInfo() const noexcept		= 0;

		// The render target formats the state was created with, and the depth format it
		// writes. D3D12 refuses a draw whose bound target differs from either, so the
		// render layer compares them against the targets it bound (IRRHITexture::
		// GetViewFormat) and names the mismatch where a device-removed error would
		// otherwise be the first sign of it.
		//
		// A state that declares nothing (a compute state, or a back-end that does not track
		// them) answers zero formats, and the comparison is skipped rather than guessed at.
		virtual UINT32							GetRenderTargetFormatNum() const noexcept { return 0u; }
		virtual RFormatType						GetRenderTargetFormat(UINT32 InIndex) const noexcept { (void)(InIndex); return RFormatType::FORMAT_UNKNOWN; }
		virtual RFormatType						GetDepthStencilFormat() const noexcept { return RFormatType::FORMAT_UNKNOWN; }
	};

	class IRRHISampler : public IRRHIResource
	{
	public:
		virtual ERHIResourceTypeTag GetResourceType() const noexcept override
		{
			return ERHIResourceTypeTag::RHI_RESOURCE_TYPE_SAMPLER;
		}
	};

};  // PigeonEngine
