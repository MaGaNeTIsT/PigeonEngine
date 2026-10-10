#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <MultiThread/Atomic.h>
#include <MultiThread/MpscQueue.h>
#include <MultiThread/Mutex.h>

namespace PigeonEngine
{

	/*
	* AssetLoadManager - the one place an asset load is asked for, and the one thread it runs on.
	*
	* The rule it exists to make structural:
	*
	*   Any thread may ASK for an asset. Only the load thread LOADS one.
	*
	* A caller that wants an asset calls a family's RequireLoad* entry point. It answers with the
	* object when the asset is already there, and otherwise records one request here and answers
	* with null - it never blocks, never reads a file and never compiles. The load thread runs the
	* queued requests when it drains, which today is the game thread inside EMainManager::Update
	* and would be a dedicated load thread the moment one is created: the queue, the dedup and
	* every caller stay exactly as they are, only the caller of DoLoads moves.
	*
	* Why a queue rather than a lock: a producer must not wait on the load thread. A frame that
	* asks for an asset while a three second shader compile is running cannot stall behind it -
	* it records the request and draws what it has. The queue is the MPSC template, so the
	* producer side is lock free; the pending key set behind it takes one short lock per request,
	* which is only ever held against another producer's insert.
	*
	* GPU work stays where it belongs: a load that needs a GPU object does not create it. The
	* asset queues a creation command through RAssetRenderBinding::QueueCreate, and the render
	* thread drains that queue and builds the object.
	*/

	// Identity of one load request: which family, and which full path name. Two callers that
	// name the same asset build the same key, which is what the manager dedups on.
	//
	// The family is a tag rather than the EAssetType enum: the enum classifies assets for the
	// editor's browser and has no value for the distinctions that matter here (a shader asset
	// and one of its stage assets are two tables, and a request for one must not answer for the
	// other), so the families that load name themselves.
	PE_NODISCARD extern EString	MakeAssetLoadRequestKey(const CHAR* InAssetFamilyName, const EString& InAssetFullPathName);
	// The name a family manager stores an asset under: <path><name><asset name type>. It is the
	// same string EAssetBase caches as AssetFullPathName, so a lookup from a requester and the
	// store the loader performs agree on the identity without building an asset first.
	PE_NODISCARD extern EString	MakeAssetFullPathName(const EString& InAssetPath, const EString& InAssetName);

	/*
	* What a request's work is: TRUE when the asset is in its family's table by the time it
	* returns. The return value is what a completion is told, so a job that can only report
	* progress - a loader that answers with a pointer instead of a result - reads the outcome
	* back from whatever it just called and returns it here.
	*/
	typedef TFunction<BOOL32()>			EAssetLoadJobType;

	/*
	* What runs once a request is done, ON THE LOAD THREAD, with whether the load succeeded.
	*
	* It runs where the load ran, which is the point: the module that asked knows what its own
	* consequences are and on which thread they have to happen. A module that owns render tier
	* objects does not touch them here - it records a command through RRenderResourceManager
	* (which any thread may write) and the render thread performs the work. A module that owns
	* plain data swaps it right here, because this is the thread that produced the new value.
	*
	* It runs after the request's pending key is released, so a completion that decides to ask
	* again (the load failed, the asset has to be rebuilt) can queue a new request.
	*/
	typedef TFunction<void(BOOL32)>		EAssetLoadCompleteType;

	/*
	* The raw source a family's importer reads when the PAsset is not on disk yet. This is the
	* editor's fallback and nothing else: a shipping build compiles the import out, and a caller
	* with nothing to import passes null. The load thread runs the import as part of serving the
	* request, so the caller neither waits for it nor reads a source file.
	*/
	struct FAssetImportSource
	{
	public:
		// The importer reads <ImportPath><ImportName>.<ImportFileType>.
		EString	ImportPath;
		EString	ImportName;
		EString	ImportFileType;
		// Mesh import only: fold the source's submeshes into the mesh's slot table.
		BOOL32	bIsCombineSubmeshes	= TRUE;
		// Texture import only: answers for an image that declares no colour space of its own.
		BOOL32	bFallbackSRGB		= FALSE;
	public:
		PE_NODISCARD BOOL32	IsValid() const
		{
			return (((ImportPath.Length() > 0u) && (ImportName.Length() > 0u) && (ImportFileType.Length() > 0u)) ? TRUE : FALSE);
		}
	};

