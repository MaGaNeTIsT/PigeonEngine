#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

	class EShaderProgram;
	class IRRHIDevice;
	class IRRHIRootSignature;

	/*
	* Register ceilings of one stage's parameter space. The shader compiler already rejects a
	* json that exceeds them (constant buffer b0..b13, SRV t0..t127, UAV u0..u15, sampler
	* s0..s15), so these are the generator's defensive bounds: a hand edited reflection cannot
	* produce a root signature the back-end would have to reject.
	*/
	PE_CONSTEXPR_STATIC UINT32 PIPELINE_MAX_CONSTANT_BUFFER_SLOTS	= 14u;
	PE_CONSTEXPR_STATIC UINT32 PIPELINE_MAX_SRV_SLOTS				= 128u;
	PE_CONSTEXPR_STATIC UINT32 PIPELINE_MAX_UAV_SLOTS				= 16u;
	PE_CONSTEXPR_STATIC UINT32 PIPELINE_MAX_SAMPLER_SLOTS			= 16u;
	// Root parameter budget of the whole signature, in DWORDs: a constant buffer view costs
	// two, a descriptor table one, a 32 bit constant block one per value. Mirrors the API's
	// own ceiling, which the back-end checks again before creating the signature.
	PE_CONSTEXPR_STATIC UINT32 PIPELINE_MAX_ROOT_DWORDS				= 64u;

	/*
	* RenderCore/Pipeline - the parameter layout of one shader program.
	*
	* A layout is what a program's reflected parameter space becomes in the RHI: the root
	* signature the back-end builds from it, plus the RRHIPipelineLayoutInfo the command list
	* binds against. Both are produced together and never disagree, because the layout info is
	* derived from the same root parameter list the signature is built from.
	*
	* One layout per program variant (see the plan's decision on granularity): a program's
	* stages are compiled separately and may cull different parameters, so the signature has to
	* cover the union of every stage that will run with it.
	*
	* Generation rules (the order is the contract the binding side relies on):
	*   - Constant buffer views occupy the low roots, one per distinct register slot in
	*     ascending order, so a block's root index is its position among the program's buffers.
	*   - A block the reflection marked RootConstant becomes one 32 bit constant root after all
	*     of them, wide enough for the widest stage's use of it.
	*   - SRV / UAV / sampler descriptor tables come last, each starting at register 0 and as
	*     wide as the highest register any stage uses, so a resource's offset inside its table
	*     is its register slot.
	*   - Visibility is the union of the stages that declared the parameter.
	*   - A bindless variant gets no tables: its shaders index the descriptor heaps directly.
	*
	* Known limits of this stage:
	*   - One layout per program, so a program that carries both graphics and compute stages
	*     produces a signature that has to serve both. That stays legal because a compute-only
	*     parameter's COMPUTE visibility is mapped to ALL by the back-end (a compute root
	*     signature has no stage visibility at all) and ALLOW_INPUT_ASSEMBLER is only a
	*     permission. The engine has no such program today; if one appears, its graphics and
	*     compute pipelines still share one signature, which is correct but narrower than two
	*     separate layouts would be.
	*   - A table can be wider than the registers one stage may address (the compiler's slot
	*     ceiling is wider than the per-stage register count for UAVs in particular). That is
	*     harmless: the table describes the program's registers, and a shader that cannot
	*     address u8 simply never does.
	*/
	struct RPipelineLayout
	{
	public:
		UINT64					LayoutHash		= 0ull;
		// The signature object the back-end created for this layout. A back-end without root
		// signatures (D3D11) still hands one back, but its LayoutInfo stays the zero value: that
		// zero is what says "root indices are register slots here", and the generator skips the
		// binding plan write-back because of it.
		IRRHIRootSignature*		RootSignature	= nullptr;
		RRHIPipelineLayoutInfo	LayoutInfo{};
	};

	// The generator and per-layout cache. Owns the layout records it hands out; the root
	// signature objects belong to the device (they are shared by layout hash and outlive the
	// PSOs that reference them), so invalidating a layout never destroys a signature a live
	// PSO may still be drawing with.
	class RPipelineLayoutCache
	{
	public:
		RPipelineLayoutCache();
		~RPipelineLayoutCache();

		RPipelineLayoutCache(const RPipelineLayoutCache&) = delete;
		RPipelineLayoutCache& operator=(const RPipelineLayoutCache&) = delete;

	public:
		void	SetDevice(IRRHIDevice* InDevice);
		PE_NODISCARD IRRHIDevice*	GetDevice() const { return Device; }

		// Layout of one program variant. The first call for a program generates the root
		// signature, rewrites the program's binding plans to the root / table indices the
		// layout uses, and caches the result under its layout hash; later calls return the
		// cached record. Returns null when the program has no valid stage, when its parameter
		// space cannot be expressed (too many root parameters, more than one root constant
		// block) or when the device refused the signature.
		PE_NODISCARD const RPipelineLayout*	GetOrCreate(const EShaderProgram& InProgram);

		// Drops the cached records. Signatures survive, so a program that is regenerated into
		// the same layout keeps handing out the same signature object to its PSOs.
		void	Invalidate(const EShaderProgram& InProgram);
		void	InvalidateAll();

		PE_NODISCARD UINT32	GetLayoutNum() const { return Layouts.Num<UINT32>(); }

	private:
		PE_NODISCARD RPipelineLayout*	FindLayout(const UINT64 InLayoutHash) const;

	private:
		IRRHIDevice*					Device;
		TMap<UINT64, RPipelineLayout*>	Layouts;
		// Heap allocated and owned here: the map hands out pointers that have to stay valid
		// while the cache lives.
		TArray<RPipelineLayout*>		LayoutPool;

	};

}	// PigeonEngine
