#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Animation/AnimationTypes.h>

namespace PigeonEngine
{

	struct FTransportState
	{
	public:
		BOOL8				bPlaying = TRUE;
		FLOAT				PlayRate = 1.0f;
		EAnimationLoopMode	LoopMode = ANIMATION_LOOP_REPEAT;
		FLOAT				Duration = 0.0f;
		FLOAT				Time = 0.0f;
	};

	struct FTransportInteraction
	{
	public:
		BOOL32	bPlaybackChanged = FALSE;
		BOOL32	bSeekRequested = FALSE;
		FLOAT	SeekTime = 0.0f;
	};

	class FTransportBar
	{
	public:
		static void Draw(const CHAR* InId, FTransportState& InOutState, FTransportInteraction& OutInteraction);
	};

}

#endif
