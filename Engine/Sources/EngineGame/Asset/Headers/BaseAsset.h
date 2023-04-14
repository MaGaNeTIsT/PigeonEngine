#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "../../../EngineCore/Base/Headers/ObjectBase.h"
#include "../../../EngineCore/Base/Headers/RTTI.h"

namespace PigeonEngine
{

	template<typename TCPUResourceType, typename TGPUResourceType>
	class TBaseAsset : public EObjectBase
	{
		EClass(TBaseAsset, EObjectBase);
	public:
		TBaseAsset() : m_CPUResource(nullptr), m_GPUResource(nullptr) {}
		virtual ~TBaseAsset()
		{
			ReleaseCPUResourceInternal();
			ReleaseGPUResourceInternal();
		}
	public:
		BOOL IsCPUResourceValid()const { return (!!m_CPUResource); }
		BOOL IsGPUResourceValid()const { return (!!m_GPUResource); }
	public:
		template<typename TCPUResourceLambdaType, typename TGPUResourceLambdaType>
		BOOL InitResource(const TCPUResourceLambdaType& lCPUFunc, const TGPUResourceLambdaType& lGPUFunc, const BOOL& bHoldCPUResource)
		{
			if (m_CPUResource || m_GPUResource)
			{
				//TODO Output log with "already storage data".
				return FALSE;
			}
			{
				m_CPUResource = lCPUFunc();
			}
			if (!m_CPUResource)
			{
				//TODO Output log with "CPU data init failed".
				return FALSE;
			}
			{
				m_GPUResource = lGPUFunc(m_CPUResource);
			}
			if (!m_GPUResource)
			{
				//TODO Output log with "GPU data init failed".
				ReleaseCPUResourceInternal();
				return FALSE;
			}
			if (!bHoldCPUResource)
			{
				ReleaseCPUResourceInternal();
			}
		}
		void UninitResource()
		{
			ReleaseCPUResourceInternal();
			ReleaseGPUResourceInternal();
		}
	protected:
		void ReleaseCPUResourceInternal()
		{
			if (m_CPUResource)
			{
				m_CPUResource->Release();
				delete m_CPUResource;
				m_CPUResource = nullptr;
			}
		}
		void ReleaseGPUResourceInternal()
		{
			if (m_GPUResource)
			{
				m_GPUResource->Release();
				delete m_GPUResource;
				m_GPUResource = nullptr;
			}
		}
	protected:
		TCPUResourceType* m_CPUResource;
		TGPUResourceType* m_GPUResource;
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
		UINT Add(TKeyType&& keyValue, TAssetType* dataValue, const BOOL& replaceIfHaveKey = false)
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