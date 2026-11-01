#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RHI/RHITypes.h>
#include <ShaderAsset/ShaderReflection.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - binding plan.
	*
	* One plan per (program, stage), derived once from that stage's reflection and
	* then shared and immutable. It is the only place that knows where a parameter
	* lands in the pipeline, so render code never does slot arithmetic.
	*
	* Two binding paths are supported:
	*   - register bound (default): root index equals the register slot reported by
	*     the reflection, resources are bound by their reflected register slot.
	*   - bindless: the engine binds a fixed common constant buffer plus the root
	*     signature parameters it owns and indexes its own resource tables. The plan
	*     only records the mode; the mechanism itself belongs to the render layer.
	*     The mode is read from data (the variant defines in the manifest), never
	*     from a hardcoded target check.
	*
	* Root / table indices start out as the register bound values the legacy shared
	* root signature uses, and are overwritten by the layout generator with the real
	* indices of the program's own signature (see Pipeline/RPipelineLayout.h). The
	* overwrite is idempotent, so a program that was laid out once keeps the same
	* indices for its lifetime.
	*/

	enum EShaderBindingKindType : UINT8
	{
		SHADER_BINDING_KIND_SRV		= 0,
		SHADER_BINDING_KIND_UAV		= 1,
		SHADER_BINDING_KIND_SAMPLER	= 2,
		SHADER_BINDING_KIND_COUNT,
	};

	/*
	* Three-state ownership of one resource binding, resolved once when the plan is built.
	*
	*   CLAIMED    a C++ module registered the name, so exactly that module is expected to
	*              publish a value. A publisher that is the only claimant reports a missing
	*              value; a publisher that is not one of the claimants stays silent.
	*   EXPOSURE   nothing claims the name, but the reflection marked it exposed. That is the
	*              compiler's statement that the declaration belongs to the editor side (a
	*              material's texture, an artist authored parameter), which has no static
	*              claim table because its names cannot be enumerated from C++. A missing
	*              value is a legal intermediate state and falls back to an engine default.
	*   UNCLAIMED  neither. No module can ever publish it, so the plan build reports it as a
	*              defect and asserts.
	*/
	enum EShaderBindingClaimStateType : UINT8
	{
		SHADER_BINDING_CLAIM_STATE_UNCLAIMED	= 0,
		SHADER_BINDING_CLAIM_STATE_CLAIMED		= 1,
		SHADER_BINDING_CLAIM_STATE_EXPOSURE		= 2,
		SHADER_BINDING_CLAIM_STATE_COUNT,
	};

	// What a claim allows. ONLY_ONE says the name has exactly one owner in the whole engine;
	// OVERLAPPING says several modules legitimately publish the same name for different draws
	// (a mesh's bones and a procedural factory's bones, an instance value and its fallback),
	// so the runtime cannot tell which one a given draw was supposed to provide.
	enum EShaderBindingClaimPolicyType : UINT8
	{
		SHADER_BINDING_CLAIM_ONLY_ONE		= 0,
		SHADER_BINDING_CLAIM_OVERLAPPING	= 1,
		SHADER_BINDING_CLAIM_POLICY_COUNT,
	};

	// One name a module promises to publish. Scope identity is the module's own EName, so the
	// same name claimed by two modules is representable without a shared owner enumeration.
	struct FShaderBindingClaim
	{
	public:
		EName							Name;
		EShaderBindingClaimPolicyType	Policy	= EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE;
	};

	// The answer the plan build stores for one binding. ClaimScopeNum counts distinct claiming
	// scopes; SoleClaimScope is valid only when that count is one, which is what the runtime
	// compares a publisher against.
	struct FShaderBindingClaimResolution
	{
	public:
		EShaderBindingClaimStateType	State			= EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_UNCLAIMED;
		UINT32							ClaimScopeNum	= 0u;
		UINT32							OnlyOneClaimNum	= 0u;
		EName							SoleClaimScope;
	};

	// Registration is static initialization: every module's claim table is a namespace scope
	// array in its own translation unit, and the registrar object below runs before main. The
	// registry itself is a function local static, so it is built on first use and does not
	// depend on the order the compiler initializes translation units in.
	extern void	RegisterShaderBindingClaims(const FShaderBindingClaim* InClaims, const UINT32 InClaimNum, const EName& InScopeName);
	// Resolves one name against every registered table and combines the answer with the
	// reflection's Exposure flag. Called once per resource binding by the plan build.
	PE_NODISCARD extern FShaderBindingClaimResolution	ResolveShaderBindingClaim(const EName& InName, const BOOL32 InIsExposure);

	class FShaderBindingClaimRegistrar
	{
	public:
		FShaderBindingClaimRegistrar(const FShaderBindingClaim* InClaims, const UINT32 InClaimNum, const EName& InScopeName)
		{
			RegisterShaderBindingClaims(InClaims, InClaimNum, InScopeName);
		}
	};

