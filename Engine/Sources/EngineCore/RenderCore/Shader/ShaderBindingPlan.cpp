#include <Shader/ShaderBindingPlan.h>

#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	namespace
	{
		// One module's claim table. The claim array has static storage duration in the module's
		// own translation unit; the registry only points at it, so no copy and no lifetime
		// management is involved.
		struct FShaderBindingClaimTable
		{
		public:
			const FShaderBindingClaim*	Claims		= nullptr;
			UINT32						ClaimNum	= 0u;
			EName						ScopeName;
		};

		// Function local static on purpose: the registrars run during static initialization,
		// and a namespace scope container would depend on the order this translation unit's
		// dynamic initialization happens in relative to theirs.
		TArray<FShaderBindingClaimTable>&	GetShaderBindingClaimTables()
		{
			static TArray<FShaderBindingClaimTable> ClaimTables;
			return ClaimTables;
		}

		// Reports the two plan-time defects the three-state model can name. Both mean no C++
		// module can ever provide the binding for the program as compiled, which is a source
		// defect rather than a value that merely has not arrived yet.
		void	AuditShaderBindingClaim(const EShaderReflection& InReflection, const EName& InName
			, const FShaderBindingClaimResolution& InResolution)
		{
			EString StageName;
			if (TranslateShaderStageToString(InReflection.Stage, StageName) == FALSE)
			{
				StageName = EString(PE_TEXT("?"));
			}

			if (InResolution.State == EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_UNCLAIMED)
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] '")) + InReflection.ShaderName.ToString()
					+ EString(PE_TEXT("' stage '")) + StageName
					+ EString(PE_TEXT("' reads binding '")) + InName.ToString()
					+ EString(PE_TEXT("' that no C++ module claims and the reflection does not expose: no publisher can ever provide it.")));
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("A shader binding is neither claimed nor exposed.")), (FALSE));
			}
			else if ((InResolution.ClaimScopeNum >= 2u) && (InResolution.OnlyOneClaimNum > 0u))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] '")) + InReflection.ShaderName.ToString()
					+ EString(PE_TEXT("' stage '")) + StageName
					+ EString(PE_TEXT("' binding '")) + InName.ToString()
					+ EString(PE_TEXT("' is claimed by ")) + ToString(InResolution.ClaimScopeNum)
					+ EString(PE_TEXT(" scopes while at least one claim is ONLY_ONE; overlapping claims must all be OVERLAPPING.")));
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("A shader binding with an ONLY_ONE policy has more than one claiming scope.")), (FALSE));
			}
		}

		// Resolves one binding's ownership and stores the runtime half of the answer. The audit
		// itself runs here as well because this is the one point that has the reflection (and
		// with it the program identity) together with the resolved name.
		void	ApplyBindingClaim(const EShaderReflection& InReflection, const EName& InName
			, const BOOL32 InIsExposure, EShaderResourceBinding& OutBinding)
		{
			const FShaderBindingClaimResolution Resolution = ResolveShaderBindingClaim(InName, InIsExposure);
			OutBinding.ClaimState = Resolution.State;
			OutBinding.ClaimScopeNum = Resolution.ClaimScopeNum;
			OutBinding.SoleClaimScope = Resolution.SoleClaimScope;
			AuditShaderBindingClaim(InReflection, InName, Resolution);
		}
	}

	void	RegisterShaderBindingClaims(const FShaderBindingClaim* InClaims, const UINT32 InClaimNum, const EName& InScopeName)
	{
		if ((!InClaims) || (InClaimNum == 0u) || (InScopeName.IsValid() == FALSE))
		{
			return;
		}
		FShaderBindingClaimTable ClaimTable;
		ClaimTable.Claims = InClaims;
		ClaimTable.ClaimNum = InClaimNum;
		ClaimTable.ScopeName = InScopeName;
		GetShaderBindingClaimTables().Add(ClaimTable);
	}

	FShaderBindingClaimResolution	ResolveShaderBindingClaim(const EName& InName, const BOOL32 InIsExposure)
	{
		FShaderBindingClaimResolution Resolution;
		const TArray<FShaderBindingClaimTable>& ClaimTables = GetShaderBindingClaimTables();
		for (UINT32 TableIndex = 0u, TableNum = ClaimTables.Num<UINT32>(); TableIndex < TableNum; TableIndex++)
		{
			const FShaderBindingClaimTable& ClaimTable = ClaimTables[TableIndex];
			BOOL32 bClaimsName = FALSE;
			BOOL32 bHasOnlyOnePolicy = FALSE;
			for (UINT32 ClaimIndex = 0u; ClaimIndex < ClaimTable.ClaimNum; ClaimIndex++)
			{
				const FShaderBindingClaim& Claim = ClaimTable.Claims[ClaimIndex];
				if (Claim.Name != InName)
				{
					continue;
				}
				bClaimsName = TRUE;
				if (Claim.Policy == EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE)
				{
					bHasOnlyOnePolicy = TRUE;
				}
			}
			if (bClaimsName == FALSE)
			{
				continue;
			}
			// One scope claims the name once, however many of its own entries carry it. The
			// first claimant is remembered so a single claimer can be compared by name later.
			if (Resolution.ClaimScopeNum == 0u)
			{
				Resolution.SoleClaimScope = ClaimTable.ScopeName;
			}
			Resolution.ClaimScopeNum++;
			if (bHasOnlyOnePolicy != FALSE)
			{
				Resolution.OnlyOneClaimNum++;
			}
		}

		if (Resolution.ClaimScopeNum == 0u)
		{
			Resolution.SoleClaimScope = EName();
			Resolution.State = ((InIsExposure != FALSE)
				? EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_EXPOSURE
				: EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_UNCLAIMED);
		}
		else
		{
			Resolution.State = EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_CLAIMED;
			if (Resolution.ClaimScopeNum != 1u)
			{
				Resolution.SoleClaimScope = EName();
			}
		}
		return Resolution;
	}

	EShaderBindingPlan::EShaderBindingPlan()
		: Stage(EShaderStageType::SHADER_STAGE_VERTEX)
		, bBindless(FALSE)
		, bValid(FALSE)
		, ConstantBufferBindings{}
		, ResourceBindings{}
	{
	}

	EShaderBindingPlan::~EShaderBindingPlan()
	{
		Reset();
	}

	void EShaderBindingPlan::Reset()
	{
		Stage = EShaderStageType::SHADER_STAGE_VERTEX;
		bBindless = FALSE;
		bValid = FALSE;
		if (ConstantBufferBindings.Num<UINT32>() > 0u)
		{
			ConstantBufferBindings.Empty();
		}
		if (ResourceBindings.Num<UINT32>() > 0u)
		{
			ResourceBindings.Empty();
		}
	}

	// Copies one declaration's bindless statement into its binding and folds it into the
	// plan-level answer. Written once because every declaration section carries the same triple,
	// and because "any resource is index driven" is the only thing the plan level needs to know
	// about it.
	template<typename _TReflection>
	static void ApplyBindingBindless(const _TReflection& InReflection, EShaderResourceBinding& OutBinding, BOOL32& InOutBindless)
	{
		OutBinding.bBindless = InReflection.Bindless;
		OutBinding.BindlessConstantBuffer = InReflection.BindConstantBuffer;
		if (OutBinding.bBindless)
		{
			InOutBindless = TRUE;
		}
	}

	BOOL32 EShaderBindingPlan::BuildFromReflection(const EShaderReflection& InReflection)
	{
		Reset();

		Stage = InReflection.Stage;
		// Whether the stage is bindless at all comes from the reflection, never from a caller:
		// the per-resource flags are the compiler's own statement about this stage, so an answer
		// from anywhere else could disagree with the shader the plan describes.
		bBindless = FALSE;

		// Constant buffers. A culled buffer is not present in the compiled shader, so
		// binding it would be wrong; it stays visible in the reflection only.
		for (UINT32 ConstantBufferIndex = 0u, ConstantBufferNum = InReflection.ConstantBuffers.Num<UINT32>(); ConstantBufferIndex < ConstantBufferNum; ConstantBufferIndex++)
		{
			const EShaderReflectionConstantBuffer& ConstantBuffer = InReflection.ConstantBuffers[ConstantBufferIndex];
			if (ConstantBuffer.bCompilerCulled)
			{
				continue;
			}

			EShaderConstantBufferBinding Binding;
			Binding.BlockName = ConstantBuffer.Name;
			Binding.RegisterSlot = ConstantBuffer.Slot;
			// Register bound default: the legacy shared root signature maps one root
			// parameter per constant buffer register, so the root index equals the register
			// slot. A program that gets a signature of its own has this overwritten by the
			// layout generator with the root index that signature actually uses.
			Binding.RootIndex = ConstantBuffer.Slot;
			Binding.ByteSize = ConstantBuffer.ByteSize;
			// Root constant blocks have no view to bind; the flag travels with the binding
			// so the render layer does not have to walk the reflection to find them, and the
			// kind is the mode the binding side branches on.
			Binding.RootConstant = ConstantBuffer.RootConstant;
			Binding.Kind = (ConstantBuffer.RootConstant
				? EShaderConstantBufferBindingKindType::SHADER_CONSTANT_BUFFER_BINDING_KIND_ROOT_CONSTANTS
				: EShaderConstantBufferBindingKindType::SHADER_CONSTANT_BUFFER_BINDING_KIND_CBV);
			Binding.Num32BitValues = ((ConstantBuffer.ByteSize + 3u) / 4u);
			ConstantBufferBindings.Add(Binding);
		}

		// Shader resource views and unordered access views. A culled entry is not register bound:
		// the compiler marks everything the DXIL reflection does not list. A bindless resource is
		// exactly that - it is reached through the descriptor heap, so it has no binding to list -
		// and it still has to be in the plan, because the heap index write and the root signature's
		// heap flags are both driven from here. Only a culled resource that is not index driven
		// drops, which is what "declared but unused" means.
		for (UINT32 TextureIndex = 0u, TextureNum = InReflection.Textures.Num<UINT32>(); TextureIndex < TextureNum; TextureIndex++)
		{
			const EShaderReflectionTexture& Texture = InReflection.Textures[TextureIndex];
			if ((Texture.bCompilerCulled != FALSE) && (Texture.Bindless == FALSE))
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Texture.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_SRV;
			Binding.RegisterSlot = Texture.Slot;
			Binding.TextureType = Texture.Type;
			Binding.bIsTexture = TRUE;
			ApplyBindingBindless(Texture, Binding, bBindless);
			ApplyBindingClaim(InReflection, Binding.Name, Texture.Exposure, Binding);
			ResourceBindings.Add(Binding);
		}
		for (UINT32 TextureIndex = 0u, TextureNum = InReflection.RWTextures.Num<UINT32>(); TextureIndex < TextureNum; TextureIndex++)
		{
			const EShaderReflectionTexture& Texture = InReflection.RWTextures[TextureIndex];
			if ((Texture.bCompilerCulled != FALSE) && (Texture.Bindless == FALSE))
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Texture.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_UAV;
			Binding.RegisterSlot = Texture.Slot;
			Binding.TextureType = Texture.Type;
			Binding.bIsTexture = TRUE;
			ApplyBindingBindless(Texture, Binding, bBindless);
			ApplyBindingClaim(InReflection, Binding.Name, Texture.Exposure, Binding);
			ResourceBindings.Add(Binding);
		}
		for (UINT32 ResourceIndex = 0u, ResourceNum = InReflection.Resources.Num<UINT32>(); ResourceIndex < ResourceNum; ResourceIndex++)
		{
			const EShaderReflectionResource& Resource = InReflection.Resources[ResourceIndex];
			if ((Resource.bCompilerCulled != FALSE) && (Resource.Bindless == FALSE))
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Resource.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_SRV;
			Binding.RegisterSlot = Resource.Slot;
			Binding.ResourceKind = Resource.Kind;
			ApplyBindingBindless(Resource, Binding, bBindless);
			ApplyBindingClaim(InReflection, Binding.Name, Resource.Exposure, Binding);
			ResourceBindings.Add(Binding);
		}
		for (UINT32 ResourceIndex = 0u, ResourceNum = InReflection.RWResources.Num<UINT32>(); ResourceIndex < ResourceNum; ResourceIndex++)
		{
			const EShaderReflectionResource& Resource = InReflection.RWResources[ResourceIndex];
			if ((Resource.bCompilerCulled != FALSE) && (Resource.Bindless == FALSE))
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Resource.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_UAV;
			Binding.RegisterSlot = Resource.Slot;
			Binding.ResourceKind = Resource.Kind;
			ApplyBindingBindless(Resource, Binding, bBindless);
			ApplyBindingClaim(InReflection, Binding.Name, Resource.Exposure, Binding);
			ResourceBindings.Add(Binding);
		}

		// Samplers. Their slot is independent from the texture slot, so they get their
		// own entries rather than being folded into the texture binding.
		for (UINT32 SamplerIndex = 0u, SamplerNum = InReflection.Samplers.Num<UINT32>(); SamplerIndex < SamplerNum; SamplerIndex++)
		{
			const EShaderReflectionSampler& Sampler = InReflection.Samplers[SamplerIndex];
			if ((Sampler.bCompilerCulled != FALSE) && (Sampler.Bindless == FALSE))
			{
				continue;
			}
			EShaderResourceBinding Binding;
			Binding.Name = Sampler.Name;
			Binding.Kind = EShaderBindingKindType::SHADER_BINDING_KIND_SAMPLER;
			Binding.RegisterSlot = Sampler.Slot;
			Binding.SamplerDesc = Sampler.Desc;
			ApplyBindingBindless(Sampler, Binding, bBindless);
			// Samplers stay outside the claim surface: the renderer binds its sampler table once
			// per frame, so no per draw module is expected to publish one, and a sampler carries
			// no object an engine default could stand in for. The binding keeps the unclaimed
			// default state, and runtime reporting already excludes the sampler kind.
			ResourceBindings.Add(Binding);
		}

		// Table offsets start out equal to the register slot: the legacy shared root signature
		// gives every table range a base register of 0, so a resource sits at its register
		// inside its table. The layout generator rewrites this together with the table root
		// once the program has a signature of its own.
		for (UINT32 BindingIndex = 0u, BindingNum = ResourceBindings.Num<UINT32>(); BindingIndex < BindingNum; BindingIndex++)
		{
			ResourceBindings[BindingIndex].OffsetInTable = ResourceBindings[BindingIndex].RegisterSlot;
		}

		bValid = TRUE;
		return TRUE;
	}

	const EShaderConstantBufferBinding* EShaderBindingPlan::GetConstantBufferBindingByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= ConstantBufferBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ConstantBufferBindings[InIndex]));
	}

	const EShaderResourceBinding* EShaderBindingPlan::GetResourceBindingByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= ResourceBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ResourceBindings[InIndex]));
	}

	EShaderConstantBufferBinding* EShaderBindingPlan::GetMutableConstantBufferBindingByIndex(const UINT32 InIndex)
	{
		if (InIndex >= ConstantBufferBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ConstantBufferBindings[InIndex]));
	}

	EShaderResourceBinding* EShaderBindingPlan::GetMutableResourceBindingByIndex(const UINT32 InIndex)
	{
		if (InIndex >= ResourceBindings.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(ResourceBindings[InIndex]));
	}

	const EShaderConstantBufferBinding* EShaderBindingPlan::FindConstantBufferBinding(const EName& InBlockName) const
	{
		for (UINT32 BindIdx = 0u, BindNum = ConstantBufferBindings.Num<UINT32>(); BindIdx < BindNum; BindIdx++)
		{
			if (ConstantBufferBindings[BindIdx].BlockName == InBlockName)
			{
				return (&(ConstantBufferBindings[BindIdx]));
			}
		}
		return nullptr;
	}

	const EShaderResourceBinding* EShaderBindingPlan::FindResourceBinding(const EName& InName) const
	{
		for (UINT32 BindIdx = 0u, BindNum = ResourceBindings.Num<UINT32>(); BindIdx < BindNum; BindIdx++)
		{
			if (ResourceBindings[BindIdx].Name == InName)
			{
				return (&(ResourceBindings[BindIdx]));
			}
		}
		return nullptr;
	}

	BOOL32 EShaderBindingPlan::FindConstantBufferRootIndex(const EName& InBlockName, UINT32& OutRootIndex) const
	{
		const EShaderConstantBufferBinding* Binding = FindConstantBufferBinding(InBlockName);
		if (!Binding)
		{
			return FALSE;
		}
		OutRootIndex = Binding->RootIndex;
		return TRUE;
	}

} // namespace PigeonEngine
