#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <MultiThread/Mutex.h>
#include <ShaderAsset/ShaderJobBuilder.h>

namespace PigeonEngine
{

	class EShaderInstance;
	class EShaderAsset;

	/*
	* RenderCore/Shader - process wide owner of the engine's own shader instances.
	*
	* Two kinds of instance live as long as the process rather than as long as one object:
	*   - a program the render layer runs without a material, such as the sky dome, the full
	*     screen blit, the lighting pass or the debug wireframe
	*   - a material fallback, which exists only because the mesh it belongs to carries no
	*     material of its own
	* Each of those used to keep a function local static, which left the constant buffers the
	* instance owns in the render resource registry with nobody to release them. The shutdown
	* backstop then reported them as an owner that was never torn down.
	*
	* This is that owner. It keeps one instance per distinct request, and releases every instance
	* it handed out in ShutDown - which the manager teardown runs before the render resource
	* manager drains. Releasing an instance enqueues the destroy of its buffers, so that order is
	* what gets those destroys executed.
	*
	* Threading, and this is the point of the split below:
	*   - TryFindInstance / RequireInstance / RequireMaterialInstance / RequirePassInstance may be
	*     called from ANY thread, including the render thread inside a draw. They only read the
	*     table, cache a derived pass instance they built, and, on a miss, record one request for
	*     the load thread. They never read a file, never assemble and never wait.
	*   - FindOrCreate / FindOrCreateMaterial, the two Create*Request bodies and
	*     PublishShaderAssetSwap are the LOAD THREAD's half: they resolve descriptors, read json,
	*     run the compiler and publish what was loaded. A draw must not reach them.
	* The table takes a lock (the load thread inserts while a draw reads). A lookup hashes the
	* request fields and walks the one bucket that hash names; it constructs no string and
	* allocates nothing, which is what keeps a pass that resolves a material per draw off the
	* render thread's critical path.
	*
	* A request is keyed by the fields that name the assembly, not by the resolved variant: the
	* resolved key costs a program and material json read to compute, which a draw may not pay.
	* The consequence is that two different materials that happen to resolve to the same variant
	* get an instance each instead of sharing one - which is what they want anyway, because the
	* instance owns the parameter values of the material it was built from.
	*
	* A pass instance is derived from the material instance that the primitive reported, and the
	* derivation is cached on that material's entry. An owned copy of a material takes a unique
	* InstanceKey, so its derived pass instance is its own rather than the shared one its source
	* uses: sharing would mean re-parenting one instance between two materials on every draw.
	*
	* The instances are owned here but referenced everywhere: callers keep the returned pointer
	* without owning it, and it stays valid until the cache is shut down.
	*/
	class REngineShaderInstanceCache final : public EManagerBase
	{
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		virtual void	Uninit() override;
	public:
		/*
		* Load thread only. Assembles InRequest when it is not cached yet and returns the instance
		* for it, or the cached one. Null when the assembly or the program resolution fails; a
		* failure is not cached, so a caller that retries until it succeeds keeps retrying.
		*
		* This is the start up form: the renderer and a few proxies resolve their programs before
		* the first frame, where the assembly has to be finished before anything can be drawn.
		* Anything a frame asks for goes through RequireInstance instead.
		*/
		PE_NODISCARD const EShaderInstance* FindOrCreate(const EShaderAssemblyRequest& InRequest);
		/*
		* Load thread only. Resolves a material the caller named, for a caller that has an object
		* to keep visible. A material that cannot be assembled is answered with the engine error
		* material in the same program and for the same vertex axis, so a mesh whose material is
		* broken is still drawn and can be found in the frame instead of disappearing; the log
		* names the material once. Null only when the error material cannot be assembled either,
		* which means the program itself is broken and there is nothing left to substitute.
		*/
		PE_NODISCARD const EShaderInstance* FindOrCreateMaterial(const EShaderAssemblyRequest& InRequest);
		/*
		* Load thread only. A second instance of an assembled material: the same program, its own
		* values. A scene that draws one material with several texture sets - one instance per
		* source material, for instance - needs exactly this, and the cache is the owner that
		* releases it in ShutDown.
		*
		* It exists here because an instance kept in a file scope static leaves the constant buffers
		* it allocates in the render resource registry with nobody to release them, which the
		* shutdown backstop reports as an owner that was never torn down. Null only when the source
		* carries no asset, which is the one way the copy cannot be made.
		*/
		PE_NODISCARD EShaderInstance* CreateOwnedInstance(const EShaderInstance& InSource);
		/*
		* Any thread: the instance for InRequest when it is already assembled, otherwise null.
		* Pure memory read - this is what a draw calls.
		*/
		PE_NODISCARD const EShaderInstance* TryFindInstance(const EShaderAssemblyRequest& InRequest) const;
		/*
		* Any thread: TryFindInstance and, on a miss, one recorded assembly request for the load
		* thread. Null while the instance is not there yet: the caller draws without it this frame
		* and asks again next frame, which is the "retry until it is available" contract the old
		* draw time assembly had - minus every draw blocking on the compiler.
		*/
		PE_NODISCARD const EShaderInstance* RequireInstance(const EShaderAssemblyRequest& InRequest);
		// Any thread: the material form of RequireInstance, including the engine error material
		// substitution and the once-per-material failure report.
		PE_NODISCARD const EShaderInstance* RequireMaterialInstance(const EShaderAssemblyRequest& InRequest);
		/*
		* Any thread: the instance of an already assembled material for another pass.
		*
		* A material is assembled once, for the pass that authored it; a pass that draws the same
		* geometry with a different program - a depth only shadow pass is the first one - derives
		* its own instance from that material instead of requiring the author to configure a second
		* assembly. The derived request keeps the material, its vertex axis and its surface mode and
		* swaps only the program and the shader name (the name must differ: the assembly cache keys
		* on it and the two programs must not collide).
		*
		* The derived instance's parameter set is parented to the primary's, so a value or texture
		* the game sets on the material reaches the pass instance without being copied. Null while
		* the derived assembly is not there yet - the caller skips that draw this frame, the same
		* contract RequireInstance has.
		*/
		PE_NODISCARD const EShaderInstance* RequirePassInstance(const EShaderInstance& InPrimary, const EName& InPassName);
		/*
		* Load thread only: a NEW asset object was just published behind this shader name - an assembly
		* that folded a variant in, a reload, or a republish that a replaced stage asset forced - and
		* this is how the readers are moved onto it. Every published instance of the name is re-pointed
		* to whatever the manager hands out now, and the render thread is told to drop what it derived
		* from the previous object.
		*
		* The re-point is a pointer swap per instance; the program resolution an instance holds is left
		* alone, because it belongs to the thread that reads it - the swap is what makes that thread
		* re-resolve on its next read. Cheap and idempotent, so a caller that cannot tell whether a swap
		* happened publishes anyway. It is part of the publication protocol rather than of the assembly
		* because the publisher is not always this cache: an asset layer that rebuilt a shader asset
		* itself publishes it here, and this is the one step that must not be forgotten then.
		*/
		void	PublishShaderAssetSwap(const EName& InShaderName);
		PE_NODISCARD UINT32	GetInstanceNum() const
		{
			EScopedLock ScopedLock(TableMutex);
			return Instances.Num<UINT32>();
		}
	private:
		// One derived pass instance of a material, cached on the entry it was derived from.
		//
		// The pass is the whole key: a material asks for one instance per pass it is drawn in,
		// and the set is small enough (BasePass, the depth pass, the mask pass) that a linear
		// scan over it is cheaper than hashing an EName per draw.
		struct FPassInstance
		{
			EName					PassName;
			const EShaderInstance*	Instance = nullptr;
		};

