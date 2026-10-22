#include "DeviceD3D12.h"
#include "D3D12Native.h"
#include "CommandListD3D12.h"
#include "DXGIFormatTranslate.h"
#include <CoreMinimal.h>
#include <Config/EngineConfig.h>
#if _EDITOR_ONLY
#include <Editor/EditorLogManager.h>
#endif

namespace PigeonEngine
{
	// CD3DX12 replacements (d3dx12.h not available in this SDK)
	static inline D3D12_CPU_DESCRIPTOR_HANDLE OffsetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE InBase, UINT32 InOffset, UINT32 InDescriptorSize) noexcept
	{
		D3D12_CPU_DESCRIPTOR_HANDLE Result = InBase;
		Result.ptr += static_cast<SIZE_T>(InOffset) * static_cast<SIZE_T>(InDescriptorSize);
		return Result;
	}
	static inline D3D12_GPU_DESCRIPTOR_HANDLE OffsetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE InBase, UINT32 InOffset, UINT32 DescriptorSize) noexcept
	{
		D3D12_GPU_DESCRIPTOR_HANDLE Result = InBase;
		Result.ptr += static_cast<UINT64>(InOffset) * static_cast<UINT64>(DescriptorSize);
		return Result;
	}

	// =========================================================================
	// Static translation helpers
	// =========================================================================

	void RDeviceD3D12::TranslateResourceFormat(DXGI_FORMAT& OutFormat, RFormatType InFormat) noexcept
	{
		// The table is not in this file: it is shared with the D3D11 back-end
		// (RenderDevice/DXGIFormatTranslate), because two copies of it is how a format ends
		// up supported on one API and missing on the other. A format the table does not
		// carry is reported there instead of turning into DXGI_FORMAT_UNKNOWN in silence.
		RDXGIFormatTranslate::TranslateToDXGI(OutFormat, InFormat);
	}

	// Returns a pointer into a static literal table, never a temporary: the API keeps the pointer
	// until CreatePipelineState, and an EString returned by value would be destroyed at the end of
	// the calling expression (which is what this used to do).
	static const CHAR* GetSemanticName(RShaderSemanticType InType) noexcept
	{
		// Semantic types are bitflags; find the set bit index to look up the name.
		constexpr const UINT16 SemanticLookup[] =
		{
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_PSIZE),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_POSITIONT),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_BINORMAL),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_COLOR),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_TANGENT),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_NORMAL),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_POSITION),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD),
		};
		static const CHAR* const SemanticNames[] =
		{
			"PSIZE",
			"POSITIONT",
			"BINORMAL",
			"BLENDWEIGHT",
			"BLENDINDICES",
			"COLOR",
			"TANGENT",
			"NORMAL",
			"POSITION",
			"TEXCOORD"
		};
		constexpr UINT32 SemanticCount = PE_ARRAYSIZE(SemanticLookup);
		const UINT16 Value = static_cast<UINT16>(InType) & 0xffc0u;
		for (UINT32 i = 0u; i < SemanticCount; i++)
		{
			if (Value == SemanticLookup[i])
			{
				return SemanticNames[i];
			}
		}
		Check((FALSE), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Semantic")));
		return "UNKNOWN";
	}

	static UINT32 GetSemanticIndex(RShaderSemanticType InType) noexcept
	{
		constexpr const UINT16 SemanticLookup[] =
		{
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_PSIZE),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_POSITIONT),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_BINORMAL),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_COLOR),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_TANGENT),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_NORMAL),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_POSITION),
			static_cast<UINT16>(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD),
		};
		constexpr UINT32 SemanticCount = PE_ARRAYSIZE(SemanticLookup);
		const UINT32 Index = static_cast<UINT32>(InType) & 0xfu;
		const UINT16 Value = static_cast<UINT16>(InType) & 0xffc0u;
		for (UINT32 i = 0u; i < SemanticCount; i++)
		{
			if (Value == SemanticLookup[i])
			{
				return Index;
			}
		}
		Check((FALSE), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Semantic")));
		constexpr UINT32 MaxUint32 = static_cast<UINT32>(-1);
		return MaxUint32;
	}

	// Maps one input element to the API format that reads its bytes. The scalar width
	// (MemberStride) picks the channel size and MemberNum the component count, so the same
	// descriptor can describe 32 bit floats, 16 bit half floats and packed normalized bytes.
	// A width of 0 means "the class's natural one" (32 bit for float / int / uint, 8 bit for
	// normalized), which keeps a descriptor built before the width was part of the layout working
	// exactly as it did.
	// Combinations the API has no format for (8 bit float, 3 channel 8 / 16 bit, 32 bit
	// normalized) answer UNKNOWN; the caller refuses the pipeline instead of handing the driver a
	// format that would read the wrong bytes.
	static DXGI_FORMAT InputLayoutMemberFormatToDXGIFormat(RInputLayoutFormatType InFormat, UINT32 InMemberStride, UINT32 InMemberNum) noexcept
	{
		// Rows are the four format classes in RInputLayoutFormatType order, columns the component
		// count 0..4; entry [class][count].
		constexpr DXGI_FORMAT OneByteFormats[4][5] =
		{
			// 8 bit floats do not exist.
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_UNKNOWN,		DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_UNKNOWN },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R8_SINT,	DXGI_FORMAT_R8G8_SINT,		DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R8G8B8A8_SINT },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R8_UINT,	DXGI_FORMAT_R8G8_UINT,		DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R8G8B8A8_UINT },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R8_UNORM,	DXGI_FORMAT_R8G8_UNORM,		DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R8G8B8A8_UNORM }
		};
		constexpr DXGI_FORMAT TwoByteFormats[4][5] =
		{
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16_FLOAT,	DXGI_FORMAT_R16G16_FLOAT,	DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16G16B16A16_FLOAT },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16_SINT,	DXGI_FORMAT_R16G16_SINT,	DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16G16B16A16_SINT },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16_UINT,	DXGI_FORMAT_R16G16_UINT,	DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16G16B16A16_UINT },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16_UNORM,	DXGI_FORMAT_R16G16_UNORM,	DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R16G16B16A16_UNORM }
		};
		constexpr DXGI_FORMAT FourByteFormats[4][5] =
		{
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R32_FLOAT,	DXGI_FORMAT_R32G32_FLOAT,	DXGI_FORMAT_R32G32B32_FLOAT,	DXGI_FORMAT_R32G32B32A32_FLOAT },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R32_SINT,	DXGI_FORMAT_R32G32_SINT,	DXGI_FORMAT_R32G32B32_SINT,		DXGI_FORMAT_R32G32B32A32_SINT },
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_R32_UINT,	DXGI_FORMAT_R32G32_UINT,	DXGI_FORMAT_R32G32B32_UINT,		DXGI_FORMAT_R32G32B32A32_UINT },
			// 32 bit normalized data is not a thing the API can read.
			{ DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_UNKNOWN,	DXGI_FORMAT_UNKNOWN,		DXGI_FORMAT_UNKNOWN,			DXGI_FORMAT_UNKNOWN }
		};

		const UINT32 FormatIndex = ((InFormat < RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_COUNT)
			? static_cast<UINT32>(InFormat) : static_cast<UINT32>(RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_COUNT));
		const UINT32 ComponentIndex = ((InMemberNum < 4u) ? InMemberNum : 4u);

		const UINT32 Stride = ((InMemberStride == 0u)
			? ((InFormat == RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM) ? 1u : 4u)
			: InMemberStride);
		switch (Stride)
		{
		case 1u:
			return ((FormatIndex < 4u) ? OneByteFormats[FormatIndex][ComponentIndex] : DXGI_FORMAT_UNKNOWN);
		case 2u:
			return ((FormatIndex < 4u) ? TwoByteFormats[FormatIndex][ComponentIndex] : DXGI_FORMAT_UNKNOWN);
		case 4u:
			return ((FormatIndex < 4u) ? FourByteFormats[FormatIndex][ComponentIndex] : DXGI_FORMAT_UNKNOWN);
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	static D3D12_COMPARISON_FUNC TranslateComparisonFunc(RComparisonFunctionType InFunc) noexcept
	{
		constexpr D3D12_COMPARISON_FUNC D3DComparisonFunc[] =
		{
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NOT_EQUAL,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
			D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS
		};
		const UINT8 Func = static_cast<UINT8>(InFunc);
		Check(((Func > 0u) && (Func < (PE_ARRAYSIZE(D3DComparisonFunc)))), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Comparison Func")));
		return (D3DComparisonFunc[Func]);
	}

	static UINT64 CombineHashBlendDesc(const RBlendDesc& InBlend) noexcept
	{
		UINT64 Hash;
		auto HashCombine = [&Hash](UINT64 InValue)
		{
			Hash ^= InValue + 0x9e3779b97f4a7c15ull + (Hash << 6) + (Hash >> 2);
		};

		auto StateHashCombine = [&Hash, &HashCombine](const RBlendState& InState)
		{
			HashCombine(InState.BlendEnable ? 1ull : 0ull);
			HashCombine(static_cast<UINT64>(InState.SrcBlend));
			HashCombine(static_cast<UINT64>(InState.DstBlend));
			HashCombine(static_cast<UINT64>(InState.BlendOp));
			HashCombine(static_cast<UINT64>(InState.SrcBlendAlpha));
			HashCombine(static_cast<UINT64>(InState.DstBlendAlpha));
			HashCombine(static_cast<UINT64>(InState.BlendOpAlpha));
			HashCombine(static_cast<UINT64>(InState.RenderTargetWriteMask));
		};

		HashCombine(((InBlend.AlphaToCoverageEnable) ? 2ull : 1ull) | ((InBlend.IndependentBlendEnable) ? 8ull : 4ull));

		const INT32 NumBlendStates = InBlend.BlendStates.Num<INT32>();
		if (NumBlendStates > 0)
		{
			StateHashCombine(InBlend.BlendStates[0]);
		}
		if (InBlend.IndependentBlendEnable)
		{
			for (INT32 Idx = 1; Idx < NumBlendStates; Idx++)
			{
				StateHashCombine(InBlend.BlendStates[Idx]);
			}
		}

		return Hash;
	}

	void RDeviceD3D12::TranslateInputElementDesc(D3D12_INPUT_ELEMENT_DESC& OutDesc, const RInputLayoutDesc& InDesc) noexcept
	{
		// Literal from a static table: it stays valid until the pipeline state is created, which is
		// how long the API keeps this pointer.
		OutDesc.SemanticName = GetSemanticName(InDesc.SemanticName);
		OutDesc.SemanticIndex = GetSemanticIndex(InDesc.SemanticName);
		OutDesc.Format = InputLayoutMemberFormatToDXGIFormat(InDesc.MemberFormat, InDesc.MemberStride, InDesc.MemberNum);
		OutDesc.InputSlot = InDesc.InputSlot;
		// The caller may declare where the element sits inside its vertex, which is what an
		// interleaved or padded stream needs; the sentinel means the API's own appending rule.
		OutDesc.AlignedByteOffset = ((InDesc.UsesAppendOffset() != FALSE)
			? D3D12_APPEND_ALIGNED_ELEMENT : static_cast<UINT>(InDesc.AlignedByteOffset));
		OutDesc.InputSlotClass = (InDesc.InputSlotClass == RInputClassificationType::INPUT_PER_INSTANCE_DATA)
			? D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
			: D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		OutDesc.InstanceDataStepRate = InDesc.InstanceDataStepRate;
	}

	void RDeviceD3D12::TranslatePrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY& OutTopology, RPrimitiveTopologyType InTopology) noexcept
	{
		constexpr const D3D_PRIMITIVE_TOPOLOGY TopologyList[] =
		{
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST,
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
		};
		OutTopology = D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		const UINT8 TopologyIdx = static_cast<UINT8>(InTopology);
		constexpr UINT32 NumTopologyList = PE_ARRAYSIZE(TopologyList);
		if (TopologyIdx < NumTopologyList)
		{
			OutTopology = TopologyList[TopologyIdx];
		}
	}

	void RDeviceD3D12::TranslateResourceStates(D3D12_RESOURCE_STATES& OutStates, UINT16 InStates) noexcept
	{
		OutStates = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		if (InStates == static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_COMMON))
		{
			OutStates = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
			return;
		}
		UINT32 Result = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_VERTEX_BUFFER))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_INDEX_BUFFER))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_CONSTANT_BUFFER))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_READ))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_COPY_SOURCE))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_COPY_DEST))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_PRESENT))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
		}
		if (InStates & static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT))
		{
			Result |= D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		}
		OutStates = static_cast<D3D12_RESOURCE_STATES>(Result);
	}

	void RDeviceD3D12::TranslateCullMode(D3D12_CULL_MODE& OutCull, RCullModeType InCull) noexcept
	{
		switch (InCull)
		{
			case RCullModeType::CULL_NONE:
				OutCull = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
				break;
			case RCullModeType::CULL_FRONT:
				OutCull = D3D12_CULL_MODE::D3D12_CULL_MODE_FRONT;
				break;
			case RCullModeType::CULL_BACK:
				OutCull = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
				break;
			default:
				Check((FALSE), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Cull Mode")));
				break;
		}
	}

	void RDeviceD3D12::TranslateFillMode(D3D12_FILL_MODE& OutFill, RFillModeType InFill) noexcept
	{
		switch (InFill)
		{
			case RFillModeType::FILL_WIREFRAME:
				OutFill = D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
				break;
			case RFillModeType::FILL_SOLID:
				OutFill = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
				break;
			default:
				Check((FALSE), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Fill Mode")));
				break;
		}
	}

	void RDeviceD3D12::TranslateRenderTargetBlendDesc(D3D12_BLEND_DESC& OutBlend, const RBlendDesc& InBlend) noexcept
	{
		const INT32 BlendStateNum = InBlend.BlendStates.Num<INT32>();
		if ((BlendStateNum <= 0) || (BlendStateNum > 8))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create blend state check failed."));
			return;
		}

		auto TranslateD3DBlendState = [](D3D12_RENDER_TARGET_BLEND_DESC& OutBlend, const RBlendState& InBlendState)->void
		{
			{
				OutBlend.BlendEnable				= (InBlendState.BlendEnable ? TRUE : FALSE);
				OutBlend.LogicOpEnable				= FALSE;		// TODO
				OutBlend.SrcBlend					= D3D12_BLEND::D3D12_BLEND_ONE;
				OutBlend.DestBlend					= D3D12_BLEND::D3D12_BLEND_ZERO;
				OutBlend.BlendOp					= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
				OutBlend.SrcBlendAlpha				= D3D12_BLEND::D3D12_BLEND_ONE;
				OutBlend.DestBlendAlpha				= D3D12_BLEND::D3D12_BLEND_ZERO;
				OutBlend.BlendOpAlpha				= D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
				OutBlend.LogicOp					= D3D12_LOGIC_OP::D3D12_LOGIC_OP_CLEAR;	// TODO
				OutBlend.RenderTargetWriteMask		= D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
			}

			if (InBlendState.BlendEnable)
			{
				auto TranslateBlendOption = [](RBlendOptionType InOpt) -> D3D12_BLEND
				{
					constexpr D3D12_BLEND D3DBlend[] =
					{
						D3D12_BLEND::D3D12_BLEND_ZERO,
						D3D12_BLEND::D3D12_BLEND_ZERO,
						D3D12_BLEND::D3D12_BLEND_ONE,
						D3D12_BLEND::D3D12_BLEND_SRC_COLOR,
						D3D12_BLEND::D3D12_BLEND_INV_SRC_COLOR,
						D3D12_BLEND::D3D12_BLEND_SRC_ALPHA,
						D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA,
						D3D12_BLEND::D3D12_BLEND_DEST_ALPHA,
						D3D12_BLEND::D3D12_BLEND_INV_DEST_ALPHA,
						D3D12_BLEND::D3D12_BLEND_DEST_COLOR,
						D3D12_BLEND::D3D12_BLEND_INV_DEST_COLOR,
						D3D12_BLEND::D3D12_BLEND_SRC_ALPHA_SAT,
						D3D12_BLEND::D3D12_BLEND_BLEND_FACTOR,
						D3D12_BLEND::D3D12_BLEND_INV_BLEND_FACTOR
					};
					// Referenced by the Check below only, which compiles to nothing in Shipping.
					[[maybe_unused]] constexpr UINT32 NumD3DBlends = PE_ARRAYSIZE(D3DBlend);
					const UINT8 BlendOpt = static_cast<UINT8>(InOpt);
					Check(((BlendOpt > 0u) && (BlendOpt < NumD3DBlends)), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Blend Type")));
					return (D3DBlend[BlendOpt]);
				};
				auto TranslateBlendOp = [](RBlendOperationType InOp) -> D3D12_BLEND_OP
				{
					constexpr D3D12_BLEND_OP D3DBlendOp[] =
					{
						D3D12_BLEND_OP::D3D12_BLEND_OP_ADD,
						D3D12_BLEND_OP::D3D12_BLEND_OP_ADD,
						D3D12_BLEND_OP::D3D12_BLEND_OP_SUBTRACT,
						D3D12_BLEND_OP::D3D12_BLEND_OP_REV_SUBTRACT,
						D3D12_BLEND_OP::D3D12_BLEND_OP_MIN,
						D3D12_BLEND_OP::D3D12_BLEND_OP_MAX
					};
					// Referenced by the Check below only, which compiles to nothing in Shipping.
					[[maybe_unused]] constexpr UINT32 NumD3DBlendOps = PE_ARRAYSIZE(D3DBlendOp);
					const UINT8 BlendOp = static_cast<UINT8>(InOp);
					Check(((BlendOp > 0u) && (BlendOp < NumD3DBlendOps)), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Blend Op Type")));
					return (D3DBlendOp[BlendOp]);
				};
				OutBlend.SrcBlend = TranslateBlendOption(InBlendState.SrcBlend);
				OutBlend.DestBlend = TranslateBlendOption(InBlendState.DstBlend);
				OutBlend.BlendOp = TranslateBlendOp(InBlendState.BlendOp);
				OutBlend.SrcBlendAlpha = TranslateBlendOption(InBlendState.SrcBlendAlpha);
				OutBlend.DestBlendAlpha = TranslateBlendOption(InBlendState.DstBlendAlpha);
				OutBlend.BlendOpAlpha = TranslateBlendOp(InBlendState.BlendOpAlpha);
			}

			{
				UINT8 Mask = 0u;
				if (InBlendState.RenderTargetWriteMask & static_cast<UINT8>(RColorWriteMaskType::COLOR_WRITE_MASK_RED))
				{
					Mask |= D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_RED;
				}
				if (InBlendState.RenderTargetWriteMask & static_cast<UINT8>(RColorWriteMaskType::COLOR_WRITE_MASK_GREEN))
				{
					Mask |= D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_GREEN;
				}
				if (InBlendState.RenderTargetWriteMask & static_cast<UINT8>(RColorWriteMaskType::COLOR_WRITE_MASK_BLUE))
				{
					Mask |= D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_BLUE;
				}
				if (InBlendState.RenderTargetWriteMask & static_cast<UINT8>(RColorWriteMaskType::COLOR_WRITE_MASK_ALPHA))
				{
					Mask |= D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALPHA;
				}
				OutBlend.RenderTargetWriteMask = Mask;
			}
		};

		{
			::ZeroMemory(&OutBlend, sizeof(OutBlend));
			OutBlend.AlphaToCoverageEnable = InBlend.AlphaToCoverageEnable;
			OutBlend.IndependentBlendEnable = InBlend.IndependentBlendEnable;
			TranslateD3DBlendState(OutBlend.RenderTarget[0], InBlend.BlendStates[0]);
			if (OutBlend.IndependentBlendEnable)
			{
				for (INT32 Idx = 1; Idx < BlendStateNum; Idx++)
				{
					TranslateD3DBlendState(OutBlend.RenderTarget[Idx], InBlend.BlendStates[Idx]);
				}
			}
		}
	}

	void RDeviceD3D12::TranslateDepthStencilDesc(D3D12_DEPTH_STENCIL_DESC& OutDS, const RDepthState& InDepth, const RStencilState& InStencil) noexcept
	{
		OutDS.DepthEnable = (InDepth.DepthEnable ? TRUE : FALSE);

		constexpr D3D12_DEPTH_WRITE_MASK D3DDepthWriteMask[] =
		{
			D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO,
			D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL
		};

		{
			const UINT8 DepthWriteMask = static_cast<UINT8>(InDepth.DepthWriteMask);
			Check((DepthWriteMask < (PE_ARRAYSIZE(D3DDepthWriteMask))), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Depth Write Mask")));
			OutDS.DepthWriteMask = D3DDepthWriteMask[DepthWriteMask];
		}

		OutDS.DepthFunc = TranslateComparisonFunc(InDepth.DepthFunc);

		OutDS.StencilEnable = (InStencil.StencilEnable ? TRUE : FALSE);
		OutDS.StencilReadMask = InStencil.StencilReadMask;
		OutDS.StencilWriteMask = InStencil.StencilWriteMask;

		auto TranslateStencilOp = [](RStencilOperationType InOp) -> D3D12_STENCIL_OP
		{
			constexpr D3D12_STENCIL_OP D3DStencilOp[] =
			{
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_ZERO,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_REPLACE,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR_SAT,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR_SAT,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_INVERT,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR,
				D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR
			};
			const UINT8 Op = static_cast<UINT8>(InOp);
			Check(((Op > 0u) && (Op < (PE_ARRAYSIZE(D3DStencilOp)))), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Stencil Op")));
			return (D3DStencilOp[Op]);
		};

		{
			OutDS.FrontFace.StencilFailOp = TranslateStencilOp(InStencil.FrontFace.StencilFailOp);
			OutDS.FrontFace.StencilDepthFailOp = TranslateStencilOp(InStencil.FrontFace.StencilDepthFailOp);
			OutDS.FrontFace.StencilPassOp = TranslateStencilOp(InStencil.FrontFace.StencilPassOp);
			OutDS.FrontFace.StencilFunc = TranslateComparisonFunc(InStencil.FrontFace.StencilFunc);
			OutDS.BackFace.StencilFailOp = TranslateStencilOp(InStencil.BackFace.StencilFailOp);
			OutDS.BackFace.StencilDepthFailOp = TranslateStencilOp(InStencil.BackFace.StencilDepthFailOp);
			OutDS.BackFace.StencilPassOp = TranslateStencilOp(InStencil.BackFace.StencilPassOp);
			OutDS.BackFace.StencilFunc = TranslateComparisonFunc(InStencil.BackFace.StencilFunc);
		}
	}

	void RDeviceD3D12::TranslateSamplerDesc(D3D12_SAMPLER_DESC& OutSampler, const RSamplerState& InSampler) noexcept
	{
		constexpr D3D12_FILTER D3DFilter[] =
		{
			D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_FILTER::D3D12_FILTER_ANISOTROPIC
		};
		const UINT8 Filter = static_cast<UINT8>(InSampler.Filter);
		Check(((Filter > 0u) && (Filter < (PE_ARRAYSIZE(D3DFilter)))), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Filter Type")));
		OutSampler.Filter = D3DFilter[Filter];

		auto TranslateAddressMode = [](RTextureAddressModeType InMode) -> D3D12_TEXTURE_ADDRESS_MODE
		{
			constexpr D3D12_TEXTURE_ADDRESS_MODE D3DTexAddrMode[] =
			{
				D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
				D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_BORDER,
				D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
			};
			const UINT8 Mode = static_cast<UINT8>(InMode);
			Check(((Mode > 0u) && (Mode < (PE_ARRAYSIZE(D3DTexAddrMode)))), (ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Unknown Filter Type")));
			return (D3DTexAddrMode[Mode]);
		};
		OutSampler.AddressU = TranslateAddressMode(InSampler.AddressU);
		OutSampler.AddressV = TranslateAddressMode(InSampler.AddressV);
		OutSampler.AddressW = TranslateAddressMode(InSampler.AddressW);
		OutSampler.MipLODBias = InSampler.MipLODBias;
		OutSampler.MaxAnisotropy = InSampler.MaxAnisotropy;
		// D3D12 honors a comparison function only for the comparison filter variants, and the
		// engine filter enum has none, so a non-NEVER function would be dropped silently here
		// (STATE_CREATION WARNING #1361). The desc is the caller's to get right.
		Check((InSampler.ComparisonFunc == RComparisonFunctionType::COMPARISON_NEVER),
			(ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Sampler comparison function is set on a filter without a comparison variant.")));
		OutSampler.ComparisonFunc = TranslateComparisonFunc(InSampler.ComparisonFunc);
		OutSampler.BorderColor[0] = InSampler.BorderColor.r;
		OutSampler.BorderColor[1] = InSampler.BorderColor.g;
		OutSampler.BorderColor[2] = InSampler.BorderColor.b;
		OutSampler.BorderColor[3] = InSampler.BorderColor.a;
		OutSampler.MinLOD = InSampler.MinLOD;
		OutSampler.MaxLOD = InSampler.MaxLOD;
	}

	void RDeviceD3D12::TranslateRenderTargetClearValue(D3D12_CLEAR_VALUE& OutClear, RFormatType InFormat, const Color4& InColor) noexcept
	{
		TranslateResourceFormat(OutClear.Format, InFormat);
		// Colour only. The depth/stencil union member shares this storage; see the note on the
		// declarations in the header.
		OutClear.Color[0] = InColor.r;
		OutClear.Color[1] = InColor.g;
		OutClear.Color[2] = InColor.b;
		OutClear.Color[3] = InColor.a;
	}

	void RDeviceD3D12::TranslateDepthStencilClearValue(D3D12_CLEAR_VALUE& OutClear, RFormatType InFormat, FLOAT InDepth, UINT8 InStencil) noexcept
	{
		TranslateResourceFormat(OutClear.Format, InFormat);
		// Depth and stencil only, for the same reason. The caller patches the format to the typed
		// depth format when the texture was created typeless.
		OutClear.DepthStencil.Depth = InDepth;
		OutClear.DepthStencil.Stencil = InStencil;
	}

	UINT64 RDeviceD3D12::HashPipelineStateDesc(const RRHIGraphicsPipelineDesc& InDesc, UINT64 InLayoutHash) noexcept
	{
		UINT64 Hash = 0u;

		auto HashCombine = [&Hash](UINT64 InValue)
		{
			Hash ^= InValue + 0x9e3779b97f4a7c15ull + (Hash << 6) + (Hash >> 2);
		};

		// The root signature is part of the PSO: two descs that differ only in their
		// parameter layout are different pipelines, so the layout identity joins the key.
		HashCombine(InLayoutHash);

		union EPtrUint64
		{
			UINT64		_Uint64;
			void*		_Ptr;
		};

		EPtrUint64 HashVS;
		HashVS._Ptr = InDesc.VertexShader;
		EPtrUint64 HashPS;
		HashPS._Ptr = InDesc.PixelShader;

		HashCombine(HashVS._Uint64);
		HashCombine(HashPS._Uint64);

		if (InDesc.VertexShader)
		{
			HashCombine(static_cast<const IRD3D12Shader*>(InDesc.VertexShader)->GetByteCodeSize());
		}
		if (InDesc.PixelShader)
		{
			HashCombine(static_cast<const IRD3D12Shader*>(InDesc.PixelShader)->GetByteCodeSize());
		}

		// Fixed-function state
		HashCombine(static_cast<UINT64>(InDesc.Rasterizer.CullMode));
		HashCombine(static_cast<UINT64>(InDesc.Rasterizer.FillMode));
		HashCombine(CombineHashBlendDesc(InDesc.Blend));
		HashCombine(static_cast<UINT64>(InDesc.Depth.DepthEnable));
		HashCombine(static_cast<UINT64>(InDesc.Depth.DepthWriteMask));
		HashCombine(static_cast<UINT64>(InDesc.Depth.DepthFunc));
		HashCombine(static_cast<UINT64>(InDesc.Stencil.StencilEnable));
		HashCombine(static_cast<UINT64>(InDesc.Stencil.StencilReadMask));
		HashCombine(static_cast<UINT64>(InDesc.Stencil.StencilWriteMask));
		HashCombine(static_cast<UINT64>(InDesc.PrimitiveTopology));

		// Render-target / depth-stencil format set. Only the slots the PSO factory
		// consumes take part in the key, so a desc hashes to the PSO it will create.
		const UINT32 HashedRenderTargetCount = (InDesc.RenderTargetCount > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
			? D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT : InDesc.RenderTargetCount;
		HashCombine(static_cast<UINT64>(HashedRenderTargetCount));
		for (UINT32 i = 0u; i < HashedRenderTargetCount; i++)
		{
			HashCombine(static_cast<UINT64>(InDesc.RenderTargetFormats[i]));
		}
		HashCombine(static_cast<UINT64>(InDesc.DepthBits));
		HashCombine(static_cast<UINT64>(InDesc.HasStencil));
		HashCombine(static_cast<UINT64>(InDesc.SampleCount));
		HashCombine(static_cast<UINT64>(InDesc.SampleQuality));

		// Input layout - hash every contributing field so different vertex
		// formats with the same semantics don't collide.
		HashCombine(static_cast<UINT64>(InDesc.InputLayoutCount));
		if (InDesc.InputLayouts && InDesc.InputLayoutCount > 0u)
		{
			for (UINT32 i = 0u; i < InDesc.InputLayoutCount; i++)
			{
				const RInputLayoutDesc& Layout = InDesc.InputLayouts[i];
				HashCombine(static_cast<UINT64>(Layout.SemanticName));
				HashCombine(static_cast<UINT64>(Layout.MemberFormat));
				HashCombine(static_cast<UINT64>(Layout.MemberNum));
				HashCombine(static_cast<UINT64>(Layout.MemberStride));
				HashCombine(static_cast<UINT64>(Layout.InputSlot));
				HashCombine(static_cast<UINT64>(Layout.InputSlotClass));
				HashCombine(static_cast<UINT64>(Layout.InstanceDataStepRate));
				HashCombine(static_cast<UINT64>(Layout.AlignedByteOffset));
			}
		}
		return Hash;
	}

	// =========================================================================
	// Core D3D12 infrastructure
	// =========================================================================

	BOOL8 RDeviceD3D12::CreateDeviceAndQueue()
	{
		UINT32 DxgiFactoryFlags = 0u;
		if (bEnableDebugLayer)
		{
			Microsoft::WRL::ComPtr<ID3D12Debug> DebugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(DebugController.GetAddressOf()))))
			{
				DebugController->EnableDebugLayer();
				if (bEnableGPUValidation)
				{
					Microsoft::WRL::ComPtr<ID3D12Debug1> DebugController1;
					if (SUCCEEDED(DebugController.As(&DebugController1)))
					{
						DebugController1->SetEnableGPUBasedValidation(TRUE);
					}
				}
			}
			DxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}

		Microsoft::WRL::ComPtr<IDXGIFactory4> Factory;
		if (FAILED(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(Factory.GetAddressOf()))))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateDXGIFactory2 failed."));
			return FALSE;
		}

		// Select adapter (prefer dedicated GPU)
		Microsoft::WRL::ComPtr<IDXGIAdapter1> Adapter;
		{
			Microsoft::WRL::ComPtr<IDXGIAdapter1> CandidateAdapter;
			SIZE_T MaxDedicatedVideoMemory = 0u;
			for (UINT32 AdapterIndex = 0u;
				Factory->EnumAdapters1(AdapterIndex, CandidateAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND;
				++AdapterIndex)
			{
				DXGI_ADAPTER_DESC1 Desc;
				CandidateAdapter->GetDesc1(&Desc);
				if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}
				if (Desc.DedicatedVideoMemory > MaxDedicatedVideoMemory)
				{
					MaxDedicatedVideoMemory = Desc.DedicatedVideoMemory;
					Adapter = CandidateAdapter;
				}
			}
		}
		if (!Adapter)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: No suitable adapter found."));
			return FALSE;
		}

		if (FAILED(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(Device.ReleaseAndGetAddressOf()))))
		{
			// Fallback to 11_0
			if (FAILED(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(Device.ReleaseAndGetAddressOf()))))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: D3D12CreateDevice failed."));
				return FALSE;
			}
		}

		// Command queue
		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
		QueueDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
		QueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		QueueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
		if (FAILED(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(CommandQueue.ReleaseAndGetAddressOf()))))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateCommandQueue failed."));
			return FALSE;
		}

		// Prevent DXGI from intercepting Alt+Enter
		{
			Factory->MakeWindowAssociation(nullptr, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
		}

		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateSwapChain(HWND InWindowHandle)
	{
		Microsoft::WRL::ComPtr<IDXGIFactory4> Factory;
		if (FAILED(CreateDXGIFactory2(0u, IID_PPV_ARGS(Factory.GetAddressOf()))))
		{
			return FALSE;
		}

		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Width              = (BackBufferWidth > 0u) ? BackBufferWidth : 1920u;
		SwapChainDesc.Height             = (BackBufferHeight > 0u) ? BackBufferHeight : 1080u;
		SwapChainDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
		{
			DXGI_FORMAT Fmt;
			TranslateResourceFormat(Fmt, BackBufferFormat);
			if (Fmt != DXGI_FORMAT_UNKNOWN)
			{
				SwapChainDesc.Format = Fmt;
			}
		}
		SwapChainDesc.Stereo             = FALSE;
		SwapChainDesc.SampleDesc.Count   = 1u;
		SwapChainDesc.SampleDesc.Quality = 0u;
		SwapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.BufferCount        = BackBufferCount;
		SwapChainDesc.Scaling            = DXGI_SCALING_NONE;
		SwapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
		SwapChainDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain1;
		if (FAILED(Factory->CreateSwapChainForHwnd(CommandQueue.Get(), InWindowHandle,
			&SwapChainDesc, nullptr, nullptr, SwapChain1.GetAddressOf())))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateSwapChainForHwnd failed."));
			return FALSE;
		}

		if (FAILED(SwapChain1.As(&SwapChain)))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: QueryInterface IDXGISwapChain3 failed."));
			return FALSE;
		}

		// Store effective dimensions
		DXGI_SWAP_CHAIN_DESC1 EffectiveDesc;
		SwapChain->GetDesc1(&EffectiveDesc);
		BackBufferWidth  = EffectiveDesc.Width;
		BackBufferHeight = EffectiveDesc.Height;

		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateDescriptorHeaps()
	{
		// RTV heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			HeapDesc.NumDescriptors = RTV_HEAP_SIZE;
			HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			if (FAILED(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(RTVHeap.ReleaseAndGetAddressOf()))))
			{
				return FALSE;
			}
			RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// DSV heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			HeapDesc.NumDescriptors = DSV_HEAP_SIZE;
			HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			if (FAILED(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(DSVHeap.ReleaseAndGetAddressOf()))))
			{
				return FALSE;
			}
			DSVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		// CBV_SRV_UAV heap (shader-visible for bindless)
		{
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			HeapDesc.NumDescriptors = CBVSRVUAV_HEAP_SIZE;
			HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (FAILED(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(CBVSRVUAVHeap.ReleaseAndGetAddressOf()))))
			{
				return FALSE;
			}
			CBVSRVUAVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// CBV_SRV_UAV staging heap (CPU-only). Descriptors are first created here
		// so CopyDescriptorsSimple can later read from them (shader-visible heaps
		// are CPU-write-only, so reading back fails with error #654). The two
		// heaps share the same slot layout - index N on staging maps to index N
		// on the shader-visible heap.
		{
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			HeapDesc.NumDescriptors = CBVSRVUAV_HEAP_SIZE;
			HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			if (FAILED(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(CBVSRVUAVStagingHeap.ReleaseAndGetAddressOf()))))
			{
				return FALSE;
			}
		}

		// Sampler heap (shader-visible)
		{
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			HeapDesc.NumDescriptors = SAMPLER_HEAP_SIZE;
			HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (FAILED(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(SamplerHeap.ReleaseAndGetAddressOf()))))
			{
				return FALSE;
			}
			SamplerDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		}

		// Sampler staging heap (CPU-only). Same staging pattern as CBV/SRV/UAV.
		{
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			HeapDesc.NumDescriptors = SAMPLER_HEAP_SIZE;
			HeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			if (FAILED(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(SamplerStagingHeap.ReleaseAndGetAddressOf()))))
			{
				return FALSE;
			}
		}

		// Prime the two per-frame set arenas with null views. Every SRV a frame binds is copied
		// in on top of one of these, so a slot a pass reads without having bound anything reads
		// zero instead of whatever an earlier frame left in that descriptor - descriptors are
		// not cleared between frames, and a stale one can point at a resource that has since
		// been released. A set's UAV table is expected to be filled by the pass that binds it:
		// a UAV read from an unwritten slot is a pass bug, and the descriptor kind mismatch
		// surfaces it instead of quietly reading an old resource.
		{
			const UINT32 ArenaSlotNum = SLOT_TABLE_STRIDE_PER_FRAME * FRAME_IN_FLIGHT;
			for (UINT32 SlotIndex = 0u; SlotIndex < ArenaSlotNum; SlotIndex++)
			{
				WriteNullShaderResourceView(SlotIndex);
			}
		}

		return TRUE;
	}

	void RDeviceD3D12::TranslateRootParameterVisibility(D3D12_SHADER_VISIBILITY& OutVisibility, ERHIShaderVisibility InVisibility) noexcept
	{
		switch (InVisibility)
		{
		case ERHIShaderVisibility::RHI_SHADER_VISIBILITY_VERTEX:
			OutVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			break;
		case ERHIShaderVisibility::RHI_SHADER_VISIBILITY_PIXEL:
			OutVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			break;
		case ERHIShaderVisibility::RHI_SHADER_VISIBILITY_COMPUTE:
			// A compute root signature has no stage visibility: the field exists for
			// graphics pipelines and has to be ALL for compute to see the parameter.
			OutVisibility = D3D12_SHADER_VISIBILITY_ALL;
			break;
		default:
			OutVisibility = D3D12_SHADER_VISIBILITY_ALL;
			break;
		}
	}

	void RDeviceD3D12::BuildLayoutInfo(const RRHIRootSignatureDesc& InDesc, RRHIPipelineLayoutInfo& OutLayoutInfo) noexcept
	{
		OutLayoutInfo = RRHIPipelineLayoutInfo{};
		const UINT32 ParameterCount = InDesc.Parameters.Num<UINT32>();
		OutLayoutInfo.ParameterCount = ParameterCount;

		for (UINT32 i = 0u; i < ParameterCount; i++)
		{
			const RRHIRootParameterDesc& Parameter = InDesc.Parameters[i];
			switch (Parameter.Type)
			{
			case ERHIRootParameterType::RHI_ROOT_PARAMETER_CBV:
				// Constant buffers occupy the low roots in register order, so the run of
				// CBV parameters from root 0 is exactly what the binding side binds.
				if (OutLayoutInfo.ConstantBufferCount == i)
				{
					OutLayoutInfo.ConstantBufferCount = i + 1u;
				}
				break;
			case ERHIRootParameterType::RHI_ROOT_PARAMETER_32BIT_CONSTANTS:
				if (OutLayoutInfo.Constants32BitRoot == RHI_INVALID_ROOT_INDEX)
				{
					OutLayoutInfo.Constants32BitRoot  = i;
					OutLayoutInfo.Constants32BitCount = Parameter.Num32BitValues;
				}
				break;
			case ERHIRootParameterType::RHI_ROOT_PARAMETER_TABLE:
				switch (Parameter.Range.Type)
				{
				case ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_UAV:
					if (OutLayoutInfo.UAVTableRoot == RHI_INVALID_ROOT_INDEX)
					{
						OutLayoutInfo.UAVTableRoot = i;
						OutLayoutInfo.UAVTableSize = Parameter.Range.NumDescriptors;
					}
					break;
				case ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_SAMPLER:
					if (OutLayoutInfo.SamplerTableRoot == RHI_INVALID_ROOT_INDEX)
					{
						OutLayoutInfo.SamplerTableRoot = i;
						OutLayoutInfo.SamplerTableSize = Parameter.Range.NumDescriptors;
					}
					break;
				default:
					if (OutLayoutInfo.SRVTableRoot == RHI_INVALID_ROOT_INDEX)
					{
						OutLayoutInfo.SRVTableRoot = i;
						OutLayoutInfo.SRVTableSize = Parameter.Range.NumDescriptors;
					}
					break;
				}
				break;
			default:
				break;
			}
		}
	}

	BOOL8 RDeviceD3D12::CreateRootSignature(const RRHIRootSignatureDesc& InDesc, IRRHIRootSignature** OutSignature) noexcept
	{
		if (!OutSignature || !Device)
		{
			return FALSE;
		}

		const UINT64 LayoutHash = ((InDesc.LayoutHash != 0ull) ? InDesc.LayoutHash : HashRHIRootSignatureDesc(InDesc));

		// Signatures are shared objects: the layout hash is the identity, so asking for a
		// layout that is already live returns the object that is already bound to PSOs.
		RD3D12RootSignature* Cached = nullptr;
		RootSignatureCache.FindValue(LayoutHash, Cached);
		if (Cached != nullptr)
		{
			*OutSignature = Cached;
			return TRUE;
		}

		const UINT32 ParameterCount = InDesc.Parameters.Num<UINT32>();

		TArray<D3D12_ROOT_PARAMETER> RootParams;
		RootParams.SetNum(ParameterCount);
		// One range per parameter, indexed by parameter index: only table parameters use
		// their entry, and taking a range's address after the array is fully sized keeps
		// the pointer valid for the serialize call below.
		TArray<D3D12_DESCRIPTOR_RANGE> Ranges;
		Ranges.SetNum(ParameterCount);

		// Root parameter budget. D3D12 rejects a signature that costs more than
		// D3D12_MAX_ROOT_COST DWORDs: a descriptor table costs one, a root CBV two, and a
		// 32 bit constant block one per value.
		UINT32 RootCost = 0u;

		for (UINT32 i = 0u; i < ParameterCount; i++)
		{
			const RRHIRootParameterDesc& Source = InDesc.Parameters[i];
			D3D12_ROOT_PARAMETER& Target = RootParams[i];
			TranslateRootParameterVisibility(Target.ShaderVisibility, Source.Visibility);

			switch (Source.Type)
			{
			case ERHIRootParameterType::RHI_ROOT_PARAMETER_CBV:
				Target.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
				Target.Descriptor.ShaderRegister = Source.ShaderRegister;
				Target.Descriptor.RegisterSpace  = Source.RegisterSpace;
				RootCost += 2u;
				break;
			case ERHIRootParameterType::RHI_ROOT_PARAMETER_TABLE:
			{
				D3D12_DESCRIPTOR_RANGE& Range = Ranges[i];
				switch (Source.Range.Type)
				{
				case ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_UAV:
					Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
					break;
				case ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_SAMPLER:
					Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					break;
				default:
					Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					break;
				}
				Range.NumDescriptors                    = Source.Range.NumDescriptors;
				Range.BaseShaderRegister                = Source.Range.BaseShaderRegister;
				Range.RegisterSpace                     = Source.Range.RegisterSpace;
				// Ranges are appended, so a table's descriptors follow each other and a
				// resource's offset inside its table equals its register slot.
				Range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

				Target.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				Target.DescriptorTable.NumDescriptorRanges = 1u;
				Target.DescriptorTable.pDescriptorRanges   = &Range;
				RootCost += 1u;
				break;
			}
			case ERHIRootParameterType::RHI_ROOT_PARAMETER_32BIT_CONSTANTS:
				// A root constant block is 1..D3D12_MAX_ROOT_COST DWORDs wide; an empty block
				// would only fail inside the serializer with a message that hides the cause,
				// so it is rejected here with the actual reason.
				if ((Source.Num32BitValues == 0u) || (Source.Num32BitValues > D3D12_MAX_ROOT_COST))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: root 32-bit constant block width is out of range."));
					return FALSE;
				}
				Target.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
				Target.Constants.ShaderRegister = Source.ShaderRegister;
				Target.Constants.RegisterSpace  = Source.RegisterSpace;
				Target.Constants.Num32BitValues = Source.Num32BitValues;
				RootCost += Source.Num32BitValues;
				break;
			default:
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Root signature parameter type is not translatable."));
				return FALSE;
			}
		}

		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12: root signature layout exceeds the API's root parameter budget.")),
			(RootCost <= D3D12_MAX_ROOT_COST));
		if (RootCost > D3D12_MAX_ROOT_COST)
		{
			return FALSE;
		}

		D3D12_ROOT_SIGNATURE_DESC SignatureDesc = {};
		SignatureDesc.NumParameters     = ParameterCount;
		SignatureDesc.pParameters       = ((ParameterCount > 0u) ? RootParams.GetData() : nullptr);
		SignatureDesc.NumStaticSamplers = 0u;
		SignatureDesc.pStaticSamplers   = nullptr;
		SignatureDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		if (InDesc.bAllowInputAssembler)
		{
			SignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		}
		if (InDesc.bHeapDirectlyIndexed)
		{
			SignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
			SignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
		}

		Microsoft::WRL::ComPtr<ID3DBlob> Serialized;
		Microsoft::WRL::ComPtr<ID3DBlob> Error;
		if (FAILED(D3D12SerializeRootSignature(&SignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			Serialized.GetAddressOf(), Error.GetAddressOf())))
		{
			if (Error)
			{
				OutputDebugStringA(static_cast<const char*>(Error->GetBufferPointer()));
			}
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: D3D12SerializeRootSignature failed for a generated layout."));
			return FALSE;
		}

		RD3D12RootSignature* Wrapper = new RD3D12RootSignature();
		if (FAILED(Device->CreateRootSignature(0u, Serialized->GetBufferPointer(), Serialized->GetBufferSize(),
			IID_PPV_ARGS(Wrapper->Signature.ReleaseAndGetAddressOf()))))
		{
			delete Wrapper;
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateRootSignature failed for a generated layout."));
			return FALSE;
		}

		Wrapper->LayoutHash = LayoutHash;
		BuildLayoutInfo(InDesc, Wrapper->LayoutInfo);

		RootSignaturePool.Add(Wrapper);
		RootSignatureCache.Add(LayoutHash, Wrapper);
		*OutSignature = Wrapper;
		return TRUE;
	}

	void RDeviceD3D12::DestroyRootSignature(IRRHIRootSignature* InSignature) noexcept
	{
		if (!InSignature)
		{
			return;
		}

		RD3D12RootSignature* Signature = static_cast<RD3D12RootSignature*>(InSignature);
		RootSignatureCache.Remove(Signature->LayoutHash);
		for (INT32 i = static_cast<INT32>(RootSignaturePool.Num()) - 1; i >= 0; i--)
		{
			if (RootSignaturePool[i] == Signature)
			{
				RootSignaturePool.RemoveAt(i);
				// The wrapper is the caller's to release (see the interface contract: it
				// must not be called while live PSOs still reference the layout). A PSO
				// holds its own reference to the native signature, so a destroy that
				// races a live PSO keeps the native object alive rather than dangling.
				delete Signature;
				break;
			}
		}
	}

	BOOL8 RDeviceD3D12::CreateFrameFence()
	{
		FrameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		if (!FrameFenceEvent)
		{
			return FALSE;
		}
		if (FAILED(Device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(FrameFence.ReleaseAndGetAddressOf()))))
		{
			return FALSE;
		}
		for (UINT32 i = 0u; i < FRAME_IN_FLIGHT; i++)
		{
			FrameFenceValues[i] = 0ull;
		}
		CurrentFrameIndex = 0u;
		FrameInFlightCount = FRAME_IN_FLIGHT;
		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateUploadHeap()
	{
		UploadHeapSize = 64ull * 1024ull * 1024ull; // 64 MB

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResourceDesc.Alignment          = 0u;
		ResourceDesc.Width              = UploadHeapSize;
		ResourceDesc.Height             = 1u;
		ResourceDesc.DepthOrArraySize   = 1u;
		ResourceDesc.MipLevels          = 1u;
		ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
		ResourceDesc.SampleDesc.Count   = 1u;
		ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResourceDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

		if (FAILED(Device->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(UploadHeap.ReleaseAndGetAddressOf()))))
		{
			return FALSE;
		}

		UploadHeap->Map(0u, nullptr, &UploadHeapBase);
		UploadHeapOffset = 0u;

		// Dedicated upload command list (separate from per-frame allocator)
		if (FAILED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(UploadCommandAllocator.ReleaseAndGetAddressOf()))))
		{
			return FALSE;
		}
		if (FAILED(Device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT,
			UploadCommandAllocator.Get(), nullptr,
			IID_PPV_ARGS(UploadCommandList.ReleaseAndGetAddressOf()))))
		{
			return FALSE;
		}
		// Lists are created in the open state; close so the first lazy-open
		// Reset() works.
		UploadCommandList->Close();
		bUploadCommandListOpen = FALSE;

		return TRUE;
	}

	void RDeviceD3D12::CreateBackBufferViews()
	{
		// Back buffers occupy fixed RTV heap slots [0, BackBufferCount). The
		// persistent RTV allocator skips these via the offset bump in Initialize.
		for (UINT32 i = 0u; i < BackBufferCount; i++)
		{
			RD3D12Texture* Buffer = new RD3D12Texture();
			BackBuffers.Add(Buffer);

			Buffer->Width  = BackBufferWidth;
			Buffer->Height = BackBufferHeight;
			Buffer->Depth  = 1u;
			Buffer->MipLevels = 1u;
			Buffer->ArraySize = 1u;
			Buffer->Format    = BackBufferFormat;
			Buffer->BindFlags = static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET);
			// The swap chain buffer is not created through CreateTexture, so its view formats are
			// recorded here instead: without them a pass that draws straight into the back buffer
			// would be the one target the render target / pipeline state comparison cannot check.
			// DXGI creates the buffer with the swap chain format, and the RTV below is created
			// with no desc, which means "the resource's own format".
			Buffer->ViewFormats.Resource = BackBufferFormat;
			Buffer->ViewFormats.RTV      = BackBufferFormat;
			Buffer->DefaultState = D3D12_RESOURCE_STATE_PRESENT;
			Buffer->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_PRESENT);

			D3D12_CPU_DESCRIPTOR_HANDLE RTVStart = RTVHeap->GetCPUDescriptorHandleForHeapStart();
			Buffer->RTVHandle = OffsetCPUHandle(RTVStart, static_cast<INT32>(i), static_cast<INT32>(RTVDescriptorSize));

			HRESULT hr = SwapChain->GetBuffer(i, IID_PPV_ARGS(Buffer->Resource.ReleaseAndGetAddressOf()));
			if (SUCCEEDED(hr))
			{
				Device->CreateRenderTargetView(Buffer->Resource.Get(), nullptr, Buffer->RTVHandle);
			}
		}
	}

	void RDeviceD3D12::ReleaseBackBufferViews()
	{
		for (INT32 i = 0; i < BackBuffers.Num(); i++)
		{
			delete BackBuffers[i];
		}
		BackBuffers.Empty();
	}

	void RDeviceD3D12::WaitForFrameFence(UINT32 InFrameIndex)
	{
		if (FrameFence->GetCompletedValue() < FrameFenceValues[InFrameIndex])
		{
			FrameFence->SetEventOnCompletion(FrameFenceValues[InFrameIndex], FrameFenceEvent);
			WaitForSingleObject(FrameFenceEvent, INFINITE);
		}
	}

	// =========================================================================
	// IRRHIDevice - Lifecycle
	// =========================================================================

	BOOL8 RDeviceD3D12::Initialize(const RRHIDeviceInitDesc& InDesc) noexcept
	{
		if (bInitialized)
		{
			return TRUE;
		}

		BackBufferWidth      = InDesc.BackBufferWidth;
		BackBufferHeight     = InDesc.BackBufferHeight;
		BackBufferCount      = InDesc.BackBufferCount;
		BackBufferFormat     = InDesc.BackBufferFormat;
		DepthStencilFormat   = InDesc.DepthStencilFormat;
		bIsWindowed          = InDesc.bIsWindowed;
		bEnableDebugLayer    = InDesc.bEnableDebugLayer;
		bEnableGPUValidation = InDesc.bEnableGPUValidation;

		if (BackBufferCount < 2u)
		{
			BackBufferCount = 2u;
		}
		// The back-buffer views occupy RTV heap slots [0, BackBufferCount) and DXGI caps
		// a swap chain at DXGI_MAX_SWAP_CHAIN_BUFFERS, so the count has to stay bounded.
		if (BackBufferCount > DXGI_MAX_SWAP_CHAIN_BUFFERS)
		{
			BackBufferCount = DXGI_MAX_SWAP_CHAIN_BUFFERS;
		}

		if (!CreateDeviceAndQueue())
		{
			return FALSE;
		}

		if (!CreateDescriptorHeaps())
		{
			return FALSE;
		}

		if (!CreateSwapChain(InDesc.WindowHandle))
		{
			return FALSE;
		}

		CreateBackBufferViews();

		// Reserve the first BackBufferCount RTV slots for swapchain back buffers.
		RTVDescriptorOffset = BackBufferCount;

		if (!CreateFrameFence())
		{
			return FALSE;
		}

		for (UINT32 i = 0u; i < FRAME_IN_FLIGHT; i++)
		{
			if (FAILED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(CommandAllocators[i].ReleaseAndGetAddressOf()))))
			{
				return FALSE;
			}
		}

		if (!CreateUploadHeap())
		{
			return FALSE;
		}

		bInitialized = TRUE;
		return TRUE;
	}

	void RDeviceD3D12::Shutdown() noexcept
	{
		if (!bInitialized)
		{
			return;
		}

		WaitForGPUIdle();

		// Process deferred deletions. A PSO that reached this queue was already dropped from
		// PipelineStateCache by DestroyResource, so the two containers never hold the same
		// object and the drain below cannot double-free.
		for (FDeferredDeleteEntry& Entry : DeferredDeleteQueue)
		{
			delete (Entry.Resource);
		}
		DeferredDeleteQueue.Empty();

		// Clear any PSOs that are still in the cache (never destroyed by the
		// engine - happens when a Phase 1 stub PSO is created but the owner
		// shuts down without calling DestroyResource on it).
		for (auto& Pair : PipelineStateCache)
		{
			delete Pair.second;
		}
		PipelineStateCache.Empty();

		// Clear command list pool
		CommandListFreeList.Empty();
		CommandListPool.Empty();

		// Release back buffers
		ReleaseBackBufferViews();

		// Release descriptor heaps (COM)
		RTVHeap.Reset();
		DSVHeap.Reset();
		CBVSRVUAVHeap.Reset();
		SamplerHeap.Reset();
		CBVSRVUAVStagingHeap.Reset();
		SamplerStagingHeap.Reset();

		// Release root signatures. Every PSO that was still alive had already been
		// released above and held its own reference to the native object, so the
		// signatures are unreferenced by the time the pool is cleared here.
		for (RD3D12RootSignature* Signature : RootSignaturePool)
		{
			delete Signature;
		}
		RootSignaturePool.Empty();
		RootSignatureCache.Empty();

		// Release frame resources
		for (UINT32 i = 0u; i < FRAME_IN_FLIGHT; i++)
		{
			CommandAllocators[i].Reset();
		}
		FrameFence.Reset();
		if (FrameFenceEvent)
		{
			CloseHandle(FrameFenceEvent);
			FrameFenceEvent = nullptr;
		}

		// Release upload heap
		if (UploadHeap)
		{
			UploadHeap->Unmap(0u, nullptr);
			UploadHeap.Reset();
		}
		UploadHeapBase = nullptr;
		UploadCommandList.Reset();
		UploadCommandAllocator.Reset();
		bUploadCommandListOpen = FALSE;

		SwapChain.Reset();
		CommandQueue.Reset();
		Device.Reset();

		bInitialized = FALSE;
	}

	// =========================================================================
	// IRRHIDevice - Frame fencing
	// =========================================================================

	void RDeviceD3D12::BeginFrame() noexcept
	{
		// Flush any upload commands that were queued during engine init or by
		// the previous frame but never submitted. Without this, the upload-heap
		// offset reset below would recycle memory the GPU has not yet read,
		// which silently corrupts e.g. the FullScreenTriangle's static VB/IB
		// (init-time queued CopyBufferRegion reads upload-heap[0..N], then this
		// frame's view CB upload overwrites the same bytes before GPU executes).
		// FlushPendingUploads is a no-op when bUploadCommandListOpen == FALSE,
		// so the steady-state cost is one branch.
		FlushPendingUploads();

		// Wait for GPU work (including the upload work we may have just flushed
		// above, and the previous frame's main cmdlist) that previously occupied
		// this slot to complete. FrameFenceValues[CurrentFrameIndex] is set by
		// FlushPendingUploads / SubmitCommandList to the latest fence value
		// associated with this slot, so this wait covers both.
		WaitForFrameFence(CurrentFrameIndex);

		// Upload ring resets per frame; slot tables rotate by frame index so
		// no per-frame CBV/SRV/UAV bump reset is needed.
		UploadHeapOffset = 0u;

		// Process deferred deletions whose retire frame now matches us.
		for (INT32 i = static_cast<INT32>(DeferredDeleteQueue.Num()) - 1; i >= 0; i--)
		{
			if (DeferredDeleteQueue[i].RetireFrameIndex == CurrentFrameIndex)
			{
				delete (DeferredDeleteQueue[i].Resource);
				DeferredDeleteQueue.RemoveAt(i);
			}
		}

		// Reset the command allocator now that the GPU is done with it.
		CommandAllocators[CurrentFrameIndex]->Reset();

		// Release pooled command-list ComPtrs so AcquireCommandList rebuilds
		// against the current frame's allocator.
		for (RCommandListD3D12* List : CommandListFreeList)
		{
			List->SetNativeList(nullptr, CommandAllocators[CurrentFrameIndex]);
		}
	}

	void RDeviceD3D12::EndFrame() noexcept
	{
		CurrentFrameIndex = (CurrentFrameIndex + 1u) % FRAME_IN_FLIGHT;
	}

	void RDeviceD3D12::Present(UINT32 InSyncInterval) noexcept
	{
		if (SwapChain)
		{
			SwapChain->Present(InSyncInterval, 0u);
		}
	}

	BOOL8 RDeviceD3D12::ResizeSwapChain(UINT32 InWidth, UINT32 InHeight) noexcept
	{
		if (InWidth == 0u || InHeight == 0u)
		{
			return FALSE;
		}
		if (InWidth == BackBufferWidth && InHeight == BackBufferHeight)
		{
			return TRUE;
		}

		WaitForGPUIdle();

		ReleaseBackBufferViews();

		BackBufferWidth  = InWidth;
		BackBufferHeight = InHeight;

		if (FAILED(SwapChain->ResizeBuffers(BackBufferCount, BackBufferWidth, BackBufferHeight, DXGI_FORMAT_UNKNOWN, 0u)))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: ResizeBuffers failed."));
			return FALSE;
		}

		CreateBackBufferViews();

		return TRUE;
	}

	void RDeviceD3D12::WaitForGPUIdle() noexcept
	{
		if (!CommandQueue || !FrameFence)
		{
			return;
		}
		// Issue a fresh signal and wait on it so we know everything submitted
		// up to this point has retired, regardless of per-slot bookkeeping.
		++GlobalFenceCounter;
		CommandQueue->Signal(FrameFence.Get(), GlobalFenceCounter);
		if (FrameFence->GetCompletedValue() < GlobalFenceCounter)
		{
			FrameFence->SetEventOnCompletion(GlobalFenceCounter, FrameFenceEvent);
			WaitForSingleObject(FrameFenceEvent, INFINITE);
		}
		for (UINT32 i = 0u; i < FRAME_IN_FLIGHT; i++)
		{
			FrameFenceValues[i] = GlobalFenceCounter;
		}
	}

	/*
	* A second swap chain for an ImGui platform window.
	*
	* The device's main chain (SwapChain + BackBuffers) is untouched: this one creates its own
	* fleet of back buffers, its own RTV descriptor heap and its own Present, so a viewport
	* window can never resize or present the main window by accident. The back buffers are
	* RD3D12Texture wrappers like the main ones, which is what lets the command list record
	* the ImGui pass into them through the same BeginRenderPass path.
	*/
	class RD3D12WindowSwapChain final : public IRRHISwapChain
	{
	public:
		RD3D12WindowSwapChain(ID3D12Device* InDevice, IDXGIFactory4* InFactory, ID3D12CommandQueue* InQueue
			, HWND InWindowHandle, UINT32 InWidth, UINT32 InHeight, DXGI_FORMAT InFormat, RFormatType InFormatType)
			: Device(InDevice), Width(InWidth), Height(InHeight), Format(InFormat), FormatType(InFormatType)
		{
			Create(InFactory, InQueue, InWindowHandle);
		}
		RD3D12WindowSwapChain(const RD3D12WindowSwapChain&) = delete;
		RD3D12WindowSwapChain& operator=(const RD3D12WindowSwapChain&) = delete;
		virtual ~RD3D12WindowSwapChain() override
		{
			ReleaseBuffers();
			RTVHeap.Reset();
			SwapChain.Reset();
			Device = nullptr;
		}

	public:
		virtual IRRHITexture* GetBackBuffer() override
		{
			if ((!SwapChain) || (Buffers.Num() <= 0))
			{
				return nullptr;
			}
			const UINT32 BufferIndex = SwapChain->GetCurrentBackBufferIndex();
			if (BufferIndex >= static_cast<UINT32>(Buffers.Num()))
			{
				return nullptr;
			}
			return Buffers[static_cast<INT32>(BufferIndex)];
		}
		virtual BOOL8 Resize(UINT32 InWidth, UINT32 InHeight) override
		{
			if ((!SwapChain) || (InWidth == 0u) || (InHeight == 0u))
			{
				return FALSE;
			}
			ReleaseBuffers();
			if (FAILED(SwapChain->ResizeBuffers(BufferCount, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0u)))
			{
				return FALSE;
			}
			Width = InWidth;
			Height = InHeight;
			return CreateBuffers();
		}
		virtual void Present(UINT32 InSyncInterval) override
		{
			if (SwapChain)
			{
				SwapChain->Present(InSyncInterval, 0u);
			}
		}
		BOOL8 IsValid() const
		{
			return (SwapChain != nullptr) && (Buffers.Num() > 0);
		}

	private:
		BOOL8 Create(IDXGIFactory4* InFactory, ID3D12CommandQueue* InQueue, HWND InWindowHandle)
		{
			if ((!Device) || (!InFactory) || (!InQueue) || (!InWindowHandle))
			{
				return FALSE;
			}

			DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
			SwapChainDesc.Width					= Width;
			SwapChainDesc.Height				= Height;
			SwapChainDesc.Format				= Format;
			SwapChainDesc.Stereo				= FALSE;
			SwapChainDesc.SampleDesc.Count		= 1u;
			SwapChainDesc.SampleDesc.Quality	= 0u;
			SwapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
			SwapChainDesc.BufferCount			= BufferCount;
			// STRETCH, not NONE: these windows are resized by dragging their frame, and the
			// presentation has to follow the window while the resize is in flight.
			SwapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
			SwapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
			SwapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_UNSPECIFIED;
			SwapChainDesc.Flags					= 0u;

			Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain1;
			if (FAILED(InFactory->CreateSwapChainForHwnd(InQueue, InWindowHandle, &SwapChainDesc, nullptr, nullptr, SwapChain1.GetAddressOf())))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateSwapChainForHwnd (viewport) failed."));
				return FALSE;
			}
			if (FAILED(SwapChain1.As(&SwapChain)))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: viewport swap chain is not an IDXGISwapChain3."));
				return FALSE;
			}
			InFactory->MakeWindowAssociation(InWindowHandle, DXGI_MWA_NO_ALT_ENTER);

			// A heap of its own, so the viewport's descriptors cannot collide with the main
			// window's reserved RTV slots or with anything else the device allocates.
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			HeapDesc.NumDescriptors	= BufferCount;
			HeapDesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			if (FAILED(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(RTVHeap.GetAddressOf()))))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: viewport RTV heap creation failed."));
				return FALSE;
			}
			RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			return CreateBuffers();
		}
		BOOL8 CreateBuffers()
		{
			if ((!SwapChain) || (!RTVHeap) || (!Device))
			{
				return FALSE;
			}
			const D3D12_CPU_DESCRIPTOR_HANDLE RTVStart = RTVHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT32 BufferIndex = 0u; BufferIndex < BufferCount; BufferIndex++)
			{
				RD3D12Texture* Buffer = new RD3D12Texture();
				Buffer->Width		= Width;
				Buffer->Height		= Height;
				Buffer->Depth		= 1u;
				Buffer->MipLevels	= 1u;
				Buffer->ArraySize	= 1u;
				Buffer->Format		= FormatType;
				Buffer->BindFlags	= static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET);
				Buffer->ViewFormats.Resource	= FormatType;
				Buffer->ViewFormats.RTV			= FormatType;
				Buffer->DefaultState = D3D12_RESOURCE_STATE_PRESENT;
				Buffer->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_PRESENT);
				Buffer->RTVHandle = OffsetCPUHandle(RTVStart, BufferIndex, RTVDescriptorSize);

				if (FAILED(SwapChain->GetBuffer(BufferIndex, IID_PPV_ARGS(Buffer->Resource.ReleaseAndGetAddressOf()))))
				{
					delete Buffer;
					ReleaseBuffers();
					return FALSE;
				}
				Device->CreateRenderTargetView(Buffer->Resource.Get(), nullptr, Buffer->RTVHandle);
				Buffers.Add(Buffer);
			}
			return TRUE;
		}
		void ReleaseBuffers()
		{
			for (INT32 BufferIndex = 0; BufferIndex < Buffers.Num(); BufferIndex++)
			{
				delete Buffers[BufferIndex];
			}
			Buffers.Empty();
		}

	private:
		ID3D12Device*						Device;
		Microsoft::WRL::ComPtr<IDXGISwapChain3>			SwapChain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	RTVHeap;
		UINT32								RTVDescriptorSize = 0u;
		TArray<RD3D12Texture*>				Buffers;
		UINT32								BufferCount = 2u;
		UINT32								Width;
		UINT32								Height;
		DXGI_FORMAT							Format;
		RFormatType							FormatType;
	};

	IRRHISwapChain* RDeviceD3D12::CreateSwapChainForWindow(HWND InWindowHandle, UINT32 InWidth, UINT32 InHeight, RFormatType InFormat) noexcept
	{
		if ((!Device) || (!CommandQueue) || (!InWindowHandle) || (InWidth == 0u) || (InHeight == 0u))
		{
			return nullptr;
		}

		Microsoft::WRL::ComPtr<IDXGIFactory4> Factory;
		if (FAILED(CreateDXGIFactory2(0u, IID_PPV_ARGS(Factory.GetAddressOf()))) || (!Factory))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateDXGIFactory2 (viewport) failed."));
			return nullptr;
		}

		DXGI_FORMAT DXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		TranslateResourceFormat(DXGIFormat, InFormat);
		if (DXGIFormat == DXGI_FORMAT_UNKNOWN)
		{
			DXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		RD3D12WindowSwapChain* Chain = new RD3D12WindowSwapChain(Device.Get(), Factory.Get(), CommandQueue.Get()
			, InWindowHandle, InWidth, InHeight, DXGIFormat, InFormat);
		if (Chain->IsValid() == FALSE)
		{
			delete Chain;
			return nullptr;
		}
		return Chain;
	}

	// =========================================================================
	// IRRHIDevice - Resource creation
	// =========================================================================
	BOOL8 RDeviceD3D12::CreateBuffer(const RBufferDesc& InDesc, const RRHISubresourceData& InInitialData, IRRHIBuffer** OutBuffer) noexcept
	{
		if (!OutBuffer)
		{
			return FALSE;
		}

		RD3D12Buffer* Buffer = new RD3D12Buffer{};
		Buffer->ByteSize            = InDesc.ByteWidth;
		Buffer->StructureByteStride = InDesc.StructureByteStride;
		Buffer->BindFlags           = InDesc.BindFlags;

		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		// For structured buffers the SRV/UAV descriptor uses UNKNOWN + structure stride.
		// For raw / typed buffers a specific format would be set here.

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResourceDesc.Alignment        = 0u;
		ResourceDesc.Width            = InDesc.ByteWidth;
		ResourceDesc.Height           = 1u;
		ResourceDesc.DepthOrArraySize = 1u;
		ResourceDesc.MipLevels        = 1u;
		ResourceDesc.Format           = DXGI_FORMAT_UNKNOWN;
		ResourceDesc.SampleDesc.Count = 1u;
		ResourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResourceDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)))
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		// D3D12 mandates that buffers be created in COMMON state regardless of
		// intended usage; the runtime promotes them implicitly on first use.
		// Passing a "real" state here triggers warning #1328
		// (CREATERESOURCE_STATE_IGNORED). We track the engine-side state via
		// SetCurrentState below so subsequent ResourceBarrier transitions still
		// see the right "before" state.
		const D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		if (FAILED(Device->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			InitialState,
			nullptr,
			IID_PPV_ARGS(Buffer->Resource.ReleaseAndGetAddressOf()))))
		{
			delete (Buffer);
			return FALSE;
		}

		Buffer->GPUVirtualAddress = Buffer->Resource->GetGPUVirtualAddress();

		// Buffers always start in COMMON (D3D12 rule). The runtime implicitly
		// promotes to VERTEX_AND_CONSTANT_BUFFER / INDEX_BUFFER / etc. on first
		// use, so explicit barriers from COMMON are valid when actual state
		// tracking is needed (UploadBuffer transitions COMMON -> COPY_DEST).
		Buffer->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_COMMON);

		// Create SRV if needed
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)))
		{
			UINT32 HeapIndex = AllocatePersistentSRVUAV();
			// Phase 4.1: write descriptor to staging heap first (CPU-readable),
			// then mirror to shader-visible heap for GPU bindless access.
			D3D12_CPU_DESCRIPTOR_HANDLE StagingStart = CBVSRVUAVStagingHeap->GetCPUDescriptorHandleForHeapStart();
			Buffer->SRVHandle = OffsetCPUHandle(StagingStart, static_cast<INT32>(HeapIndex), static_cast<INT32>(CBVSRVUAVDescriptorSize));

			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
			SRVDesc.Format                  = Format;
			SRVDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
			SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			SRVDesc.Buffer.FirstElement     = 0u;
			SRVDesc.Buffer.NumElements      = static_cast<UINT>((InDesc.StructureByteStride > 0u) ? (InDesc.ByteWidth / InDesc.StructureByteStride) : (InDesc.ByteWidth / 4u));
			SRVDesc.Buffer.StructureByteStride = InDesc.StructureByteStride;
			SRVDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_NONE;
			Device->CreateShaderResourceView(Buffer->Resource.Get(), &SRVDesc, Buffer->SRVHandle);

			D3D12_CPU_DESCRIPTOR_HANDLE ShaderVisibleHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, ShaderVisibleHandle, Buffer->SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// Create UAV if needed
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)))
		{
			UINT32 HeapIndex = AllocatePersistentSRVUAV();
			// Same staging -> shader-visible mirror pattern as SRV above.
			D3D12_CPU_DESCRIPTOR_HANDLE StagingStart = CBVSRVUAVStagingHeap->GetCPUDescriptorHandleForHeapStart();
			Buffer->UAVHandle = OffsetCPUHandle(StagingStart, static_cast<INT32>(HeapIndex), static_cast<INT32>(CBVSRVUAVDescriptorSize));

			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
			UAVDesc.Format              = Format;
			UAVDesc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
			UAVDesc.Buffer.FirstElement = 0u;
			UAVDesc.Buffer.NumElements  = static_cast<UINT>((InDesc.StructureByteStride > 0u) ? (InDesc.ByteWidth / InDesc.StructureByteStride) : (InDesc.ByteWidth / 4u));
			UAVDesc.Buffer.StructureByteStride = InDesc.StructureByteStride;
			Device->CreateUnorderedAccessView(Buffer->Resource.Get(), nullptr, &UAVDesc, Buffer->UAVHandle);

			D3D12_CPU_DESCRIPTOR_HANDLE ShaderVisibleHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, ShaderVisibleHandle, Buffer->UAVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// Upload initial data if provided
		if ((!!(InInitialData.Data)) && (InInitialData.DataSize >= InDesc.ByteWidth))
		{
			UploadBuffer(Buffer, InInitialData.Data, InDesc.ByteWidth, 0u);
		}

		*OutBuffer = Buffer;
		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateTexture(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, IRRHITexture** OutTexture) noexcept
	{
		if (!OutTexture)
		{
			return FALSE;
		}

		RD3D12Texture* Texture = new RD3D12Texture();
		Texture->Width      = InDesc.Width;
		Texture->Height     = InDesc.Height;
		Texture->Depth      = InDesc.Depth;
		Texture->MipLevels  = InDesc.MipLevels;
		Texture->ArraySize  = InDesc.ArraySize;
		// GetFormat answers the content format the desc asked for, an sRGB one included. The
		// formats the API objects actually carry are the resolved set below, and they are what
		// GetViewFormat hands to the render target comparison.
		Texture->Format     = InDesc.BufferFormat;
		Texture->BindFlags  = InDesc.BindFlags;

		// One resolution for both back-ends: which format the resource is created with, and
		// which format each of its views gets. The rules that would otherwise be repeated here
		// live in it - a resource is never created with an sRGB format, a view that differs
		// from the storage format needs a typeless resource, and a depth bit count turns into
		// the typed DSV / SRV formats that go with it. See RFormatTraits.
		RTextureFormatSet FormatSet;
		if (RFormatTraits::ResolveTextureFormats(InDesc, FormatSet) == FALSE)
		{
			delete Texture;
			return FALSE;
		}
		Texture->ViewFormats = FormatSet;

		DXGI_FORMAT ResourceFormat = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT SRVFormat      = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT RTVFormat      = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT DSVFormat      = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT UAVFormat      = DXGI_FORMAT_UNKNOWN;
		// Every one of these came out of the resolver, so each is a format the engine knows;
		// a failure here means the format tables disagree with each other, which the
		// translation reports by name.
		if ((RDXGIFormatTranslate::TranslateToDXGI(ResourceFormat, FormatSet.Resource) == FALSE) ||
			((FormatSet.SRV != RFormatType::FORMAT_UNKNOWN) && (RDXGIFormatTranslate::TranslateToDXGI(SRVFormat, FormatSet.SRV) == FALSE)) ||
			((FormatSet.RTV != RFormatType::FORMAT_UNKNOWN) && (RDXGIFormatTranslate::TranslateToDXGI(RTVFormat, FormatSet.RTV) == FALSE)) ||
			((FormatSet.DSV != RFormatType::FORMAT_UNKNOWN) && (RDXGIFormatTranslate::TranslateToDXGI(DSVFormat, FormatSet.DSV) == FALSE)) ||
			((FormatSet.UAV != RFormatType::FORMAT_UNKNOWN) && (RDXGIFormatTranslate::TranslateToDXGI(UAVFormat, FormatSet.UAV) == FALSE)))
		{
			delete Texture;
			return FALSE;
		}

		// RTextureDesc.Depth is overloaded in this codebase: for 3D textures it is the volume
		// depth, but for 2D depth textures with BIND_DEPTH_STENCIL it carries the depth bit
		// count (16/24/32). The resolver already recognised the second reading to derive the
		// depth views; here it only decides whether Depth describes a third dimension.
		const BOOL8 bDepthFieldIsBitCount = RFormatTraits::IsDepthBitCountDesc(InDesc);
		const BOOL8 bIsDepthStencil = (FormatSet.DSV != RFormatType::FORMAT_UNKNOWN) ? TRUE : FALSE;

		const UINT32 EffectiveDepth = (bDepthFieldIsBitCount != FALSE) ? 1u : InDesc.Depth;

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Dimension        = (EffectiveDepth > 1u) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Alignment        = 0u;
		ResourceDesc.Width            = InDesc.Width;
		ResourceDesc.Height           = InDesc.Height;
		ResourceDesc.DepthOrArraySize = static_cast<UINT16>(EffectiveDepth > 1u ? EffectiveDepth : InDesc.ArraySize);
		ResourceDesc.MipLevels        = static_cast<UINT16>(InDesc.MipLevels);
		ResourceDesc.Format           = ResourceFormat;
		ResourceDesc.SampleDesc.Count = InDesc.SampleDesc.Count;
		ResourceDesc.SampleDesc.Quality = InDesc.SampleDesc.Quality;
		ResourceDesc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)))
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL)))
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)))
		{
			ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)))
		{
			InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		else if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL)))
		{
			InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
		Texture->DefaultState = InitialState;
		// Track GPU initial state in the engine-level resource state field.
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)))
		{
			Texture->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
		}
		else if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL)))
		{
			Texture->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
		}
		else
		{
			Texture->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_COMMON);
		}

		// The optimised clear value a resource is created with is what a later clear is measured
		// against: D3D12 compares the two and reports EXECUTION WARNING #820 when they differ,
		// then clears correctly but by the slow path. So the value registered here has to be the
		// one the pass that clears the resource actually uses, and a colour target and a depth
		// target need different halves of the struct - see TranslateRenderTargetClearValue.
		//
		// The renderer clears the seven scene colour targets to (0,0,0,0) and the depth buffer to
		// RENDER_DEPTH_FAR, which is what the two branches below register. A target cleared to
		// anything else would have to state it here too.
		//
		// The format is the VIEW's, never the content format: a depth resource is created
		// typeless, and D3D12 refuses a typeless clear value; a colour target clears through the
		// format its RTV was created with. Depth is tested first, and that order is the safe one
		// rather than an arbitrary pick: a resource carrying both flags is not a resource D3D12
		// has a meaning for, so the order only decides which reading of it wins.
		D3D12_CLEAR_VALUE ClearValue = {};
		BOOL8 bHasClearValue = FALSE;
		if (bIsDepthStencil != FALSE)
		{
			TranslateDepthStencilClearValue(ClearValue, FormatSet.DSV, RCommonSettings::RENDER_DEPTH_FAR, 0x0u);
			bHasClearValue = TRUE;
		}
		else if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)))
		{
			// (0,0,0,0) is the clear the renderer uses, and the one value an sRGB target and a
			// linear one read identically, so an sRGB RTV needs no special clear value.
			TranslateRenderTargetClearValue(ClearValue, FormatSet.RTV, InClearColor);
			bHasClearValue = TRUE;
		}

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		if (FAILED(Device->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			InitialState,
			bHasClearValue ? &ClearValue : nullptr,
			IID_PPV_ARGS(Texture->Resource.ReleaseAndGetAddressOf()))))
		{
			delete Texture;
			return FALSE;
		}

		// Create SRV
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)))
		{
			UINT32 HeapIndex = AllocatePersistentSRVUAV();
			// Phase 4.1: write the descriptor to the CPU-only staging heap first
			// so any later CopyDescriptorsSimple (e.g. RegisterBindlessSRV,
			// slot-based binding fallback) can source from it. Then mirror to
			// the same slot index in the shader-visible heap so HLSL sees the
			// descriptor at ResourceDescriptorHeap[HeapIndex].
			D3D12_CPU_DESCRIPTOR_HANDLE StagingStart = CBVSRVUAVStagingHeap->GetCPUDescriptorHandleForHeapStart();
			Texture->SRVHandle = OffsetCPUHandle(StagingStart, static_cast<INT32>(HeapIndex), static_cast<INT32>(CBVSRVUAVDescriptorSize));

			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
			// The SRV's own format: for an sRGB texture this is the sRGB member while the
			// resource is the typeless one, and that is what makes the sampler decode.
			SRVDesc.Format                  = SRVFormat;
			SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			const BOOL8 bIsCube = (static_cast<UINT8>(InDesc.MiscFlags) & static_cast<UINT8>(RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE)) != 0u;
			if (bIsCube)
			{
				SRVDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURECUBE;
				SRVDesc.TextureCube.MostDetailedMip = 0u;
				SRVDesc.TextureCube.MipLevels       = InDesc.MipLevels;
			}
			else if (EffectiveDepth > 1u)
			{
				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				SRVDesc.Texture3D.MostDetailedMip = 0u;
				SRVDesc.Texture3D.MipLevels = InDesc.MipLevels;
			}
			else
			{
				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				SRVDesc.Texture2D.MostDetailedMip = 0u;
				SRVDesc.Texture2D.MipLevels = InDesc.MipLevels;
			}
			// D3D12's view creation returns void - nothing here reports a failure - so the desc has
			// to be right rather than checked: the format and the view dimension came out of
			// RFormatTraits, and a desc D3D12 still refuses is reported by the debug layer.
			// (D3D11's CreateXxxView do return HRESULTs, which is why that back-end checks them.)
			Device->CreateShaderResourceView(Texture->Resource.Get(), &SRVDesc, Texture->SRVHandle);

			// Mirror into shader-visible heap so the same slot index is valid for
			// bindless access.
			D3D12_CPU_DESCRIPTOR_HANDLE ShaderVisibleHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, ShaderVisibleHandle, Texture->SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// Create RTV - must come from the RTV heap (CPU-only, separate from CBV_SRV_UAV)
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)))
		{
			UINT32 HeapIndex = AllocatePersistentRTV();
			D3D12_CPU_DESCRIPTOR_HANDLE HeapStart = RTVHeap->GetCPUDescriptorHandleForHeapStart();
			Texture->RTVHandle = OffsetCPUHandle(HeapStart, static_cast<INT32>(HeapIndex), static_cast<INT32>(RTVDescriptorSize));

			D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
			// The RTV's own format. An sRGB target is legal and is how the hardware encodes
			// on write, while the resource underneath stays typeless so the same bytes can
			// still be read as data through another view.
			RTVDesc.Format = RTVFormat;
			RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			// Void like the other view creations; see the SRV above.
			Device->CreateRenderTargetView(Texture->Resource.Get(), &RTVDesc, Texture->RTVHandle);
		}

		// Create DSV - must come from the DSV heap
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL)))
		{
			UINT32 HeapIndex = AllocatePersistentDSV();
			D3D12_CPU_DESCRIPTOR_HANDLE HeapStart = DSVHeap->GetCPUDescriptorHandleForHeapStart();
			Texture->DSVHandle = OffsetCPUHandle(HeapStart, static_cast<INT32>(HeapIndex), static_cast<INT32>(DSVDescriptorSize));

			D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
			// The typed depth format of the resource's family: a depth view cannot be created
			// with the typeless format the resource was created with, and the family table is
			// what turns one into the other.
			DSVDesc.Format = DSVFormat;
			DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			// Void like the other view creations; see the SRV above.
			Device->CreateDepthStencilView(Texture->Resource.Get(), &DSVDesc, Texture->DSVHandle);
		}

		// Create UAV
		if (!!(InDesc.BindFlags & static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)))
		{
			UINT32 HeapIndex = AllocatePersistentSRVUAV();
			// Same staging+mirror pattern as the SRV path above.
			D3D12_CPU_DESCRIPTOR_HANDLE StagingStart = CBVSRVUAVStagingHeap->GetCPUDescriptorHandleForHeapStart();
			Texture->UAVHandle = OffsetCPUHandle(StagingStart, static_cast<INT32>(HeapIndex), static_cast<INT32>(CBVSRVUAVDescriptorSize));

			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
			// Never an sRGB format - that view does not exist - which the resolver has already
			// settled, reporting it when the desc asked for one.
			UAVDesc.Format = UAVFormat;
			UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			// Void like the other view creations; see the SRV above.
			Device->CreateUnorderedAccessView(Texture->Resource.Get(), nullptr, &UAVDesc, Texture->UAVHandle);

			D3D12_CPU_DESCRIPTOR_HANDLE ShaderVisibleHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, ShaderVisibleHandle, Texture->UAVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// Creation carries the payload: the RHI hands over one subresource and the
		// back-end fills it through the same upload path UploadBuffer uses.
		if (InInitialSubresources.Data)
		{
			UploadTexture(Texture, &InInitialSubresources, 1u, 0u);
		}

		*OutTexture = Texture;
		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateShader(const RRHIShaderDesc& InDesc, IRRHIShader** OutShader) noexcept
	{
		if (!OutShader || !InDesc.ByteCode || InDesc.ByteCodeSize == 0u)
		{
			return FALSE;
		}

		RD3D12Shader* Shader = new RD3D12Shader{};
		Shader->ByteCode.SetNum(InDesc.ByteCodeSize);
		EMemory::Memcpy(Shader->ByteCode.GetData(), EMemory::GetPtr(InDesc.ByteCode), InDesc.ByteCodeSize);
		Shader->Stage = InDesc.Stage;
		*(OutShader) = Shader;
		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateGraphicsPipelineState(const RRHIGraphicsPipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept
	{
		if (!OutPipelineState)
		{
			return FALSE;
		}

		// The signature decides the parameter layout the PSO binds against, so a PSO without one
		// has no defined roots: every caller takes its signature from the pipeline state cache,
		// which derives it from the program's reflection. A desc that names none is refused here
		// rather than silently built against a signature its shaders were not compiled for.
		RD3D12RootSignature* RootSignature = static_cast<RD3D12RootSignature*>(InDesc.RootSignature);
		if (RootSignature == nullptr)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12 PSO: the desc names no root signature; build it through RPipelineStateCache so it carries its program's layout."));
			return FALSE;
		}

		// Check PSO cache
		UINT64 Hash = HashPipelineStateDesc(InDesc, RootSignature->LayoutHash);
		RD3D12PipelineState* Cached = nullptr;
		PipelineStateCache.FindValue(Hash, Cached);
		if (Cached)
		{
			*OutPipelineState = Cached;
			return TRUE;
		}

		// Graphics PSO requires at least VS+PS. Without bytecode there is no way
		// to construct a valid D3D12_GRAPHICS_PIPELINE_STATE_DESC, so we stop the
		// caller here (env passes that legitimately have no shaders still hit the
		// stub path below, returning an empty PSO that CommandListD3D12 skips).
		// Engine code always reaches a shader through its RShaderStageResource, so the
		// interface handed in here is the one this back-end produced. The container magic
		// check below stays as a cheap guard: it costs two loads and turns a mis-wired
		// binding into an empty PSO instead of a crash.
		const IRD3D12Shader* VS = static_cast<const IRD3D12Shader*>(InDesc.VertexShader);
		const IRD3D12Shader* PS = static_cast<const IRD3D12Shader*>(InDesc.PixelShader);
		// Validate the DXBC/DXIL container magic before trusting the pointer, and fall
		// back to a stub PSO if the byte layout is implausible.
		auto LooksLikeDxContainer = [](const void* InBytes, SIZE_T InSize) -> BOOL8
		{
			if (!InBytes || InSize < 32u) return FALSE;
			const BYTE* B = static_cast<const BYTE*>(InBytes);
			return (B[0] == 'D' && B[1] == 'X' && B[2] == 'B' && B[3] == 'C') ? TRUE : FALSE;
		};
		const BOOL8 bVSValid = VS && VS->GetByteCode() && VS->GetByteCodeSize() > 0u
			&& LooksLikeDxContainer(VS->GetByteCode(), VS->GetByteCodeSize());
		const BOOL8 bPSValid = PS && PS->GetByteCode() && PS->GetByteCodeSize() > 0u
			&& LooksLikeDxContainer(PS->GetByteCode(), PS->GetByteCodeSize());
		const BOOL8 bHasShaders = bVSValid && bPSValid;
	#if _EDITOR_ONLY
		if ((VS || PS) && !bHasShaders)
		{
			auto FormatFirstBytes = [](const IRD3D12Shader* InShader) -> EString
			{
				if (!InShader) { return EString(PE_TEXT("<null shader>")); }
				const void* Bytes = InShader->GetByteCode();
				const SIZE_T Size = InShader->GetByteCodeSize();
				if (!Bytes) { return EString(PE_TEXT("<null bytes>")); }
				const BYTE* B = static_cast<const BYTE*>(Bytes);
				static const CHAR kHexDigits[] = "0123456789ABCDEF";
				EString Hex;
				const SIZE_T DumpCount = (Size < 16u) ? Size : 16u;
				for (SIZE_T i = 0u; i < DumpCount; i++)
				{
					CHAR Two[3] = { kHexDigits[B[i] >> 4], kHexDigits[B[i] & 0x0F], 0 };
					Hex += EString(Two);
					Hex += EString(PE_TEXT(" "));
				}
				Hex += EString(PE_TEXT("size=")) + EString::FromInt(static_cast<INT32>(Size));
				return Hex;
			};
			PE_LOG_WARN(EString(PE_TEXT("[D3D12 PSO] Rejected shader bytecode without DXBC/DXIL container magic. VS first16=")) + FormatFirstBytes(VS));
			PE_LOG_WARN(EString(PE_TEXT("[D3D12 PSO] Rejected shader bytecode without DXBC/DXIL container magic. PS first16=")) + FormatFirstBytes(PS));
		}
	#endif

		RD3D12PipelineState* PSO = new RD3D12PipelineState();
		PSO->bIsCompute = FALSE;
		PSO->PipelineStateHash = Hash;
		// The PSO references the signature (keeping the native object alive) and copies
		// the layout, so binding never depends on the device's cache still being intact.
		PSO->RootSignature    = RootSignature->Signature;
		PSO->RHIRootSignature = RootSignature;
		PSO->LayoutInfo       = RootSignature->LayoutInfo;
		TranslatePrimitiveTopology(PSO->PrimitiveTopology, InDesc.PrimitiveTopology);

		if (!bHasShaders)
		{
			// Env / placeholder PSO with no shaders - kept as a null-native stub
			// so CommandListD3D12::SetPipelineState silently no-ops downstream
			// draws (HasBoundPipeline stays FALSE). Phase 4.4 multi-material
			// renderer will require non-null shaders for every drawn primitive.
			PipelineStateCache.Add(Hash, PSO);
			*OutPipelineState = PSO;
			return TRUE;
		}

		// DXBC blobs may be padded by the asset pipeline; D3D12 validation
		// rejects trailing bytes whereas D3D11 tolerates them. DXIL is
		// self-describing too and benefits from the same trim. Read the
		// container header's reported size and clamp.
		auto TrimBytecodeSize = [](const void* InBytes, SIZE_T InSize) -> SIZE_T
		{
			if (!InBytes || InSize < 32u) { return InSize; }
			const BYTE* B = static_cast<const BYTE*>(InBytes);
			// Both DXBC (SM5) and DXIL container ("DXBC" + DXIL part) start
			// with the same 4-byte magic; container size is at offset 24.
			if (B[0] == 'D' && B[1] == 'X' && B[2] == 'B' && B[3] == 'C')
			{
				UINT32 ContainerSize = 0u;
				memcpy(&ContainerSize, B + 24, sizeof(UINT32));
				if (ContainerSize > 0u && ContainerSize <= InSize)
				{
					return static_cast<SIZE_T>(ContainerSize);
				}
			}
			return InSize;
		};

		const SIZE_T VSContainerSize = TrimBytecodeSize(VS->GetByteCode(), VS->GetByteCodeSize());
		const SIZE_T PSContainerSize = TrimBytecodeSize(PS->GetByteCode(), PS->GetByteCodeSize());
	#if _EDITOR_ONLY
		{
			auto FormatHeader = [](const void* InBytes, SIZE_T InSize, SIZE_T InTrimmed) -> EString
			{
				const BYTE* B = static_cast<const BYTE*>(InBytes);
				EString Out;
				Out += EString("magic=");
				if (InSize >= 4u)
				{
					CHAR Magic[5] = { (CHAR)B[0], (CHAR)B[1], (CHAR)B[2], (CHAR)B[3], 0 };
					Out += EString(Magic);
				}
				Out += EString(" raw=") + ToString(static_cast<UINT32>(InSize));
				Out += EString(" trimmed=") + ToString(static_cast<UINT32>(InTrimmed));
				if (InSize >= 28u)
				{
					UINT32 ReportedContainerSize = 0u;
					memcpy(&ReportedContainerSize, B + 24, sizeof(UINT32));
					Out += EString(" container=") + ToString(ReportedContainerSize);
				}
				return Out;
			};
			PE_LOG_LOG(EString("[D3D12 PSO] VS ") + FormatHeader(VS->GetByteCode(), VS->GetByteCodeSize(), VSContainerSize));
			PE_LOG_LOG(EString("[D3D12 PSO] PS ") + FormatHeader(PS->GetByteCode(), PS->GetByteCodeSize(), PSContainerSize));
		}
	#endif

		D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};
		Desc.pRootSignature = RootSignature->GetD3D12RootSignature();

		Desc.VS.pShaderBytecode = VS->GetByteCode();
		Desc.VS.BytecodeLength  = VSContainerSize;
		Desc.PS.pShaderBytecode = PS->GetByteCode();
		Desc.PS.BytecodeLength  = PSContainerSize;

		// Input layout - translate each RInputLayoutDesc to D3D12_INPUT_ELEMENT_DESC.
		// SemanticName pointers must remain live until CreatePipelineState returns;
		// the existing TranslateInputElementDesc fills them from a static table.
		TArray<D3D12_INPUT_ELEMENT_DESC> InputElems;
		InputElems.SetNum(InDesc.InputLayoutCount);
		for (UINT32 i = 0u; i < InDesc.InputLayoutCount; i++)
		{
			TranslateInputElementDesc(InputElems[i], InDesc.InputLayouts[i]);
			// A descriptor whose format / component combination the API cannot express (8 bit float,
			// 3 channel 8 or 16 bit, 32 bit normalized) reaches the driver as an unknown format and
			// comes back as an opaque pipeline creation failure. Refusing it here names the element.
			PE_CHECK((ENGINE_RENDER_CORE_ERROR),
				(PE_TEXT("D3D12 PSO: input element has no API format for its format / width / component count.")),
				(InputElems[i].Format != DXGI_FORMAT_UNKNOWN));
			if (InputElems[i].Format == DXGI_FORMAT_UNKNOWN)
			{
				return FALSE;
			}
		}
		Desc.InputLayout.pInputElementDescs = InputElems.GetData();
		Desc.InputLayout.NumElements        = InDesc.InputLayoutCount;

		// Rasterizer / blend / depth-stencil
		{
			D3D12_RASTERIZER_DESC& Rast = Desc.RasterizerState;
			// No d3dx12.h in this SDK; reconstruct D3D12_DEFAULT rasterizer
			// defaults explicitly, then override fill/cull from InDesc. Other
			// fields aren't exposed on RRasterizerState (project keeps the RHI
			// rasterizer minimal), so they stay at D3D12 defaults here.
			Rast = {};
			Rast.FrontCounterClockwise = FALSE;
			Rast.DepthBias             = 0;
			Rast.DepthBiasClamp        = 0.0f;
			Rast.SlopeScaledDepthBias  = 0.0f;
			Rast.DepthClipEnable       = TRUE;
			Rast.MultisampleEnable     = FALSE;
			Rast.AntialiasedLineEnable = FALSE;
			Rast.ForcedSampleCount     = 0u;
			Rast.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			TranslateFillMode(Rast.FillMode, InDesc.Rasterizer.FillMode);
			TranslateCullMode(Rast.CullMode, InDesc.Rasterizer.CullMode);
		}

		TranslateRenderTargetBlendDesc(Desc.BlendState, InDesc.Blend);
		TranslateDepthStencilDesc(Desc.DepthStencilState, InDesc.Depth, InDesc.Stencil);

		Desc.SampleMask        = UINT_MAX;
		Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // matches PSO->PrimitiveTopology
		switch (InDesc.PrimitiveTopology)
		{
		case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST:
		case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINESTRIP:
			Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;
		case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_POINTLIST:
			Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;
		default:
			break;
		}

		// The RHI desc carries RHI_MAX_RENDER_TARGETS (16) format slots, the native desc
		// only D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT (8): clamp so the loop cannot write
		// past D3D12_GRAPHICS_PIPELINE_STATE_DESC::RTVFormats.
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12 PSO: RenderTargetCount exceeds the simultaneous render-target limit.")),
			(InDesc.RenderTargetCount <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT));
		const UINT32 NumRenderTargets = (InDesc.RenderTargetCount > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
			? D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT : InDesc.RenderTargetCount;
		Desc.NumRenderTargets = NumRenderTargets;
		// The declared formats are kept on the PSO as well as translated into it: the command
		// list compares them against the formats of the targets a pass actually bound, which is
		// the check that turns a device-removed draw into a named engine error.
		PSO->RenderTargetFormatNum = NumRenderTargets;
		for (UINT32 i = 0u; i < NumRenderTargets; i++)
		{
			PSO->RenderTargetFormats[i] = InDesc.RenderTargetFormats[i];
			if (RDXGIFormatTranslate::TranslateToDXGI(Desc.RTVFormats[i], InDesc.RenderTargetFormats[i]) == FALSE)
			{
				delete PSO;
				return FALSE;
			}
		}
		// Map (DepthBits, HasStencil) -> typed DSV format, through the same table the resource
		// side uses (CreateTexture): the pipeline state and the depth resource have to agree on
		// the depth format, so deriving both from one source is what keeps them agreeing.
		// DepthBits=0 means no DSV bound - left DXGI_FORMAT_UNKNOWN so the PSO has no DSV.
		//
		// The bit count is checked first so an unsupported value is named as such instead of
		// being rejected deeper down as "no depth format for this bit count".
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12 PSO: unsupported DepthBits, must be 0 / 16 / 24 / 32.")),
			(InDesc.DepthBits == 0u  || InDesc.DepthBits == 16u ||
			InDesc.DepthBits == 24u || InDesc.DepthBits == 32u));
		Desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		PSO->DepthStencilFormat = RFormatType::FORMAT_UNKNOWN;
		if (InDesc.DepthBits != 0u)
		{
			RFormatType DepthResourceFormat = RFormatType::FORMAT_UNKNOWN;
			RFormatType DepthDSVFormat = RFormatType::FORMAT_UNKNOWN;
			RFormatType DepthSRVFormat = RFormatType::FORMAT_UNKNOWN;
			if (RFormatTraits::ResolveDepthStencilFormats(InDesc.DepthBits, InDesc.HasStencil,
				DepthResourceFormat, DepthDSVFormat, DepthSRVFormat) == FALSE)
			{
				delete PSO;
				return FALSE;
			}
			PSO->DepthStencilFormat = DepthDSVFormat;
			if (RDXGIFormatTranslate::TranslateToDXGI(Desc.DSVFormat, DepthDSVFormat) == FALSE)
			{
				delete PSO;
				return FALSE;
			}
		}

		Desc.SampleDesc.Count   = InDesc.SampleCount > 0u ? InDesc.SampleCount : 1u;
		Desc.SampleDesc.Quality = InDesc.SampleQuality;

		if (FAILED(Device->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(PSO->PipelineState.ReleaseAndGetAddressOf()))))
		{
			delete PSO;
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateGraphicsPipelineState failed."));
			return FALSE;
		}

		PipelineStateCache.Add(Hash, PSO);
		*OutPipelineState = PSO;
		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateComputePipelineState(const RRHIComputePipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept
	{
		if (!OutPipelineState || !InDesc.ComputeShader)
		{
			return FALSE;
		}

		const IRD3D12Shader* CS = static_cast<const IRD3D12Shader*>(InDesc.ComputeShader);

		// Same signature rule as the graphics factory: the desc has to name the signature its
		// program's reflection generated, which is what RPipelineStateCache hands it.
		RD3D12RootSignature* RootSignature = static_cast<RD3D12RootSignature*>(InDesc.RootSignature);
		if (RootSignature == nullptr)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12 PSO: the desc names no root signature; build it through RPipelineStateCache so it carries its program's layout."));
			return FALSE;
		}

		RD3D12PipelineState* PSO = new RD3D12PipelineState();
		// Compute PSOs are handed straight to the caller: no cache key (0), so
		// Shutdown's cache eviction must not treat it as an entry key.
		PSO->bIsCompute = TRUE;
		PSO->PipelineStateHash = 0ull;
		PSO->RootSignature    = RootSignature->Signature;
		PSO->RHIRootSignature = RootSignature;
		PSO->LayoutInfo       = RootSignature->LayoutInfo;

		D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {};
		Desc.pRootSignature = RootSignature->GetD3D12RootSignature();
		Desc.CS.pShaderBytecode = CS->GetByteCode();
		// Same DXBC-self-reported-size trim as the graphics PSO factory (see
		// the comment there). Asset files may have trailing padding; D3D11
		// tolerates it, D3D12 doesn't.
		{
			const BYTE* Bytes = static_cast<const BYTE*>(CS->GetByteCode());
			SIZE_T InSize = CS->GetByteCodeSize();
			SIZE_T Resolved = InSize;
			if (Bytes && InSize >= 32u && Bytes[0] == 'D' && Bytes[1] == 'X' && Bytes[2] == 'B' && Bytes[3] == 'C')
			{
				UINT32 DXBCSize = 0u;
				memcpy(&DXBCSize, Bytes + 24, sizeof(UINT32));
				if (DXBCSize > 0u && DXBCSize <= InSize)
				{
					Resolved = static_cast<SIZE_T>(DXBCSize);
				}
			}
			Desc.CS.BytecodeLength = Resolved;
		}

		if (FAILED(Device->CreateComputePipelineState(&Desc, IID_PPV_ARGS(PSO->PipelineState.ReleaseAndGetAddressOf()))))
		{
			delete PSO;
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: CreateComputePipelineState failed."));
			return FALSE;
		}

		*OutPipelineState = PSO;
		return TRUE;
	}

	BOOL8 RDeviceD3D12::CreateSampler(const RSamplerState& InDesc, IRRHISampler** OutSampler) noexcept
	{
		if (!OutSampler)
		{
			return FALSE;
		}

		RD3D12Sampler* Sampler = new RD3D12Sampler();

		UINT32 HeapIndex = AllocatePersistentSampler();
		// Phase 4.1: staging + mirror pattern. SamplerHandle points at the
		// CPU-only staging heap so CopyDescriptorsSimple can read from it; the
		// shader-visible heap is mirrored at the same slot for HLSL bindless
		// access through SamplerDescriptorHeap[HeapIndex].
		D3D12_CPU_DESCRIPTOR_HANDLE StagingStart = SamplerStagingHeap->GetCPUDescriptorHandleForHeapStart();
		Sampler->SamplerHandle = OffsetCPUHandle(StagingStart, static_cast<INT32>(HeapIndex), static_cast<INT32>(SamplerDescriptorSize));

		D3D12_SAMPLER_DESC SamplerDesc = {};
		TranslateSamplerDesc(SamplerDesc, InDesc);
		Device->CreateSampler(&SamplerDesc, Sampler->SamplerHandle);

		D3D12_CPU_DESCRIPTOR_HANDLE ShaderVisibleHandle = OffsetCPUHandle(
			SamplerHeap->GetCPUDescriptorHandleForHeapStart(),
			static_cast<INT32>(HeapIndex),
			static_cast<INT32>(SamplerDescriptorSize));
		Device->CopyDescriptorsSimple(1u, ShaderVisibleHandle, Sampler->SamplerHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		*OutSampler = Sampler;
		return TRUE;
	}

	void RDeviceD3D12::DestroyResource(IRRHIResource* InResource) noexcept
	{
		if (!InResource)
		{
			return;
		}

		if (InResource->GetResourceType() == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_PIPELINE_STATE)
		{
			// PSOs are cache owned: dropping the cache entry here is what keeps a later
			// create for the same desc from handing out a pointer that is already queued for
			// deletion. The object itself goes through the same deferred queue as everything
			// else, because the GPU may still be executing draws that used it.
			RD3D12PipelineState* PipelineState = static_cast<RD3D12PipelineState*>(InResource);
			if (PipelineState->bPendingDelete)
			{
				return;
			}
			PipelineState->bPendingDelete = TRUE;
			// Compute PSOs are never cached (hash 0), only graphics ones are.
			if (PipelineState->PipelineStateHash != 0ull)
			{
				PipelineStateCache.Remove(PipelineState->PipelineStateHash);
			}
		}

		// Defer deletion for N+2 frames
		FDeferredDeleteEntry Entry;
		Entry.Resource				= InResource;
		Entry.RetireFrameIndex		= (CurrentFrameIndex + 2u) % FRAME_IN_FLIGHT;
		DeferredDeleteQueue.Add(Entry);
	}

	// =========================================================================
	// IRRHIDevice - Bindless
	// =========================================================================

	UINT32 RDeviceD3D12::RegisterBindlessSRV(IRRHITexture* InTexture) noexcept
	{
		if (!InTexture)
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}

		// Indices returned to callers are absolute CBV_SRV_UAV heap offsets so
		// HLSL `ResourceDescriptorHeap[idx]` indexes directly. Free list tracks
		// region-relative slot numbers.
		UINT32 RelativeSlot;
		if (BindlessSRVFreeIndices.Num() > 0)
		{
			RelativeSlot = BindlessSRVFreeIndices[BindlessSRVFreeIndices.Num() - 1];
			BindlessSRVFreeIndices.RemoveAt(BindlessSRVFreeIndices.Num() - 1);
			BindlessSRVTable[RelativeSlot] = InTexture;
		}
		else
		{
			if (static_cast<UINT32>(BindlessSRVTable.Num()) >= BINDLESS_SRV_CAPACITY)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Bindless SRV pool exhausted."));
				return RHI_INVALID_BINDLESS_INDEX;
			}
			RelativeSlot = static_cast<UINT32>(BindlessSRVTable.Num());
			BindlessSRVTable.Add(InTexture);
		}

		UINT32 HeapIndex = BINDLESS_SRV_BASE_INDEX + RelativeSlot;
		{
			IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTexture);
			D3D12_CPU_DESCRIPTOR_HANDLE SrcHandle = Tex->GetD3D12SRVHandle();
			if (SrcHandle.ptr == 0u)
			{
				return RHI_INVALID_BINDLESS_INDEX;
			}
			D3D12_CPU_DESCRIPTOR_HANDLE DstHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, DstHandle, SrcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		return HeapIndex;
	}

	UINT32 RDeviceD3D12::RegisterBindlessSRV(IRRHIBuffer* InBuffer) noexcept
	{
		if (!InBuffer)
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}

		UINT32 RelativeSlot;
		if (BindlessSRVFreeIndices.Num() > 0)
		{
			RelativeSlot = BindlessSRVFreeIndices[BindlessSRVFreeIndices.Num() - 1];
			BindlessSRVFreeIndices.RemoveAt(BindlessSRVFreeIndices.Num() - 1);
			BindlessSRVTable[RelativeSlot] = InBuffer;
		}
		else
		{
			if (static_cast<UINT32>(BindlessSRVTable.Num()) >= BINDLESS_SRV_CAPACITY)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Bindless SRV pool exhausted."));
				return RHI_INVALID_BINDLESS_INDEX;
			}
			RelativeSlot = static_cast<UINT32>(BindlessSRVTable.Num());
			BindlessSRVTable.Add(InBuffer);
		}

		UINT32 HeapIndex = BINDLESS_SRV_BASE_INDEX + RelativeSlot;
		{
			IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffer);
			D3D12_CPU_DESCRIPTOR_HANDLE SrcHandle = Buf->GetD3D12SRVHandle();
			if (SrcHandle.ptr == 0u)
			{
				return RHI_INVALID_BINDLESS_INDEX;
			}
			D3D12_CPU_DESCRIPTOR_HANDLE DstHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, DstHandle, SrcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		return HeapIndex;
	}

	UINT32 RDeviceD3D12::RegisterBindlessUAV(IRRHITexture* InTexture) noexcept
	{
		if (!InTexture)
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}

		UINT32 RelativeSlot;
		if (BindlessUAVFreeIndices.Num() > 0)
		{
			RelativeSlot = BindlessUAVFreeIndices[BindlessUAVFreeIndices.Num() - 1];
			BindlessUAVFreeIndices.RemoveAt(BindlessUAVFreeIndices.Num() - 1);
			BindlessUAVTable[RelativeSlot] = InTexture;
		}
		else
		{
			if (static_cast<UINT32>(BindlessUAVTable.Num()) >= BINDLESS_UAV_CAPACITY)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Bindless UAV pool exhausted."));
				return RHI_INVALID_BINDLESS_INDEX;
			}
			RelativeSlot = static_cast<UINT32>(BindlessUAVTable.Num());
			BindlessUAVTable.Add(InTexture);
		}

		UINT32 HeapIndex = BINDLESS_UAV_BASE_INDEX + RelativeSlot;
		{
			IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTexture);
			D3D12_CPU_DESCRIPTOR_HANDLE SrcHandle = Tex->GetD3D12UAVHandle();
			if (SrcHandle.ptr == 0u)
			{
				return RHI_INVALID_BINDLESS_INDEX;
			}
			D3D12_CPU_DESCRIPTOR_HANDLE DstHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, DstHandle, SrcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		return HeapIndex;
	}

	UINT32 RDeviceD3D12::RegisterBindlessUAV(IRRHIBuffer* InBuffer) noexcept
	{
		if (!InBuffer)
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}

		UINT32 RelativeSlot;
		if (BindlessUAVFreeIndices.Num() > 0)
		{
			RelativeSlot = BindlessUAVFreeIndices[BindlessUAVFreeIndices.Num() - 1];
			BindlessUAVFreeIndices.RemoveAt(BindlessUAVFreeIndices.Num() - 1);
			BindlessUAVTable[RelativeSlot] = InBuffer;
		}
		else
		{
			if (static_cast<UINT32>(BindlessUAVTable.Num()) >= BINDLESS_UAV_CAPACITY)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Bindless UAV pool exhausted."));
				return RHI_INVALID_BINDLESS_INDEX;
			}
			RelativeSlot = static_cast<UINT32>(BindlessUAVTable.Num());
			BindlessUAVTable.Add(InBuffer);
		}

		UINT32 HeapIndex = BINDLESS_UAV_BASE_INDEX + RelativeSlot;
		{
			IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffer);
			D3D12_CPU_DESCRIPTOR_HANDLE SrcHandle = Buf->GetD3D12UAVHandle();
			if (SrcHandle.ptr == 0u)
			{
				return RHI_INVALID_BINDLESS_INDEX;
			}
			D3D12_CPU_DESCRIPTOR_HANDLE DstHandle = OffsetCPUHandle(
				CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<INT32>(HeapIndex),
				static_cast<INT32>(CBVSRVUAVDescriptorSize));
			Device->CopyDescriptorsSimple(1u, DstHandle, SrcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		return HeapIndex;
	}

	void RDeviceD3D12::ReleaseBindlessIndex(UINT32 InIndex) noexcept
	{
		if (InIndex == RHI_INVALID_BINDLESS_INDEX)
		{
			return;
		}
		// Indices are absolute heap offsets; classify by region. The slot is emptied as well as
		// returned to the free list, which is what D3D11 does and what the readers of an index need:
		// a caller that still holds the index (a recorded command, a cached id) has to resolve it to
		// nothing, because the resource it named is about to be destroyed. Leaving the entry in place
		// hands that caller a pointer to a dead object through a table that says it is alive - and the
		// second, subtler half of the same rule: a released slot is emptied exactly once, so a double
		// release cannot put one slot on the free list twice and give two live resources the same one.
		if (InIndex >= BINDLESS_SRV_BASE_INDEX && InIndex < (BINDLESS_SRV_BASE_INDEX + BINDLESS_SRV_CAPACITY))
		{
			const UINT32 RelativeSlot = InIndex - BINDLESS_SRV_BASE_INDEX;
			if ((RelativeSlot < static_cast<UINT32>(BindlessSRVTable.Num())) && (BindlessSRVTable[RelativeSlot] != nullptr))
			{
				BindlessSRVTable[RelativeSlot] = nullptr;
				BindlessSRVFreeIndices.Add(RelativeSlot);
			}
		}
		else if (InIndex >= BINDLESS_UAV_BASE_INDEX && InIndex < (BINDLESS_UAV_BASE_INDEX + BINDLESS_UAV_CAPACITY))
		{
			const UINT32 RelativeSlot = InIndex - BINDLESS_UAV_BASE_INDEX;
			if ((RelativeSlot < static_cast<UINT32>(BindlessUAVTable.Num())) && (BindlessUAVTable[RelativeSlot] != nullptr))
			{
				BindlessUAVTable[RelativeSlot] = nullptr;
				BindlessUAVFreeIndices.Add(RelativeSlot);
			}
		}
	}

	IRRHIResource* RDeviceD3D12::GetBindlessResource(UINT32 InIndex)const noexcept
	{
		// Indices are absolute heap offsets; classify by region then dereference
		// the per-region table at the relative slot.
		if (InIndex >= BINDLESS_SRV_BASE_INDEX && InIndex < (BINDLESS_SRV_BASE_INDEX + BINDLESS_SRV_CAPACITY))
		{
			const UINT32 Rel = InIndex - BINDLESS_SRV_BASE_INDEX;
			if (Rel < static_cast<UINT32>(BindlessSRVTable.Num()))
			{
				return BindlessSRVTable[Rel];
			}
		}
		else if (InIndex >= BINDLESS_UAV_BASE_INDEX && InIndex < (BINDLESS_UAV_BASE_INDEX + BINDLESS_UAV_CAPACITY))
		{
			const UINT32 Rel = InIndex - BINDLESS_UAV_BASE_INDEX;
			if (Rel < static_cast<UINT32>(BindlessUAVTable.Num()))
			{
				return BindlessUAVTable[Rel];
			}
		}
		return nullptr;
	}

	// =========================================================================
	// IRRHIDevice - Upload
	// =========================================================================

	void RDeviceD3D12::UploadBuffer(IRRHIBuffer* InDest, const UINT8* InData, UINT64 InSize, UINT64 InDestOffset) noexcept
	{
		if (!InDest || !InData || InSize == 0u)
		{
			return;
		}

		void* CPUAddress = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS GPUAddress = 0;
		UINT32 UploadOffset = AllocateUploadMemory(InSize, 256u, &CPUAddress, &GPUAddress);
		if (!CPUAddress)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Upload heap exhausted."));
			return;
		}

		memcpy(CPUAddress, InData, InSize);

		IRD3D12Buffer* DstBuffer = static_cast<IRD3D12Buffer*>(InDest);
		ID3D12Resource* DstResource = DstBuffer->GetD3D12Resource();
		if (!DstResource)
		{
			return;
		}

		// Open the dedicated upload command list lazily.
		if (!bUploadCommandListOpen)
		{
			if (LastUploadFenceValue > 0ull && FrameFence->GetCompletedValue() < LastUploadFenceValue)
			{
				FrameFence->SetEventOnCompletion(LastUploadFenceValue, FrameFenceEvent);
				WaitForSingleObject(FrameFenceEvent, INFINITE);
			}
			UploadCommandAllocator->Reset();
			if (FAILED(UploadCommandList->Reset(UploadCommandAllocator.Get(), nullptr)))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Upload command list Reset failed."));
				return;
			}
			bUploadCommandListOpen = TRUE;
		}

		D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
		RDeviceD3D12::TranslateResourceStates(InitialState, InDest->GetCurrentState());

		// Transition: current -> COPY_DEST
		if (InitialState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			D3D12_RESOURCE_BARRIER Barrier = {};
			Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			Barrier.Transition.pResource   = DstResource;
			Barrier.Transition.StateBefore = InitialState;
			Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
			Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			UploadCommandList->ResourceBarrier(1u, &Barrier);
		}

		UploadCommandList->CopyBufferRegion(DstResource, InDestOffset, UploadHeap.Get(), UploadOffset, InSize);

		// Pick a usable post-copy state for the destination based on its bind flags.
		D3D12_RESOURCE_STATES PostState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		UINT8 BindFlags = DstBuffer->GetBindFlags();
		if (!!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_INDEX_BUFFER)))
		{
			PostState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		}
		else if (!!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)))
		{
			PostState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
		else if (!!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)))
		{
			PostState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		D3D12_RESOURCE_BARRIER PostBarrier = {};
		PostBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		PostBarrier.Transition.pResource   = DstResource;
		PostBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		PostBarrier.Transition.StateAfter  = PostState;
		PostBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		UploadCommandList->ResourceBarrier(1u, &PostBarrier);

		// Update tracked state; the actual GPU transition will run on FlushPendingUploads.
		if (PostState == D3D12_RESOURCE_STATE_INDEX_BUFFER)
		{
			InDest->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_INDEX_BUFFER);
		}
		else if (PostState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			InDest->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
		}
		else if (PostState == (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))
		{
			InDest->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		}
		else if (!!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_CONSTANT_BUFFER)))
		{
			InDest->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_CONSTANT_BUFFER);
		}
		else
		{
			InDest->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_VERTEX_BUFFER);
		}
	}

	void RDeviceD3D12::FlushPendingUploads()
	{
		if (!bUploadCommandListOpen)
		{
			return;
		}
		if (FAILED(UploadCommandList->Close()))
		{
			bUploadCommandListOpen = FALSE;
			return;
		}
		ID3D12CommandList* Lists[] = { UploadCommandList.Get() };
		CommandQueue->ExecuteCommandLists(1u, Lists);

		++GlobalFenceCounter;
		CommandQueue->Signal(FrameFence.Get(), GlobalFenceCounter);
		// Record against this slot so the upload heap / mapped region isn't
		// reused by the next occupant of this slot before the GPU is done.
		FrameFenceValues[CurrentFrameIndex] = GlobalFenceCounter;
		LastUploadFenceValue = GlobalFenceCounter;
		bUploadCommandListOpen = FALSE;
	}

	void RDeviceD3D12::UploadTexture(IRRHITexture* InDest, const RRHISubresourceData* InSubresources, UINT32 InSubresourceCount, UINT32 InFirstSubresource) noexcept
	{
		if (!InDest || !InSubresources || InSubresourceCount == 0u)
		{
			return;
		}

		IRD3D12Texture* DstTex     = static_cast<IRD3D12Texture*>(InDest);
		ID3D12Resource* DstResource = DstTex->GetD3D12Resource();
		if (!DstResource)
		{
			return;
		}

		// Cubemap-with-full-mips is at most ~78 subresources; cap on the safe side.
		constexpr UINT32 MAX_SUBRESOURCES = 128u;
		if (InSubresourceCount > MAX_SUBRESOURCES)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: UploadTexture subresource count exceeds MAX_SUBRESOURCES."));
			return;
		}

		D3D12_RESOURCE_DESC ResourceDesc = DstResource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MAX_SUBRESOURCES] = {};
		UINT   NumRows[MAX_SUBRESOURCES] = {};
		UINT64 RowSizeInBytes[MAX_SUBRESOURCES] = {};
		UINT64 TotalBytes = 0ull;
		Device->GetCopyableFootprints(&ResourceDesc, InFirstSubresource, InSubresourceCount, 0ull,
			Layouts, NumRows, RowSizeInBytes, &TotalBytes);

		if (TotalBytes == 0ull)
		{
			return;
		}

		void* CPUBase = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS GPUBase = 0;
		UINT32 UploadStartOffset = AllocateUploadMemory(TotalBytes,
			D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, &CPUBase, &GPUBase);
		if (!CPUBase)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Upload heap exhausted for texture upload."));
			return;
		}

		// CPU-side: copy each subresource into upload heap with D3D12's expected
		// row pitch (which may differ from source RowPitch).
		for (UINT32 i = 0u; i < InSubresourceCount; i++)
		{
			const RRHISubresourceData& Sub = InSubresources[i];
			if (!Sub.Data)
			{
				continue;
			}
			// Every row is copied RowSizeInBytes long out of the caller's buffer, so a
			// smaller source pitch would read past its end.
			if (static_cast<UINT64>(Sub.RowPitch) < RowSizeInBytes[i])
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: UploadTexture source RowPitch is smaller than one row of the subresource."));
				continue;
			}
			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& Layout = Layouts[i];
			BYTE*       DstSub  = static_cast<BYTE*>(CPUBase) + Layout.Offset;
			const BYTE* SrcSub  = static_cast<const BYTE*>(Sub.Data);
			const UINT32 NumSlices = Layout.Footprint.Depth;
			const SIZE_T DstSlicePitch = static_cast<SIZE_T>(Layout.Footprint.RowPitch) * static_cast<SIZE_T>(NumRows[i]);
			for (UINT32 z = 0u; z < NumSlices; z++)
			{
				BYTE*       DstSlice = DstSub + DstSlicePitch * z;
				const BYTE* SrcSlice = SrcSub + static_cast<SIZE_T>(Sub.SlicePitch) * z;
				for (UINT32 y = 0u; y < NumRows[i]; y++)
				{
					memcpy(DstSlice + static_cast<SIZE_T>(Layout.Footprint.RowPitch) * y,
						SrcSlice + static_cast<SIZE_T>(Sub.RowPitch) * y,
						static_cast<SIZE_T>(RowSizeInBytes[i]));
				}
			}
		}

		// Open the dedicated upload command list lazily.
		if (!bUploadCommandListOpen)
		{
			if (LastUploadFenceValue > 0ull && FrameFence->GetCompletedValue() < LastUploadFenceValue)
			{
				FrameFence->SetEventOnCompletion(LastUploadFenceValue, FrameFenceEvent);
				WaitForSingleObject(FrameFenceEvent, INFINITE);
			}
			UploadCommandAllocator->Reset();
			if (FAILED(UploadCommandList->Reset(UploadCommandAllocator.Get(), nullptr)))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Upload command list Reset failed."));
				return;
			}
			bUploadCommandListOpen = TRUE;
		}

		// Transition: current -> COPY_DEST
		D3D12_RESOURCE_STATES BeforeState = D3D12_RESOURCE_STATE_COMMON;
		RDeviceD3D12::TranslateResourceStates(BeforeState, InDest->GetCurrentState());
		if (BeforeState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			D3D12_RESOURCE_BARRIER Barrier = {};
			Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			Barrier.Transition.pResource   = DstResource;
			Barrier.Transition.StateBefore = BeforeState;
			Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
			Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			UploadCommandList->ResourceBarrier(1u, &Barrier);
		}

		// GPU-side: record CopyTextureRegion per subresource. Each footprint's
		// Offset was returned relative to BaseOffset=0; shift by UploadStartOffset
		// to address the actual upload heap location.
		for (UINT32 i = 0u; i < InSubresourceCount; i++)
		{
			// Subresources the caller left empty were skipped by the CPU fill above;
			// copying their footprint would publish upload-heap garbage.
			if (!InSubresources[i].Data)
			{
				continue;
			}
			D3D12_TEXTURE_COPY_LOCATION Src = {};
			Src.pResource                = UploadHeap.Get();
			Src.Type                     = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			Src.PlacedFootprint          = Layouts[i];
			Src.PlacedFootprint.Offset  += UploadStartOffset;

			D3D12_TEXTURE_COPY_LOCATION Dst = {};
			Dst.pResource        = DstResource;
			Dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			Dst.SubresourceIndex = InFirstSubresource + i;

			UploadCommandList->CopyTextureRegion(&Dst, 0u, 0u, 0u, &Src, nullptr);
		}

		// Transition COPY_DEST -> a usable read state based on texture bind flags.
		UINT8 BindFlags = DstTex->GetBindFlags();
		D3D12_RESOURCE_STATES PostState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ERHIResourceState     PostStateRHI = ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE;
		if (!!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)))
		{
			PostState     = D3D12_RESOURCE_STATE_RENDER_TARGET;
			PostStateRHI  = ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET;
		}
		else if (!!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL)))
		{
			PostState     = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			PostStateRHI  = ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE;
		}
		else if (!!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)))
		{
			PostState     = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			PostStateRHI  = ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		D3D12_RESOURCE_BARRIER PostBarrier = {};
		PostBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		PostBarrier.Transition.pResource   = DstResource;
		PostBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		PostBarrier.Transition.StateAfter  = PostState;
		PostBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		UploadCommandList->ResourceBarrier(1u, &PostBarrier);

		InDest->SetCurrentState(PostStateRHI);
	}

	// =========================================================================
	// IRRHIDevice - Command lists
	// =========================================================================

	IRCommandList* RDeviceD3D12::AcquireCommandList() noexcept
	{
		RCommandListD3D12* CommandList = nullptr;

		if (CommandListFreeList.Num() > 0)
		{
			CommandList = CommandListFreeList[CommandListFreeList.Num() - 1];
			CommandListFreeList.RemoveAt(CommandListFreeList.Num() - 1);
		}
		else
		{
			RDeviceD3D12* DeviceD3D12 = this;
			TUniquePtr<RCommandListD3D12> NewList = EMemory::MakeUnique<RCommandListD3D12>(DeviceD3D12);
			CommandList = EMemory::GetPtr(NewList);
			CommandListPool.Add(std::move(NewList));
		}

		// Create or reset native command list
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> NativeList;
		Device->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[CurrentFrameIndex].Get(),
			nullptr, IID_PPV_ARGS(NativeList.ReleaseAndGetAddressOf()));
		CommandList->SetNativeList(NativeList, CommandAllocators[CurrentFrameIndex]);

		return CommandList;
	}

	void RDeviceD3D12::SubmitCommandList(IRCommandList* InCommandList) noexcept
	{
		if (!InCommandList)
		{
			return;
		}

		// Flush any pending upload work first so dependent draws see committed data.
		FlushPendingUploads();

		RCommandListD3D12* CL = static_cast<RCommandListD3D12*>(InCommandList);
		CL->Close();

		ID3D12CommandList* NativeLists[] = { CL->GetNativeCommandList().Get() };
		CommandQueue->ExecuteCommandLists(1u, NativeLists);

		// Bump monotonic counter, signal, and record the value against the slot
		// this work was submitted from so BeginFrame can wait when it cycles back.
		++GlobalFenceCounter;
		CommandQueue->Signal(FrameFence.Get(), GlobalFenceCounter);
		FrameFenceValues[CurrentFrameIndex] = GlobalFenceCounter;

		// Return to pool
		CommandListFreeList.Add(CL);
	}

	void RDeviceD3D12::SubmitCommandLists(IRCommandList* const* InCommandLists, UINT32 InCount) noexcept
	{
		if (!InCommandLists || InCount == 0u)
		{
			return;
		}

		FlushPendingUploads();

		TArray<ID3D12CommandList*> NativeLists;
		for (UINT32 i = 0u; i < InCount; i++)
		{
			RCommandListD3D12* CL = static_cast<RCommandListD3D12*>(InCommandLists[i]);
			CL->Close();
			NativeLists.Add(CL->GetNativeCommandList().Get());
		}

		CommandQueue->ExecuteCommandLists(InCount, NativeLists.GetData());

		++GlobalFenceCounter;
		CommandQueue->Signal(FrameFence.Get(), GlobalFenceCounter);
		FrameFenceValues[CurrentFrameIndex] = GlobalFenceCounter;

		for (UINT32 i = 0u; i < InCount; i++)
		{
			CommandListFreeList.Add(static_cast<RCommandListD3D12*>(InCommandLists[i]));
		}
	}

	// =========================================================================
	// IRRHIDevice - Backbuffer access
	// =========================================================================

	IRRHITexture* RDeviceD3D12::GetCurrentBackBuffer()const noexcept
	{
		UINT32 Index = SwapChain->GetCurrentBackBufferIndex();
		if (Index < static_cast<UINT32>(BackBuffers.Num()))
		{
			return BackBuffers[Index];
		}
		return nullptr;
	}

	// =========================================================================
	// IRRHIDevice - GPU timestamp queries (Phase P.d)
	// =========================================================================

	BOOL8 RDeviceD3D12::CreateTimestampHeap(UINT32 InCapacity, RRHITimestampHeap** OutHeap) noexcept
	{
		if (!OutHeap || InCapacity == 0u) { return FALSE; }
		*OutHeap = nullptr;
		if (!Device || !CommandQueue) { return FALSE; }

		RRHITimestampHeapD3D12* Heap = new RRHITimestampHeapD3D12();
		Heap->Capacity = InCapacity;

		D3D12_QUERY_HEAP_DESC HeapDesc = {};
		HeapDesc.Type		= D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		HeapDesc.Count		= InCapacity;
		HeapDesc.NodeMask	= 0u;
		if (FAILED(Device->CreateQueryHeap(&HeapDesc, IID_PPV_ARGS(Heap->QueryHeap.ReleaseAndGetAddressOf()))))
		{
			delete Heap;
			return FALSE;
		}

		// Readback sized exactly Capacity * 8B. ResolveQueryData writes the
		// timestamps here from the query heap and the CPU then memcpys from
		// the mapped pointer in ResolveTimestamps.
		const UINT64 ReadbackBytes = static_cast<UINT64>(InCapacity) * sizeof(UINT64);

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type					= D3D12_HEAP_TYPE_READBACK;
		HeapProps.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask		= 0u;
		HeapProps.VisibleNodeMask		= 0u;

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
		ResourceDesc.Alignment			= 0u;
		ResourceDesc.Width				= ReadbackBytes;
		ResourceDesc.Height				= 1u;
		ResourceDesc.DepthOrArraySize	= 1u;
		ResourceDesc.MipLevels			= 1u;
		ResourceDesc.Format				= DXGI_FORMAT_UNKNOWN;
		ResourceDesc.SampleDesc.Count	= 1u;
		ResourceDesc.SampleDesc.Quality	= 0u;
		ResourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResourceDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;

		if (FAILED(Device->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(Heap->ReadbackBuffer.ReleaseAndGetAddressOf()))))
		{
			delete Heap;
			return FALSE;
		}

		// Timestamp frequency is fixed per device for the queue type; latch it
		// here so GetGPUTicksPerSecond doesn't have to touch the queue later.
		UINT64 Freq = 0u;
		if (FAILED(CommandQueue->GetTimestampFrequency(&Freq))) { Freq = 0u; }
		Heap->TicksPerSecond = Freq;

		*OutHeap = Heap;
		return TRUE;
	}

	void RDeviceD3D12::DestroyTimestampHeap(RRHITimestampHeap* InHeap) noexcept
	{
		if (!InHeap) { return; }
		// ComPtr members release on dtor.
		delete InHeap;
	}

	void RDeviceD3D12::BeginTimestampFrame(RRHITimestampHeap* /*InHeap*/) noexcept
	{
		// D3D12 has no disjoint-style framing; slot tracking lives in the
		// renderer ring above this layer.
	}

	void RDeviceD3D12::EndTimestampFrame(RRHITimestampHeap* /*InHeap*/) noexcept
	{
		// No-op for the same reason as BeginTimestampFrame. The GPU-side copy
		// from query heap -> readback is emitted via IRCommandList::ResolveTimestampHeap
		// at the renderer's discretion (typically right before close-and-submit).
	}

	BOOL8 RDeviceD3D12::ResolveTimestamps(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount, UINT64* OutTicks) noexcept
	{
		RRHITimestampHeapD3D12* Heap = static_cast<RRHITimestampHeapD3D12*>(InHeap);
		if (!Heap || !OutTicks || InCount == 0u) { return FALSE; }
		if (!Heap->ReadbackBuffer) { return FALSE; }
		if ((InBeginSlot + InCount) > Heap->Capacity) { return FALSE; }

		// Tight read range so the runtime can skip flushing the whole readback.
		// Caller contract: the GPU has retired the frame whose ResolveQueryData
		// populated this slot range before we Map.
		D3D12_RANGE ReadRange = {};
		ReadRange.Begin	= static_cast<SIZE_T>(InBeginSlot) * sizeof(UINT64);
		ReadRange.End	= ReadRange.Begin + static_cast<SIZE_T>(InCount) * sizeof(UINT64);

		void* Mapped = nullptr;
		if (FAILED(Heap->ReadbackBuffer->Map(0u, &ReadRange, &Mapped))) { return FALSE; }
		if (!Mapped) { Heap->ReadbackBuffer->Unmap(0u, nullptr); return FALSE; }

		const UINT64* Src = reinterpret_cast<const UINT64*>(static_cast<const BYTE*>(Mapped) + ReadRange.Begin);
		for (UINT32 i = 0u; i < InCount; i++)
		{
			OutTicks[i] = Src[i];
		}

		// Pass an empty write range so the runtime knows we wrote nothing back.
		D3D12_RANGE EmptyWrite = { 0u, 0u };
		Heap->ReadbackBuffer->Unmap(0u, &EmptyWrite);
		return TRUE;
	}

	UINT64 RDeviceD3D12::GetGPUTicksPerSecond(const RRHITimestampHeap* InHeap)const noexcept
	{
		const RRHITimestampHeapD3D12* Heap = static_cast<const RRHITimestampHeapD3D12*>(InHeap);
		if (!Heap) { return 0u; }
		return Heap->TicksPerSecond;
	}

	// =========================================================================
	// Descriptor / upload allocators
	// =========================================================================

	UINT32 RDeviceD3D12::AllocatePersistentRTV()
	{
		if (RTVDescriptorOffset >= RTV_HEAP_SIZE)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: RTV heap exhausted."));
			return 0u;
		}
		return (RTVDescriptorOffset++);
	}

	UINT32 RDeviceD3D12::AllocatePersistentDSV()
	{
		if (DSVDescriptorOffset >= DSV_HEAP_SIZE)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: DSV heap exhausted."));
			return 0u;
		}
		return (DSVDescriptorOffset++);
	}

	UINT32 RDeviceD3D12::AllocatePersistentSRVUAV()
	{
		if (PersistentDescriptorOffset >= PERSISTENT_DESC_CAPACITY)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Persistent CBV_SRV_UAV region exhausted."));
			return PERSISTENT_DESC_BASE_INDEX;
		}
		UINT32 HeapIndex = PERSISTENT_DESC_BASE_INDEX + PersistentDescriptorOffset;
		PersistentDescriptorOffset++;
		return HeapIndex;
	}

	UINT32 RDeviceD3D12::AllocatePersistentSampler()
	{
		if (PersistentSamplerOffset >= PERSISTENT_SAMPLER_CAPACITY)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: Persistent sampler region exhausted."));
			return PERSISTENT_SAMPLER_BASE_INDEX;
		}
		UINT32 HeapIndex = PERSISTENT_SAMPLER_BASE_INDEX + PersistentSamplerOffset;
		PersistentSamplerOffset++;
		return HeapIndex;
	}

	UINT32 RDeviceD3D12::GetSlotSetArenaBase()const
	{
		// The frame's own arena, so two frames in flight never write the same descriptors.
		// The reserved prefix is what the fixed slot table bases used to occupy; the arena
		// follows it and is handed out by the command list one resource set at a time.
		return (((CurrentFrameIndex % FRAME_IN_FLIGHT) * SLOT_TABLE_STRIDE_PER_FRAME) + SLOT_SET_ARENA_OFFSET);
	}

	UINT32 RDeviceD3D12::GetSlotSetArenaCapacity()const
	{
		return (SLOT_TABLE_STRIDE_PER_FRAME - SLOT_SET_ARENA_OFFSET);
	}

	UINT32 RDeviceD3D12::GetSlotSamplerTableBase()const
	{
		return ((CurrentFrameIndex % FRAME_IN_FLIGHT) * SLOT_SAMPLER_STRIDE_PER_FRAME);
	}

	void RDeviceD3D12::WriteNullShaderResourceView(UINT32 InHeapIndex)
	{
		if (!Device || !CBVSRVUAVHeap)
		{
			return;
		}
		// A null view still has to state a viewable format and a dimension; the read result of a
		// null descriptor is zero whatever it says, so the format only has to be a valid one.
		D3D12_SHADER_RESOURCE_VIEW_DESC NullSRVDesc = {};
		NullSRVDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
		NullSRVDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
		NullSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		NullSRVDesc.Texture2D.MipLevels     = 1u;

		const D3D12_CPU_DESCRIPTOR_HANDLE Slot = OffsetCPUHandle(
			CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
			static_cast<INT32>(InHeapIndex),
			static_cast<INT32>(CBVSRVUAVDescriptorSize));
		Device->CreateShaderResourceView(nullptr, &NullSRVDesc, Slot);
	}

	void RDeviceD3D12::WriteNullUnorderedAccessView(UINT32 InHeapIndex)
	{
		if (!Device || !CBVSRVUAVHeap)
		{
			return;
		}
		D3D12_UNORDERED_ACCESS_VIEW_DESC NullUAVDesc = {};
		NullUAVDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
		NullUAVDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
		NullUAVDesc.Texture2D.MipSlice = 0u;

		const D3D12_CPU_DESCRIPTOR_HANDLE Slot = OffsetCPUHandle(
			CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(),
			static_cast<INT32>(InHeapIndex),
			static_cast<INT32>(CBVSRVUAVDescriptorSize));
		Device->CreateUnorderedAccessView(nullptr, nullptr, &NullUAVDesc, Slot);
	}

	UINT32 RDeviceD3D12::AllocateUploadMemory(UINT64 InSize, UINT64 InAlignment, void** OutCPUAddress, D3D12_GPU_VIRTUAL_ADDRESS* OutGPUAddress)
	{
		UINT64 AlignedOffset = (UploadHeapOffset + InAlignment - 1u) & ~(InAlignment - 1u);
		if ((AlignedOffset + InSize) > UploadHeapSize)
		{
			return 0u;
		}

		UploadHeapOffset = AlignedOffset + InSize;
		*OutCPUAddress  = static_cast<BYTE*>(UploadHeapBase) + AlignedOffset;
		*OutGPUAddress  = UploadHeap->GetGPUVirtualAddress() + AlignedOffset;
		return static_cast<UINT32>(AlignedOffset);
	}

	// =========================================================================
	// EManagerBase overrides
	// =========================================================================

	void RDeviceD3D12::Initialize()
	{
		// IRRHIDevice::Initialize handles actual init.
		// EManagerBase::Initialize is called by the engine singleton system.
	}

	void RDeviceD3D12::ShutDown()
	{
		Shutdown();
	}

	// =========================================================================
	// Constructor / Destructor
	// =========================================================================

	RDeviceD3D12::RDeviceD3D12()
		: BackBufferCount(2u)
		, BackBufferWidth(0u)
		, BackBufferHeight(0u)
		, BackBufferFormat(RFormatType::FORMAT_R8G8B8A8_UNORM)
		, DepthStencilFormat(RFormatType::FORMAT_R24G8_TYPELESS)
		, bIsWindowed(TRUE)
		, RTVDescriptorSize(0u)
		, DSVDescriptorSize(0u)
		, CBVSRVUAVDescriptorSize(0u)
		, SamplerDescriptorSize(0u)
		, RTVDescriptorOffset(0u)
		, DSVDescriptorOffset(0u)
		, PersistentDescriptorOffset(0u)
		, PersistentSamplerOffset(0u)
		, FrameFenceEvent(nullptr)
		, CurrentFrameIndex(0u)
		, FrameInFlightCount(FRAME_IN_FLIGHT)
		, GlobalFenceCounter(0ull)
		, UploadHeapBase(nullptr)
		, UploadHeapSize(0u)
		, UploadHeapOffset(0u)
		, bUploadCommandListOpen(FALSE)
		, LastUploadFenceValue(0ull)
		, bEnableDebugLayer(FALSE)
		, bEnableGPUValidation(FALSE)
		, bInitialized(FALSE)
	{
		for (UINT32 i = 0u; i < FRAME_IN_FLIGHT; i++)
		{
			FrameFenceValues[i] = 0ull;
		}
	}

	RDeviceD3D12::~RDeviceD3D12()
	{
		if (bInitialized)
		{
			Shutdown();
		}
	}

};	// PigeonEngine
