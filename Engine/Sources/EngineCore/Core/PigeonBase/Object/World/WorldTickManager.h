#pragma once
#pragma once

#include <CoreMinimal.h>
#include "Base/Delegate/Delegate.h"
#include "Base/DataStructure/ObjectBase.h"

namespace PigeonEngine
{
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldPrePhysicsTick, FLOAT)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldPhysicsTick, FLOAT)
	MAKE_DELEGATE_MULTI_ONE_PARAM(FOnWorldPostPhysicsTick, FLOAT)

    class EWorldTickManager final : public EManagerBase
	{
	public:
		void Initialize() override;
		void ShutDown() override;
		void Init();
		void Uninit();

		void Tick(FLOAT deltaTime)
		{
			OnPrePhysicsTick.Broadcast(deltaTime);
			OnPhysicsTick.Broadcast(deltaTime);
			OnPostPhysicsTick.Broadcast(deltaTime);
		}

		void RegisterPrePhysicsTick(const TFunction<void(FLOAT)>& InHandler)
		{
			OnPrePhysicsTick.Add(InHandler);
		}

		void UnregisterPrePhysicsTick(const TFunction<void(FLOAT)>& InHandler)
		{
			OnPrePhysicsTick.Remove(InHandler);
		}

		void RegisterPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
		{
			OnPhysicsTick.Add(InHandler);
		}

		void UnregisterPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
		{
			OnPhysicsTick.Remove(InHandler);
		}

		void RegisterPostPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
		{
			OnPostPhysicsTick.Add(InHandler);
		}

		void UnregisterPostPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
		{
			OnPostPhysicsTick.Remove(InHandler);
		}

		FOnWorldPrePhysicsTick OnPrePhysicsTick;
		FOnWorldPhysicsTick OnPhysicsTick;
		FOnWorldPostPhysicsTick OnPostPhysicsTick;

 private:
		CLASS_MANAGER_SINGLETON_BODY(EWorldTickManager)
	};

	inline EWorldTickManager::EWorldTickManager()
	{
	}

	inline EWorldTickManager::~EWorldTickManager()
	{
	}

	inline void EWorldTickManager::Initialize()
	{
	}

	inline void EWorldTickManager::ShutDown()
	{
		OnPrePhysicsTick.RemoveAll();
		OnPhysicsTick.RemoveAll();
		OnPostPhysicsTick.RemoveAll();
	}

	inline void EWorldTickManager::Init()
	{
	}

	inline void EWorldTickManager::Uninit()
	{
	}
}
