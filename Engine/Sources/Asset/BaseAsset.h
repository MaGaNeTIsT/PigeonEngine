#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	template<typename TResourceType>
	class TBaseAsset : public EObjectBase
	{
		//EClass(TBaseAsset<TResourceType>, EObjectBase)

	public:
		TBaseAsset(
#ifdef _EDITOR_ONLY
			const EString& name
#endif
		)
			: ResourceData(nullptr)
			, bIsInitialized(FALSE)
#ifdef _EDITOR_ONLY
			, DebugName(name)
#endif
		{
		}
		virtual ~TBaseAsset()
		{
			UninitResource();
		}
	public:
		virtual BOOL	IsValid() { return IsResourceValid(); }
		virtual BOOL	IsInitialized() { return bIsInitialized; }
		virtual BOOL	InitResource() { return FALSE; }
		virtual void	UninitResource()
		{
			bIsInitialized = FALSE;
			ReleaseResourceInternal();
		}
	public:
		BOOL	IsResourceValid()const { return (!!ResourceData); }
		const TResourceType*	GetStoragedResource()const { return ResourceData; }
	protected:
		template<typename TInitResourceLambdaType>
		BOOL StorageResourceInternal(const TInitResourceLambdaType& lStorageFunc)
		{
			if (ResourceData)
			{
#ifdef _EDITOR_ONLY
				EString errorInfo = DebugName + " try to init a new resource, but already storage a resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			{
				ResourceData = lStorageFunc();
			}
			if (!ResourceData)
			{
#ifdef _EDITOR_ONLY
				EString errorInfo = DebugName + " try to storage a null resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			return TRUE;
		}
		void ReleaseResourceInternal()
		{
			if (ResourceData)
			{
				ResourceData->Release();
				delete ResourceData;
				ResourceData = nullptr;
			}
		}
	protected:
		BOOL			bIsInitialized;
		TResourceType*	ResourceData;

	public:
		TBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TBaseAsset)

	};

	template<typename TResourceType, typename TRenderResourceType>
	class TRenderBaseAsset : public TBaseAsset<TResourceType>
	{
		//EClass(TRenderBaseAsset<TResourceType, TRenderResourceType>, TBaseAsset<TResourceType>)

	public:
		TRenderBaseAsset(
#ifdef _EDITOR_ONLY
			const EString& name
#endif
		)
#ifdef _EDITOR_ONLY
			: TBaseAsset<TResourceType>(name), RenderResourceData(nullptr)
#else
			: RenderResourceData(nullptr)
#endif
			, bHoldResource(TRUE)
		{
		}
		virtual ~TRenderBaseAsset()
		{
		}
	public:
		BOOL IsHoldResource()const { return bHoldResource; }
		virtual BOOL IsValid()override
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
		BOOL	IsRenderResourceValid()const { return (!!RenderResourceData); }
		const TRenderResourceType*	GetRenderResource()const { return RenderResourceData; }
	protected:
		template<typename TCreateRenderResourceLambdaType>
		BOOL CreateRenderResourceInternal(const TCreateRenderResourceLambdaType& lCreateFunc, const BOOL& bHoldStoragedResource)
		{
			if (RenderResourceData)
			{
#ifdef _EDITOR_ONLY
				EString errorInfo = DebugName + " try to create gpu resource, but already has a resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			{
				RenderResourceData = lCreateFunc(ResourceData);
			}
			if (!RenderResourceData)
			{
#ifdef _EDITOR_ONLY
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
				RenderResourceData->Release();
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

	template<typename TKeyType, typename TAssetType>
	class TAssetManager
	{
	public:
		TAssetManager()
		{
		}
		virtual ~TAssetManager()
		{
			if (SavedDatas.size() > 0)
			{
				for (auto it = SavedDatas.begin(); it != SavedDatas.end(); it++)
				{
					delete (it->second);
				}
				SavedDatas.clear();
			}
		}
	public:
		/*
		* Add item into mapped datas. This action may reconstruct whole memories.
		* Params [replaceIfHaveKey]: true = If already contain data with keyValue will replace it. This action will delete old asset. false = If already contain data with keyValue will not do anything.
		* Return [UINT]: 0 = Add failed. value = Mapped datas' size(after add value).
		*/
		UINT Add(TKeyType&& keyValue, TAssetType* dataValue, const BOOL& replaceIfHaveKey = FALSE)
		{
			auto result = SavedDatas.insert_or_assign(std::forward<TKeyType>(keyValue), dataValue);
			if (!result.second)
			{
				if (replaceIfHaveKey)
				{
					TAssetType* oldValue = SavedDatas[std::forward<TKeyType>(keyValue)];
					SavedDatas[std::forward<TKeyType>(keyValue)] = dataValue;
					delete oldValue;
					return static_cast<UINT>(SavedDatas.size());
				}
				return 0u;
			}
			return static_cast<UINT>(SavedDatas.size());
		}
		/*
		* Remove item with kayValue in mapped datas.
		* Return [UINT]: Mapped datas' size(after remove value).
		*/
		UINT Remove(TKeyType&& keyValue)
		{
			auto findIt = SavedDatas.find(std::forward<TKeyType>(keyValue));
			if (findIt != SavedDatas.end())
			{
				TAssetType* oldValue = findIt->second;
				SavedDatas.erase(std::forward<TKeyType>(keyValue));
				delete oldValue;
			}
			return static_cast<UINT>(SavedDatas.size());
		}
		/*
		* Find item with kayValue in mapped datas.
		* Return [TAssetType*]: If contain key return asset. If not then return nullptr.
		*/
		TAssetType* Find(const TKeyType& keyValue)
		{
			auto findIt = SavedDatas.find(keyValue);
			if (findIt != SavedDatas.end())
			{
				return (findIt->second);
			}
			return nullptr;
		}
		/*
		* Check if contains item with kayValue in mapped datas.
		* Return [BOOL]: If contains return TRUE. If not then return FALSE.
		*/
		BOOL Contain(const TKeyType& keyValue)const
		{
			auto findIt = SavedDatas.find(keyValue);
			if (findIt != SavedDatas.end())
			{
				return TRUE;
			}
			return FALSE;
		}
		/*
		* Clear whole data list.
		*/
		void Clear()
		{
			for (auto it = SavedDatas.begin(); it != SavedDatas.end(); it++)
			{
				delete (it->second);
			}
			SavedDatas.clear();
		}
		/*
		* Get size of mapped datas.
		*/
		UINT Size()const
		{
			return static_cast<UINT>(SavedDatas.size());
		}
	protected:
		std::unordered_map<TKeyType, TAssetType*> SavedDatas;

		CLASS_REMOVE_COPY_BODY(TAssetManager)
	};

};