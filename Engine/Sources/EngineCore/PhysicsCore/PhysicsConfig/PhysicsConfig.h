#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class FCommonSettings
	{
	public:
		//Initialized Parameters		
		PE_CONSTEXPR_STATIC UINT32 PHYSICS_PRE_ALLOCATED_SIZE		= 70 * 1024 * 1024;			//pre-allocated memory for simulation.
		PE_CONSTEXPR_STATIC UINT32 PHYSICS_MAX_BODIES				= 65535;					//Max amount of rigid bodies.
		PE_CONSTEXPR_STATIC UINT32 PHYSICS_NUM_BODY_MUTEXES			= 0;						//Mutexes count,0 to default setting.
		PE_CONSTEXPR_STATIC UINT32 PHYSICS_MAX_BODY_PAIRS			= 65535;					//Max amount of body pairs that can be queued at any time.
		PE_CONSTEXPR_STATIC UINT32 PHYSICS_MAX_CONTACT_CONSTRAINTS	= 65535;					//This is the maximum size of the contact constraint buffer.

		//Runtime Parameters
		PE_CONSTEXPR_STATIC INT32 PHYSICS_COLLISION_STEPS			= 1;						// Do n collision step per cDeltaTime
	public:
		static FCommonSettings* GetSingleton()
		{
			static FCommonSettings _StaticSingletonObject;
			return (&_StaticSingletonObject);
		}
		FCommonSettings(const FCommonSettings&) = delete;
		FCommonSettings& operator=(const FCommonSettings&) = delete;
	private:
		FCommonSettings() {}
		~FCommonSettings() {}
	};
}