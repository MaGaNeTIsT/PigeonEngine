#include "AssetLoadManager.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<EAssetLoadManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EString MakeAssetLoadRequestKey(const CHAR* InAssetFamilyName, const EString& InAssetFullPathName)
	{
		// The family is part of the key. Two families can be asked for the same path - a mesh and
		// its skeleton, a shader asset and one of its stage assets - and a request for one must
		// not answer for the other.
		EString RequestKey(InAssetFamilyName);
		RequestKey = RequestKey + EString(PE_TEXT("|")) + InAssetFullPathName;
		return RequestKey;
	}

	EString MakeAssetFullPathName(const EString& InAssetPath, const EString& InAssetName)
	{
		EString FullPathName(InAssetPath);
		FullPathName = FullPathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		return FullPathName;
	}

	EAssetLoadManager::EAssetLoadManager()
		: bIsShutDown(TRUE)		// closed until the engine opens its lifetime
	{
	}
	EAssetLoadManager::~EAssetLoadManager()
	{
	}

	void EAssetLoadManager::Initialize()
	{
		// A fresh lifetime starts empty and accepting. Engine start up does not come through
		// here: the assets the renderer and the world load before the first frame use the
		// family's synchronous entry point, so the first drain of a run finds only what a
		// frame asked for.
		ClearRequests();
		bIsShutDown.Store(FALSE);
	}
	void EAssetLoadManager::ShutDown()
	{
		/*
		* Queued requests are dropped, not run. The managers are shut down after the render
		* thread stopped, so running one would queue GPU creation commands that no frame will
		* ever drain, and the asset it built would have to be released by a teardown that is
		* already past this manager.
		*/
		bIsShutDown.Store(TRUE);
		ClearRequests();
	}
	void EAssetLoadManager::Uninit()
	{
		ShutDown();
	}

	BOOL32 EAssetLoadManager::RequestLoad(const EString& InRequestKey, EAssetLoadJobType InPerformLoad
		, EAssetLoadCompleteType InOnLoadComplete)
	{
		if ((!InPerformLoad.IsValid()) || (bIsShutDown.Load() != FALSE))
		{
			return FALSE;
		}
		{
			// One lookup and one insert, held against nothing but another producer doing the
			// same. The load itself runs outside this lock, which is what keeps a three second
			// compile from blocking the frame that asks for a different asset.
			EScopedLock ScopedLock(PendingKeysMutex);
			if (PendingKeys.Contains(InRequestKey))
			{
				return FALSE;
			}
			PendingKeys.Add(InRequestKey);
		}
		FAssetLoadRequest Request;
		Request.RequestKey = InRequestKey;
		Request.PerformLoad = static_cast<EAssetLoadJobType&&>(InPerformLoad);
		Request.OnLoadComplete = static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete);
		Requests.Enqueue(static_cast<FAssetLoadRequest&&>(Request));
		return TRUE;
	}

	void EAssetLoadManager::DoLoads()
	{
		Requests.DequeueAll([this](FAssetLoadRequest&& InRequest) -> void
		{
			BOOL32 bLoaded = FALSE;
			if (InRequest.PerformLoad.IsValid())
			{
				bLoaded = InRequest.PerformLoad();
			}
			/*
			* The key is released after the load ran, not before it: a caller that polls while
			* its asset is being loaded keeps hitting the dedup, so a compile that takes seconds
			* does not collect one duplicate request per frame. A load that failed leaves nothing
			* in the family table, so the next poll queues it again - which is what a retry is.
			*
			* Released before the completion runs, so a completion may queue a new request for the
			* same asset (a failed load it wants retried, a rebuild it decided it needs).
			*/
			RemovePendingKey(InRequest.RequestKey);
			if (InRequest.OnLoadComplete.IsValid())
			{
				// On this thread on purpose: the module that asked knows what its own consequences
				// are, and this is where the load that produced them ran. See
				// EAssetLoadCompleteType for what a completion may and may not touch.
				InRequest.OnLoadComplete(bLoaded);
			}
		});
	}

	BOOL32 EAssetLoadManager::IsRequestPending(const EString& InRequestKey) const
	{
		EScopedLock ScopedLock(PendingKeysMutex);
		return (PendingKeys.Contains(InRequestKey) ? TRUE : FALSE);
	}

	UINT32 EAssetLoadManager::GetPendingRequestNum() const
	{
		return static_cast<UINT32>(Requests.GetPendingCount());
	}

	void EAssetLoadManager::ClearRequests()
	{
		Requests.Clear();
		EScopedLock ScopedLock(PendingKeysMutex);
		PendingKeys.Empty();
	}

	void EAssetLoadManager::RemovePendingKey(const EString& InRequestKey)
	{
		EScopedLock ScopedLock(PendingKeysMutex);
		const INT32 KeyIndex = PendingKeys.Find(InRequestKey);
		if (KeyIndex >= 0)
		{
			PendingKeys.RemoveAt(static_cast<UINT32>(KeyIndex));
		}
	}

}	// namespace PigeonEngine
