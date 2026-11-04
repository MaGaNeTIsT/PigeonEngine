#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Map.h>
#include <MultiThread/Mutex.h>
#include <RenderResourceLifetime.h>

namespace PigeonEngine
{

	enum EAssetType : UINT32
	{
		ASSET_TYPE_UNKNOWN		= 0,
		ASSET_TYPE_TEXTURE,
		ASSET_TYPE_MESH,
		ASSET_TYPE_SKELETON,
		ASSET_TYPE_ANIMATION,
		ASSET_TYPE_SHADER,
		// New asset families are appended before COUNT so the values already stored in
		// existing PAssets keep their meaning.
		ASSET_TYPE_ANIM_STATE_MACHINE,
		ASSET_TYPE_ANIM_MONTAGE,
		ASSET_TYPE_ANIM_NOTIFY,
		ASSET_TYPE_COUNT,
	};

	/*
	* EAssetBase - the game side asset root.
	*
	* Every asset the game thread owns derives from this, and it exists so an asset cache can
	* be written against one type instead of one template instantiation per asset family. It
	* carries the three things every cache needs:
	*
	*   - identity: the path and name the cache keys on, plus the full path name a load or
	*     save call takes,
	*   - the lifecycle: InitResource / UninitResource / IsValid, all virtual so a cache can
	*     drive an asset it only knows through this base,
	*   - the RTTI root, so unrelated asset classes sit behind one pointer.
	*
	* It deliberately knows nothing about the render tier. An asset that owns exactly one
	* render proxy derives from TRenderBaseAsset; an asset whose GPU objects belong to other
	* assets - EShaderAsset hands every stage to its EShaderStageAsset - derives straight
	* from TBaseAsset and holds no render resource of its own.
	*/
	class EAssetBase : public EObjectBase
	{
	public:
		// Defined in BaseAsset.cpp, which is also where this layer registers itself in the RTTI
		// chain. A non-template base belongs in its own translation unit.
		EAssetBase(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EAssetBase();
	public:
		PE_NODISCARD virtual BOOL32	IsValid() const { return FALSE; }
		PE_NODISCARD BOOL32			IsInitialized() const { return bIsInitialized; }
		// Loads the cold data; for an asset that owns a render proxy this also queues the
		// proxy creation. Returns FALSE when the data could not be restored.
		virtual BOOL32	InitResource() { return FALSE; }
		virtual void	UninitResource()
		{
			bIsInitialized = FALSE;
		}
	public:
		PE_NODISCARD const EString&	GetAssetPath() const { return AssetPath; }
		PE_NODISCARD const EString&	GetAssetName() const { return AssetName; }
		// Cached in the constructor. Building it on every call returned a reference to a
		// temporary, which dangles the moment the caller keeps the reference.
		PE_NODISCARD const EString&	GetAssetFullPathName() const { return AssetFullPathName; }
	protected:
		BOOL32		bIsInitialized;
		EString		AssetPath;
		EString		AssetName;
		EString		AssetFullPathName;

	public:
		EAssetBase() = delete;

		CLASS_REMOVE_COPY_BODY(EAssetBase)

	};

