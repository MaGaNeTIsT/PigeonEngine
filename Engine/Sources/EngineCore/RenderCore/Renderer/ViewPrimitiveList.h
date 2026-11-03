#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	class RPrimitiveSceneProxy;
	class EShaderInstance;

	/*
	* One primitive a view has decided to collect.
	*
	* The list is the view's visibility result and every pass's input: a view walks its own
	* list instead of the scene's primitive array, and each pass filters that list into its
	* own work items. The list is not a draw list - a compute or ray tracing pass consumes
	* the same entries - so nothing here names a graphics operation.
	*
	* Identity is kept beside the pointer because the per primitive state every pass reads is
	* keyed by it on the CPU side, and a pass that groups or sorts its items needs a stable
	* tie break without asking the proxy again.
	*/
	struct RViewPrimitiveItem
	{
		RPrimitiveSceneProxy*	Primitive		= nullptr;
		ObjectIdentityType		Identity		= 0u;
		// The shader instance this view selected for the primitive. It normally equals the
		// primitive's reported primary instance; a view level override (LOD, alternate material)
		// writes a different one here, and the pass data key includes it.
		const EShaderInstance*	ShaderInstance	= nullptr;
		// View dependent flags a pass may read. The collection filter and the passes agree on
		// the meaning per bit; no engine wide bit is defined here.
		UINT64					ViewFlags		= 0u;
	};

	/*
	* The world space union of the primitives a list kept, and whether that union is complete.
	*
	* A directional shadow view uses it to fit its orthographic projection to the geometry it
	* actually collected: an orthographic light preserves an occluder's light space x and y in
	* the shadow it casts, so a box around the collected casters also contains every shadow
	* they can cast.
	*
	* A primitive that cannot state a bound stays in the list - the collection never drops one
	* on a guess - but its geometry is not in the union. A fit built from an incomplete union
	* could clip that primitive, so the caller has to keep the conservative projection instead.
	*/
	struct RViewPrimitiveBounds
	{
		EBoundAABB	Bounds{ Vector3::Zero(), Vector3::Zero(), FALSE };
		BOOL32		bComplete = TRUE;
	public:
		void	Reset();
		void	Add(const Vector3& InMin, const Vector3& InMax);
		void	MarkUnknown();
	};

	/*
	* The primitive list of one view.
	*
	* A list is reset and refilled by that view's collection pass, and every pass that consumes
	* the view reads it afterwards. The list owns no resources and no lifetime: the proxies it
	* names are owned by the scene, and the list is a statement about this frame's visibility
	* rather than about the objects.
	*/
	class RViewPrimitiveList final
	{
	public:
		RViewPrimitiveList() = default;
	public:
		// Drops every entry while keeping the storage, so a view that is filtered to nothing
		// every frame does not reallocate its list every frame.
		void	Reset();
		void	Add(RPrimitiveSceneProxy* InPrimitive, const EShaderInstance* InShaderInstance);
	public:
		PE_NODISCARD UINT32		Num() const;
		PE_NODISCARD BOOL32		IsEmpty() const;
		PE_NODISCARD TArray<RViewPrimitiveItem>&		GetItems();
		PE_NODISCARD const TArray<RViewPrimitiveItem>&	GetItems() const;
		PE_NODISCARD RViewPrimitiveItem&			operator[](const UINT32 InIndex);
		PE_NODISCARD const RViewPrimitiveItem&		operator[](const UINT32 InIndex) const;
	private:
		TArray<RViewPrimitiveItem>	Items;

	};

}	// namespace PigeonEngine
