#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

#include "AnimationClipPlayer.h"
#include "AnimationNotify.h"
#include "AnimationTypes.h"

namespace PigeonEngine
{

	class EAnimInstance;
	class ESkeleton;
	class ESkeletonAnimation;
	class ESkeletonBinding;

	/*
	* A bone mask is authored by bone names and compiled to a per-bone weight array when a
	* montage is bound. An empty list means "all bones", which is the full-body default.
	*/
	struct ESkeletonBoneMask
	{
		EString			Name;
		TArray<EString>	BoneNames;
		FLOAT			Weight = 1.0f;
	};

	struct EAnimMontageSlotGroup
	{
		EString			Name;
		TArray<EString>	SlotNames;
	};

	/*
	* One playable montage section. The slot names the layer's identity; the bone mask is
	* what decides which bones the layer actually writes. NextSectionName chains a one-shot
	* section into another section when it reaches its end; an empty name ends the montage.
	* Notifies are authored on the section timeline and use the same event data as clip
	* notifies, so gameplay code does not care which asset produced the event.
	*/
	struct EAnimMontageSection
	{
		EString					Name;
		EString					ClipName;
		EString					SlotName;
		EString					NextSectionName;
		EAnimationLoopMode		LoopMode = ANIMATION_LOOP_ONCE;
		FLOAT					PlayRate = 1.0f;
		FLOAT					BlendInSeconds = 0.1f;
		FLOAT					BlendOutSeconds = 0.2f;
		ESkeletonBoneMask		BoneMask;
		TArray<FAnimNotifyEvent>	Notifies;
	};

	class EAnimMontage : public EObjectBase, public EResourceInterface
	{
		CLASS_VIRTUAL_NOCOPY_BODY(EAnimMontage)

	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;

	public:
		EString								Name;
		EString								SkeletonName;
		EString								AnimationAssetName;
		EString								SourceJson;
		TArray<EAnimMontageSlotGroup>		SlotGroups;
		TArray<EAnimMontageSection>			Sections;
		TMap<EString, INT32>				SectionMapping;
		TMap<EString, INT32>				SlotGroupMapping;

	public:
		void	Reset();
		BOOL32	RebuildMappings();
		BOOL32	IsValid()const;
		PE_NODISCARD INT32 FindSection(const EString& InName)const;
		PE_NODISCARD INT32 FindSlotGroup(const EString& InName)const;
		PE_NODISCARD BOOL32 HasSlot(const EString& InSlotName)const;
	};

	/*
	* Per-character playback state for one montage section. Update() advances time and the
	* fade weight; Evaluate() samples the layer and blends it onto the pose the base clip has
	* already produced, using the section's bone mask.
	*
	* When a one-shot section ends and names a next section, the transition is deferred until
	* after the caller dispatches that section's final notifies. This keeps an event authored
	* at the section end from being lost to the next section's time reset.
	*/
	class EAnimMontageInstance
	{
	public:
		BOOL32 SetMontage(const EAnimMontage* InMontage, const EString& InSectionName
			, const ESkeleton* InSkeleton, const ESkeletonAnimation* InAnimation, FLOAT InPlayRate = 1.0f);
		void Clear();

		// Returns FALSE when the montage has finished blending out and should be removed.
		BOOL32 Update(EAnimInstance& InAnimInstance, FLOAT InDeltaSeconds);
		// Blends this layer's sampled pose onto InOutPose. Returns FALSE when the layer has
		// no weight or no valid clip.
		BOOL32 Evaluate(EAnimInstance& InAnimInstance, const ESkeletonBinding& InBinding, TArray<EBoneTransform>& InOutPose);

		void Stop(FLOAT InBlendOutSeconds = -1.0f);
		void Pause();
		void Resume();
		PE_NODISCARD BOOL32 IsPlaying()const;
		PE_NODISCARD BOOL32 IsStopping()const;
		PE_NODISCARD INT32 GetSectionIndex()const;
		PE_NODISCARD FLOAT GetLayerWeight()const;
		PE_NODISCARD const EAnimationClipPlayer& GetClipPlayer()const;
		PE_NODISCARD const EAnimMontageSection* GetCurrentSection()const;
		// Index of the current section's slot group in the skeleton. Higher values blend
		// later and therefore override lower layers.
		PE_NODISCARD INT32 GetLayerPriority()const;
		// Per-layer state-window list. EAnimInstance owns dispatch; the instance only
		// stores which events are open for its current timeline.
		PE_NODISCARD TArray<FActiveAnimNotifyState>& GetActiveNotifyStates();
		PE_NODISCARD const TArray<FActiveAnimNotifyState>& GetActiveNotifyStates()const;

		// Section chaining: a finished one-shot section leaves a pending target. The caller
		// dispatches notifies first, then applies the transition for the next advance.
		PE_NODISCARD BOOL32 HasPendingSectionTransition()const;
		BOOL32 ApplyPendingSectionTransition();

	private:
		BOOL32 StartSection(INT32 InSectionIndex, BOOL32 bInKeepLayerWeight);
		BOOL32 CompileBoneMask(const ESkeleton* InSkeleton);

	private:
		const EAnimMontage*			Montage = nullptr;
		const ESkeleton*			BoundSkeleton = nullptr;
		const ESkeletonAnimation*	BoundAnimation = nullptr;
		INT32						SectionIndex = -1;
		INT32					PendingSectionIndex = -1;
		EAnimationClipPlayer	ClipPlayer;
		TArray<FLOAT>			BoneMaskWeights;
		TArray<EBoneTransform>	LayerPose;
		FLOAT					LayerWeight = 0.0f;
		INT32					LayerPriority = 0;
		TArray<FActiveAnimNotifyState> ActiveNotifyStates;
		FLOAT					InstancePlayRate = 1.0f;
		FLOAT					BlendInSeconds = 0.1f;
		FLOAT					BlendOutSeconds = 0.2f;
		BOOL32					bStopping = FALSE;
		BOOL32					bPaused = FALSE;
		BOOL32					bFinished = FALSE;
	};

}
