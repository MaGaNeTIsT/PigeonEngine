#pragma once

#include <CoreMinimal.h>

#include "AnimationClipPlayer.h"
#include "AnimationNotify.h"
#include "SkeletonBinding.h"

namespace PigeonEngine
{

	class PSkeletalMeshComponent;
	class ESkeleton;
	class ESkeletonAnimationAsset;
	class EAnimStateMachine;
	class EAnimStateMachineInstance;
	class EAnimMontage;
	class EAnimMontageInstance;
	struct ESkeletonAnimationClip;

	/*
	* The runtime animation evaluation core.
	*
	* It owns the binding between the component's skeleton and one animation asset, plus the
	* clip player that writes into the component's bone pool. Game code drives it through
	* the owning PSkeletalMeshComponent; a game-specific subclass can later add state
	* machine or montage logic without the component having to know about it.
	*
	* Evaluation happens on the game thread from PSkeletalMeshComponent::TickRenderImpl,
	* before the component sends its render state, so the pose the render thread receives is
	* the pose produced this frame rather than the previous one.
	*/
	class EAnimInstance
	{
	public:
		EAnimInstance();
		virtual ~EAnimInstance();
		EAnimInstance(const EAnimInstance&) = delete;
		EAnimInstance(EAnimInstance&&) = delete;
		EAnimInstance& operator=(const EAnimInstance&) = delete;
		EAnimInstance& operator=(EAnimInstance&&) = delete;

		void SetSkeletalMeshComponent(PSkeletalMeshComponent* InComponent);
		PE_NODISCARD PSkeletalMeshComponent* GetSkeletalMeshComponent()const;

		/*
		* Stores the asset and binds it against the component's current skeleton. Returns
		* FALSE when either side is not loaded yet or when no curve could be bound; the
		* asset is still remembered so a later skeleton change can retry the bind.
		*/
		BOOL32 SetAnimationAsset(const ESkeletonAnimationAsset* InAnimationAsset);
		PE_NODISCARD const ESkeletonAnimationAsset* GetAnimationAsset()const;

		/*
		* InFadeSeconds blends from the clip that is currently playing to the new one. Zero
		* switches immediately; a positive value keeps the previous player alive and blends
		* its pose out over that time.
		*/
		BOOL32 Play(const EString& InClipName, BOOL32 bInLoop = TRUE, FLOAT InPlayRate = 1.0f, FLOAT InFadeSeconds = 0.0f);
		BOOL32 Play(INT32 InClipIndex, BOOL32 bInLoop = TRUE, FLOAT InPlayRate = 1.0f, FLOAT InFadeSeconds = 0.0f);
		// Exact loop mode; used by the state machine so PINGPONG/CLAMP survive a transition.
		BOOL32 Play(const EString& InClipName, EAnimationLoopMode InLoopMode, FLOAT InPlayRate = 1.0f, FLOAT InFadeSeconds = 0.0f);
		void Pause();
		void Resume();
		void Stop();

		void SetPlayRate(FLOAT InPlayRate);
		PE_NODISCARD FLOAT GetPlayRate()const;
		// Convenience form: TRUE maps to REPEAT, FALSE maps to ONCE.
		void SetLoop(BOOL32 bInLoop);
		void SetLoopMode(EAnimationLoopMode InLoopMode);
		PE_NODISCARD EAnimationLoopMode GetLoopMode()const;
		PE_NODISCARD BOOL32 IsLooping()const;
		void SetPositionSeconds(FLOAT InPositionSeconds);
		PE_NODISCARD FLOAT GetPositionSeconds()const;

		PE_NODISCARD BOOL32 IsPlaying()const;
		PE_NODISCARD const ESkeletonAnimationClip* GetCurrentClip()const;
		PE_NODISCARD INT32 GetCurrentClipIndex()const;

		// State machine START
	public:
		/*
		* Binds a parsed state machine. The machine controls which clip is played; its clip
		* names are resolved against the animation asset already bound to this instance.
		* Returns FALSE when the machine is invalid or the current animation asset does not
		* contain its entry clip yet.
		*/
		BOOL32 SetStateMachine(const EAnimStateMachine* InMachine);
		void ClearStateMachine();
		PE_NODISCARD EAnimStateMachineInstance* GetStateMachineInstance()const;
		// State machine END

