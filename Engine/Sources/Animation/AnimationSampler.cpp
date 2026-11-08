#include "AnimationSampler.h"

namespace PigeonEngine
{

	FLOAT EAnimationSampler::WrapKeyTime(FLOAT InKeyTime, FLOAT InDuration)
	{
		if (InDuration <= 0.0f)
		{
			return 0.0f;
		}
		FLOAT WrappedTime = EMath::FMod(InKeyTime, InDuration);
		if (WrappedTime < 0.0f)
		{
			WrappedTime += InDuration;
		}
		return WrappedTime;
	}

}
