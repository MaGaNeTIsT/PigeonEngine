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
	*   - TryFindInstance / RequireInstance / RequireMaterialInstance may be called from ANY
	*     thread, including the render thread inside a draw. They only read the table and, on a
	*     miss, record one request for the load thread. They never read a file, never assemble
	*     and never wait.
	*   - FindOrCreate / FindOrCreateMaterial, the two Create*Request bodies and
	*     PublishShaderAssetSwap are the LOAD THREAD's half: they resolve descriptors, read json,
	*     run the compiler and publish what was loaded. A draw must not reach them.
	* The table takes a lock (the load thread inserts while a draw reads); the critical section
	* is one linear scan of a small array of keys.
	*
	* A request is keyed by the fields that name the assembly, not by the resolved variant: the
	* resolved key costs a program and material json read to compute, which a draw may not pay.
	* The consequence is that two different materials that happen to resolve to the same variant
	* get an instance each instead of sharing one - which is what they want anyway, because the
	* instance owns the parameter values of the material it was built from.
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
		struct FEntry
		{
			// Identity of the request this instance answers. Built from the request fields only,
			// see the class comment.
			EString					RequestKey;
			// The shader it belongs to, kept apart from the key: a reload replaces the asset object
			// behind a shader name, and this is what finds every instance that has to be re-pointed
			// when that happens.
			EName					ShaderName;
			const EShaderInstance*	Instance = nullptr;
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
	private:
		TArray<FEntry>	Instances;
		// Written by the load thread, read by whoever holds a pointer to an instance (the render
		// thread inside a draw).
		mutable EMutex	TableMutex;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(REngineShaderInstanceCache)

	};

}	// PigeonEngine