	// A cube map imports six faces, in the +X Right, -X Left, +Y Top, -Y Bottom, +Z Forward,
	// -Z Back order the importer expects. One entry each, indexed by face.
	struct FAssetImportCubeSource
	{
	public:
		TArray<EString>	ImportPaths;
		TArray<EString>	ImportNames;
		TArray<EString>	ImportFileTypes;
		BOOL32			bFallbackSRGB	= FALSE;
	public:
		PE_NODISCARD BOOL32	IsValid() const
		{
			return (((ImportPaths.Num<UINT32>() == 6u) && (ImportNames.Num<UINT32>() == 6u) && (ImportFileTypes.Num<UINT32>() == 6u)) ? TRUE : FALSE);
		}
	};

	/*
	* One requested load, by value. The closures own every input the load needs - paths, flags,
	* an assembly request - which is what lets one queue serve every asset family without the
	* manager knowing any of them.
	*/
	struct FAssetLoadRequest
	{
	public:
		EString					RequestKey;
		EAssetLoadJobType		PerformLoad;
		// Optional: a requester that has to react to the result - refresh what it derived from
		// the asset, rebuild a render tier object, retry - fills this in. Null is the common case.
		EAssetLoadCompleteType	OnLoadComplete;
	};

	class EAssetLoadManager final : public EManagerBase
	{
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		virtual void	Uninit() override;
	public:
		/*
		* Any thread. Records InPerformLoad for the load thread. Returns TRUE when this call
		* queued it, FALSE when an identical request is already pending (a caller that polls
		* every frame therefore cannot flood the queue) or when the manager no longer accepts
		* work. Never runs the load on the calling thread.
		*
		* InOnLoadComplete runs on the load thread once the request is done; see
		* EAssetLoadCompleteType for what a module may do there.
		*/
		BOOL32	RequestLoad(const EString& InRequestKey, EAssetLoadJobType InPerformLoad
			, EAssetLoadCompleteType InOnLoadComplete = nullptr);

		/*
		* The load thread only. Runs every request that is queued right now, in the order the
		* producers published them. A request that was queued while this runs belongs to the
		* next drain, so the batch being walked cannot grow under itself.
		*
		* Today EMainManager::Update calls this on the game thread; a dedicated load thread
		* calls it from its own loop instead.
		*/
		void	DoLoads();

		// Any thread: TRUE while a load for InRequestKey has been queued and not finished.
		PE_NODISCARD BOOL32	IsRequestPending(const EString& InRequestKey) const;
		// Diagnostics: requests queued and not yet drained.
		PE_NODISCARD UINT32	GetPendingRequestNum() const;
		PE_NODISCARD BOOL32	IsAcceptingRequests() const { return ((bIsShutDown.Load() == FALSE) ? TRUE : FALSE); }

	private:
		void	ClearRequests();
		void	RemovePendingKey(const EString& InRequestKey);
	private:
		// Producers of every thread push here; only the load thread drains.
		TMpscQueue<FAssetLoadRequest>	Requests;
		// Guards PendingKeys only. Held for one lookup and one insert, and never across a load.
		mutable EMutex					PendingKeysMutex;
		TArray<EString>					PendingKeys;
		// TRUE outside the engine lifetime: requests are refused rather than queued for a drain
		// that will never come.
		TAtomic<BOOL32>					bIsShutDown;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAssetLoadManager)

	};

}	// namespace PigeonEngine