#define PE_SHADER_BINDING_CLAIM_CONCAT_INNER(__Left, __Right)	__Left##__Right
#define PE_SHADER_BINDING_CLAIM_CONCAT(__Left, __Right)			PE_SHADER_BINDING_CLAIM_CONCAT_INNER(__Left, __Right)
#define PE_REGISTER_SHADER_BINDING_CLAIMS(__Claims, __ScopeName) \
	static const ::PigeonEngine::FShaderBindingClaimRegistrar PE_SHADER_BINDING_CLAIM_CONCAT(ShaderBindingClaimRegistrar, __LINE__)(__Claims, PE_ARRAYSIZE(__Claims), (__ScopeName))

	// How a constant buffer block reaches the shader. Derived from the reflection's
	// RootConstant flag; kept as its own enum because the binding side branches on the
	// mode, not on the reflection field.
	enum EShaderConstantBufferBindingKindType : UINT8
	{
		// Bound as a root constant buffer view: the render layer hands the command list
		// the buffer object.
		SHADER_CONSTANT_BUFFER_BINDING_KIND_CBV				= 0,
		// Fed through the root signature's 32 bit constant space: there is no view to
		// bind, the render layer pushes the block's cpu bytes instead.
		SHADER_CONSTANT_BUFFER_BINDING_KIND_ROOT_CONSTANTS	= 1,
		SHADER_CONSTANT_BUFFER_BINDING_KIND_COUNT,
	};

	struct EShaderConstantBufferBinding
	{
	public:
		EName	BlockName;
		// Root parameter index used by the command list. It starts as RegisterSlot, which is
		// the identity mapping a back-end without root signatures reads, and the layout
		// generator rewrites it to the root the program's own signature puts the block at.
		UINT32	RootIndex		= 0u;
		UINT32	RegisterSlot	= 0u;
		// Shader visible size, taken from the reflection. Culled buffers are not
		// part of the plan.
		UINT32	ByteSize		= 0u;
		// Binding mode. Keep it consistent with RootConstant: the flag is the reflection's
		// answer, the kind is what the binding side reads, and only BuildFromReflection
		// writes either of them.
		EShaderConstantBufferBindingKindType	Kind = EShaderConstantBufferBindingKindType::SHADER_CONSTANT_BUFFER_BINDING_KIND_CBV;
		// Width of the block in 32 bit values (ByteSize rounded up). Only meaningful for a
		// root constant block, where it is what the root signature's constant space holds.
		UINT32	Num32BitValues	= 0u;
		// TRUE when this block is fed through root 32 bit constants: there is no constant
		// buffer view to bind, the render layer pushes the cpu bytes of the parameter
		// buffer with the root constant binding call instead (ByteSize / 4 32 bit values).
		BOOL32	RootConstant	= FALSE;
	};

	struct EShaderResourceBinding
	{
	public:
		EName						Name;
		EShaderBindingKindType		Kind			= EShaderBindingKindType::SHADER_BINDING_KIND_SRV;
		UINT32						RegisterSlot	= 0u;
		// Root index of the descriptor table this resource's kind lands in, filled by the
		// layout generator. RHI_INVALID_ROOT_INDEX until then, and forever on a back-end
		// without descriptor tables (D3D11), where the command list binds the slot
		// directly. The command list resolves the table root from the bound pipeline's
		// layout, so this field is the plan's own record of it, not the binding input.
		UINT32						TableRootIndex	= RHI_INVALID_ROOT_INDEX;
		// Offset of the resource inside its table. The layout generator keeps a table's
		// range starting at register 0, so it equals RegisterSlot; it is a separate field
		// so the binding side never has to rely on that invariant.
		UINT32						OffsetInTable	= 0u;
		// Narrows down what has to be bound: the texture type for textures, the
		// buffer kind for buffers, the sampler description for samplers.
		EShaderTextureType			TextureType		= EShaderTextureType::SHADER_TEXTURE_TYPE_2D;
		EShaderResourceKindType		ResourceKind	= EShaderResourceKindType::SHADER_RESOURCE_KIND_BUFFER;
		EShaderSamplerDesc			SamplerDesc;
		// TRUE for a texture declaration, FALSE for a structured or byte address buffer. Textures
		// and buffers share the SRV / UAV tables, so the binding kind alone cannot say which
		// engine default resource an exposed binding without a value falls back to.
		BOOL32						bIsTexture		= FALSE;
		// Three-state ownership, filled by BuildFromReflection. Runtime reporting and the default
		// resource fallback read these fields; Exposure is consumed there and not stored twice.
		EShaderBindingClaimStateType	ClaimState		= EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_UNCLAIMED;
		UINT32							ClaimScopeNum	= 0u;
		EName							SoleClaimScope;
		// TRUE when this resource is index driven: the shader reaches it through
		// ResourceDescriptorHeap / SamplerDescriptorHeap with the index the compiler stored in
		// the "<Name>_BindlessIdx" member of BindlessConstantBuffer, so the binding side writes
		// that index instead of binding a view at the table slot. The flag is per binding rather
		// than per plan because a bindless program mixes index driven resources with register
		// bound ones - the resources that did not declare it keep their slot.
		BOOL32						bBindless				= FALSE;
		EName						BindlessConstantBuffer;
	};

	class EShaderBindingPlan
	{
	public:
		EShaderBindingPlan();
		~EShaderBindingPlan();
	public:
		void	Reset();
		// Builds the plan from one stage's reflection. Culled constant buffers and
		// culled variables are skipped, everything else is taken verbatim, whether the stage
		// is bindless is read off the reflection instead of being passed in, and every resource
		// binding is resolved to its claim state: the per-resource flags are the compiler's
		// statement about the stage, so the plan cannot disagree with the shader it describes.
		BOOL32	BuildFromReflection(const EShaderReflection& InReflection);
	public:
		PE_NODISCARD EShaderStageType	GetStage() const { return Stage; }
		PE_NODISCARD BOOL32				IsBindless() const { return bBindless; }
		PE_NODISCARD BOOL32				IsValid() const { return bValid; }
		PE_NODISCARD UINT32				GetConstantBufferBindingNum() const { return ConstantBufferBindings.Num<UINT32>(); }
		PE_NODISCARD UINT32				GetResourceBindingNum() const { return ResourceBindings.Num<UINT32>(); }
		PE_NODISCARD const EShaderConstantBufferBinding*	GetConstantBufferBindingByIndex(const UINT32 InIndex) const;
		PE_NODISCARD const EShaderResourceBinding*			GetResourceBindingByIndex(const UINT32 InIndex) const;
		// Mutable views, used by the layout generator while it rewrites a plan's root and table
		// indices (see Pipeline/RPipelineLayout.h). Everything else reads the const accessors.
		PE_NODISCARD EShaderConstantBufferBinding*			GetMutableConstantBufferBindingByIndex(const UINT32 InIndex);
		PE_NODISCARD EShaderResourceBinding*				GetMutableResourceBindingByIndex(const UINT32 InIndex);
		PE_NODISCARD const EShaderConstantBufferBinding*	FindConstantBufferBinding(const EName& InBlockName) const;
		PE_NODISCARD const EShaderResourceBinding*			FindResourceBinding(const EName& InName) const;
		// Convenience accessor for the common case: the root index a constant buffer
		// has to be bound to. Returns FALSE when the buffer is not part of the plan.
		PE_NODISCARD BOOL32	FindConstantBufferRootIndex(const EName& InBlockName, UINT32& OutRootIndex) const;
	private:
		EShaderStageType						Stage			= EShaderStageType::SHADER_STAGE_VERTEX;
		BOOL32									bBindless		= FALSE;
		BOOL32									bValid			= FALSE;
		TArray<EShaderConstantBufferBinding>	ConstantBufferBindings;
		TArray<EShaderResourceBinding>			ResourceBindings;
	};

} // namespace PigeonEngine
