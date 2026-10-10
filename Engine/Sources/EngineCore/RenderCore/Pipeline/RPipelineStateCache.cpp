#include <Pipeline/RPipelineStateCache.h>

#include <Pipeline/RInputLayoutCompatibility.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/IRRHIResource.h>
#include <Shader/ShaderAssetBinding.h>
#include <ShaderAsset/ShaderProgram.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	RPipelineStateCache::RPipelineStateCache()
		: Device(nullptr)
		, LayoutCache{}
		, States{}
	{
	}

	RPipelineStateCache::~RPipelineStateCache()
	{
		InvalidateAll();
	}

	void RPipelineStateCache::SetDevice(IRRHIDevice* InDevice)
	{
		Device = InDevice;
		LayoutCache.SetDevice(InDevice);
	}

	void RPipelineStateCache::InvalidateAll()
	{
		// Release what this cache created. The device defers the real destruction until the
		// GPU is done with the work that used it, so a state that is still in flight is safe.
		if (Device)
		{
			for (const auto& Entry : States)
			{
				if (Entry.second.State)
				{
					Device->DestroyResource(Entry.second.State);
				}
			}
		}
		States.Empty();
		LayoutCache.InvalidateAll();
	}

	void RPipelineStateCache::InvalidateForAsset(const EShaderAsset* InAsset)
	{
		if (!InAsset)
		{
			return;
		}
		InvalidateForShaderName(InAsset->GetName());
	}

	void RPipelineStateCache::InvalidateForShaderName(const EName& InShaderName)
	{
		// Collect first, remove after: TMap only offers key based removal, and the entries have
		// to be walked to find out which shader a state came from.
		TArray<UINT64> DroppedKeys;
		for (const auto& Entry : States)
		{
			const FPipelineStateEntry& StateEntry = Entry.second;
			if ((StateEntry.Program != nullptr) && (StateEntry.Program->GetShaderName() == InShaderName))
			{
				DroppedKeys.Add(Entry.first);
			}
		}

		for (UINT32 KeyIndex = 0u, KeyNum = DroppedKeys.Num<UINT32>(); KeyIndex < KeyNum; KeyIndex++)
		{
			FPipelineStateEntry StateEntry;
			if (States.FindValue(DroppedKeys[KeyIndex], StateEntry) && StateEntry.State && Device)
			{
				Device->DestroyResource(StateEntry.State);
			}
			States.Remove(DroppedKeys[KeyIndex]);
		}
	}

	UINT64 RPipelineStateCache::HashGraphicsDesc(const RGraphicsPipelineStateDesc& InDesc, const UINT64 InLayoutHash)
	{
		UINT64 Hash = 0ull;

		auto HashCombine = [&Hash](UINT64 InValue)
		{
			Hash ^= InValue + 0x9e3779b97f4a7c15ull + (Hash << 6) + (Hash >> 2);
		};

		auto CombineBlendState = [&HashCombine](const RBlendState& InState)
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

		union EPtrUint64
		{
			UINT64		_Uint64;
			const void*	_Ptr;
		};

		EPtrUint64 ProgramIdentity;
		ProgramIdentity._Ptr = InDesc.Program;
		HashCombine(ProgramIdentity._Uint64);
		// The parameter layout is part of the pipeline: two descs on the same program always
		// share it, and including it keeps the key honest if a program is ever laid out twice.
		HashCombine(InLayoutHash);

		HashCombine(static_cast<UINT64>(InDesc.Rasterizer.CullMode));
		HashCombine(static_cast<UINT64>(InDesc.Rasterizer.FillMode));

		HashCombine(((InDesc.Blend.AlphaToCoverageEnable ? 2ull : 1ull)
			| (InDesc.Blend.IndependentBlendEnable ? 8ull : 4ull)));
		const INT32 BlendStateNum = InDesc.Blend.BlendStates.Num<INT32>();
		if (BlendStateNum > 0)
		{
			CombineBlendState(InDesc.Blend.BlendStates[0]);
		}
		if (InDesc.Blend.IndependentBlendEnable)
		{
			for (INT32 StateIndex = 1; StateIndex < BlendStateNum; StateIndex++)
			{
				CombineBlendState(InDesc.Blend.BlendStates[StateIndex]);
			}
		}

		HashCombine(static_cast<UINT64>(InDesc.Depth.DepthEnable));
		HashCombine(static_cast<UINT64>(InDesc.Depth.DepthWriteMask));
		HashCombine(static_cast<UINT64>(InDesc.Depth.DepthFunc));
		HashCombine(static_cast<UINT64>(InDesc.Stencil.StencilEnable));
		HashCombine(static_cast<UINT64>(InDesc.Stencil.StencilReadMask));
		HashCombine(static_cast<UINT64>(InDesc.Stencil.StencilWriteMask));

		HashCombine(static_cast<UINT64>(InDesc.Topology));
		HashCombine(static_cast<UINT64>(InDesc.RenderTargetCount));
		for (UINT32 TargetIndex = 0u; TargetIndex < InDesc.RenderTargetCount && TargetIndex < RHI_MAX_RENDER_TARGETS; TargetIndex++)
		{
			HashCombine(static_cast<UINT64>(InDesc.RenderTargetFormats[TargetIndex]));
		}
		HashCombine(static_cast<UINT64>(InDesc.DepthBits));
		HashCombine(static_cast<UINT64>(InDesc.HasStencil));
		HashCombine(static_cast<UINT64>(InDesc.SampleCount));
		HashCombine(static_cast<UINT64>(InDesc.SampleQuality));

		// The input layout is part of the pipeline only when the desc overrides it; the
		// program's own layout is already covered by the program identity.
		HashCombine(static_cast<UINT64>(InDesc.InputLayoutCount));
		if (InDesc.InputLayouts && InDesc.InputLayoutCount > 0u)
		{
			for (UINT32 LayoutIndex = 0u; LayoutIndex < InDesc.InputLayoutCount; LayoutIndex++)
			{
				const RInputLayoutDesc& Layout = InDesc.InputLayouts[LayoutIndex];
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

	IRRHIPipelineState* RPipelineStateCache::GetOrCreateGraphics(const RGraphicsPipelineStateDesc& InDesc)
	{
		if ((!Device) || (!InDesc.Program))
		{
			return nullptr;
		}

		const RPipelineLayout* Layout = LayoutCache.GetOrCreate(*InDesc.Program);
		if (!Layout)
		{
			return nullptr;
		}

		const UINT64 Hash = HashGraphicsDesc(InDesc, Layout->LayoutHash);
		FPipelineStateEntry Cached;
		if (States.FindValue(Hash, Cached) && Cached.State)
		{
			return Cached.State;
		}

		RRHIGraphicsPipelineDesc RHIDesc;
		if (!FillGraphicsPipelineShaders(*InDesc.Program, RHIDesc))
		{
			// The program is missing a stage the pipeline needs. The caller has nothing to draw
			// with, so this is a refusal rather than an empty pipeline state.
			return nullptr;
		}
		// The signature the program's reflection produced. On a back-end without root signatures
		// the object it hands back carries the zero layout and no native signature, and the device
		// substitutes its own, which is what a null here would mean too.
		RHIDesc.RootSignature = Layout->RootSignature;
		RHIDesc.Rasterizer = InDesc.Rasterizer;
		RHIDesc.Blend = InDesc.Blend;
		RHIDesc.Depth = InDesc.Depth;
		RHIDesc.Stencil = InDesc.Stencil;
		RHIDesc.PrimitiveTopology = InDesc.Topology;
		RHIDesc.DepthBits = InDesc.DepthBits;
		RHIDesc.HasStencil = InDesc.HasStencil;
		RHIDesc.SampleCount = InDesc.SampleCount;
		RHIDesc.SampleQuality = InDesc.SampleQuality;
		RHIDesc.RenderTargetCount = InDesc.RenderTargetCount;
		for (UINT32 TargetIndex = 0u; TargetIndex < InDesc.RenderTargetCount && TargetIndex < RHI_MAX_RENDER_TARGETS; TargetIndex++)
		{
			RHIDesc.RenderTargetFormats[TargetIndex] = InDesc.RenderTargetFormats[TargetIndex];
		}
		// Merged layout of a desc that declares its own vertex data: the declaration replaces the
		// element it feeds and the shader's own stays for everything else. It has to outlive the
		// device call below.
		TArray<RInputLayoutDesc> DeclaredLayouts;
		if (InDesc.InputLayouts && InDesc.InputLayoutCount > 0u)
		{
			// The caller declared vertex data it is going to bind. Pair it with what the vertex shader
			// reads before the back-end sees it: the API only reports a class mismatch as a generic
			// pipeline creation failure, and there is no way back from that to the element that caused
			// it. The declaration only has to name the elements whose real layout differs from what the
			// shader assumes; the rest keep the shader's own, which is what the meshes are packed for.
			const RShaderStageResource* VertexStage = ResolveShaderStageResource(*InDesc.Program, EShaderStageType::SHADER_STAGE_VERTEX);
			if (!VertexStage)
			{
				return nullptr;
			}
			const TArray<RInputLayoutDesc>& ShaderInputs = VertexStage->GetInputLayouts();

			TArray<RInputElementPair> Pairs;
			TArray<INT32> UnmatchedDeclaredIndices;
			const BOOL32 bDeclaredUsable = BuildPipelineInputLayout(ShaderInputs.GetData(), ShaderInputs.Num<UINT32>()
				, InDesc.InputLayouts, InDesc.InputLayoutCount, DeclaredLayouts, Pairs, UnmatchedDeclaredIndices);

			// A declaration that named no shader input is not an error (a stream may carry attributes
			// for another pass), but it is exactly what a mistyped semantic looks like, so it is worth
			// a warning rather than silence.
			for (UINT32 UnmatchedIndex = 0u, UnmatchedNum = UnmatchedDeclaredIndices.Num<UINT32>(); UnmatchedIndex < UnmatchedNum; UnmatchedIndex++)
			{
				const UINT32 DeclaredIndex = static_cast<UINT32>(UnmatchedDeclaredIndices[UnmatchedIndex]);
				EString Message = EString(PE_TEXT("[PipelineStateCache] input layout: declared element names no input of this shader, ignored: "));
				Message += MakeInputElementText(InDesc.InputLayouts[DeclaredIndex]);
				PE_LOG_WARN(Message);
			}

			for (UINT32 PairIndex = 0u, PairNum = Pairs.Num<UINT32>(); PairIndex < PairNum; PairIndex++)
			{
				const RInputElementPair& Pair = Pairs[PairIndex];
				// A pair with no declared element behind it is the shader's own layout; nothing to say.
				if ((Pair.Match == ERHIInputElementMatchType::RHI_INPUT_ELEMENT_MATCH) || (Pair.DataElementIndex < 0))
				{
					continue;
				}

				EString Message = EString(PE_TEXT("[PipelineStateCache] input layout: shader input "));
				Message += MakeInputElementText(ShaderInputs[static_cast<UINT32>(Pair.ShaderElementIndex)]);
				Message += EString(PE_TEXT(" -> "));
				Message += EString(GetInputElementMatchTypeString(Pair.Match));
				Message += EString(PE_TEXT("; declared element "));
				Message += MakeInputElementText(InDesc.InputLayouts[static_cast<UINT32>(Pair.DataElementIndex)]);
				// A fatal pairing is one the declaration cannot be honored for; a short element still
				// creates a pipeline, so it is reported without refusing the draw.
				if (IsInputElementMatchFatal(Pair.Match) != FALSE)
				{
					PE_LOG_ERROR(Message);
				}
				else
				{
					PE_LOG_WARN(Message);
				}
			}
			if (bDeclaredUsable == FALSE)
			{
				return nullptr;
			}

			// The merged layout is what the pipeline has to declare, and it only lives for this call:
			// both back-ends turn it into their own description immediately.
			RHIDesc.InputLayouts = DeclaredLayouts.GetData();
			RHIDesc.InputLayoutCount = DeclaredLayouts.Num<UINT32>();
		}

		IRRHIPipelineState* PipelineState = nullptr;
		if ((!Device->CreateGraphicsPipelineState(RHIDesc, &PipelineState)) || (!PipelineState))
		{
			return nullptr;
		}

		FPipelineStateEntry Entry;
		Entry.State = PipelineState;
		Entry.Program = InDesc.Program;
		States.Add(Hash, Entry);
		return PipelineState;
	}

	IRRHIPipelineState* RPipelineStateCache::GetOrCreateCompute(const EShaderProgram& InProgram)
	{
		if (!Device)
		{
			return nullptr;
		}

		const RPipelineLayout* Layout = LayoutCache.GetOrCreate(InProgram);
		if (!Layout)
		{
			return nullptr;
		}

		// The program identity and its layout decide a compute pipeline: there is no
		// fixed-function state to tell two of them apart.
		UINT64 Hash = 0ull;
		auto HashCombine = [&Hash](UINT64 InValue)
		{
			Hash ^= InValue + 0x9e3779b97f4a7c15ull + (Hash << 6) + (Hash >> 2);
		};
		union EPtrUint64
		{
			UINT64		_Uint64;
			const void*	_Ptr;
		};
		EPtrUint64 ProgramIdentity;
		ProgramIdentity._Ptr = (&InProgram);
		HashCombine(ProgramIdentity._Uint64);
		HashCombine(Layout->LayoutHash);

		FPipelineStateEntry Cached;
		if (States.FindValue(Hash, Cached) && Cached.State)
		{
			return Cached.State;
		}

		RRHIComputePipelineDesc RHIDesc;
		if (!FillComputePipelineShader(InProgram, RHIDesc))
		{
			return nullptr;
		}
		RHIDesc.RootSignature = Layout->RootSignature;

		IRRHIPipelineState* PipelineState = nullptr;
		if ((!Device->CreateComputePipelineState(RHIDesc, &PipelineState)) || (!PipelineState))
		{
			return nullptr;
		}

		FPipelineStateEntry Entry;
		Entry.State = PipelineState;
		Entry.Program = (&InProgram);
		States.Add(Hash, Entry);
		return PipelineState;
	}

}	// PigeonEngine
