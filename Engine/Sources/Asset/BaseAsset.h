#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Map.h>

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
		ASSET_TYPE_COUNT,
	};

	template<typename TResourceType>
	class TBaseAsset : public EObjectBase
	{
	public:
		TBaseAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		)
			: bIsInitialized(FALSE)
			, AssetPath(InAssetPath)
			, AssetName(InAssetName)
			, ResourceData(nullptr)
		{
#if _EDITOR_ONLY
			DebugName = InDebugName;
#endif
		}
		virtual ~TBaseAsset()
		{
			UninitResource();
		}
	public:
		virtual BOOL	IsValid()const { return IsResourceValid(); }
		virtual BOOL	IsInitialized()const { return bIsInitialized; }
		virtual BOOL	InitResource() { return FALSE; }
		virtual void	UninitResource()
		{
			bIsInitialized = FALSE;
			ReleaseResourceInternal();
		}
	public:
		BOOL					IsResourceValid()const { return ((!!ResourceData) && (ResourceData->IsResourceValid())); }
		const TResourceType*	GetStoragedResource()const { return ResourceData; }
		const EString&			GetAssetPath()const { return AssetPath; }
		const EString&			GetAssetName()const { return AssetName; }
		const EString&			GetAssetFullPathName()const { return (AssetPath + AssetName + ENGINE_ASSET_NAME_TYPE); }
		template<typename TInitResourceLambdaType>
		BOOL StorageResourceInternal(const TInitResourceLambdaType& lStorageFunc)
		{
			TResourceType* TempData = lStorageFunc();
			if (ResourceData)
			{
#if _EDITOR_ONLY
				EString errorInfo = DebugName + " try to init a new resource, but already storage a resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
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
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
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
		BOOL			bIsInitialized;
		EString			AssetPath;
		EString			AssetName;
		TResourceType*	ResourceData;

	public:
		TBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TBaseAsset)

	};

	template <typename _TAssetResourceType>
	ENGINE_INLINE void RegisterBaseAssetClassTypes()
	{
		RegisterClassType<TBaseAsset<_TAssetResourceType>, EObjectBase>();
	}

	template<typename TResourceType, typename TRenderResourceType>
	class TRenderBaseAsset : public TBaseAsset<TResourceType>
	{
	public:
		TRenderBaseAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		)
			: TBaseAsset<TResourceType>(InAssetPath, InAssetName
#if _EDITOR_ONLY
				, InDebugName
#endif
			), bHoldResource(TRUE), RenderResourceData(nullptr)
		{
		}
		virtual ~TRenderBaseAsset()
		{
		}
	public:
		BOOL IsHoldResource()const { return bHoldResource; }
		virtual BOOL IsValid()const override
		{
			if (bHoldResource)
			{
				return IsResourceValid() && IsRenderResourceValid();
			}
			return IsRenderResourceValid();
		}
		virtual void UninitResource()override
		{
			TBaseAsset<TResourceType>::UninitResource();
			ReleaseRenderResourceInternal();
		}
	public:
		BOOL	IsRenderResourceValid()const { return ((!!RenderResourceData) && (RenderResourceData->IsRenderResourceValid())); }
		const TRenderResourceType*	GetRenderResource()const { return RenderResourceData; }
	protected:
		template<typename TCreateRenderResourceLambdaType>
		BOOL CreateRenderResourceInternal(const TCreateRenderResourceLambdaType& lCreateFunc, const BOOL& bHoldStoragedResource)
		{
			TRenderResourceType* TempData = lCreateFunc(ResourceData);
			if (RenderResourceData)
			{
#if _EDITOR_ONLY
				EString errorInfo = DebugName + " try to create gpu resource, but already has a resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				if (TempData)
				{
					TempData->ReleaseRenderResource();
					delete TempData;
				}
				return FALSE;
			}
			{
				RenderResourceData = TempData;
			}
			if (!RenderResourceData)
			{
#if _EDITOR_ONLY
				EString errorInfo = DebugName + " try to storage a null resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			if (!bHoldStoragedResource)
			{
				ReleaseResourceInternal();
			}
			bHoldResource = bHoldStoragedResource;
			return TRUE;
		}
		void ReleaseRenderResourceInternal()
		{
			if (RenderResourceData)
			{
				RenderResourceData->ReleaseRenderResource();
				delete RenderResourceData;
				RenderResourceData = nullptr;
			}
		}
	protected:
		BOOL					bHoldResource;
		TRenderResourceType*	RenderResourceData;

	public:
		TRenderBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TRenderBaseAsset)

	};

	template <typename _TAssetResourceType, typename _TAssetRenderResourceType>
	ENGINE_INLINE void RegisterRenderBaseAssetClassTypes()
	{
		RegisterBaseAssetClassTypes<_TAssetResourceType>();
		RegisterClassType<TRenderBaseAsset<_TAssetResourceType, _TAssetRenderResourceType>, TBaseAsset<_TAssetResourceType>>();
	}

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
		* Return [UINT]: 0 = Add failed. value = Mapped datas' size(after add value).
		*/
		UINT Add(const TKeyType& InKeyValue, TAssetType* InDataValue, const BOOL& InReplaceIfHaveKey = FALSE)
		{
			if (!InDataValue)
			{
				return 0u;
			}
			BOOL NeedReplace = FALSE;
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
						return (SavedDatas.Length());
					}
					SavedDatas.Remove(InKeyValue);
					delete TempData;
				}
			}
			SavedDatas.Add(InKeyValue, InDataValue);
			return (SavedDatas.Length());
		}
		/*
		* Remove item with InKeyValue in mapped datas.
		* Return [UINT]: Mapped datas' size(after remove value).
		*/
		UINT Remove(const TKeyType& InKeyValue)
		{
			TAssetType* TempData = nullptr;
			if (SavedDatas.FindValue(InKeyValue, TempData))
			{
				SavedDatas.Remove(InKeyValue);
				delete TempData;
			}
			return (SavedDatas.Length());
		}
		/*
		* Find item with InKeyValue in mapped datas.
		* Return [TAssetType*]: If contain key return asset. If not then return nullptr.
		*/
		TAssetType* Find(const TKeyType& InKeyValue)
		{
			TAssetType* TempData = nullptr;
			SavedDatas.FindValue(InKeyValue, TempData);
			return TempData;
		}
		/*
		* Check if contains item with InKeyValue in mapped datas.
		* Return [BOOL]: If contains return TRUE. If not then return FALSE.
		*/
		BOOL Contain(const TKeyType& InKeyValue)const
		{
			return (SavedDatas.ContainsKey(InKeyValue));
		}
		/*
		* Clear whole data list.
		*/
		void Clear()
		{
			if (SavedDatas.Length() > 0u)
			{
				for (auto it = SavedDatas.Begin(); it != SavedDatas.End(); it++)
				{
					delete (it->second);
				}
				SavedDatas.Clear();
			}
		}
		/*
		* Get size of mapped datas.
		*/
		UINT Size()const
		{
			return (SavedDatas.Length());
		}
	protected:
		TMap<TKeyType, TAssetType*> SavedDatas;

		CLASS_REMOVE_COPY_BODY(TAssetManager)

	};

};