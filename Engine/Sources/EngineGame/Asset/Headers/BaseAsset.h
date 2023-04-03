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
	class IAssetManager
	{
	public:
		IAssetManager()
		{
		}
		virtual ~IAssetManager()
		{
			if (m_Datas.size() > 0)
			{
				for (auto& itData : m_Datas)
				{
					if (itData.second)
					{
						delete (itData.second);
						itData.second = nullptr;
					}
				}
				m_Datas.clear();
			}
		}
	protected:
		//std::map<int, int*> m_Datas;
		std::map<TKeyType, TAssetType*> m_Datas;
	};

};