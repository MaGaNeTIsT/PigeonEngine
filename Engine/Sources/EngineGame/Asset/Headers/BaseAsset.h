#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "../../../EngineCore/Base/Headers/ObjectBase.h"
#include "../../../EngineCore/Base/Headers/RTTI.h"

namespace PigeonEngine
{

	template<typename TResourceType>
	class TBaseAsset : public EObjectBase
	{
		//EClass(TBaseAsset, EObjectBase);
	public:
		TBaseAsset(
#ifdef _DEVELOPMENT_EDITOR
			const std::string& name
#endif
		)
			: m_Resource(nullptr)
#ifdef _DEVELOPMENT_EDITOR
			, m_Name(name)
#endif
		{
		}
		virtual ~TBaseAsset()
		{
			UninitResource();
		}
	public:
		virtual BOOL	IsValid() { return IsResourceValid(); }
		virtual BOOL	InitResource() { return FALSE; }
		virtual void	UninitResource()
		{
			ReleaseResourceInternal();
		}
	public:
		BOOL	IsResourceValid()const { return (!!m_Resource); }
	protected:
		template<typename TInitResourceLambdaType>
		BOOL StorageResourceInternal(const TInitResourceLambdaType& lCPUFunc)
		{
			if (m_Resource)
			{
#ifdef _DEVELOPMENT_EDITOR
				std::string errorInfo = m_Name + " try to init a new resource, but already storage a resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			{
				m_Resource = lCPUFunc();
			}
			if (!m_Resource)
			{
#ifdef _DEVELOPMENT_EDITOR
				std::string errorInfo = m_Name + " try to storage a null resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			return TRUE;
		}
		void ReleaseResourceInternal()
		{
			if (m_Resource)
			{
				m_Resource->Release();
				delete m_Resource;
				m_Resource = nullptr;
			}
		}
	protected:
		TResourceType* m_Resource;
	};

	template<typename TResourceType, typename TRenderResourceType>
	class TRenderBaseAsset : public TBaseAsset<TResourceType>
	{
	public:
		TRenderBaseAsset(
#ifdef _DEVELOPMENT_EDITOR
			const std::string& name
#endif
		)
#ifdef _DEVELOPMENT_EDITOR
			: TBaseAsset<TResourceType>(name), m_RenderResource(nullptr)
#else
			: m_RenderResource(nullptr)
#endif
			, m_HoldResource(TRUE)
		{
		}
		~TRenderBaseAsset()
		{
		}
	public:
		virtual BOOL IsValid()override
		{
			if (m_HoldResource)
			{
				return IsResourceValid() && IsRenderResourceValid();
			}
			return IsRenderResourceValid();
		}
		virtual BOOL	InitResource() { return FALSE; }
		virtual void	UninitResource()
		{
			TBaseAsset<TResourceType>::UninitResource();
			ReleaseRenderResourceInternal();
		}
	public:
		BOOL	IsRenderResourceValid()const { return (!!m_RenderResource); }
	protected:
		template<typename TCreateRenderResourceLambdaType>
		BOOL CreateRenderResourceInternal(const TCreateRenderResourceLambdaType& lCreateFunc, const BOOL& bHoldStoragedResource)
		{
			if (m_RenderResource)
			{
#ifdef _DEVELOPMENT_EDITOR
				std::string errorInfo = m_Name + " try to create gpu resource, but already has a resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			{
				m_RenderResource = lCreateFunc(m_Resource);
			}
			if (!m_RenderResource)
			{
#ifdef _DEVELOPMENT_EDITOR
				std::string errorInfo = m_Name + " try to storage a null resource.";
				PE_FAILED(ENGINE_ASSET_ERROR, errorInfo);
#endif
				return FALSE;
			}
			if (!bHoldStoragedResource)
			{
				ReleaseResourceInternal();
			}
			m_HoldResource = bHoldStoragedResource;
			return TRUE;
		}
		void ReleaseRenderResourceInternal()
		{
			if (m_RenderResource)
			{
				m_RenderResource->Release();
				delete m_RenderResource;
				m_RenderResource = nullptr;
			}
		}
	protected:
		BOOL					m_HoldResource;
		TRenderResourceType*	m_RenderResource;
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
			if (m_Datas.size() > 0)
			{
				for (auto it = m_Datas.begin(); it != m_Datas.end(); it++)
				{
					delete (it->second);
				}
				m_Datas.clear();
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
			auto result = m_Datas.insert_or_assign(std::forward<TKeyType>(keyValue), dataValue);
			if (!result.second)
			{
				if (replaceIfHaveKey)
				{
					TAssetType* oldValue = m_Datas[std::forward<TKeyType>(keyValue)];
					m_Datas[std::forward<TKeyType>(keyValue)] = dataValue;
					delete oldValue;
					return static_cast<UINT>(m_Datas.size());
				}
				return 0u;
			}
			return static_cast<UINT>(m_Datas.size());
		}
		/*
		* Remove item with kayValue in mapped datas.
		* Return [UINT]: Mapped datas' size(after remove value).
		*/
		UINT Remove(TKeyType&& keyValue)
		{
			auto findIt = m_Datas.find(std::forward<TKeyType>(keyValue));
			if (findIt != m_Datas.end())
			{
				TAssetType* oldValue = findIt->second;
				m_Datas.erase(std::forward<TKeyType>(keyValue));
				delete oldValue;
			}
			return static_cast<UINT>(m_Datas.size());
		}
		/*
		* Find item with kayValue in mapped datas.
		* Return [TAssetType*]: If contain key return asset. If not then return nullptr.
		*/
		TAssetType* Find(TKeyType&& keyValue)
		{
			auto findIt = m_Datas.find(std::forward<TKeyType>(keyValue));
			if (findIt != m_Datas.end())
			{
				return (findIt->second);
			}
			return nullptr;
		}
		/*
		* Check if contains item with kayValue in mapped datas.
		* Return [BOOL]: If contains return TRUE. If not then return FALSE.
		*/
		BOOL Contain(TKeyType&& keyValue)const
		{
			auto findIt = m_Datas.find(std::forward<TKeyType>(keyValue));
			if (findIt != m_Datas.end())
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
			for (auto it = m_Datas.begin(); it != m_Datas.end(); it++)
			{
				delete (it->second);
			}
			m_Datas.clear();
		}
		/*
		* Get size of mapped datas.
		*/
		UINT Size()const
		{
			return static_cast<UINT>(m_Datas.size());
		}
	protected:
		std::unordered_map<TKeyType, TAssetType*> m_Datas;
	};

};