#include <Pipeline/RPipelineLayout.h>

#include <RHI/IRRHIDevice.h>
#include <RHI/IRRHIRootSignature.h>
#include <Shader/ShaderBindingPlan.h>
#include <ShaderAsset/ShaderProgram.h>

namespace PigeonEngine
{

	namespace
	{
		// Descriptor table kinds the generator emits, in the order their roots are appended
		// after the constant blocks. The order is part of the layout contract: the binding side
		// reads the roots out of the layout info, and the plan write-back walks the same order.
		enum ELayoutTableKind : UINT8
		{
			LAYOUT_TABLE_SRV		= 0,
			LAYOUT_TABLE_UAV		= 1,
			LAYOUT_TABLE_SAMPLER	= 2,
			LAYOUT_TABLE_COUNT,
		};

		// One constant buffer register slot, accumulated over every stage that declares it.
		// Stages share a register space, so the same slot in two stages is one root parameter
		// visible to both; that is also why a block's visibility is the union below.
		struct FLayoutConstantBufferSlot
		{
			BOOL32	bUsed				= FALSE;	// declared by at least one stage
			BOOL32	bPlainUsed			= FALSE;	// declared as a bound buffer somewhere
			BOOL32	bRootConstant		= FALSE;	// declared as a root constant block somewhere
			UINT32	StageMask			= 0u;
			// Widest use of the slot. Only a root constant block's width reaches the root
			// signature; a constant buffer view carries no size.
			UINT32	RootConstantDwords	= 0u;
		};

		// One resource table kind. Only the widest register and the stage union matter, because
		// every resource of one kind lands in the same table.
		struct FLayoutTableSlot
		{
			BOOL32	bUsed		= FALSE;
			UINT32	StageMask	= 0u;
			UINT32	MaxSlot		= 0u;
		};

		// Root index of every parameter kind, as the generator assigned it. This is what the
		// plan write-back needs, so it is kept next to the desc while the desc is built.
		struct FLayoutRootIndices
		{
			UINT32	CbvRootBySlot[PIPELINE_MAX_CONSTANT_BUFFER_SLOTS] = {};
			UINT32	Constants32BitRoot	= RHI_INVALID_ROOT_INDEX;
			UINT32	TableRoot[LAYOUT_TABLE_COUNT] = { RHI_INVALID_ROOT_INDEX, RHI_INVALID_ROOT_INDEX, RHI_INVALID_ROOT_INDEX };
		};

		// Visibility of a root parameter is the union of the stages that declared it: one stage
		// maps to that stage, anything else has to be visible to all of them. A stage union
		// D3D12 cannot express (graphics and compute together) also lands on ALL, which is the
		// only value that makes the parameter visible to every stage that needs it.
		void TranslateStageMaskVisibility(const UINT32 InStageMask, ERHIShaderVisibility& OutVisibility) noexcept
		{
			switch (InStageMask)
			{
			case SHADER_STAGE_MASK_VERTEX:
				OutVisibility = ERHIShaderVisibility::RHI_SHADER_VISIBILITY_VERTEX;
				return;
			case SHADER_STAGE_MASK_PIXEL:
				OutVisibility = ERHIShaderVisibility::RHI_SHADER_VISIBILITY_PIXEL;
				return;
			case SHADER_STAGE_MASK_COMPUTE:
				OutVisibility = ERHIShaderVisibility::RHI_SHADER_VISIBILITY_COMPUTE;
				return;
			default:
				OutVisibility = ERHIShaderVisibility::RHI_SHADER_VISIBILITY_ALL;
				return;
			}
		}

		ERHIDescriptorRangeType TranslateTableKind(const ELayoutTableKind InKind) noexcept
		{
			switch (InKind)
			{
			case LAYOUT_TABLE_UAV:
				return ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_UAV;
			case LAYOUT_TABLE_SAMPLER:
				return ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_SAMPLER;
			default:
				return ERHIDescriptorRangeType::RHI_DESCRIPTOR_RANGE_SRV;
			}
		}

