#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

#include "AnimationClipPlayer.h"
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
	* what decides which bones the layer actually writes. A section is a single clip for now;
	* section chaining can be added without changing the runtime instance.
	*/
	struct EAnimMontageSection
	{
		EString					Name;
		EString					ClipName;
		EString					SlotName;
		EAnimationLoopMode		LoopMode = ANIMATION_LOOP_ONCE;
		FLOAT					PlayRate = 1.0f;
		FLOAT					BlendInSeconds = 0.1f;
		FLOAT					BlendOutSeconds = 0.2f;
		ESkeletonBoneMask		BoneMask;
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

	private:
		BOOL32 CompileBoneMask(const ESkeleton* InSkeleton);

	private:
		const EAnimMontage*		Montage = nullptr;
		INT32					SectionIndex = -1;
		EAnimationClipPlayer	ClipPlayer;
		TArray<FLOAT>			BoneMaskWeights;
		TArray<EBoneTransform>	LayerPose;
		FLOAT					LayerWeight = 0.0f;
		FLOAT					BlendInSeconds = 0.1f;
		FLOAT					BlendOutSeconds = 0.2f;
		BOOL32					bStopping = FALSE;
		BOOL32					bPaused = FALSE;
		BOOL32					bFinished = FALSE;
	};

}