	template<typename TResourceType>
	class TBaseAsset : public EAssetBase
	{
	public:
		TBaseAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		)
			: EAssetBase(InAssetPath, InAssetName
#if _EDITOR_ONLY
				, InDebugName
#endif
			)
			, ResourceData(nullptr)
		{
		}
		virtual ~TBaseAsset()
		{
			UninitResource();
		}
	public:
		virtual BOOL32	IsValid()const override { return IsResourceValid(); }
		virtual void	UninitResource()override
		{
			EAssetBase::UninitResource();
			ReleaseResourceInternal();
		}
	public:
		BOOL32 IsResourceValid()const
		{
			if (!!ResourceData)
			{
				return (ResourceData->IsResourceValid());
			}
			return FALSE;
		}
		const TResourceType*	GetStoragedResource()const { return ResourceData; }
		/*
		* Stores the cold backup of a freshly built asset. There is deliberately no "replace the
		* stored backup" counterpart: overwriting the data of an asset that is already published is
		* what a reload must not do, because readers hold pointers into it. A rebuild builds a new
		* asset object, stores its backup here, and the family manager publishes it by swapping its
		* table entry - the old object then leaves through the retirement path like any other owner.
		*/
		template<typename TInitResourceLambdaType>
		BOOL32 StorageResourceInternal(const TInitResourceLambdaType& lStorageFunc)
		{
			TResourceType* TempData = lStorageFunc();
			if (ResourceData)
			{
#if _EDITOR_ONLY
				EString errorInfo = DebugName + " try to init a new resource, but already storage a resource.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*errorInfo));
#endif
				if (TempData)
				{
					TempData->ReleaseResource();
					delete TempData;
				}
				return FALSE;
			}
			{
				ResourceData = TempData;
			}
			if (!ResourceData)
			{
#if _EDITOR_ONLY
				EString errorInfo = DebugName + " try to storage a null resource.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*errorInfo));
#endif
				return FALSE;
			}
			return TRUE;
		}
	protected:
		void ReleaseResourceInternal()
		{
			if (ResourceData)
			{
				ResourceData->ReleaseResource();
				delete ResourceData;
				ResourceData = nullptr;
			}
		}
	protected:
		TResourceType*	ResourceData;