		// Rewrites the root / table indices of one stage's binding plan from the layout the
		// generator assigned. Idempotent: the same layout always produces the same indices, so a
		// program that is laid out twice ends up with the same plan.
		void WriteBackBindingPlan(EShaderBindingPlan& InOutPlan, const FLayoutRootIndices& InRoots)
		{
			for (UINT32 BindingIndex = 0u, BindingNum = InOutPlan.GetConstantBufferBindingNum(); BindingIndex < BindingNum; BindingIndex++)
			{
				EShaderConstantBufferBinding* Binding = InOutPlan.GetMutableConstantBufferBindingByIndex(BindingIndex);
				if (!Binding)
				{
					continue;
				}
				if (Binding->Kind == EShaderConstantBufferBindingKindType::SHADER_CONSTANT_BUFFER_BINDING_KIND_ROOT_CONSTANTS)
				{
					Binding->RootIndex = InRoots.Constants32BitRoot;
					continue;
				}
				if (Binding->RegisterSlot < PIPELINE_MAX_CONSTANT_BUFFER_SLOTS)
				{
					Binding->RootIndex = InRoots.CbvRootBySlot[Binding->RegisterSlot];
				}
			}

			for (UINT32 BindingIndex = 0u, BindingNum = InOutPlan.GetResourceBindingNum(); BindingIndex < BindingNum; BindingIndex++)
			{
				EShaderResourceBinding* Binding = InOutPlan.GetMutableResourceBindingByIndex(BindingIndex);
				if (!Binding)
				{
					continue;
				}
				switch (Binding->Kind)
				{
				case EShaderBindingKindType::SHADER_BINDING_KIND_UAV:
					Binding->TableRootIndex = InRoots.TableRoot[LAYOUT_TABLE_UAV];
					break;
				case EShaderBindingKindType::SHADER_BINDING_KIND_SAMPLER:
					Binding->TableRootIndex = InRoots.TableRoot[LAYOUT_TABLE_SAMPLER];
					break;
				default:
					Binding->TableRootIndex = InRoots.TableRoot[LAYOUT_TABLE_SRV];
					break;
				}
				// The tables start at register 0 and are as wide as the highest register used,
				// so a resource's offset inside its table is its register slot.
				Binding->OffsetInTable = Binding->RegisterSlot;
			}
		}

	}

	RPipelineLayoutCache::RPipelineLayoutCache()
		: Device(nullptr)
		, Layouts{}
		, LayoutPool{}
	{
	}

	RPipelineLayoutCache::~RPipelineLayoutCache()
	{
		InvalidateAll();
	}

	void RPipelineLayoutCache::SetDevice(IRRHIDevice* InDevice)
	{
		if (Device != InDevice)
		{
			// A cache serves one device: layouts carry the layout info that device derived, so
			// switching devices has to drop them rather than hand out indices from the other
			// back-end's signature.
			InvalidateAll();
		}
		Device = InDevice;
	}

	RPipelineLayout* RPipelineLayoutCache::FindLayout(const UINT64 InLayoutHash) const
	{
		RPipelineLayout* Layout = nullptr;
		Layouts.FindValue(InLayoutHash, Layout);
		return Layout;
	}

	void RPipelineLayoutCache::Invalidate(const EShaderProgram& InProgram)
	{
		(void)InProgram;
		// Layouts are shared by hash, so a program cannot drop "its" record without possibly
		// dropping one another program is using. Regeneration is driven by InvalidateAll; the
		// hook stays here because the hot reload path is what will need it.
	}

	void RPipelineLayoutCache::InvalidateAll()
	{
		for (RPipelineLayout* Layout : LayoutPool)
		{
			delete Layout;
		}
		LayoutPool.Empty();
		Layouts.Empty();
	}

