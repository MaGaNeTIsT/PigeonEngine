#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	/*
	* How a clip behaves when playback reaches one of its ends.
	*
	* ONCE      plays forward once and stops on the last frame.
	* REPEAT    wraps around in the playback direction, which also supports a negative rate.
	* PINGPONG  reverses at each end and keeps going.
	* CLAMP     clamps to the first/last frame but stays in the playing state, which is the
	*           useful behaviour for a caller that wants to hold a pose and seek later.
	*/
	enum EAnimationLoopMode : UINT8
	{
		ANIMATION_LOOP_ONCE		= 0,
		ANIMATION_LOOP_REPEAT	= 1,
		ANIMATION_LOOP_PINGPONG	= 2,
		ANIMATION_LOOP_CLAMP	= 3
	};

}