		struct FEntry
		{
			// The shader it belongs to, kept apart from the request: a reload replaces the asset
			// object behind a shader name, and this is what finds every instance that has to be
			// re-pointed when that happens.
			EName					ShaderName;
			// The request that produced it. Kept so a derived pass instance can be built from the
			// same material without the caller having to remember how it was assembled. An owned
			// copy inherits its source's request with its own InstanceKey, so the pass instance
			// derived from it is a distinct instance rather than a shared one.
			EShaderAssemblyRequest	Request;
			const EShaderInstance*	Instance = nullptr;
			// The pass instances derived from this one, built on first use and kept for the
			// session. A slot that reuses one is a pointer compare, not a resolution.
			TArray<FPassInstance>	PassInstances;
			// FALSE for an owned copy: it shares its request with the source and must not answer
			// a request lookup, which is what keeps every clone from resolving to the first one.
			BOOL32					bIsLookupable = TRUE;
		};
	private:
		// The load thread's bodies. Shared by the synchronous and the requested form so the two
		// cannot drift apart.
		const EShaderInstance*	CreateInstanceForRequest(const EShaderAssemblyRequest& InRequest);
		const EShaderInstance*	CreateMaterialInstanceForRequest(const EShaderAssemblyRequest& InRequest);
		// Hands every instance back through the render resource manager's retirement. Shared by
		// ShutDown and Uninit so the two paths cannot drift apart.
		void	ReleaseAllInstances();
		PE_NODISCARD static EString	MakeInstanceRequestKey(const EShaderAssemblyRequest& InRequest);
		// The request hash the lookup index is keyed by. It reads the fields MakeInstanceRequestKey
		// serializes, in the same order, and allocates nothing: a draw reaches this path, and the
		// string the key builder produces is only needed for the load queue's dedup.
		PE_NODISCARD static UINT64	ComputeAssemblyRequestHash(const EShaderAssemblyRequest& InRequest) noexcept;
		// Field for field comparison of the same fields, used to reject a hash bucket collision.
		PE_NODISCARD static BOOL32	IsSameAssemblyRequest(const EShaderAssemblyRequest& InLeft
			, const EShaderAssemblyRequest& InRight) noexcept;
		// The entry that owns an instance, or FALSE when the instance did not come from this cache.
		// The caller holds TableMutex.
		PE_NODISCARD BOOL32	FindEntryIndexLocked(const EShaderInstance* InInstance, UINT32& OutIndex) const;
	private:
		TArray<FEntry>	Instances;
		// Request hash -> entry indices. Owned copies are deliberately absent: they share a
		// request with their source and are reached by instance pointer instead.
		THashMap<UINT64, TArray<UINT32>>	InstanceIndicesByHash;
		// Instance pointer -> entry index. This is what turns RequirePassInstance's search for
		// the primary from a linear scan into one hash lookup, owned copies included.
		THashMap<const EShaderInstance*, UINT32>	EntryIndexByInstance;
		// Distinguishes owned copies of one material. Zero is reserved for a shared instance;
		// a clone takes the next value so the pass instance derived from it cannot collide
		// with its source's or with another clone's.
		UINT64			NextOwnedInstanceKey = 1ull;
		// Written by the load thread, read by whoever holds a pointer to an instance (the render
		// thread inside a draw).
		mutable EMutex	TableMutex;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(REngineShaderInstanceCache)

	};

}	// PigeonEngine