	const RPipelineLayout* RPipelineLayoutCache::GetOrCreate(const EShaderProgram& InProgram)
	{
		if ((!Device) || (!InProgram.IsValid()))
		{
			// No device to create a signature with, or no compiled stage to describe: there is
			// nothing a layout could be generated from.
			return nullptr;
		}

		// ---- Collect the union of every valid stage. ----
		FLayoutConstantBufferSlot CBSlots[PIPELINE_MAX_CONSTANT_BUFFER_SLOTS];
		FLayoutTableSlot Tables[LAYOUT_TABLE_COUNT];
		BOOL32 bHasVertexStage = FALSE;
		BOOL32 bHasRootConstantSlot = FALSE;
		UINT32 RootConstantSlot = 0u;

		const EShaderStageType StageTypes[] =
		{
			EShaderStageType::SHADER_STAGE_VERTEX,
			EShaderStageType::SHADER_STAGE_PIXEL,
			EShaderStageType::SHADER_STAGE_COMPUTE,
		};

		for (UINT32 StageIndex = 0u; StageIndex < PE_ARRAYSIZE(StageTypes); StageIndex++)
		{
			const EShaderStageType StageType = StageTypes[StageIndex];
			const EShaderStageEntry* StageEntry = InProgram.FindStageEntry(StageType);
			if ((!StageEntry) || (!StageEntry->bValid))
			{
				continue;
			}
			UINT32 StageBit = 0u;
			if (!GetShaderStageMaskBit(StageType, StageBit))
			{
				continue;
			}
			if (StageType == EShaderStageType::SHADER_STAGE_VERTEX)
			{
				bHasVertexStage = TRUE;
			}

			const EShaderReflection& Reflection = StageEntry->Reflection;

			// Constant buffers. The reflection slot is the folded, contiguous register the
			// compiler assigned, so it indexes the accumulator directly.
			for (UINT32 CbIndex = 0u, CbNum = Reflection.ConstantBuffers.Num<UINT32>(); CbIndex < CbNum; CbIndex++)
			{
				const EShaderReflectionConstantBuffer& ConstantBuffer = Reflection.ConstantBuffers[CbIndex];
				if (ConstantBuffer.bCompilerCulled)
				{
					continue;
				}
				if (ConstantBuffer.Slot >= PIPELINE_MAX_CONSTANT_BUFFER_SLOTS)
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: constant buffer slot exceeds the pipeline's register ceiling."));
					return nullptr;
				}

				FLayoutConstantBufferSlot& Slot = CBSlots[ConstantBuffer.Slot];
				Slot.bUsed = TRUE;
				Slot.StageMask |= StageBit;
				if (ConstantBuffer.RootConstant)
				{
					// A single root signature parameter carries 32 bit constants, so two stages
					// cannot each bring their own root constant block in different slots.
					if (bHasRootConstantSlot && (RootConstantSlot != ConstantBuffer.Slot))
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: more than one root constant block across the program's stages."));
						return nullptr;
					}
					bHasRootConstantSlot = TRUE;
					RootConstantSlot = ConstantBuffer.Slot;
					Slot.bRootConstant = TRUE;
					const UINT32 Dwords = ((ConstantBuffer.ByteSize + 3u) / 4u);
					if (Dwords == 0u)
					{
						// A root constant block is a width in DWORDs, so an empty one cannot be
						// expressed as a root parameter at all.
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: a root constant block is empty."));
						return nullptr;
					}
					if (Dwords > Slot.RootConstantDwords)
					{
						Slot.RootConstantDwords = Dwords;
					}
				}
				else
				{
					Slot.bPlainUsed = TRUE;
				}
			}

			// Resources. Textures and structured buffers share the SRV register space (and the
			// RW ones the UAV space), which is what the compiler's folding already assumes.
			for (UINT32 GroupIndex = 0u; GroupIndex < 2u; GroupIndex++)
			{
				const BOOL32 bIsUAV = ((GroupIndex != 0u) ? TRUE : FALSE);
				const ELayoutTableKind TableKind = (bIsUAV ? LAYOUT_TABLE_UAV : LAYOUT_TABLE_SRV);
				const UINT32 SlotCeiling = (bIsUAV ? PIPELINE_MAX_UAV_SLOTS : PIPELINE_MAX_SRV_SLOTS);

				const TArray<EShaderReflectionTexture>& Textures = (bIsUAV ? Reflection.RWTextures : Reflection.Textures);
				for (UINT32 TextureIndex = 0u, TextureNum = Textures.Num<UINT32>(); TextureIndex < TextureNum; TextureIndex++)
				{
					const EShaderReflectionTexture& Texture = Textures[TextureIndex];
					if (Texture.bCompilerCulled)
					{
						continue;
					}
					if (Texture.Slot >= SlotCeiling)
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: shader resource slot exceeds the pipeline's register ceiling."));
						return nullptr;
					}
					Tables[TableKind].bUsed = TRUE;
					Tables[TableKind].StageMask |= StageBit;
					if (Texture.Slot > Tables[TableKind].MaxSlot)
					{
						Tables[TableKind].MaxSlot = Texture.Slot;
					}
				}

