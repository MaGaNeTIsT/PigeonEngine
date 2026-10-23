#include "AnimationClipPlayer.h"

#include "AnimationSampler.h"
#include "SkeletonBinding.h"

#include <AnimationAsset/AnimationAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>

#include <cmath>

namespace PigeonEngine
{

	void EAnimationClipPlayer::Reset()
	{
		Clip = nullptr;
		ClipIndex = -1;
		PositionSeconds = 0.0f;
		PlayRate = 1.0f;
		LoopMode = ANIMATION_LOOP_REPEAT;
		bPlaying = FALSE;
	}

	void EAnimationClipPlayer::SetClip(const ESkeletonAnimationClip* InClip, INT32 InClipIndex)
	{
		Clip = InClip;
		ClipIndex = InClipIndex;
		PositionSeconds = 0.0f;
		if (!Clip)
		{
			bPlaying = FALSE;
		}
	}

	const ESkeletonAnimationClip* EAnimationClipPlayer::GetClip()const
	{
		return Clip;
	}

	void EAnimationClipPlayer::Play()
	{
		if (Clip)
		{
			bPlaying = TRUE;
		}
	}

	void EAnimationClipPlayer::Pause()
	{
		bPlaying = FALSE;
	}

	BOOL32 EAnimationClipPlayer::IsPlaying()const
	{
		return bPlaying;
	}

	void EAnimationClipPlayer::SetPlayRate(FLOAT InPlayRate)
	{
		PlayRate = InPlayRate;
	}

	FLOAT EAnimationClipPlayer::GetPlayRate()const
	{
		return PlayRate;
	}

	void EAnimationClipPlayer::SetLoopMode(EAnimationLoopMode InLoopMode)
	{
		LoopMode = InLoopMode;
	}

	EAnimationLoopMode EAnimationClipPlayer::GetLoopMode()const
	{
		return LoopMode;
	}

	void EAnimationClipPlayer::SetPositionSeconds(FLOAT InPositionSeconds)
	{
		const FLOAT DurationSeconds = GetClipDurationSeconds();
		if (DurationSeconds <= 0.0f)
		{
			PositionSeconds = 0.0f;
			return;
		}
		PositionSeconds = EMath::Clamp(InPositionSeconds, 0.0f, DurationSeconds);
	}

	FLOAT EAnimationClipPlayer::GetPositionSeconds()const
	{
		return PositionSeconds;
	}

	void EAnimationClipPlayer::Advance(FLOAT InDeltaSeconds)
	{
		if ((!bPlaying) || (!Clip))
		{
			return;
		}

		const FLOAT DurationSeconds = GetClipDurationSeconds();
		if (DurationSeconds <= 0.0f)
		{
			PositionSeconds = 0.0f;
			bPlaying = FALSE;
			return;
		}

		PositionSeconds += InDeltaSeconds * PlayRate;
		if (LoopMode == ANIMATION_LOOP_REPEAT)
		{
			PositionSeconds = std::fmod(PositionSeconds, DurationSeconds);
			if (PositionSeconds < 0.0f)
			{
				PositionSeconds += DurationSeconds;
			}
		}
		else
		{
			// One-shot playback clamps at both ends. Reaching either end stops the player,
			// which is what lets a caller distinguish "finished" from "paused at the end".
			if (PositionSeconds >= DurationSeconds)
			{
				PositionSeconds = DurationSeconds;
				bPlaying = FALSE;
			}
			else if (PositionSeconds <= 0.0f)
			{
				PositionSeconds = 0.0f;
				bPlaying = FALSE;
			}
		}
	}

	BOOL32 EAnimationClipPlayer::Evaluate(const ESkeletonBinding& InBinding, ESkeletonBoneMemoryPool& InOutPose)const
	{
		if ((!Clip) || (!InBinding.IsValid()) || (!InOutPose.IsValid()))
		{
			return FALSE;
		}

		const FLOAT TicksPerSecond = GetTicksPerSecond();
		FLOAT KeyTime = PositionSeconds * TicksPerSecond;
		if (LoopMode == ANIMATION_LOOP_REPEAT)
		{
			KeyTime = EAnimationSampler::WrapKeyTime(KeyTime, Clip->Duration);
		}
		else
		{
			KeyTime = EMath::Clamp(KeyTime, 0.0f, Clip->Duration);
		}

		TArray<EBoneTransform>& RelativeTransforms = InOutPose.GetBoneRelativeTransforms();
		auto VectorLerp = [](const Vector3& InValue0, const Vector3& InValue1, FLOAT InAlpha)->Vector3
		{
			return Vector3::Lerp(InValue0, InValue1, InAlpha);
		};
		auto QuaternionLerp = [](const Quaternion& InValue0, const Quaternion& InValue1, FLOAT InAlpha)->Quaternion
		{
			return Quaternion::SLerp(InValue0, InValue1, InAlpha);
		};

		for (INT32 CurveIndex = 0; CurveIndex < Clip->AnimationCurves.Num(); CurveIndex++)
		{
			const INT32 BoneIndex = InBinding.GetBoneIndex(ClipIndex, CurveIndex);
			if ((BoneIndex < 0) || (BoneIndex >= RelativeTransforms.Num()))
			{
				continue;
			}

			const EBoneAnimationCurve& Curve = Clip->AnimationCurves[CurveIndex];
			EBoneTransform& BoneTransform = RelativeTransforms[BoneIndex];
			EAnimationSampler::SampleKey(KeyTime, Curve.PositionKeys, BoneTransform.Position, VectorLerp);
			EAnimationSampler::SampleKey(KeyTime, Curve.RotationKeys, BoneTransform.Rotation, QuaternionLerp);
			EAnimationSampler::SampleKey(KeyTime, Curve.ScalingKeys, BoneTransform.Scaling, VectorLerp);
		}
		return TRUE;
	}

	FLOAT EAnimationClipPlayer::GetClipDurationSeconds()const
	{
		if ((!Clip) || (Clip->Duration <= 0.0f))
		{
			return 0.0f;
		}
		return (Clip->Duration / GetTicksPerSecond());
	}

	FLOAT EAnimationClipPlayer::GetTicksPerSecond()const
	{
		// A source that did not declare a tick rate is treated as 30 fps, which is the
		// convention the old animation component used and what the editor preview already
		// falls back to.
		return (((Clip) && (Clip->TicksPerSecond > 1e-3f)) ? Clip->TicksPerSecond : EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND);
	}

}
