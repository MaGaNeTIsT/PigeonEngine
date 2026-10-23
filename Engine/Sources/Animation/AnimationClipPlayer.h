#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	struct ESkeletonAnimationClip;
	class ESkeletonBoneMemoryPool;
	class ESkeletonBinding;

	enum EAnimationLoopMode : UINT8
	{
		ANIMATION_LOOP_ONCE		= 0,
		ANIMATION_LOOP_REPEAT	= 1
	};

	/*
	* Playback state for one animation clip.
	*
	* The player owns time, play rate, loop mode and pause state; it does not own the clip
	* or the skeleton. Evaluate() writes sampled local transforms into the pose buffer the
	* caller already filled with the skeleton's default pose, so a clip that animates only
	* part of the skeleton leaves the other bones in their bind pose.
	*
	* PositionSeconds is the playback position in seconds. The asset stores key times in
	* ticks and Duration in ticks, so sampling converts with TicksPerSecond (falling back to
	* 30 when the source did not declare one).
	*/
	class EAnimationClipPlayer
	{
	public:
		void Reset();

		void SetClip(const ESkeletonAnimationClip* InClip, INT32 InClipIndex);
		PE_NODISCARD const ESkeletonAnimationClip* GetClip()const;

		void Play();
		void Pause();
		PE_NODISCARD BOOL32 IsPlaying()const;

		void SetPlayRate(FLOAT InPlayRate);
		PE_NODISCARD FLOAT GetPlayRate()const;

		void SetLoopMode(EAnimationLoopMode InLoopMode);
		PE_NODISCARD EAnimationLoopMode GetLoopMode()const;

		void SetPositionSeconds(FLOAT InPositionSeconds);
		PE_NODISCARD FLOAT GetPositionSeconds()const;

		/*
		* Advances the playback position by InDeltaSeconds. One-shot playback stops at the
		* end of the clip; repeating playback wraps. A negative play rate is supported and
		* wraps backwards.
		*/
		void Advance(FLOAT InDeltaSeconds);

		/*
		* Samples the clip at the current position and writes the driven bones into
		* InOutPose. Returns FALSE when there is no clip or no usable binding.
		*/
		BOOL32 Evaluate(const ESkeletonBinding& InBinding, ESkeletonBoneMemoryPool& InOutPose)const;

	private:
		PE_NODISCARD FLOAT GetClipDurationSeconds()const;
		PE_NODISCARD FLOAT GetTicksPerSecond()const;

	private:
		const ESkeletonAnimationClip*	Clip = nullptr;
		INT32							ClipIndex = -1;
		FLOAT							PositionSeconds = 0.0f;
		FLOAT							PlayRate = 1.0f;
		EAnimationLoopMode				LoopMode = ANIMATION_LOOP_REPEAT;
		BOOL32							bPlaying = FALSE;
	};

}
