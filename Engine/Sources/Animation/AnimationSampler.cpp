#include "AnimationSampler.h"

#include <cmath>

namespace PigeonEngine
{

	FLOAT EAnimationSampler::WrapKeyTime(FLOAT InKeyTime, FLOAT InDuration)
	{
		if (InDuration <= 0.0f)
		{
			return 0.0f;
		}
		FLOAT WrappedTime = std::fmod(InKeyTime, InDuration);
		if (WrappedTime < 0.0f)
		{
			WrappedTime += InDuration;
		}
		return WrappedTime;
	}

}