		// Montage START
	public:
		/*
		* Plays one montage section over the current base pose. The section's clip is resolved
		* against the bound animation asset; its bone mask decides which bones the layer
		* writes. A previously active montage is replaced.
		*/
		BOOL32 PlayMontage(const EAnimMontage* InMontage, const EString& InSectionName = EString(), FLOAT InPlayRate = 1.0f);
		// Stops every active layer. Layers blend out over their section's configured time
		// unless InBlendOutSeconds is supplied.
		void StopMontage(FLOAT InBlendOutSeconds = -1.0f);
		// Stops only the layer currently playing InSlotName.
		void StopMontage(const EString& InSlotName, FLOAT InBlendOutSeconds = -1.0f);
		void PauseMontage();
		void PauseMontage(const EString& InSlotName);
		void ResumeMontage();
		void ResumeMontage(const EString& InSlotName);
		PE_NODISCARD BOOL32 IsMontagePlaying()const;
		PE_NODISCARD BOOL32 IsMontagePlaying(const EString& InSlotName)const;
		// Returns the lowest-priority active layer; useful for callers that only expect one.
		PE_NODISCARD EAnimMontageInstance* GetMontageInstance()const;
		PE_NODISCARD EAnimMontageInstance* GetMontageInstanceBySlot(const EString& InSlotName)const;
		PE_NODISCARD INT32 GetMontageInstanceCount()const;
		PE_NODISCARD EAnimMontageInstance* GetMontageInstanceAt(INT32 InIndex)const;
		// Montage END

		/*
		* Rebuilds the binding after the component's skeleton changed. When the previously
		* playing clip still exists in the new binding, its position, rate, loop mode and
		* pause state are preserved; otherwise playback stops and the pose returns to the
		* skeleton's default.
		*/
		void OnSkeletonChanged();

		/*
		* Game thread, once per frame, before the component sends its render state.
		* Returns TRUE when the pose was written, which is the caller's signal to mark the
		* bone pool dirty. A paused player returns FALSE after its last pose was written,
		* so a stationary character does not resample its clip every frame.
		*
		* Virtual so a game-specific instance can drive a state machine before calling the
		* base implementation to advance and sample the clip.
		*/
		virtual BOOL32 Update(FLOAT InDeltaSeconds);

	private:
		BOOL32 BindAnimation();
		BOOL32 PlayInternal(INT32 InClipIndex, EAnimationLoopMode InLoopMode, FLOAT InPlayRate, FLOAT InFadeSeconds);
		void CancelCrossFade();
		// Fires the notify events crossed by InPlayer's last Advance.
		void DispatchNotifies(const EAnimationClipPlayer& InPlayer);
		// Fires a montage layer's authored section events over the same time window.
		void DispatchMontageSectionNotifies(EAnimMontageInstance& InInstance);
		/*
		* Updates active state notifies by sampling the current playhead against each
		* event's [Time, Time + StateDuration) interval. Begin fires on entry, Tick on each
		* following advancing frame, End on exit or when the clip finishes. This works for
		* forward, reverse and looping playback and keeps the first version free of a
		* separate interval-crossing solver.
		*/
		void UpdateNotifyStates(TArray<FActiveAnimNotifyState>& InOutActive
			, const TArray<FAnimNotifyEvent>& InNotifies, const EAnimationClipPlayer& InPlayer
			, BOOL32 bInAdvanced, FLOAT InDeltaSeconds);
		void EndNotifyStates(TArray<FActiveAnimNotifyState>& InOutActive);
		// Ends active windows and drops every montage layer. Used by stop/asset/skeleton
		// transitions, not by a normal blend-out.
		void ClearAllMontages();
		void SortMontagesByLayerPriority();
		void ApplyDefaultPose(const ESkeleton* InSkeleton);
		void ApplyDefaultPose(const ESkeleton* InSkeleton, TArray<EBoneTransform>& InOutPose);
		PE_NODISCARD INT32 GetAnimationClipNum()const;

	private:
		PSkeletalMeshComponent*			Component = nullptr;
		const ESkeletonAnimationAsset*	AnimationAsset = nullptr;
		ESkeletonBinding				Binding;
		EAnimationClipPlayer			ClipPlayer;
		// The runtime state for the optional asset-driven state machine. The machine data
		// itself is owned by the asset manager; this object only holds parameters and the
		// current state.
		TUniquePtr<EAnimStateMachineInstance>	StateMachineInstance;
		// Active montage layers, sorted by skeleton slot group priority. Each layer owns
		// its clip player, compiled bone mask and state-window list.
		TArray<TUniquePtr<EAnimMontageInstance>>	MontageInstances;
		// The clip being blended out, and the pose it produced this frame. The pose array is
		// only allocated while a crossfade is in progress.
		EAnimationClipPlayer			PreviousClipPlayer;
		TArray<EBoneTransform>			CrossFadePose;
		// Base-clip state windows. Each montage layer owns its own list on its instance.
		TArray<FActiveAnimNotifyState>	ActiveBaseNotifyStates;
		FLOAT							CrossFadeDuration = 0.0f;
		FLOAT							CrossFadeRemaining = 0.0f;
		INT32							CurrentClipIndex = -1;
		// Set by every call that changes what the pose should be: play, stop, seek, asset
		// change, skeleton change. Cleared once Update has written the pose.
		BOOL32							bPoseDirty = FALSE;
	};

}
