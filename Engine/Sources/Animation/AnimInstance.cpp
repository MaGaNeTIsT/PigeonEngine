#include "AnimInstance.h"

#include "AnimationMontage.h"
#include "AnimationNotify.h"
#include "AnimationStateMachine.h"

#include <AnimationAsset/AnimationAsset.h>
#include <Editor/EditorLogManager.h>
#include <Development/Profiler.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	namespace
	{
		/*
		* Fires every event in InNotifies whose authored time lies in the interval the player
		* just crossed. Clip notifies and montage section notifies share this path, so the
		* timing policy (including a REPEAT wrap) is identical for both.
		*/
		void DispatchNotifyEventList(EAnimInstance& InInstance, const TArray<FAnimNotifyEvent>& InNotifies
			, FLOAT InPreviousTime, FLOAT InCurrentTime, FLOAT InDurationSeconds, BOOL32 bInLooped)
		{
			for (INT32 NotifyIndex = 0; NotifyIndex < InNotifies.Num(); NotifyIndex++)
			{
				const FAnimNotifyEvent& Notify = InNotifies[NotifyIndex];
				if (!Notify.CachedNotify)
				{
					continue;
				}

				const FLOAT NotifyTime = EMath::Clamp(Notify.Time, 0.0f, InDurationSeconds);
				BOOL32 bFire = FALSE;
				if (bInLooped)
				{
					// REPEAT crossed the end this frame: the first half runs from PreviousTime
					// to the clip end, the second from the start to CurrentTime. The wrap
					// segment includes 0 so an event authored there fires once per loop.
					bFire = (((NotifyTime > InPreviousTime) && (NotifyTime <= InDurationSeconds))
						|| ((NotifyTime >= 0.0f) && (NotifyTime <= InCurrentTime))) ? TRUE : FALSE;
				}
				else if (InCurrentTime >= InPreviousTime)
				{
					bFire = ((NotifyTime > InPreviousTime) && (NotifyTime <= InCurrentTime)) ? TRUE : FALSE;
				}
				else
				{
					bFire = ((NotifyTime < InPreviousTime) && (NotifyTime >= InCurrentTime)) ? TRUE : FALSE;
				}

				if (bFire)
				{
					Notify.CachedNotify->Notify(InInstance, Notify);
				}
			}
		}
	}

	EAnimInstance::EAnimInstance()
	{
	}

	EAnimInstance::~EAnimInstance()
	{
	}

	void EAnimInstance::SetSkeletalMeshComponent(PSkeletalMeshComponent* InComponent)
	{
		EndNotifyStates(ActiveBaseNotifyStates);
		ClearAllMontages();
		ResetRootMotionState();
		Component = InComponent;
		Binding.Unbind();
		ClipPlayer.Reset();
		CancelCrossFade();
		ClearStateMachine();
		CurrentClipIndex = -1;
		bPoseDirty = ((AnimationAsset != nullptr) ? TRUE : FALSE);
	}

	PSkeletalMeshComponent* EAnimInstance::GetSkeletalMeshComponent() const
	{
		return Component;
	}

	BOOL32 EAnimInstance::SetAnimationAsset(const ESkeletonAnimationAsset* InAnimationAsset)
	{
		EndNotifyStates(ActiveBaseNotifyStates);
		ClearAllMontages();
		ResetRootMotionState();
		const EAnimStateMachine* ActiveMachine = (StateMachineInstance ? StateMachineInstance->GetMachine() : nullptr);
		const BOOL32 bKeepStateMachine = ((ActiveMachine != nullptr) && (InAnimationAsset != nullptr)
			&& ((ActiveMachine->AnimationAssetName.Length() <= 0u)
				|| (ActiveMachine->AnimationAssetName == InAnimationAsset->GetAssetName()))) ? TRUE : FALSE;

		AnimationAsset = InAnimationAsset;
		CurrentClipIndex = -1;
		ClipPlayer.Reset();
		CancelCrossFade();
		if (!bKeepStateMachine)
		{
			if (ActiveMachine)
			{
				PE_LOG_WARN(EString("State machine '") + ActiveMachine->Name
					+ EString("' stopped because the animation asset changed."));
			}
			ClearStateMachine();
		}
		const BOOL32 bBound = BindAnimation();
		if ((bBound) && (bKeepStateMachine) && (StateMachineInstance) && (ActiveMachine))
		{
			// The new asset is the one the machine was authored against; restart the state's
			// clip so the player is not left empty until the next transition.
			const INT32 StateIndex = StateMachineInstance->GetCurrentStateIndex();
			if ((StateIndex >= 0) && (StateIndex < ActiveMachine->States.Num()))
			{
				const EAnimStateMachineState& State = ActiveMachine->States[StateIndex];
				if (!Play(State.ClipName, State.LoopMode, State.PlayRate, 0.0f))
				{
					PE_LOG_WARN(EString("State machine '") + ActiveMachine->Name + EString("' could not restart state '")
						+ State.Name + EString("'."));
				}
			}
		}
		bPoseDirty = TRUE;
		return bBound;
	}

	const ESkeletonAnimationAsset* EAnimInstance::GetAnimationAsset() const
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

	BOOL32 EAnimInstance::Play(const EString& InClipName, EAnimationLoopMode InLoopMode, FLOAT InPlayRate, FLOAT InFadeSeconds)
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
		return PlayInternal(ClipIndex, InLoopMode, InPlayRate, InFadeSeconds);
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

		// The outgoing clip's state windows end before its data stops being the active
		// timeline. This also covers restarting the same clip.
		EndNotifyStates(ActiveBaseNotifyStates);
		ResetRootMotionState();

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
		EndNotifyStates(ActiveBaseNotifyStates);
		ClearAllMontages();
		ResetRootMotionState();
		ClipPlayer.Reset();
		CancelCrossFade();
		CurrentClipIndex = -1;
		bPoseDirty = TRUE;
	}

	void EAnimInstance::SetPlayRate(FLOAT InPlayRate)
	{
		ClipPlayer.SetPlayRate(InPlayRate);
	}

	FLOAT EAnimInstance::GetPlayRate() const
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

	EAnimationLoopMode EAnimInstance::GetLoopMode() const
	{
		return ClipPlayer.GetLoopMode();
	}

	BOOL32 EAnimInstance::IsLooping() const
	{
		const EAnimationLoopMode LoopMode = ClipPlayer.GetLoopMode();
		return (((LoopMode == ANIMATION_LOOP_REPEAT) || (LoopMode == ANIMATION_LOOP_PINGPONG)) ? TRUE : FALSE);
	}

	void EAnimInstance::SetPositionSeconds(FLOAT InPositionSeconds)
	{
		// A seek is an explicit jump; blending a jumped pose with the old clip would look
		// wrong, so the crossfade is dropped. Active windows are closed and can reopen if
		// the new position is inside them.
		CancelCrossFade();
		EndNotifyStates(ActiveBaseNotifyStates);
		ResetRootMotionState();
		ClipPlayer.SetPositionSeconds(InPositionSeconds);
		bPoseDirty = TRUE;
	}

	FLOAT EAnimInstance::GetPositionSeconds() const
	{
		return ClipPlayer.GetPositionSeconds();
	}

	BOOL32 EAnimInstance::IsPlaying() const
	{
		return ClipPlayer.IsPlaying();
	}

	const ESkeletonAnimationClip* EAnimInstance::GetCurrentClip() const
	{
		return ClipPlayer.GetClip();
	}

	INT32 EAnimInstance::GetCurrentClipIndex() const
	{
		return CurrentClipIndex;
	}

	BOOL32 EAnimInstance::SetStateMachine(const EAnimStateMachine* InMachine)
	{
		ClearStateMachine();
		if ((!InMachine) || (!InMachine->IsValid()))
		{
			return FALSE;
		}

		StateMachineInstance = EMemory::MakeUnique<EAnimStateMachineInstance>();
		if (!StateMachineInstance)
		{
			return FALSE;
		}
		if (!StateMachineInstance->SetMachine(InMachine))
		{
			ClearStateMachine();
			return FALSE;
		}
		// The entry clip is entered on the next Update, once the animation asset binding is
		// available; if the clip is missing the instance logs once and retries.
		return TRUE;
	}

	void EAnimInstance::ClearStateMachine()
	{
		StateMachineInstance.reset();
	}

	EAnimStateMachineInstance* EAnimInstance::GetStateMachineInstance() const
	{
		return StateMachineInstance.get();
	}

	BOOL32 EAnimInstance::PlayMontage(const EAnimMontage* InMontage, const EString& InSectionName, FLOAT InPlayRate)
	{
		if ((!Component) || (!Binding.IsValid()) || (!InMontage) || (!InMontage->IsValid()))
		{
			return FALSE;
		}
		if ((AnimationAsset) && (InMontage->AnimationAssetName.Length() > 0u)
			&& (InMontage->AnimationAssetName != AnimationAsset->GetAssetName()))
		{
			PE_LOG_WARN(EString("Montage '") + InMontage->Name + EString("' expects animation asset '")
				+ InMontage->AnimationAssetName + EString("' but the component has '")
				+ AnimationAsset->GetAssetName() + EString("'."));
			return FALSE;
		}

		TUniquePtr<EAnimMontageInstance> NewInstance = EMemory::MakeUnique<EAnimMontageInstance>();
		if (!NewInstance)
		{
			return FALSE;
		}
		if (!NewInstance->SetMontage(InMontage, InSectionName, Binding.GetSkeleton(), Binding.GetAnimation(), InPlayRate))
		{
			return FALSE;
		}
		const EAnimMontageSection* NewSection = NewInstance->GetCurrentSection();
		if (!NewSection)
		{
			return FALSE;
		}

		// A slot can only host one montage. Replace the layer with the same current slot;
		// other slots remain stacked.
		for (INT32 Index = MontageInstances.Num() - 1; Index >= 0; Index--)
		{
			EAnimMontageInstance* Existing = MontageInstances[Index].get();
			const EAnimMontageSection* ExistingSection = (Existing ? Existing->GetCurrentSection() : nullptr);
			if ((ExistingSection) && (ExistingSection->SlotName == NewSection->SlotName))
			{
				EndNotifyStates(Existing->GetActiveNotifyStates());
				MontageInstances.RemoveAt(Index);
			}
		}

		MontageInstances.Add(static_cast<TUniquePtr<EAnimMontageInstance>&&>(NewInstance));
		SortMontagesByLayerPriority();
		bPoseDirty = TRUE;
		return TRUE;
	}

	void EAnimInstance::StopMontage(FLOAT InBlendOutSeconds)
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			if (Montage)
			{
				// A stop closes the windows immediately; the pose can still blend out.
				EndNotifyStates(Montage->GetActiveNotifyStates());
				Montage->Stop(InBlendOutSeconds);
			}
		}
	}

	void EAnimInstance::StopMontage(const EString& InSlotName, FLOAT InBlendOutSeconds)
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			const EAnimMontageSection* Section = (Montage ? Montage->GetCurrentSection() : nullptr);
			if ((Section) && (Section->SlotName == InSlotName))
			{
				EndNotifyStates(Montage->GetActiveNotifyStates());
				Montage->Stop(InBlendOutSeconds);
			}
		}
	}

	void EAnimInstance::PauseMontage()
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if (MontageInstances[Index])
			{
				MontageInstances[Index]->Pause();
			}
		}
	}

	void EAnimInstance::PauseMontage(const EString& InSlotName)
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			const EAnimMontageSection* Section = (Montage ? Montage->GetCurrentSection() : nullptr);
			if ((Section) && (Section->SlotName == InSlotName))
			{
				Montage->Pause();
			}
		}
	}

	void EAnimInstance::ResumeMontage()
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if (MontageInstances[Index])
			{
				MontageInstances[Index]->Resume();
			}
		}
	}

	void EAnimInstance::ResumeMontage(const EString& InSlotName)
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			const EAnimMontageSection* Section = (Montage ? Montage->GetCurrentSection() : nullptr);
			if ((Section) && (Section->SlotName == InSlotName))
			{
				Montage->Resume();
			}
		}
	}

	void EAnimInstance::SetMontagePositionSeconds(FLOAT InPositionSeconds)
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			if (Montage)
			{
				EndNotifyStates(Montage->GetActiveNotifyStates());
				Montage->SetPositionSeconds(InPositionSeconds);
			}
		}
		bPoseDirty = TRUE;
	}

	void EAnimInstance::SetMontagePlayRate(FLOAT InPlayRate)
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if (MontageInstances[Index])
			{
				MontageInstances[Index]->SetPlayRate(InPlayRate);
			}
		}
	}

	void EAnimInstance::SetMontageLoopMode(EAnimationLoopMode InLoopMode)
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if (MontageInstances[Index])
			{
				MontageInstances[Index]->SetLoopMode(InLoopMode);
			}
		}
	}

	BOOL32 EAnimInstance::IsMontagePlaying() const
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if ((MontageInstances[Index]) && (MontageInstances[Index]->IsPlaying()))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	BOOL32 EAnimInstance::IsMontagePlaying(const EString& InSlotName) const
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			const EAnimMontageSection* Section = (Montage ? Montage->GetCurrentSection() : nullptr);
			if ((Section) && (Section->SlotName == InSlotName) && (Montage->IsPlaying()))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	EAnimMontageInstance* EAnimInstance::GetMontageInstance() const
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if (MontageInstances[Index])
			{
				return MontageInstances[Index].get();
			}
		}
		return nullptr;
	}

	EAnimMontageInstance* EAnimInstance::GetMontageInstanceBySlot(const EString& InSlotName) const
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			const EAnimMontageSection* Section = (Montage ? Montage->GetCurrentSection() : nullptr);
			if ((Section) && (Section->SlotName == InSlotName))
			{
				return Montage;
			}
		}
		return nullptr;
	}

	INT32 EAnimInstance::GetMontageInstanceCount() const
	{
		return MontageInstances.Num();
	}

	EAnimMontageInstance* EAnimInstance::GetMontageInstanceAt(INT32 InIndex) const
	{
		if ((InIndex < 0) || (InIndex >= MontageInstances.Num()))
		{
			return nullptr;
		}
		return MontageInstances[InIndex].get();
	}

	void EAnimInstance::SetRootMotionEnabled(BOOL32 bInEnabled)
	{
		bRootMotionEnabled = (bInEnabled ? TRUE : FALSE);
		ResetRootMotionState();
	}

	BOOL32 EAnimInstance::IsRootMotionEnabled() const
	{
		return bRootMotionEnabled;
	}

	void EAnimInstance::SetRootMotionBoneIndex(INT32 InBoneIndex)
	{
		RootMotionBoneIndex = EMath::Max(InBoneIndex, 0);
		ResetRootMotionState();
	}

	INT32 EAnimInstance::GetRootMotionBoneIndex() const
	{
		return RootMotionBoneIndex;
	}

	void EAnimInstance::OnSkeletonChanged()
	{
		EndNotifyStates(ActiveBaseNotifyStates);
		ClearAllMontages();
		ResetRootMotionState();
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
		PE_PROFILE_SCOPE(PROFILER_SCOPE_ANIM);

		FAnimAdvanceResult AdvanceResult;
		if (!Advance(InDeltaSeconds, AdvanceResult))
		{
			return FALSE;
		}

		// The skeleton that owns the bones this instance writes. A valid binding already
		// resolved it; the fallback path reads it from the component's asset.
		const ESkeleton* Skeleton = nullptr;
		if (Binding.IsValid())
		{
			Skeleton = Binding.GetSkeleton();
		}
		else if (Component)
		{
			const ESkeletonAsset* SkeletonAsset = Component->GetSkeletonAsset();
			Skeleton = (SkeletonAsset ? SkeletonAsset->GetStoragedResource() : nullptr);
		}

		if (!EvaluatePose(AdvanceResult, Skeleton, Component->GetBoneMemoryPool()))
		{
			return FALSE;
		}
		CommitSideEffects(AdvanceResult, PendingRootMotion);
		return TRUE;
	}

	BOOL32 EAnimInstance::Advance(FLOAT InDeltaSeconds, FAnimAdvanceResult& OutAdvance)
	{
		PE_PROFILE_SCOPE(PROFILER_SCOPE_ANIM_ADVANCE);

		OutAdvance = FAnimAdvanceResult();
		OutAdvance.DeltaSeconds = InDeltaSeconds;

		if (!Component)
		{
			return FALSE;
		}

		if (!Binding.IsValid())
		{
			// The asset and the skeleton could not be bound. A caller that asked for a pose
			// change (stop, asset swap, skeleton swap) still gets the skeleton's default pose
			// instead of keeping a pose from an animation that is no longer bound.
			OutAdvance.bDefaultPoseOnly = TRUE;
			OutAdvance.bShouldEvaluate = bPoseDirty;
			return (OutAdvance.bShouldEvaluate != FALSE);
		}

		// A state transition may start a new clip, so the machine runs before the clip player
		// is advanced and sampled this frame.
		if (StateMachineInstance)
		{
			StateMachineInstance->Update(*this, InDeltaSeconds);
		}

		const BOOL32 bCrossFading = ((CrossFadeRemaining > 0.0f) && (PreviousClipPlayer.GetClip() != nullptr)) ? TRUE : FALSE;
		const BOOL32 bAdvancing = ((ClipPlayer.IsPlaying()) && (ClipPlayer.GetClip() != nullptr)) ? TRUE : FALSE;
		const BOOL32 bPreviousAdvancing = ((bCrossFading) && (PreviousClipPlayer.IsPlaying())) ? TRUE : FALSE;
		const BOOL32 bMontageActive = ((!MontageInstances.IsEmpty()) ? TRUE : FALSE);
		if ((!bAdvancing) && (!bPreviousAdvancing) && (!bMontageActive) && (!bPoseDirty))
		{
			return FALSE;
		}
		// A zero delta cannot move a playing clip. Skip the resample unless something else
		// already asked for a new pose (the first frame after Play, a seek, an asset swap).
		// A queued montage section transition also has to run even on a zero-delta frame.
		BOOL32 bMontageTransitionPending = FALSE;
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if ((MontageInstances[Index]) && (MontageInstances[Index]->HasPendingSectionTransition()))
			{
				bMontageTransitionPending = TRUE;
				break;
			}
		}
		if ((!bPoseDirty) && (InDeltaSeconds == 0.0f) && (!bMontageTransitionPending))
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

		// Montage time and weight advance here rather than inside the pose build: they are
		// state the pose is a function of, and moving them earlier keeps EvaluatePose free
		// of anything a worker would have to serialize.
		MontageAliveScratch.SetNum(MontageInstances.Num());
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			MontageAliveScratch[Index] = (Montage ? Montage->Update(InDeltaSeconds) : FALSE);
		}

		OutAdvance.bShouldEvaluate = TRUE;
		OutAdvance.bAdvancing = bAdvancing;
		OutAdvance.bPreviousAdvancing = bPreviousAdvancing;
		OutAdvance.bCrossFading = bCrossFading;
		OutAdvance.bMontageActive = bMontageActive;
		OutAdvance.bMontageTransitionPending = bMontageTransitionPending;
		return TRUE;
	}

	BOOL32 EAnimInstance::EvaluatePose(const FAnimAdvanceResult& InAdvance, const ESkeleton* InSkeleton, ESkeletonBoneMemoryPool& InOutBonePool)
	{
		PE_PROFILE_SCOPE(PROFILER_SCOPE_ANIM_POSE);

		if (!InAdvance.bShouldEvaluate)
		{
			return FALSE;
		}

		if (InAdvance.bDefaultPoseOnly)
		{
			if (InOutBonePool.IsValid())
			{
				ApplyDefaultPose(InSkeleton, InOutBonePool.GetBoneRelativeTransforms());
				InOutBonePool.RecursionToRootTransforms();
			}
			return TRUE;
		}

		TArray<EBoneTransform>& CurrentPose = InOutBonePool.GetBoneRelativeTransforms();

		// The clip only drives the bones it has curves for. Starting from the skeleton's
		// default pose is what keeps every other bone at its bind transform instead of
		// keeping whatever the previous clip left there.
		ApplyDefaultPose(InSkeleton, CurrentPose);
		if (ClipPlayer.GetClip())
		{
			ClipPlayer.Evaluate(Binding, CurrentPose);
		}

		if (InAdvance.bCrossFading)
		{
			CrossFadeRemaining = EMath::Max(CrossFadeRemaining - InAdvance.DeltaSeconds, 0.0f);
			const FLOAT Weight = ((CrossFadeDuration > 0.0f)
				? EMath::Clamp(1.0f - (CrossFadeRemaining / CrossFadeDuration), 0.0f, 1.0f)
				: 1.0f);

			// The previous clip is sampled into its own buffer, starting from the default
			// pose, then the two poses are blended per bone. Bones neither clip drives stay
			// at the default pose on both sides, so the blend changes nothing there.
			if (CrossFadePose.Num() != CurrentPose.Num())
			{
				CrossFadePose.SetNum(CurrentPose.Num());
			}
			ApplyDefaultPose(InSkeleton, CrossFadePose);
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

		// Layers are ordered from base to highest; each Evaluate() blends its masked bones
		// onto the pose produced by the previous layer. The alive flags were captured by
		// Advance, so a layer that finished this frame is skipped here.
		const INT32 MontageNum = EMath::Min(MontageInstances.Num(), MontageAliveScratch.Num());
		for (INT32 Index = 0; Index < MontageNum; Index++)
		{
			if (!MontageAliveScratch[Index])
			{
				continue;
			}
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			if (Montage)
			{
				Montage->Evaluate(Binding, CurrentPose);
			}
		}

		// Root motion consumes the root bone delta before the pose-to-root matrices are
		// rebuilt, so the mesh renders at the bind root while the actor carries the motion.
		// The actor itself is moved by CommitSideEffects: the delta is a game-thread side
		// effect, but the bone reset belongs to the pose.
		PendingRootMotion = FAnimRootMotionDelta();
		ComputeRootMotion(CurrentPose, InAdvance.DeltaSeconds, PendingRootMotion);
		InOutBonePool.RecursionToRootTransforms();
		return TRUE;
	}

	void EAnimInstance::CommitSideEffects(const FAnimAdvanceResult& InAdvance, const FAnimRootMotionDelta& InRootMotionDelta)
	{
		PE_PROFILE_SCOPE(PROFILER_SCOPE_ANIM_NOTIFY);

		ApplyRootMotionDelta(InRootMotionDelta);

		if (InAdvance.bDefaultPoseOnly)
		{
			bPoseDirty = FALSE;
			return;
		}

		if (InAdvance.bAdvancing)
		{
			// Notify states and instant events run after the pose and bone-to-root matrices
			// are current, so a notify that reads a socket sees this frame's pose.
			if (const ESkeletonAnimationClip* BaseClip = ClipPlayer.GetClip())
			{
				UpdateNotifyStates(ActiveBaseNotifyStates, BaseClip->Notifies, ClipPlayer, TRUE, InAdvance.DeltaSeconds);
			}
			DispatchNotifies(ClipPlayer);
		}
		BOOL32 bMontageOrderDirty = FALSE;
		const INT32 NotifyPassCount = MontageAliveScratch.Num();
		for (INT32 Index = 0; (Index < NotifyPassCount) && (Index < MontageInstances.Num()); Index++)
		{
			EAnimMontageInstance* Montage = MontageInstances[Index].get();
			if (!Montage)
			{
				continue;
			}
			const EAnimMontageSection* MontageSection = Montage->GetCurrentSection();
			if ((!MontageAliveScratch[Index]) || (Montage->IsStopping()))
			{
				EndNotifyStates(Montage->GetActiveNotifyStates());
			}
			else if ((MontageSection) && (Montage->IsPlaying()))
			{
				UpdateNotifyStates(Montage->GetActiveNotifyStates(), MontageSection->Notifies
					, Montage->GetClipPlayer(), TRUE, InAdvance.DeltaSeconds);
			}
			DispatchNotifies(Montage->GetClipPlayer());
			DispatchMontageSectionNotifies(*Montage);
			// The section's final notify window is consumed above; only now is it safe to
			// reset the clip time for the chained section.
			if (Montage->HasPendingSectionTransition())
			{
				// The chained section has its own timeline; close the old windows first.
				EndNotifyStates(Montage->GetActiveNotifyStates());
				Montage->ApplyPendingSectionTransition();
				bMontageOrderDirty = TRUE;
			}
		}
		// Remove finished layers after every callback has seen this frame. A notify that
		// starts or stops a montage can change the list; bounds checks keep the pass safe,
		// and newly added layers are picked up next frame.
		for (INT32 Index = NotifyPassCount - 1; Index >= 0; Index--)
		{
			if ((Index >= MontageInstances.Num()) || (MontageAliveScratch[Index]))
			{
				continue;
			}
			if (MontageInstances[Index])
			{
				EndNotifyStates(MontageInstances[Index]->GetActiveNotifyStates());
			}
			MontageInstances.RemoveAt(Index);
		}
		if (bMontageOrderDirty)
		{
			SortMontagesByLayerPriority();
		}
		bPoseDirty = FALSE;
	}

	void EAnimInstance::DispatchNotifies(const EAnimationClipPlayer& InPlayer)
	{
		const ESkeletonAnimationClip* Clip = InPlayer.GetClip();
		if (!Clip)
		{
			return;
		}
		DispatchNotifyEventList(*this, Clip->Notifies
			, InPlayer.GetPreviousPositionSeconds(), InPlayer.GetPositionSeconds()
			, InPlayer.GetClipDurationSeconds(), InPlayer.HasLoopedLastAdvance());
	}

	void EAnimInstance::DispatchMontageSectionNotifies(EAnimMontageInstance& InInstance)
	{
		const EAnimMontageSection* Section = InInstance.GetCurrentSection();
		if ((!Section) || (Section->Notifies.Num() <= 0))
		{
			return;
		}
		const EAnimationClipPlayer& Player = InInstance.GetClipPlayer();
		DispatchNotifyEventList(*this, Section->Notifies
			, Player.GetPreviousPositionSeconds(), Player.GetPositionSeconds()
			, Player.GetClipDurationSeconds(), Player.HasLoopedLastAdvance());
	}

	void EAnimInstance::ClearAllMontages()
	{
		for (INT32 Index = 0; Index < MontageInstances.Num(); Index++)
		{
			if (MontageInstances[Index])
			{
				EndNotifyStates(MontageInstances[Index]->GetActiveNotifyStates());
			}
		}
		MontageInstances.Empty();
	}

	void EAnimInstance::SortMontagesByLayerPriority()
	{
		// Stable so two montages in the same slot group keep their insertion order.
		MontageInstances.StableSort([](const TUniquePtr<EAnimMontageInstance>& InA, const TUniquePtr<EAnimMontageInstance>& InB) -> bool
			{
				if (!InA)
				{
					return FALSE;
				}
				if (!InB)
				{
					return TRUE;
				}
				return (InA->GetLayerPriority() < InB->GetLayerPriority());
			});
	}

	void EAnimInstance::UpdateNotifyStates(TArray<FActiveAnimNotifyState>& InOutActive
		, const TArray<FAnimNotifyEvent>& InNotifies, const EAnimationClipPlayer& InPlayer
		, BOOL32 bInAdvanced, FLOAT InDeltaSeconds)
	{
		if (!bInAdvanced)
		{
			return;
		}

		const FLOAT PreviousTime = InPlayer.GetPreviousPositionSeconds();
		const FLOAT CurrentTime = InPlayer.GetPositionSeconds();
		const FLOAT DurationSeconds = InPlayer.GetClipDurationSeconds();
		const BOOL32 bLooped = InPlayer.HasLoopedLastAdvance();

		// Open windows the playhead entered this frame. Checking the swept interval, not
		// only the current position, keeps a short window from disappearing when a long
		// frame (or the first frame after an import stall) jumps over it. A window that was
		// both entered and left in this frame still receives Begin, Tick and End.
		for (INT32 NotifyIndex = 0; NotifyIndex < InNotifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& Event = InNotifies[NotifyIndex];
			if ((!Event.bIsState) || (!Event.CachedNotify))
			{
				continue;
			}
			BOOL32 bAlreadyActive = FALSE;
			for (INT32 ActiveIndex = 0; ActiveIndex < InOutActive.Num(); ActiveIndex++)
			{
				// Identity only: the source pointer may already be dangling after an asset edit.
				if (InOutActive[ActiveIndex].SourceEvent == &Event)
				{
					bAlreadyActive = TRUE;
					break;
				}
			}
			if (bAlreadyActive)
			{
				continue;
			}

			const FLOAT StateEndTime = Event.Time + EMath::Max(Event.StateDuration, 0.001f);
			const BOOL32 bInside = ((CurrentTime >= Event.Time) && (CurrentTime < StateEndTime)) ? TRUE : FALSE;
			BOOL32 bEntered = FALSE;
			if (bLooped)
			{
				// REPEAT crossed the end: the frame covered [PreviousTime, Duration] and
				// [0, CurrentTime]. Check both segments instead of the complement between
				// the two positions.
				const BOOL32 bOverlapsTail = (((StateEndTime > PreviousTime) && (Event.Time <= DurationSeconds)) ? TRUE : FALSE);
				const BOOL32 bOverlapsHead = (((StateEndTime > 0.0f) && (Event.Time <= CurrentTime)) ? TRUE : FALSE);
				bEntered = ((bOverlapsTail || bOverlapsHead) ? TRUE : FALSE);
			}
			else
			{
				const FLOAT SweptMin = EMath::Min(PreviousTime, CurrentTime);
				const FLOAT SweptMax = EMath::Max(PreviousTime, CurrentTime);
				bEntered = (((SweptMax >= Event.Time) && (SweptMin < StateEndTime)) ? TRUE : FALSE);
			}
			if ((!bInside) && (!bEntered))
			{
				continue;
			}

			Event.CachedNotify->NotifyBegin(*this, Event);
			if (bInside)
			{
				// The active loop below performs the first Tick, so Begin and Tick still
				// happen on the same frame without ticking twice.
				// Snapshot the authored event. If the notify array is rebuilt while this state
				// is active, NotifyTick/NotifyEnd still have valid data and a live CachedNotify.
				FActiveAnimNotifyState& ActiveState = InOutActive.AddDefaultGetRef();
				ActiveState.SourceEvent = &Event;
				ActiveState.Event = Event;
			}
			else
			{
				// The playhead crossed the whole window in one frame.
				Event.CachedNotify->NotifyTick(*this, Event, InDeltaSeconds);
				Event.CachedNotify->NotifyEnd(*this, Event);
			}
		}

		// Tick the windows still open, and close the ones the playhead has left. Iterating
		// backwards makes removal safe.
		for (INT32 ActiveIndex = InOutActive.Num() - 1; ActiveIndex >= 0; ActiveIndex--)
		{
			// Use the snapshot rather than the authored array: it may have been rebuilt since
			// NotifyBegin, which used to leave this loop dereferencing freed memory.
			FAnimNotifyEvent& Event = InOutActive[ActiveIndex].Event;
			const FLOAT StateEndTime = Event.Time + EMath::Max(Event.StateDuration, 0.001f);
			const BOOL32 bInside = ((CurrentTime >= Event.Time) && (CurrentTime < StateEndTime)) ? TRUE : FALSE;
			if (!bInside)
			{
				if (Event.CachedNotify)
				{
					Event.CachedNotify->NotifyEnd(*this, Event);
				}
				InOutActive.RemoveAt(ActiveIndex);
			}
			else if (Event.CachedNotify)
			{
				Event.CachedNotify->NotifyTick(*this, Event, InDeltaSeconds);
			}
		}

		// A one-shot clip that reached its end must not leave a hit window open forever.
		if (InPlayer.HasFinished())
		{
			EndNotifyStates(InOutActive);
		}
	}

	void EAnimInstance::EndNotifyStates(TArray<FActiveAnimNotifyState>& InOutActive)
	{
		for (INT32 ActiveIndex = 0; ActiveIndex < InOutActive.Num(); ActiveIndex++)
		{
			FAnimNotifyEvent& Event = InOutActive[ActiveIndex].Event;
			if (Event.CachedNotify)
			{
				Event.CachedNotify->NotifyEnd(*this, Event);
			}
		}
		InOutActive.Empty();
	}

	void EAnimInstance::ResetRootMotionState()
	{
		bHasPreviousRootTransform = FALSE;
		PreviousRootTransform = EBoneTransform::Identity();
	}

	void EAnimInstance::ComputeRootMotion(TArray<EBoneTransform>& InOutPose, FLOAT InDeltaSeconds, FAnimRootMotionDelta& OutDelta)
	{
		OutDelta = FAnimRootMotionDelta();
		if ((!bRootMotionEnabled) || (InDeltaSeconds <= 0.0f) || (!Binding.IsValid()))
		{
			return;
		}
		if ((RootMotionBoneIndex < 0) || (RootMotionBoneIndex >= InOutPose.Num()))
		{
			return;
		}

		const EBoneTransform CurrentRoot = InOutPose[RootMotionBoneIndex];
		if ((bHasPreviousRootTransform) && (!ClipPlayer.HasLoopedLastAdvance()))
		{
			OutDelta.DeltaPosition = Vector3(CurrentRoot.Position.x - PreviousRootTransform.Position.x
				, CurrentRoot.Position.y - PreviousRootTransform.Position.y
				, CurrentRoot.Position.z - PreviousRootTransform.Position.z);
			OutDelta.DeltaRotation = Quaternion::MultiplyQuaternion(CurrentRoot.Rotation
				, Quaternion::Inverse(PreviousRootTransform.Rotation));
			OutDelta.bHasDelta = TRUE;
		}

		PreviousRootTransform = CurrentRoot;
		bHasPreviousRootTransform = TRUE;

		// The actor/component consumes the motion in CommitSideEffects; keep the mesh at its
		// bind root so it is not displaced a second time by the sampled clip. The recursion
		// below uses the pool's own SkeletonRootTransform, which is component-local and not
		// the actor transform this delta will change, so applying the delta later cannot
		// feed back into the matrices built here.
		const ESkeleton* Skeleton = Binding.GetSkeleton();
		if ((Skeleton) && (RootMotionBoneIndex < Skeleton->GetBoneCount()))
		{
			const EBoneData& Bone = Skeleton->GetBonePart()[RootMotionBoneIndex];
			InOutPose[RootMotionBoneIndex] = EBoneTransform(Bone.DefaultPosition, Bone.DefaultRotation, Bone.DefaultScaling);
		}
	}

	void EAnimInstance::ApplyRootMotionDelta(const FAnimRootMotionDelta& InDelta)
	{
		if ((!InDelta.bHasDelta) || (!Component))
		{
			return;
		}

		if (PActor* OwnerActor = Component->GetOwnerActor())
		{
			if (InDelta.DeltaPosition.Length() > 1e-6f)
			{
				OwnerActor->SetActorLocation(OwnerActor->GetActorLocation() + InDelta.DeltaPosition);
			}
			OwnerActor->SetActorRotation(Quaternion::MultiplyQuaternion(InDelta.DeltaRotation, OwnerActor->GetActorRotation()));
		}
		else
		{
			const ETransform& CurrentTransform = Component->GetTransform();
			ETransform NewTransform(CurrentTransform.GetLocation_Local() + InDelta.DeltaPosition
				, Quaternion::MultiplyQuaternion(InDelta.DeltaRotation, CurrentTransform.GetRotation_Local())
				, CurrentTransform.GetScaling_Local());
			Component->SetComponentTransform(NewTransform);
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

	INT32 EAnimInstance::GetAnimationClipNum() const
	{
		const ESkeletonAnimation* Animation = (AnimationAsset ? AnimationAsset->GetStoragedResource() : nullptr);
		return (Animation ? Animation->GetAnimationClipNum() : 0);
	}

}