	public:
		TBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TBaseAsset)

	};

	/*
	* Every asset family registers its own chain, one link per layer, and the abstract layers
	* register themselves where they are defined:
	*
	*   EAssetBase          registered in BaseAsset.cpp, once, by its own definition
	*   TBaseAsset<TData>   registered by RegisterBaseAssetClassTypes<TData>()
	*   concrete asset      registered by its own RegisterClassTypes()
	*
	* The engine has a handful of asset types, so the whole chain is readable by inspection
	* and nothing here tries to be clever about being called twice.
	*/
	template <typename _TAssetResourceType>
	PE_INLINE void RegisterBaseAssetClassTypes()
	{
		RegisterClassType<TBaseAsset<_TAssetResourceType>, EAssetBase>();
	}

	/*
	* The one thing inside an asset that knows a render proxy exists.
	*
	* Composition, not inheritance. The asset type chain stays purely game side
	* (EObjectBase -> EAssetBase -> TBaseAsset), and an asset that owns a GPU object holds one
	* of these as a member. RAssetProxy is the render side counterpart; this is the game side
	* handle that points at it. An asset whose GPU objects belong to other objects - EShaderAsset
	* hands every stage to its EShaderStageAsset - simply does not have one.
	*
	* Two step creation is what lets the game thread hand out the reference the moment the
	* asset is loaded while the proxy itself is still built on the render thread: the slot is
	* reserved here, and the render thread fills it when it drains the queue, before the scene
	* commands of the same frame, so a proxy added in that frame already finds it alive.
	*/
	class RAssetRenderBinding
	{
	public:
		RAssetRenderBinding() noexcept;
		~RAssetRenderBinding();
	public:
		PE_NODISCARD BOOL32				IsBound() const noexcept { return RenderRef.IsValid(); }
		PE_NODISCARD FRenderResourceRef	GetRef() const noexcept { return RenderRef; }
		// Render thread only: the proxy, or null while the creation command is still queued.
		PE_NODISCARD RRenderResourceInterface*	Resolve() const;
	public:
		/*
		* Game thread: reserves a slot and queues the proxy creation. InCreateFunc runs on the
		* render thread and returns the proxy it built.
		*
		* Note what this does NOT do: it never hands the asset to the command. A creation
		* callback therefore cannot read asset members by accident, which is the rule the
		* earlier revision had to state in a comment because the framework made it possible.
		* Whatever the callback needs has to be captured by value where it is enqueued.
		*
		* Caller contract: the cold data a callback captures must outlive the queued command.
		* Assets are released through their manager, and managers are torn down after the render
		* thread stopped, so the command never runs against freed data.
		*/
		template<typename TCreateFuncType>
		BOOL32	QueueCreate(TCreateFuncType InCreateFunc)
		{
			if (RenderRef.IsValid())
			{
#if _EDITOR_ONLY
				PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[RAssetRenderBinding::QueueCreate] A render proxy is already queued or bound.")));
#endif
				return FALSE;
			}
			RRenderResourceManager* ResourceManager = RRenderResourceManager::GetManagerSingleton();
			RenderRef = ResourceManager->GetRegistry().ReserveSlot();
			const FRenderResourceRef TargetRef = RenderRef;
			ResourceManager->EnqueueCommand([TargetRef, InCreateFunc]() -> void
			{
				RRenderResourceInterface* Proxy = InCreateFunc();
				if (!Proxy)
				{
					// Nothing was built, so hand the reservation back. Leaving it reserved would
					// keep the slot out of circulation for the whole life of the owner, and
					// Resolve would keep returning null with no way to tell why.
					RRenderResourceManager::GetManagerSingleton()->GetRegistry().Unregister(TargetRef);
					return;
				}
				if (!RRenderResourceManager::GetManagerSingleton()->GetRegistry().Commit(TargetRef, Proxy))
				{
					// The owner released before this command ran, so nothing could ever reach
					// the object again; tear it down here.
					Proxy->ReleaseRenderResource();
					delete Proxy;
				}
			});
			return TRUE;
		}
		// Game thread: gives the reference back. The render thread resolves, releases and
		// unregisters it in one queued command, so the registry is never touched from here and
		// the proxy is never deleted from here either.
		void	Release();
	private:
		// The proxy lives in the registry, not here.
		FRenderResourceRef	RenderRef;

		CLASS_REMOVE_COPY_BODY(RAssetRenderBinding)

	};

	/*
	* One asset family's table, keyed by the asset's full path name.
	*
	* Threading: the load thread writes it (a load stores what it built) and any thread may read
	* it (RequireLoad* asks the table before it queues a request), so the table operations take a
	* lock. The critical sections cover the table only - releasing an asset is deliberately
	* outside them, because a release parks the asset through the render resource manager, which
	* takes a lock of its own and may run an asset destructor.
	*
	* What the API hands out is a borrowed pointer: it stays valid until the manager releases the
	* asset, which happens through a render resource retirement and therefore never inside the
	* frame that is using it.
	*/
	template<typename TKeyType, typename TAssetType>
	class TAssetManager
	{
	public:
		TAssetManager()
		{
		}
		virtual ~TAssetManager()
		{
			Clear();
		}
	public:
		/*
		* Add item into mapped datas. This action may reconstruct whole memories.
		* Params [InReplaceIfHaveKey]: true = If already contain data with InKeyValue will replace it. This action will delete old asset. false = If already contain data with InKeyValue will not do anything.
		* Return [UINT32]: 0 = Add failed. value = Mapped datas' size(after add value).
		*/
		UINT32 Add(const TKeyType& InKeyValue, TAssetType* InDataValue, const BOOL32& InReplaceIfHaveKey = FALSE)
		{
			if (!InDataValue)
			{
				return 0u;
			}
			TAssetType* ReplacedData = nullptr;
			UINT32 ResultNum = 0u;
			{
				EScopedLock ScopedLock(TableMutex);
				BOOL32 NeedReplace = FALSE;
				if (SavedDatas.ContainsKey(InKeyValue))
				{
					NeedReplace = TRUE;
					if (!InReplaceIfHaveKey)
					{
						return 0u;
					}
				}
				if (NeedReplace)
				{
					TAssetType* TempData = nullptr;
					if (SavedDatas.FindValue(InKeyValue, TempData))
					{
						if (TempData && TempData == InDataValue)
						{
							return (SavedDatas.Num<UINT32>());
						}
						SavedDatas.Remove(InKeyValue);
						ReplacedData = TempData;
					}
				}
				SavedDatas.Add(InKeyValue, InDataValue);
				ResultNum = SavedDatas.Num<UINT32>();
			}
			if (ReplacedData)
			{
				ReleaseAsset(ReplacedData);
			}
			return ResultNum;
		}
		/*
		* Remove item with InKeyValue in mapped datas.
		* Return [UINT32]: Mapped datas' size(after remove value).
		*/
		UINT32 Remove(const TKeyType& InKeyValue)
		{
			TAssetType* RemovedData = nullptr;
			UINT32 ResultNum = 0u;
			{
				EScopedLock ScopedLock(TableMutex);
				if (SavedDatas.FindValue(InKeyValue, RemovedData))
				{
					SavedDatas.Remove(InKeyValue);
				}
				ResultNum = SavedDatas.Num<UINT32>();
			}
			if (RemovedData)
			{
				ReleaseAsset(RemovedData);
			}
			return ResultNum;
		}
		/*
		* Find item with InKeyValue in mapped datas.
		* Return [TAssetType*]: If contain key return asset. If not then return nullptr.
		*/
		TAssetType* Find(const TKeyType& InKeyValue)
		{
			EScopedLock ScopedLock(TableMutex);
			TAssetType* TempData = nullptr;
			SavedDatas.FindValue(InKeyValue, TempData);
			return TempData;
		}
		/*
		* Every asset the table holds, taken under the table's lock. For a caller that has to touch
		* assets it cannot name in advance - the republish a stage asset replacement triggers walks the
		* shader assets for the ones referencing a replaced name. The pointers are the borrowed handles
		* Find returns and stay valid under the same rule: until the manager releases the asset, which
		* only the loading side does, so a caller on that side may keep the snapshot across its work.
		*/
		void GenerateValueArray(TArray<TAssetType*>& OutAssets)
		{
			EScopedLock ScopedLock(TableMutex);
			SavedDatas.GenerateValueArray(OutAssets);
		}
		/*
		* Check if contains item with InKeyValue in mapped datas.
		* Return [BOOL32]: If contains return TRUE. If not return FALSE.
		*/
		BOOL32 Contain(const TKeyType& InKeyValue)const
		{
			EScopedLock ScopedLock(TableMutex);
			return (SavedDatas.ContainsKey(InKeyValue));
		}
		/*
		* Clear whole data list.
		*/
		void Clear()
		{
			TArray<TAssetType*> ReleasedAssets;
			{
				EScopedLock ScopedLock(TableMutex);
				if (SavedDatas.Num() <= 0)
				{
					return;
				}
				for (auto it = SavedDatas.Begin(); it != SavedDatas.End(); it++)
				{
					ReleasedAssets.Add(it->second);
				}
				SavedDatas.Empty();
			}
			for (UINT32 Index = 0u, Num = ReleasedAssets.Num<UINT32>(); Index < Num; Index++)
			{
				ReleaseAsset(ReleasedAssets[Index]);
			}
		}
		/*
		* Releases one asset the cache no longer hands out.
		*
		* The asset is parked WHOLE - cold data included - rather than released here, and that is
		* the fix for the second lifetime hazard: UninitResource frees the asset's cold data, and
		* a queued render resource creation command (or a proxy on the render thread, which reads
		* the asset through a pointer it was reported) may still be reading it. The render
		* resource manager runs this once the render thread has drained a frame that started after
		* this point, which is exactly the contract RAssetRenderBinding::QueueCreate states for
		* the values its commands capture.
		*
		* TAssetBase's destructor would call UninitResource on its own; it is called explicitly
		* here so "what a manager releases" is readable in one place. The call is idempotent.
		*/
		void ReleaseAsset(TAssetType* InAsset)
		{
			if (!InAsset)
			{
				return;
			}
			RRenderResourceManager::GetManagerSingleton()->RetireRelease([InAsset]() -> void
			{
				InAsset->UninitResource();
				delete InAsset;
			});
		}
		/*
		* Get size of mapped datas.
		*/
		UINT32 Size()const
		{
			EScopedLock ScopedLock(TableMutex);
			return (SavedDatas.Num<UINT32>());
		}
	protected:
		TMap<TKeyType, TAssetType*> SavedDatas;
		// Guards SavedDatas. Mutable because the const entry points (Contain, Size) read the same
		// table the load thread writes.
		mutable EMutex TableMutex;

		CLASS_REMOVE_COPY_BODY(TAssetManager)

	};

};