				const TArray<EShaderReflectionResource>& Resources = (bIsUAV ? Reflection.RWResources : Reflection.Resources);
				for (UINT32 ResourceIndex = 0u, ResourceNum = Resources.Num<UINT32>(); ResourceIndex < ResourceNum; ResourceIndex++)
				{
					const EShaderReflectionResource& Resource = Resources[ResourceIndex];
					if (Resource.bCompilerCulled)
					{
						continue;
					}
					if (Resource.Slot >= SlotCeiling)
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: shader resource slot exceeds the pipeline's register ceiling."));
						return nullptr;
					}
					Tables[TableKind].bUsed = TRUE;
					Tables[TableKind].StageMask |= StageBit;
					if (Resource.Slot > Tables[TableKind].MaxSlot)
					{
						Tables[TableKind].MaxSlot = Resource.Slot;
					}
				}
			}

			for (UINT32 SamplerIndex = 0u, SamplerNum = Reflection.Samplers.Num<UINT32>(); SamplerIndex < SamplerNum; SamplerIndex++)
			{
				const EShaderReflectionSampler& Sampler = Reflection.Samplers[SamplerIndex];
				if (Sampler.bCompilerCulled)
				{
					continue;
				}
				if (Sampler.Slot >= PIPELINE_MAX_SAMPLER_SLOTS)
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: sampler slot exceeds the pipeline's register ceiling."));
					return nullptr;
				}
				Tables[LAYOUT_TABLE_SAMPLER].bUsed = TRUE;
				Tables[LAYOUT_TABLE_SAMPLER].StageMask |= StageBit;
				if (Sampler.Slot > Tables[LAYOUT_TABLE_SAMPLER].MaxSlot)
				{
					Tables[LAYOUT_TABLE_SAMPLER].MaxSlot = Sampler.Slot;
				}
			}
		}

		// A slot that one stage feeds through 32 bit constants and another reads as a bound
		// buffer has no single root parameter that serves both.
		for (UINT32 SlotIndex = 0u; SlotIndex < PIPELINE_MAX_CONSTANT_BUFFER_SLOTS; SlotIndex++)
		{
			if (CBSlots[SlotIndex].bUsed && CBSlots[SlotIndex].bPlainUsed && CBSlots[SlotIndex].bRootConstant)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: a constant buffer slot is a root constant block in one stage and a bound buffer in another."));
				return nullptr;
			}
		}

		// ---- Turn the union into a root parameter list. ----
		RRHIRootSignatureDesc Desc;
		Desc.bAllowInputAssembler = bHasVertexStage;
		// A bindless variant indexes the descriptor heaps directly, so it gets the flags but no
		// tables at all (see RRHIRootSignatureDesc::bHeapDirectlyIndexed).
		Desc.bHeapDirectlyIndexed = InProgram.IsBindlessVariant();

		FLayoutRootIndices Roots;
		UINT32 RootCost = 0u;

		// Constant buffer views first, one root per used slot in register order.
		for (UINT32 SlotIndex = 0u; SlotIndex < PIPELINE_MAX_CONSTANT_BUFFER_SLOTS; SlotIndex++)
		{
			const FLayoutConstantBufferSlot& Slot = CBSlots[SlotIndex];
			if ((!Slot.bUsed) || Slot.bRootConstant)
			{
				continue;
			}

			RRHIRootParameterDesc Parameter;
			Parameter.Type = ERHIRootParameterType::RHI_ROOT_PARAMETER_CBV;
			// The root keeps the register the reflection declared: two roots that map to the
			// same register slot are the same buffer seen from two stages, never a remap.
			Parameter.ShaderRegister = SlotIndex;
			Parameter.RegisterSpace = 0u;
			TranslateStageMaskVisibility(Slot.StageMask, Parameter.Visibility);

			Roots.CbvRootBySlot[SlotIndex] = Desc.Parameters.Num<UINT32>();
			RootCost += 2u;
			Desc.Parameters.Add(Parameter);
		}

		// Then the root constant block, if the program has one. Its width only has to cover the
		// widest stage that uses it; a stage pushing fewer values stays inside it.
		if (bHasRootConstantSlot)
		{
			const FLayoutConstantBufferSlot& Slot = CBSlots[RootConstantSlot];

			RRHIRootParameterDesc Parameter;
			Parameter.Type = ERHIRootParameterType::RHI_ROOT_PARAMETER_32BIT_CONSTANTS;
			Parameter.ShaderRegister = RootConstantSlot;
			Parameter.RegisterSpace = 0u;
			Parameter.Num32BitValues = Slot.RootConstantDwords;
			TranslateStageMaskVisibility(Slot.StageMask, Parameter.Visibility);

			Roots.Constants32BitRoot = Desc.Parameters.Num<UINT32>();
			RootCost += Parameter.Num32BitValues;
			Desc.Parameters.Add(Parameter);
		}

		// Descriptor tables last.
		if (!Desc.bHeapDirectlyIndexed)
		{
			for (UINT32 TableIndex = 0u; TableIndex < LAYOUT_TABLE_COUNT; TableIndex++)
			{
				const FLayoutTableSlot& Table = Tables[TableIndex];
				if (!Table.bUsed)
				{
					continue;
				}

				RRHIRootParameterDesc Parameter;
				Parameter.Type = ERHIRootParameterType::RHI_ROOT_PARAMETER_TABLE;
				Parameter.Range.Type = TranslateTableKind(static_cast<ELayoutTableKind>(TableIndex));
				// A table that starts at register 0 and is as wide as the highest register used
				// is what makes a resource's offset inside it equal to its register slot.
				Parameter.Range.BaseShaderRegister = 0u;
				Parameter.Range.RegisterSpace = 0u;
				Parameter.Range.NumDescriptors = (Table.MaxSlot + 1u);
				TranslateStageMaskVisibility(Table.StageMask, Parameter.Visibility);

				Roots.TableRoot[TableIndex] = Desc.Parameters.Num<UINT32>();
				RootCost += 1u;
				Desc.Parameters.Add(Parameter);
			}
		}

		// Root parameter budget. The back-end checks its own limit before creating anything;
		// failing here keeps the layout from being attempted at all and names the reason.
		if (RootCost > PIPELINE_MAX_ROOT_DWORDS)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: the program's parameter space does not fit the root signature budget."));
			return nullptr;
		}

		Desc.LayoutHash = HashRHIRootSignatureDesc(Desc);

		// ---- Reuse the layout if it is already live, otherwise build the signature. ----
		RPipelineLayout* Layout = FindLayout(Desc.LayoutHash);
		if (!Layout)
		{
			IRRHIRootSignature* RootSignature = nullptr;
			if (!Device->CreateRootSignature(Desc, &RootSignature))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Pipeline layout: the device refused the generated root signature."));
				return nullptr;
			}

			Layout = new RPipelineLayout();
			Layout->LayoutHash = Desc.LayoutHash;
			Layout->RootSignature = RootSignature;
			if (RootSignature)
			{
				// The back-end derived the binding layout from the same parameter list, so it is
				// the one the command list and the PSOs will use; taking it from there keeps the
				// answer single sourced. D3D11 hands back the no-root-signature layout.
				Layout->LayoutInfo = RootSignature->GetLayoutInfo();
			}

			LayoutPool.Add(Layout);
			Layouts.Add(Desc.LayoutHash, Layout);
		}

		// ---- Rewrite every stage's binding plan to the layout's indices. ----
		// Done on every call on purpose: the plans are the programs' own derived state, and the
		// rewrite is idempotent and cheap, so a program can never bind against a stale root.
		//
		// Only a back-end that actually has root parameters gets the rewrite, and the layout info
		// is what says so: a non-zero parameter count is a signature whose roots the binding side
		// has to use. On a back-end without root signatures (D3D11) the layout info stays zero,
		// and there a root index is a register slot, so the collected defaults are already the
		// indices that back-end binds with and rewriting them would break every binding. The same
		// zero case on a root-signature back-end means the program has no parameters at all, so
		// there is nothing to rewrite either.
		if (Layout->LayoutInfo.ParameterCount > 0u)
		{
			for (UINT32 StageIndex = 0u; StageIndex < PE_ARRAYSIZE(StageTypes); StageIndex++)
			{
				EShaderBindingPlan* Plan = InProgram.GetMutableBindingPlan(StageTypes[StageIndex]);
				if (Plan && Plan->IsValid())
				{
					WriteBackBindingPlan(*Plan, Roots);
				}
			}
		}

		return Layout;
	}

}	// PigeonEngine
