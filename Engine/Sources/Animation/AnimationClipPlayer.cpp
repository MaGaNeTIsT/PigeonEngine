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
		PreviousPositionSeconds = 0.0f;
		PlayRate = 1.0f;
		LoopMode = ANIMATION_LOOP_REPEAT;
		PlayDirection = 1;
		bPlaying = FALSE;
		bFinished = FALSE;
		bLoopedLastAdvance = FALSE;
		bFirstAdvance = TRUE;
	}

	void EAnimationClipPlayer::SetClip(const ESkeletonAnimationClip* InClip, INT32 InClipIndex)
	{
		Clip = InClip;
		ClipIndex = InClipIndex;
		PositionSeconds = 0.0f;
		PreviousPositionSeconds = 0.0f;
		PlayDirection = 1;
		bFinished = FALSE;
		bLoopedLastAdvance = FALSE;
		bFirstAdvance = TRUE;
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
			bFinished = FALSE;
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

	BOOL32 EAnimationClipPlayer::HasFinished()const
	{
		return bFinished;
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
		PlayDirection = 1;
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
		// A seek is a jump, not playback; the next Advance must not treat the skipped span
		// as crossed notify events.
		PreviousPositionSeconds = PositionSeconds;
		PlayDirection = 1;
		bFinished = FALSE;
		bLoopedLastAdvance = FALSE;
		bFirstAdvance = FALSE;
	}

	FLOAT EAnimationClipPlayer::GetPositionSeconds()const
	{
		return PositionSeconds;
	}

	FLOAT EAnimationClipPlayer::GetPreviousPositionSeconds()const
	{
		return PreviousPositionSeconds;
	}

	BOOL32 EAnimationClipPlayer::HasLoopedLastAdvance()const
	{
		return bLoopedLastAdvance;
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
			bFinished = TRUE;
			return;
		}

		if (bFirstAdvance)
		{
			// Start just before zero so a notify authored at exactly 0 fires on the first
			// frame. After this frame the interval is the normal half-open (previous, current].
			PreviousPositionSeconds = -1.0f;
			bFirstAdvance = FALSE;
		}
		else
		{
			PreviousPositionSeconds = PositionSeconds;
		}
		bLoopedLastAdvance = FALSE;

		const FLOAT Travel = InDeltaSeconds * PlayRate;
		switch (LoopMode)
		{
		case ANIMATION_LOOP_REPEAT:
		{
			const FLOAT UnwrappedPosition = PositionSeconds + Travel;
			bLoopedLastAdvance = ((UnwrappedPosition < 0.0f) || (UnwrappedPosition >= DurationSeconds)) ? TRUE : FALSE;
			PositionSeconds = std::fmod(UnwrappedPosition, DurationSeconds);
			if (PositionSeconds < 0.0f)
			{
				PositionSeconds += DurationSeconds;
			}
			break;
		}
		case ANIMATION_LOOP_PINGPONG:
			PositionSeconds += Travel * static_cast<FLOAT>(PlayDirection);
			// Reflect at both ends. A normal frame crosses at most one end; the loop also
			// handles a long delta after a stall without leaving the [0, Duration] range.
			while ((PositionSeconds > DurationSeconds) || (PositionSeconds < 0.0f))
			{
				if (PositionSeconds > DurationSeconds)
				{
					PositionSeconds = DurationSeconds - (PositionSeconds - DurationSeconds);
					PlayDirection = -PlayDirection;
				}
				else
				{
					PositionSeconds = -PositionSeconds;
					PlayDirection = -PlayDirection;
				}
			}
			break;
		case ANIMATION_LOOP_CLAMP:
			// Hold the first/last frame but stay in the playing state, so a caller can seek
			// away from the end without treating the clip as finished.
			PositionSeconds = EMath::Clamp(PositionSeconds + Travel, 0.0f, DurationSeconds);
			break;
		case ANIMATION_LOOP_ONCE:
		default:
			PositionSeconds += Travel;
			if (PositionSeconds >= DurationSeconds)
			{
				PositionSeconds = DurationSeconds;
				bPlaying = FALSE;
				bFinished = TRUE;
			}
			else if (PositionSeconds <= 0.0f)
			{
				PositionSeconds = 0.0f;
				bPlaying = FALSE;
				bFinished = TRUE;
			}
			break;
		}
	}

	BOOL32 EAnimationClipPlayer::Evaluate(const ESkeletonBinding& InBinding, TArray<EBoneTransform>& InOutPose)const
	{
		if ((!Clip) || (!InBinding.IsValid()) || (InOutPose.Num() <= 0))
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

		TArray<EBoneTransform>& RelativeTransforms = InOutPose;
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
