#include "AnimInstance.h"

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
		const BOOL32 bBound = BindAnimation();
		bPoseDirty = TRUE;
		return bBound;
	}

	const ESkeletonAnimationAsset* EAnimInstance::GetAnimationAsset()const
	{
		return AnimationAsset;
	}

	BOOL32 EAnimInstance::Play(const EString& InClipName, BOOL32 bInLoop, FLOAT InPlayRate)
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
		return Play(ClipIndex, bInLoop, InPlayRate);
	}

	BOOL32 EAnimInstance::Play(INT32 InClipIndex, BOOL32 bInLoop, FLOAT InPlayRate)
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

		CurrentClipIndex = InClipIndex;
		ClipPlayer.SetClip(Clip, InClipIndex);
		ClipPlayer.SetLoopMode(bInLoop ? ANIMATION_LOOP_REPEAT : ANIMATION_LOOP_ONCE);
		ClipPlayer.SetPlayRate(InPlayRate);
		ClipPlayer.Play();
		bPoseDirty = TRUE;
		return TRUE;
	}

	void EAnimInstance::Pause()
	{
		ClipPlayer.Pause();
	}

	void EAnimInstance::Resume()
	{
		if (ClipPlayer.GetClip())
		{
			ClipPlayer.Play();
			bPoseDirty = TRUE;
		}
	}

	void EAnimInstance::Stop()
	{
		ClipPlayer.Reset();
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

	BOOL32 EAnimInstance::IsLooping()const
	{
		return ((ClipPlayer.GetLoopMode() == ANIMATION_LOOP_REPEAT) ? TRUE : FALSE);
	}

	void EAnimInstance::SetPositionSeconds(FLOAT InPositionSeconds)
	{
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
		const BOOL32 bPreviousLoop = ((ClipPlayer.GetLoopMode() == ANIMATION_LOOP_REPEAT) ? TRUE : FALSE);

		Binding.Unbind();
		ClipPlayer.Reset();
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
			if (Play(PreviousClipIndex, bPreviousLoop, PreviousPlayRate))
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

		const BOOL32 bAdvancing = ((ClipPlayer.IsPlaying()) && (ClipPlayer.GetClip() != nullptr)) ? TRUE : FALSE;
		if ((!bAdvancing) && (!bPoseDirty))
		{
			return FALSE;
		}
		// A zero delta cannot move a playing clip. Skip the resample unless something else
		// already asked for a new pose (the first frame after Play, a seek, an asset swap).
		if ((bAdvancing) && (InDeltaSeconds == 0.0f) && (!bPoseDirty))
		{
			return FALSE;
		}

		if (bAdvancing)
		{
			ClipPlayer.Advance(InDeltaSeconds);
		}

		// The clip only drives the bones it has curves for. Starting from the skeleton's
		// default pose is what keeps every other bone at its bind transform instead of
		// keeping whatever the previous clip left there.
		ApplyDefaultPose(Binding.GetSkeleton());
		if (ClipPlayer.GetClip())
		{
			ClipPlayer.Evaluate(Binding, Component->GetBoneMemoryPool());
		}
		Component->GetBoneMemoryPool().RecursionToRootTransforms();
		bPoseDirty = FALSE;
		return TRUE;
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

	void EAnimInstance::ApplyDefaultPose(const ESkeleton* InSkeleton)
	{
		if ((!InSkeleton) || (!Component) || (!Component->GetBoneMemoryPool().IsValid()))
		{
			return;
		}

		const ESkeleton::EBonePart& Bones = InSkeleton->GetBonePart();
		TArray<EBoneTransform>& RelativeTransforms = Component->GetBoneMemoryPool().GetBoneRelativeTransforms();
		const INT32 BoneNum = EMath::Min(Bones.Num(), RelativeTransforms.Num());
		for (INT32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
		{
			RelativeTransforms[BoneIndex].Position = Bones[BoneIndex].DefaultPosition;
			RelativeTransforms[BoneIndex].Rotation = Bones[BoneIndex].DefaultRotation;
			RelativeTransforms[BoneIndex].Scaling = Bones[BoneIndex].DefaultScaling;
		}
	}

	INT32 EAnimInstance::GetAnimationClipNum()const
	{
		const ESkeletonAnimation* Animation = (AnimationAsset ? AnimationAsset->GetStoragedResource() : nullptr);
		return (Animation ? Animation->GetAnimationClipNum() : 0);
	}

}
