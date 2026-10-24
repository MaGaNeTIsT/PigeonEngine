#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <SkeletonAsset/SkeletonAsset.h>

#include "AnimationTypes.h"

namespace PigeonEngine
{

	struct ESkeletonAnimationClip;
	class ESkeletonBinding;

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
		// TRUE after a one-shot clip reached its end and stopped. Pause() does not set it,
		// so a paused clip can resume without being treated as finished.
		PE_NODISCARD BOOL32 HasFinished()const;

		void SetPlayRate(FLOAT InPlayRate);
		PE_NODISCARD FLOAT GetPlayRate()const;

		void SetLoopMode(EAnimationLoopMode InLoopMode);
		PE_NODISCARD EAnimationLoopMode GetLoopMode()const;

		void SetPositionSeconds(FLOAT InPositionSeconds);
		PE_NODISCARD FLOAT GetPositionSeconds()const;
		PE_NODISCARD FLOAT GetPreviousPositionSeconds()const;
		PE_NODISCARD BOOL32 HasLoopedLastAdvance()const;
		PE_NODISCARD FLOAT GetClipDurationSeconds()const;

		/*
		* Advances the playback position by InDeltaSeconds according to the loop mode. A
		* negative play rate is supported; ping-pong keeps its own direction so a seek does
		* not leave it travelling the wrong way.
		*/
		void Advance(FLOAT InDeltaSeconds);

		/*
		* Samples the clip at the current position and writes the driven bones into
		* InOutPose. The caller owns the array and has already filled it with the base pose,
		* so bones without a curve keep their base value. Returns FALSE when there is no clip
		* or no usable binding.
		*/
		BOOL32 Evaluate(const ESkeletonBinding& InBinding, TArray<EBoneTransform>& InOutPose)const;

	private:
		PE_NODISCARD FLOAT GetTicksPerSecond()const;

	private:
		const ESkeletonAnimationClip*	Clip = nullptr;
		INT32							ClipIndex = -1;
		FLOAT							PositionSeconds = 0.0f;
		// The position at the start of the last Advance, used to find the notify events
		// crossed this frame. SetPositionSeconds moves it with the position so a seek does
		// not replay every event between the old and new time.
		FLOAT							PreviousPositionSeconds = 0.0f;
		FLOAT							PlayRate = 1.0f;
		EAnimationLoopMode				LoopMode = ANIMATION_LOOP_REPEAT;
		// +1 forward, -1 backward. Only ping-pong changes it at runtime; the other modes
		// derive their direction from the sign of PlayRate.
		INT32							PlayDirection = 1;
		BOOL32							bPlaying = FALSE;
		BOOL32							bFinished = FALSE;
		// TRUE only when REPEAT wrapped around an end. Ping-pong reflections are handled as
		// a normal forward/backward interval, which is the documented first-version policy.
		BOOL32							bLoopedLastAdvance = FALSE;
		// The first advance after Play/SetClip starts from "before zero" for notify purposes,
		// so an event authored at time 0 still fires once.
		BOOL32							bFirstAdvance = TRUE;
	};

}
