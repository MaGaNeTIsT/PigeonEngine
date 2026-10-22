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
	*
	* The layer depends on CoreMinimal and RenderCommon only. Anything that
	* lives above the RHI (assets, renderer, proxies) converts in, never the
	* other way around.
	*/

	// Forward declarations (defined under RHI/IRRHIResource.h)
	class IRRHIResource;
	class IRRHIBuffer;
	class IRRHITexture;
	class IRRHIShader;
	class IRRHIPipelineState;
	class IRRHISampler;
	// Defined under RHI/IRRHIRootSignature.h.
	class IRRHIRootSignature;

	/*
	* Which back-end the active device is.
	*
	* A few engine paths are still back-end specific (the D3D11 direct device calls, the
	* bindless virtual handle tables). They gate on this instead of on which concrete device
	* class happened to be constructed, so the gate keeps working when a third back-end
	* appears. The numeric values are the ones EEngineSettings::ENGINE_GRAPHICS_API stores.
	*/
	enum ERHIBackendType : UINT8
	{
		RHI_BACKEND_NONE	= 0u,
		RHI_BACKEND_D3D11	= 1u,
		RHI_BACKEND_D3D12	= 2u,
		RHI_BACKEND_COUNT
	};

	/*
	* Shader stage as the RHI sees it. The asset layer has its own stage enum
	* (EShaderStageType) which carries more values than a pipeline can consume;
	* the render layer translates between the two at the boundary, so the RHI
	* never has to know about compile side stages.
	*/
	enum ERHIShaderStage : UINT8
	{
		RHI_SHADER_STAGE_VERTEX		= 0u,
		RHI_SHADER_STAGE_PIXEL		= 1u,
		RHI_SHADER_STAGE_COMPUTE	= 2u,
		RHI_SHADER_STAGE_COUNT
	};

	enum ERHIIndexFormat : UINT8
	{
		RHI_INDEX_FORMAT_UINT16		= 0u,
		RHI_INDEX_FORMAT_UINT32		= 1u,
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
		RHI_RESOURCE_STATE_COMMON					= 0u,
		RHI_RESOURCE_STATE_VERTEX_BUFFER			= (1u << 0u),
		RHI_RESOURCE_STATE_INDEX_BUFFER				= (1u << 1u),
		RHI_RESOURCE_STATE_CONSTANT_BUFFER			= (1u << 2u),
		RHI_RESOURCE_STATE_RENDER_TARGET			= (1u << 3u),
		RHI_RESOURCE_STATE_DEPTH_WRITE				= (1u << 4u),
		RHI_RESOURCE_STATE_DEPTH_READ				= (1u << 5u),
		RHI_RESOURCE_STATE_SHADER_RESOURCE			= (1u << 6u),
		RHI_RESOURCE_STATE_UNORDERED_ACCESS			= (1u << 7u),
		RHI_RESOURCE_STATE_COPY_SOURCE				= (1u << 8u),
		RHI_RESOURCE_STATE_COPY_DEST				= (1u << 9u),
		RHI_RESOURCE_STATE_PRESENT					= (1u << 10u),
		RHI_RESOURCE_STATE_INDIRECT_ARGUMENT		= (1u << 11u)
	};

	enum ERHILoadOp : UINT8
	{
		RHI_LOAD_OP_LOAD			= 0,
		RHI_LOAD_OP_CLEAR			= 1,
		RHI_LOAD_OP_DONT_CARE		= 2
	};

	enum ERHIStoreOp : UINT8
	{
		RHI_STORE_OP_STORE			= 0,
		RHI_STORE_OP_DONT_CARE		= 1
	};

	/*
	* Which view of a texture is being asked about. A resource can carry all four at
	* once, and a view is where a format carries meaning: the same bytes are sRGB-encoded
	* through the SRV and raw through the UAV. Format queries - IRRHITexture::GetViewFormat,
	* the pipeline state's declared target formats - are per view for that reason.
	*/
	enum ERHIViewFormatType : UINT8
	{
		RHI_VIEW_FORMAT_SRV			= 0,
		RHI_VIEW_FORMAT_RTV			= 1,
		RHI_VIEW_FORMAT_DSV			= 2,
		RHI_VIEW_FORMAT_UAV			= 3,
		RHI_VIEW_FORMAT_COUNT		= 4
	};

	PE_CONSTEXPR_STATIC UINT32 RHI_MAX_RENDER_TARGETS			= 16u;
	PE_CONSTEXPR_STATIC UINT32 RHI_MAX_VERTEX_BUFFERS			= 32u;
	PE_CONSTEXPR_STATIC UINT32 RHI_INVALID_BINDLESS_INDEX		= 0xffffffffu;

	/*
	* Root signature descriptors. The rendering layer derives one from shader
	* reflection (per program variant) and hands it to the back-end; the back-ends
	* translate it (D3D12 serializes a real root signature, D3D11 has no root
	* signature at all and keeps only the hash). The interface that wraps a live
	* object is IRRHIRootSignature (RHI/IRRHIRootSignature.h).
	*
	* Root parameter indices are what the command list's Set*RootConstantBuffer /
	* Set*Root32BitConstants calls name, so a valid layout always keeps constant
	* buffers in the low roots and the descriptor tables / 32 bit constants after
	* them (see RRHIPipelineLayoutInfo).
	*/
	PE_CONSTEXPR_STATIC UINT32 RHI_INVALID_ROOT_INDEX			= 0xffffffffu;

	enum ERHIRootParameterType : UINT8
	{
		RHI_ROOT_PARAMETER_CBV				= 0u,	// root constant buffer view
		RHI_ROOT_PARAMETER_TABLE			= 1u,	// descriptor table (SRV / UAV / sampler range)
		RHI_ROOT_PARAMETER_32BIT_CONSTANTS	= 2u,	// root constant block (push constants)
		RHI_ROOT_PARAMETER_TYPE_COUNT
	};

	enum ERHIDescriptorRangeType : UINT8
	{
		RHI_DESCRIPTOR_RANGE_SRV		= 0u,
		RHI_DESCRIPTOR_RANGE_UAV		= 1u,
		RHI_DESCRIPTOR_RANGE_SAMPLER	= 2u,
		RHI_DESCRIPTOR_RANGE_TYPE_COUNT
	};

	enum ERHIShaderVisibility : UINT8
	{
		RHI_SHADER_VISIBILITY_ALL		= 0u,
		RHI_SHADER_VISIBILITY_VERTEX	= 1u,
		RHI_SHADER_VISIBILITY_PIXEL		= 2u,
		RHI_SHADER_VISIBILITY_COMPUTE	= 3u,
		RHI_SHADER_VISIBILITY_TYPE_COUNT
	};

	struct RRHIDescriptorRange
	{
		ERHIDescriptorRangeType		Type				= ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_SRV;
		UINT32						BaseShaderRegister	= 0u;
		UINT32						NumDescriptors		= 0u;
		UINT32						RegisterSpace		= 0u;
	};

	struct RRHIRootParameterDesc
	{
		ERHIRootParameterType		Type				= ERHIRootParameterType::RHI_ROOT_PARAMETER_CBV;
		ERHIShaderVisibility		Visibility			= ERHIShaderVisibility::RHI_SHADER_VISIBILITY_ALL;
		UINT32						ShaderRegister		= 0u;
		UINT32						RegisterSpace		= 0u;
		// Root constant block width. Only read when Type == 32BIT_CONSTANTS.
		UINT32						Num32BitValues		= 0u;
		// Table description. Only read when Type == TABLE.
		RRHIDescriptorRange			Range{};
	};

	struct RRHIRootSignatureDesc
	{
		TArray<RRHIRootParameterDesc>	Parameters;
		// D3D12: ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT. Set for programs that carry a
		// vertex stage.
		BOOL8							bAllowInputAssembler	= FALSE;
		// D3D12: the two *_HEAP_DIRECTLY_INDEXED flags. Only set for bindless
		// variants, whose shaders index the descriptor heaps by UINT32 index.
		BOOL8							bHeapDirectlyIndexed	= FALSE;
		// Identity of this layout: carried through to IRRHIRootSignature and used as the
		// signature / PSO cache key. A generator fills it with HashRHIRootSignatureDesc(desc);
		// the back-ends fall back to that same function when it arrives as 0, so a hand built
		// desc stays consistent. A value that disagrees with the parameter list would alias
		// two different layouts, so never invent one.
		UINT64							LayoutHash				= 0ull;
	};

	/*
	* Everything the command list needs to bind a pipeline's parameter space. It is
	* produced together with the root signature and copied onto every PSO created
	* with it, so binding never has to walk shader reflection or assume that a root
	* index equals a register slot.
	*
	* INVARIANT (kept by the layout generator): descriptor ranges start at register
	* 0 and cover every register the program uses, so a resource's offset inside its
	* table equals its register slot.
	*/
	struct RRHIPipelineLayoutInfo
	{
		// Number of root parameters of the signature this layout came from. The command
		// list checks a caller's root index against it, because a root index has to name
		// an existing parameter of the bound signature. Zero means the back-end has no
		// root signature concept at all (D3D11), where a root index is a register slot
		// and the back-end's own slot limits apply instead.
		UINT32		ParameterCount		= 0u;
		// Root index of each descriptor table, RHI_INVALID_ROOT_INDEX when the
		// program has no parameter of that kind.
		UINT32		SRVTableRoot		= RHI_INVALID_ROOT_INDEX;
		UINT32		UAVTableRoot		= RHI_INVALID_ROOT_INDEX;
		UINT32		SamplerTableRoot	= RHI_INVALID_ROOT_INDEX;
		// Root index of the 32 bit constant block, RHI_INVALID_ROOT_INDEX when the
		// program has no root constant buffer.
		UINT32		Constants32BitRoot	= RHI_INVALID_ROOT_INDEX;
		// Number of leading root CBV parameters, i.e. the root indices [0, this)
		// bind constant buffers.
		UINT32		ConstantBufferCount	= 0u;
		// Descriptor count of each table. Zero when the table does not exist.
		UINT32		SRVTableSize		= 0u;
		UINT32		UAVTableSize		= 0u;
		UINT32		SamplerTableSize	= 0u;
		// Width of the 32 bit constant block in DWORDs. Zero when the program has none.
		UINT32		Constants32BitCount	= 0u;
	};

	struct RRHIDeviceInitDesc
	{
		HWND				WindowHandle;
		UINT32				BackBufferWidth;
		UINT32				BackBufferHeight;
		UINT32				BackBufferCount;		// Swapchain back-buffer count (2 = double, 3 = triple)
		UINT32				RefreshRateHz;			// Swapchain refresh rate hint
		RFormatType			BackBufferFormat;
		RFormatType			DepthStencilFormat;
		BOOL8				bIsWindowed;
		BOOL8				bEnableDebugLayer;
		BOOL8				bEnableGPUValidation;

		RRHIDeviceInitDesc() noexcept
			: WindowHandle{ nullptr }
			, BackBufferWidth{ 0u }
			, BackBufferHeight{ 0u }
			, BackBufferCount{ 2u }
			, RefreshRateHz{ 60u }
			, BackBufferFormat{ RFormatType::FORMAT_R8G8B8A8_UNORM }
			, DepthStencilFormat{ RFormatType::FORMAT_R24G8_TYPELESS }
			, bIsWindowed{ TRUE }
			, bEnableDebugLayer{ FALSE }
			, bEnableGPUValidation{ FALSE }
		{
		}
	};

	struct RRHIRenderTargetBinding
	{
		IRRHITexture*		Target;
		ERHILoadOp			LoadOp;
		ERHIStoreOp			StoreOp;
		Color4				ClearColor;

		RRHIRenderTargetBinding() noexcept
			: Target{ nullptr }
			, LoadOp{ ERHILoadOp::RHI_LOAD_OP_LOAD }
			, StoreOp{ ERHIStoreOp::RHI_STORE_OP_STORE }
			, ClearColor{ Color4{0.f, 0.f, 0.f, 1.f} }
		{
		}
	};

	struct RRHIDepthStencilBinding
	{
		IRRHITexture*		Target;
		ERHILoadOp			DepthLoadOp;
		ERHIStoreOp			DepthStoreOp;
		ERHILoadOp			StencilLoadOp;
		ERHIStoreOp			StencilStoreOp;
		FLOAT				ClearDepth;
		UINT8				ClearStencil;
		BOOL8				bIsReadOnly;

		RRHIDepthStencilBinding() noexcept
			: Target{ nullptr }
			, DepthLoadOp{ ERHILoadOp::RHI_LOAD_OP_LOAD }
			, DepthStoreOp{ ERHIStoreOp::RHI_STORE_OP_STORE }
			, StencilLoadOp{ ERHILoadOp::RHI_LOAD_OP_LOAD }
			, StencilStoreOp{ ERHIStoreOp::RHI_STORE_OP_STORE }
			, ClearDepth{ RCommonSettings::RENDER_DEPTH_FAR }
			, ClearStencil{ 0u }
			, bIsReadOnly{ FALSE }
		{
		}
	};

	struct RRHIRenderPassDesc
	{
		RRHIRenderTargetBinding		RenderTargets[RHI_MAX_RENDER_TARGETS];
		UINT32						RenderTargetCount;
		RRHIDepthStencilBinding		DepthStencil;
		// Optional GPU profiler tag. When non-null, BeginRenderPass / EndRenderPass auto-emit
		// a GPU event scope and bracket the pass with a pair of timestamps via the active
		// EGPUProfiler frame. The profiler is compiled in every non-Shipping configuration,
		// so the field cannot be editor-only: Shipping simply never sets it.
		const CHAR*					PassName;

		RRHIRenderPassDesc() noexcept
			: RenderTargets{}
			, RenderTargetCount{ 0u }
			, DepthStencil{}
			, PassName(nullptr)
		{
		}
	};

	struct RRHIShaderDesc
	{
		TSharedPtr<UINT8>			ByteCode;
		SIZE_T						ByteCodeSize;
		ERHIShaderStage				Stage;
		// Vertex-shader-only: optional input layout descriptors. The D3D11
		// back-end uses these at CreateShader time to build an
		// ID3D11InputLayout; D3D12 ignores them (layouts live on the PSO).
		TArray<RInputLayoutDesc>	InputLayouts;

		RRHIShaderDesc() noexcept
			: ByteCode{ nullptr }
			, ByteCodeSize{ 0u }
			, Stage{ ERHIShaderStage::RHI_SHADER_STAGE_COUNT }
			, InputLayouts{}
		{
		}
		RRHIShaderDesc(TSharedPtr<UINT8> InByteCode, SIZE_T InByteCodeSize, ERHIShaderStage InStage) noexcept
			: ByteCode{ InByteCode }
			, ByteCodeSize{ InByteCodeSize }
			, Stage{ InStage }
			, InputLayouts{}
		{
		}
	};

	struct RRHIGraphicsPipelineDesc
	{
		// Root signature the PSO is built against. The rendering layer derives it from the
		// shader program the desc carries, through RPipelineStateCache, so the signature and
		// the shaders can never disagree. A desc that names none is refused: a state without
		// a signature has no defined roots to bind.
		IRRHIRootSignature*			RootSignature;
		IRRHIShader*				VertexShader;
		IRRHIShader*				PixelShader;
		const RInputLayoutDesc*		InputLayouts;
		UINT32						InputLayoutCount;
		RRasterizerState			Rasterizer;
		RBlendDesc					Blend;
		RDepthState					Depth;
		RStencilState				Stencil;
		RPrimitiveTopologyType		PrimitiveTopology;
		UINT32						RenderTargetCount;
		RFormatType					RenderTargetFormats[RHI_MAX_RENDER_TARGETS];
		// DSV format is expressed via (DepthBits, HasStencil) so PSO matches
		// the resource side (RTextureDesc takes the same pair). The D3D12 PSO
		// factory maps (16/24/32, stencil flag) to the typed DSV format that
		// D3D12 requires; DepthBits=0 means no DSV.
		UINT32						DepthBits;
		BOOL32						HasStencil;
		UINT32						SampleCount;
		UINT32						SampleQuality;

		RRHIGraphicsPipelineDesc() noexcept
			: RootSignature{ nullptr }
			, VertexShader{ nullptr }
			, PixelShader{ nullptr }
			, InputLayouts{ nullptr }
			, InputLayoutCount{ 0u }
			, Rasterizer{}
			, Blend{}
			, Depth{}
			, Stencil{}
			, PrimitiveTopology{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST }
			, RenderTargetCount{ 0u }
			, RenderTargetFormats{}
			, DepthBits{ 0u }
			, HasStencil{ FALSE }
			, SampleCount{ 1u }
			, SampleQuality{ 0u }
		{
			for (UINT32 i = 0u; i < RHI_MAX_RENDER_TARGETS; i++)
			{
				RenderTargetFormats[i] = RFormatType::FORMAT_UNKNOWN;
			}
		}
	};

	struct RRHIComputePipelineDesc
	{
		// See RRHIGraphicsPipelineDesc::RootSignature: a compute state carries its program's
		// signature too, and a desc without one is refused by the back-end.
		IRRHIRootSignature*		RootSignature;
		IRRHIShader*		ComputeShader;

		RRHIComputePipelineDesc() noexcept
			: RootSignature{ nullptr }
			, ComputeShader{ nullptr }
		{
		}
	};

	/*
	* CPU-side description of a subresource payload. Mirrors the legacy
	* RSubresourceDataDesc but lives here so RHI users do not need to drag
	* in RenderCommon directly when they only deal with uploads.
	*/
	struct RRHISubresourceData
	{
		const UINT8*		Data;
		UINT64				DataSize;
		UINT32				RowPitch;
		UINT32				SlicePitch;

		RRHISubresourceData() noexcept
			: Data{ nullptr }
			, DataSize{ 0ull }
			, RowPitch{ 0u }
			, SlicePitch{ 0u }
		{
		}
		constexpr RRHISubresourceData(const UINT8* InData, UINT64 InDataSize, UINT32 InRowPitch, UINT32 InSlicePitch) noexcept
			: Data{ InData }
			, DataSize{ InDataSize }
			, RowPitch{ InRowPitch }
			, SlicePitch{ InSlicePitch }
		{
		}
	};

};	// PigeonEngine
