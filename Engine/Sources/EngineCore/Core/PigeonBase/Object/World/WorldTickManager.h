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

		void Tick(FLOAT deltaTime);

		void RegisterPrePhysicsTick(const TFunction<void(FLOAT)>& InHandler);

		void UnregisterPrePhysicsTick(const TFunction<void(FLOAT)>& InHandler);

		void RegisterPhysicsTick(const TFunction<void(FLOAT)>& InHandler);

		void UnregisterPhysicsTick(const TFunction<void(FLOAT)>& InHandler);

		void RegisterPostPhysicsTick(const TFunction<void(FLOAT)>& InHandler);

		void UnregisterPostPhysicsTick(const TFunction<void(FLOAT)>& InHandler);

		FOnWorldPrePhysicsTick OnPrePhysicsTick;
		FOnWorldPhysicsTick OnPhysicsTick;
		FOnWorldPostPhysicsTick OnPostPhysicsTick;

 private:
		CLASS_MANAGER_SINGLETON_BODY(EWorldTickManager)
	};
}
