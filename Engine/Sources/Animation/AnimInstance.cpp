#include "AnimInstance.h"

#include "AnimationNotify.h"

#include <AnimationAsset/AnimationAsset.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	EAnimInstance::EAnimInstance()
	{
	}

	EAnimInstance::~EAnimInstance()
	{
	}

	void EAnimInstance::SetSkeletalMeshComponent(PSkeletalMeshComponent* InComponent)
	{
		Component = InComponent;
		Binding.Unbind();
		ClipPlayer.Reset();
		CancelCrossFade();
		CurrentClipIndex = -1;
		bPoseDirty = ((AnimationAsset != nullptr) ? TRUE : FALSE);
	}

	PSkeletalMeshComponent* EAnimInstance::GetSkeletalMeshComponent()const
	{
		return Component;
	}

	BOOL32 EAnimInstance::SetAnimationAsset(const ESkeletonAnimationAsset* InAnimationAsset)
	{
		AnimationAsset = InAnimationAsset;
		CurrentClipIndex = -1;
		ClipPlayer.Reset();
		CancelCrossFade();
		const BOOL32 bBound = BindAnimation();
		bPoseDirty = TRUE;
		return bBound;
	}

	const ESkeletonAnimationAsset* EAnimInstance::GetAnimationAsset()const
	{
		return AnimationAsset;
	}

	BOOL32 EAnimInstance::Play(const EString& InClipName, BOOL32 bInLoop, FLOAT InPlayRate, FLOAT InFadeSeconds)
	{
		const ESkeletonAnimation* Animation = (Binding.IsValid() ? Binding.GetAnimation() : nullptr);
		if (!Animation)
		{
			return FALSE;
		}

		INT32 ClipIndex = -1;
		if (!(Animation->GetAnimationClipMapping().FindValue(InClipName, ClipIndex)))
		{
			return FALSE;
		}
		return Play(ClipIndex, bInLoop, InPlayRate, InFadeSeconds);
	}

	BOOL32 EAnimInstance::Play(INT32 InClipIndex, BOOL32 bInLoop, FLOAT InPlayRate, FLOAT InFadeSeconds)
	{
		return PlayInternal(InClipIndex, (bInLoop ? ANIMATION_LOOP_REPEAT : ANIMATION_LOOP_ONCE), InPlayRate, InFadeSeconds);
	}

	BOOL32 EAnimInstance::PlayInternal(INT32 InClipIndex, EAnimationLoopMode InLoopMode, FLOAT InPlayRate, FLOAT InFadeSeconds)
	{
		const ESkeletonAnimation* Animation = (Binding.IsValid() ? Binding.GetAnimation() : nullptr);
		if (!Animation)
		{
			return FALSE;
		}

		const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(InClipIndex);
		if (!Clip)
		{
			return FALSE;
		}

		// Hand the outgoing clip to the previous player before the new one takes over. A
		// same-clip restart has nothing to blend, so it switches immediately.
		if ((InFadeSeconds > 0.0f) && (ClipPlayer.GetClip() != nullptr) && (CurrentClipIndex != InClipIndex))
		{
			PreviousClipPlayer = ClipPlayer;
			CrossFadeDuration = InFadeSeconds;
			CrossFadeRemaining = InFadeSeconds;
		}
		else
		{
			CancelCrossFade();
		}

		CurrentClipIndex = InClipIndex;
		ClipPlayer.SetClip(Clip, InClipIndex);
		ClipPlayer.SetLoopMode(InLoopMode);
		ClipPlayer.SetPlayRate(InPlayRate);
		ClipPlayer.Play();
		bPoseDirty = TRUE;
		return TRUE;
	}

	void EAnimInstance::Pause()
	{
		ClipPlayer.Pause();
		if (CrossFadeRemaining > 0.0f)
		{
			PreviousClipPlayer.Pause();
		}
	}

	void EAnimInstance::Resume()
	{
		if (ClipPlayer.GetClip())
		{
			ClipPlayer.Play();
			if (CrossFadeRemaining > 0.0f)
			{
				PreviousClipPlayer.Play();
			}
			bPoseDirty = TRUE;
		}
	}

	void EAnimInstance::Stop()
	{
		ClipPlayer.Reset();
		CancelCrossFade();
		CurrentClipIndex = -1;
		bPoseDirty = TRUE;
	}

	void EAnimInstance::SetPlayRate(FLOAT InPlayRate)
	{
		ClipPlayer.SetPlayRate(InPlayRate);
	}

	FLOAT EAnimInstance::GetPlayRate()const
	{
		return ClipPlayer.GetPlayRate();
	}

	void EAnimInstance::SetLoop(BOOL32 bInLoop)
	{
		ClipPlayer.SetLoopMode(bInLoop ? ANIMATION_LOOP_REPEAT : ANIMATION_LOOP_ONCE);
	}

	void EAnimInstance::SetLoopMode(EAnimationLoopMode InLoopMode)
	{
		ClipPlayer.SetLoopMode(InLoopMode);
	}

	EAnimationLoopMode EAnimInstance::GetLoopMode()const
	{
		return ClipPlayer.GetLoopMode();
	}

	BOOL32 EAnimInstance::IsLooping()const
	{
		const EAnimationLoopMode LoopMode = ClipPlayer.GetLoopMode();
		return (((LoopMode == ANIMATION_LOOP_REPEAT) || (LoopMode == ANIMATION_LOOP_PINGPONG)) ? TRUE : FALSE);
	}

	void EAnimInstance::SetPositionSeconds(FLOAT InPositionSeconds)
	{
		// A seek is an explicit jump; blending a jumped pose with the old clip would look
		// wrong, so the crossfade is dropped.
		CancelCrossFade();
		ClipPlayer.SetPositionSeconds(InPositionSeconds);
		bPoseDirty = TRUE;
	}

	FLOAT EAnimInstance::GetPositionSeconds()const
	{
		return ClipPlayer.GetPositionSeconds();
	}

	BOOL32 EAnimInstance::IsPlaying()const
	{
		return ClipPlayer.IsPlaying();
	}

	const ESkeletonAnimationClip* EAnimInstance::GetCurrentClip()const
	{
		return ClipPlayer.GetClip();
	}

	INT32 EAnimInstance::GetCurrentClipIndex()const
	{
		return CurrentClipIndex;
	}

	void EAnimInstance::OnSkeletonChanged()
	{
		const BOOL32 bWasPlaying = ClipPlayer.IsPlaying();
		const INT32 PreviousClipIndex = CurrentClipIndex;
		const FLOAT PreviousPositionSeconds = ClipPlayer.GetPositionSeconds();
		const FLOAT PreviousPlayRate = ClipPlayer.GetPlayRate();
		const EAnimationLoopMode PreviousLoopMode = ClipPlayer.GetLoopMode();

		Binding.Unbind();
		ClipPlayer.Reset();
		CancelCrossFade();
		CurrentClipIndex = -1;
		// A component that has no animation asset yet must not touch the bone pool: the
		// editor preview writes its own pose into that pool, and an unconditional reset here
		// would erase it before the component sends its render state.
		bPoseDirty = ((AnimationAsset != nullptr) ? TRUE : FALSE);

		if ((!AnimationAsset) || (!BindAnimation()))
		{
			return;
		}

		// Keep the character doing what it was doing when the skeleton swap is compatible.
		// A clip that no longer exists leaves the player stopped, which is the safe answer.
		if ((PreviousClipIndex >= 0) && (PreviousClipIndex < GetAnimationClipNum()))
		{
			if (PlayInternal(PreviousClipIndex, PreviousLoopMode, PreviousPlayRate, 0.0f))
			{
				ClipPlayer.SetPositionSeconds(PreviousPositionSeconds);
				if (!bWasPlaying)
				{
					ClipPlayer.Pause();
				}
			}
		}
	}

	BOOL32 EAnimInstance::Update(FLOAT InDeltaSeconds)
	{
		if (!Component)
		{
			return FALSE;
		}

		if (!Binding.IsValid())
		{
			// The asset and the skeleton could not be bound. A caller that asked for a pose
			// change (stop, asset swap, skeleton swap) still gets the skeleton's default pose
			// instead of keeping a pose from an animation that is no longer bound.
			if (bPoseDirty)
			{
				if (Component->GetBoneMemoryPool().IsValid())
				{
					const ESkeletonAsset* SkeletonAsset = Component->GetSkeletonAsset();
					ApplyDefaultPose(SkeletonAsset ? SkeletonAsset->GetStoragedResource() : nullptr);
					Component->GetBoneMemoryPool().RecursionToRootTransforms();
				}
				bPoseDirty = FALSE;
				return TRUE;
			}
			return FALSE;
		}

		const BOOL32 bCrossFading = ((CrossFadeRemaining > 0.0f) && (PreviousClipPlayer.GetClip() != nullptr)) ? TRUE : FALSE;
		const BOOL32 bAdvancing = ((ClipPlayer.IsPlaying()) && (ClipPlayer.GetClip() != nullptr)) ? TRUE : FALSE;
		const BOOL32 bPreviousAdvancing = ((bCrossFading) && (PreviousClipPlayer.IsPlaying())) ? TRUE : FALSE;
		if ((!bAdvancing) && (!bPreviousAdvancing) && (!bPoseDirty))
		{
			return FALSE;
		}
		// A zero delta cannot move a playing clip. Skip the resample unless something else
		// already asked for a new pose (the first frame after Play, a seek, an asset swap).
		if ((!bPoseDirty) && (InDeltaSeconds == 0.0f))
		{
			return FALSE;
		}

		if (bAdvancing)
		{
			ClipPlayer.Advance(InDeltaSeconds);
		}
		if (bPreviousAdvancing)
		{
			PreviousClipPlayer.Advance(InDeltaSeconds);
		}

		// The clip only drives the bones it has curves for. Starting from the skeleton's
		// default pose is what keeps every other bone at its bind transform instead of
		// keeping whatever the previous clip left there.
		ApplyDefaultPose(Binding.GetSkeleton());
		if (ClipPlayer.GetClip())
		{
			ClipPlayer.Evaluate(Binding, Component->GetBoneMemoryPool().GetBoneRelativeTransforms());
		}

		if (bCrossFading)
		{
			CrossFadeRemaining = EMath::Max(CrossFadeRemaining - InDeltaSeconds, 0.0f);
			const FLOAT Weight = ((CrossFadeDuration > 0.0f)
				? EMath::Clamp(1.0f - (CrossFadeRemaining / CrossFadeDuration), 0.0f, 1.0f)
				: 1.0f);

			// The previous clip is sampled into its own buffer, starting from the default
			// pose, then the two poses are blended per bone. Bones neither clip drives stay
			// at the default pose on both sides, so the blend changes nothing there.
			TArray<EBoneTransform>& CurrentPose = Component->GetBoneMemoryPool().GetBoneRelativeTransforms();
			if (CrossFadePose.Num() != CurrentPose.Num())
			{
				CrossFadePose.SetNum(CurrentPose.Num());
			}
			ApplyDefaultPose(Binding.GetSkeleton(), CrossFadePose);
			PreviousClipPlayer.Evaluate(Binding, CrossFadePose);

			const INT32 BoneNum = EMath::Min(CurrentPose.Num(), CrossFadePose.Num());
			for (INT32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
			{
				EBoneTransform& CurrentTransform = CurrentPose[BoneIndex];
				const EBoneTransform& PreviousTransform = CrossFadePose[BoneIndex];
				CurrentTransform.Position = Vector3::Lerp(PreviousTransform.Position, CurrentTransform.Position, Weight);
				CurrentTransform.Rotation = Quaternion::SLerp(PreviousTransform.Rotation, CurrentTransform.Rotation, Weight);
				CurrentTransform.Scaling = Vector3::Lerp(PreviousTransform.Scaling, CurrentTransform.Scaling, Weight);
			}

			if (CrossFadeRemaining <= 0.0f)
			{
				CancelCrossFade();
			}
		}

		Component->GetBoneMemoryPool().RecursionToRootTransforms();
		if (bAdvancing)
		{
			// Notifies run after the pose and bone-to-root matrices are current, so a notify
			// that spawns an effect at a socket sees this frame's pose.
			DispatchNotifies();
		}
		bPoseDirty = FALSE;
		return TRUE;
	}

	void EAnimInstance::DispatchNotifies()
	{
		const ESkeletonAnimationClip* Clip = ClipPlayer.GetClip();
		if (!Clip)
		{
			return;
		}
		const TArray<FAnimNotifyEvent>& Notifies = Clip->Notifies;
		if (Notifies.Num() <= 0)
		{
			return;
		}

		const FLOAT PreviousTime = ClipPlayer.GetPreviousPositionSeconds();
		const FLOAT CurrentTime = ClipPlayer.GetPositionSeconds();
		const FLOAT DurationSeconds = ClipPlayer.GetClipDurationSeconds();
		const BOOL32 bLooped = ClipPlayer.HasLoopedLastAdvance();

		for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& Notify = Notifies[NotifyIndex];
			if (!Notify.CachedNotify)
			{
				continue;
			}

			const FLOAT NotifyTime = EMath::Clamp(Notify.Time, 0.0f, DurationSeconds);
			BOOL32 bFire = FALSE;
			if (bLooped)
			{
				// REPEAT crossed the end this frame: the first half runs from PreviousTime to
				// the clip end, the second from the start to CurrentTime. The wrap segment
				// includes 0 so an event authored there fires once per loop.
				bFire = (((NotifyTime > PreviousTime) && (NotifyTime <= DurationSeconds))
					|| ((NotifyTime >= 0.0f) && (NotifyTime <= CurrentTime))) ? TRUE : FALSE;
			}
			else if (CurrentTime >= PreviousTime)
			{
				bFire = ((NotifyTime > PreviousTime) && (NotifyTime <= CurrentTime)) ? TRUE : FALSE;
			}
			else
			{
				bFire = ((NotifyTime < PreviousTime) && (NotifyTime >= CurrentTime)) ? TRUE : FALSE;
			}

			if (bFire)
			{
				Notify.CachedNotify->Notify(*this, Notify);
			}
		}
	}

	BOOL32 EAnimInstance::BindAnimation()
	{
		Binding.Unbind();
		if ((!Component) || (!AnimationAsset))
		{
			return FALSE;
		}

		const ESkeletonAsset* SkeletonAsset = Component->GetSkeletonAsset();
		if (!SkeletonAsset)
		{
			return FALSE;
		}

		const ESkeleton* Skeleton = SkeletonAsset->GetStoragedResource();
		const ESkeletonAnimation* Animation = AnimationAsset->GetStoragedResource();
		return Binding.Bind(Skeleton, Animation);
	}

	void EAnimInstance::CancelCrossFade()
	{
		PreviousClipPlayer.Reset();
		CrossFadePose.Empty();
		CrossFadeDuration = 0.0f;
		CrossFadeRemaining = 0.0f;
	}

	void EAnimInstance::ApplyDefaultPose(const ESkeleton* InSkeleton)
	{
		if ((!InSkeleton) || (!Component) || (!Component->GetBoneMemoryPool().IsValid()))
		{
			return;
		}
		ApplyDefaultPose(InSkeleton, Component->GetBoneMemoryPool().GetBoneRelativeTransforms());
	}

	void EAnimInstance::ApplyDefaultPose(const ESkeleton* InSkeleton, TArray<EBoneTransform>& InOutPose)
	{
		if ((!InSkeleton) || (InOutPose.Num() <= 0))
		{
			return;
		}

		const ESkeleton::EBonePart& Bones = InSkeleton->GetBonePart();
		const INT32 BoneNum = EMath::Min(Bones.Num(), InOutPose.Num());
		for (INT32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
		{
			InOutPose[BoneIndex].Position = Bones[BoneIndex].DefaultPosition;
			InOutPose[BoneIndex].Rotation = Bones[BoneIndex].DefaultRotation;
			InOutPose[BoneIndex].Scaling = Bones[BoneIndex].DefaultScaling;
		}
	}

	INT32 EAnimInstance::GetAnimationClipNum()const
	{
		const ESkeletonAnimation* Animation = (AnimationAsset ? AnimationAsset->GetStoragedResource() : nullptr);
		return (Animation ? Animation->GetAnimationClipNum() : 0);
	}

}
