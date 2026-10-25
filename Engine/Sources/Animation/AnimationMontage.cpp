#include "AnimationMontage.h"

#include "AnimInstance.h"
#include "SkeletonBinding.h"

#include <AnimationAsset/AnimationAsset.h>
#include <Editor/EditorLogManager.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	EAnimMontage::EAnimMontage()
	{
	}

	EAnimMontage::~EAnimMontage()
	{
	}

	BOOL32 EAnimMontage::IsResourceValid()const
	{
		return IsValid();
	}

	BOOL32 EAnimMontage::InitResource()
	{
		return TRUE;
	}

	void EAnimMontage::ReleaseResource()
	{
		Reset();
	}

	void EAnimMontage::Reset()
	{
		Name = EString();
		SkeletonName = EString();
		AnimationAssetName = EString();
		SourceJson = EString();
		SlotGroups.Empty();
		Sections.Empty();
		SectionMapping.Empty();
		SlotGroupMapping.Empty();
	}

	BOOL32 EAnimMontage::RebuildMappings()
	{
		SectionMapping.Empty();
		SlotGroupMapping.Empty();

		for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
		{
			const EString& SectionName = Sections[SectionIndex].Name;
			if ((SectionName.Length() <= 0u) || (SectionMapping.ContainsKey(SectionName)))
			{
				return FALSE;
			}
			SectionMapping.Add(SectionName, SectionIndex);
		}
		for (INT32 GroupIndex = 0; GroupIndex < SlotGroups.Num(); GroupIndex++)
		{
			const EString& GroupName = SlotGroups[GroupIndex].Name;
			if ((GroupName.Length() <= 0u) || (SlotGroupMapping.ContainsKey(GroupName)))
			{
				return FALSE;
			}
			SlotGroupMapping.Add(GroupName, GroupIndex);
		}
		return TRUE;
	}

	BOOL32 EAnimMontage::IsValid()const
	{
		if ((Sections.Num() <= 0) || (SectionMapping.Num() != Sections.Num()) || (SlotGroupMapping.Num() != SlotGroups.Num()))
		{
			return FALSE;
		}
		for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
		{
			const EAnimMontageSection& Section = Sections[SectionIndex];
			if ((Section.ClipName.Length() <= 0u) || (Section.SlotName.Length() <= 0u))
			{
				return FALSE;
			}
			// Slot groups may live on the montage (legacy) or on the skeleton. When the
			// montage carries none, the skeleton validates the slot at bind time.
			if ((SlotGroups.Num() > 0) && (!HasSlot(Section.SlotName)))
			{
				return FALSE;
			}
			if (Section.NextSectionName.Length() > 0u)
			{
				// A self-reference would restart a one-shot section forever, and a missing
				// target would silently end the montage instead of playing the authored chain.
				if ((Section.NextSectionName == Section.Name) || (FindSection(Section.NextSectionName) < 0))
				{
					return FALSE;
				}
			}
		}
		for (INT32 GroupIndex = 0; GroupIndex < SlotGroups.Num(); GroupIndex++)
		{
			if (SlotGroups[GroupIndex].SlotNames.Num() <= 0)
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	INT32 EAnimMontage::FindSection(const EString& InName)const
	{
		INT32 Result = -1;
		SectionMapping.FindValue(InName, Result);
		return Result;
	}

	INT32 EAnimMontage::FindSlotGroup(const EString& InName)const
	{
		INT32 Result = -1;
		SlotGroupMapping.FindValue(InName, Result);
		return Result;
	}

	BOOL32 EAnimMontage::HasSlot(const EString& InSlotName)const
	{
		for (INT32 GroupIndex = 0; GroupIndex < SlotGroups.Num(); GroupIndex++)
		{
			const TArray<EString>& SlotNames = SlotGroups[GroupIndex].SlotNames;
			for (INT32 SlotIndex = 0; SlotIndex < SlotNames.Num(); SlotIndex++)
			{
				if (SlotNames[SlotIndex] == InSlotName)
				{
					return TRUE;
				}
			}
		}
		return FALSE;
	}

	BOOL32 EAnimMontageInstance::SetMontage(const EAnimMontage* InMontage, const EString& InSectionName
		, const ESkeleton* InSkeleton, const ESkeletonAnimation* InAnimation, FLOAT InPlayRate)
	{
		Clear();
		if ((!InMontage) || (!InMontage->IsValid()) || (!InSkeleton) || (!InAnimation))
		{
			return FALSE;
		}

		const INT32 FoundSectionIndex = ((InSectionName.Length() > 0u) ? InMontage->FindSection(InSectionName) : 0);
		if ((FoundSectionIndex < 0) || (FoundSectionIndex >= InMontage->Sections.Num()))
		{
			return FALSE;
		}

		Montage = InMontage;
		BoundSkeleton = InSkeleton;
		BoundAnimation = InAnimation;
		InstancePlayRate = InPlayRate;
		if (!StartSection(FoundSectionIndex, FALSE))
		{
			Clear();
			return FALSE;
		}

		PE_LOG_LOG(EString("Montage '") + InMontage->Name + EString("' started section '")
			+ Montage->Sections[FoundSectionIndex].Name + EString("'."));
		return TRUE;
	}

	void EAnimMontageInstance::Clear()
	{
		Montage = nullptr;
		BoundSkeleton = nullptr;
		BoundAnimation = nullptr;
		SectionIndex = -1;
		PendingSectionIndex = -1;
		ClipPlayer.Reset();
		BoneMaskWeights.Empty();
		LayerPose.Empty();
		LayerWeight = 0.0f;
		LayerPriority = 0;
		bAdditive = FALSE;
		ActiveNotifyStates.Empty();
		InstancePlayRate = 1.0f;
		BlendInSeconds = 0.1f;
		BlendOutSeconds = 0.2f;
		bStopping = FALSE;
		bPaused = FALSE;
		bFinished = FALSE;
	}

	BOOL32 EAnimMontageInstance::StartSection(INT32 InSectionIndex, BOOL32 bInKeepLayerWeight)
	{
		if ((!Montage) || (!BoundAnimation) || (!BoundSkeleton)
			|| (InSectionIndex < 0) || (InSectionIndex >= Montage->Sections.Num()))
		{
			return FALSE;
		}

		const EAnimMontageSection& Section = Montage->Sections[InSectionIndex];
		const ESkeletonAnimationClip* Clip = BoundAnimation->GetAnimationClip(Section.ClipName);
		if (!Clip)
		{
			PE_LOG_WARN(EString("Montage '") + Montage->Name + EString("' could not find clip '")
				+ Section.ClipName + EString("'."));
			return FALSE;
		}
		INT32 ClipIndex = -1;
		BoundAnimation->GetAnimationClipMapping().FindValue(Section.ClipName, ClipIndex);

		SectionIndex = InSectionIndex;
		PendingSectionIndex = -1;
		bAdditive = Section.bAdditive;
		// The skeleton owns the slot order so every montage agrees which layer is on top.
		const INT32 SlotGroupIndex = BoundSkeleton->FindSlotGroup(Section.SlotName);
		LayerPriority = ((SlotGroupIndex >= 0) ? SlotGroupIndex : 0);
		if (SlotGroupIndex < 0)
		{
			PE_LOG_WARN(EString("Montage '") + Montage->Name + EString("' section '") + Section.Name
				+ EString("' uses unknown slot '") + Section.SlotName + EString("'; treating it as the base layer."));
		}
		BlendInSeconds = EMath::Max(Section.BlendInSeconds, 0.001f);
		BlendOutSeconds = EMath::Max(Section.BlendOutSeconds, 0.001f);
		if (!bInKeepLayerWeight)
		{
			LayerWeight = (Section.BlendInSeconds <= 0.001f) ? 1.0f : 0.0f;
		}
		bStopping = FALSE;
		bPaused = FALSE;
		bFinished = FALSE;

		ClipPlayer.SetClip(Clip, ClipIndex);
		ClipPlayer.SetLoopMode(Section.LoopMode);
		ClipPlayer.SetPlayRate(Section.PlayRate * InstancePlayRate);
		ClipPlayer.Play();
		return CompileBoneMask(BoundSkeleton);
	}

	BOOL32 EAnimMontageInstance::Update(EAnimInstance& InAnimInstance, FLOAT InDeltaSeconds)
	{
		(void)InAnimInstance;
		if ((!Montage) || (SectionIndex < 0) || (SectionIndex >= Montage->Sections.Num()) || (bFinished))
		{
			return FALSE;
		}
		// A paused montage keeps its current pose and pending transition; both continue on
		// Resume().
		if (bPaused)
		{
			return TRUE;
		}
		// The previous section ended last frame. Wait for the caller to dispatch its final
		// notifies before resetting the clip time (see ApplyPendingSectionTransition).
		if (PendingSectionIndex >= 0)
		{
			return TRUE;
		}

		if (!bStopping)
		{
			ClipPlayer.Advance(InDeltaSeconds);
			LayerWeight = EMath::Min(1.0f, LayerWeight + (InDeltaSeconds / BlendInSeconds));
		}
		else
		{
			LayerWeight = EMath::Max(0.0f, LayerWeight - (InDeltaSeconds / BlendOutSeconds));
			if (LayerWeight <= 0.0f)
			{
				bFinished = TRUE;
				return FALSE;
			}
		}

		const EAnimMontageSection& Section = Montage->Sections[SectionIndex];
		if ((!bStopping) && (Section.LoopMode == ANIMATION_LOOP_ONCE) && (!ClipPlayer.IsPlaying()))
		{
			if (Section.NextSectionName.Length() > 0u)
			{
				const INT32 NextSectionIndex = Montage->FindSection(Section.NextSectionName);
				if ((NextSectionIndex >= 0) && (NextSectionIndex < Montage->Sections.Num())
					&& (NextSectionIndex != SectionIndex))
				{
					PendingSectionIndex = NextSectionIndex;
					// Hold the last pose (and its notify window) until the caller has
					// dispatched this section's final events.
					ClipPlayer.Pause();
				}
				else
				{
					Stop(Section.BlendOutSeconds);
				}
			}
			else
			{
				Stop(Section.BlendOutSeconds);
			}
		}
		return TRUE;
	}

	BOOL32 EAnimMontageInstance::Evaluate(EAnimInstance& InAnimInstance, const ESkeletonBinding& InBinding, TArray<EBoneTransform>& InOutPose)
	{
		(void)InAnimInstance;
		if ((!Montage) || (LayerWeight <= 0.0f) || (!InBinding.IsValid()) || (InOutPose.Num() <= 0))
		{
			return FALSE;
		}

		const ESkeleton::EBonePart* BindBones = ((bAdditive && BoundSkeleton) ? &BoundSkeleton->GetBonePart() : nullptr);
		if (LayerPose.Num() != InOutPose.Num())
		{
			LayerPose.SetNum(InOutPose.Num());
		}
		if (BindBones)
		{
			// The bind pose is the additive reference for this first version. Sampling
			// starts from it so bones without curves contribute a zero delta.
			for (INT32 BoneIndex = 0; BoneIndex < LayerPose.Num(); BoneIndex++)
			{
				if (BoneIndex < BindBones->Num())
				{
					const EBoneData& BindData = (*BindBones)[BoneIndex];
					LayerPose[BoneIndex] = EBoneTransform(BindData.DefaultPosition, BindData.DefaultRotation, BindData.DefaultScaling);
				}
				else
				{
					LayerPose[BoneIndex] = EBoneTransform::Identity();
				}
			}
		}
		else
		{
			LayerPose = InOutPose;
		}
		if (!ClipPlayer.Evaluate(InBinding, LayerPose))
		{
			return FALSE;
		}

		const INT32 BoneNum = EMath::Min(InOutPose.Num(), EMath::Min(LayerPose.Num(), BoneMaskWeights.Num()));
		for (INT32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
		{
			const FLOAT Alpha = EMath::Clamp(BoneMaskWeights[BoneIndex] * LayerWeight, 0.0f, 1.0f);
			if (Alpha <= 0.0f)
			{
				continue;
			}
			EBoneTransform& CurrentTransform = InOutPose[BoneIndex];
			const EBoneTransform& LayerTransform = LayerPose[BoneIndex];
			if ((BindBones) && (BoneIndex < BindBones->Num()))
			{
				// Additive: layer - bind for translation/scale, and a local-space
				// rotation delta. The reference is the bind pose, not a reference clip.
				const EBoneData& BindData = (*BindBones)[BoneIndex];
				CurrentTransform.Position.x += (LayerTransform.Position.x - BindData.DefaultPosition.x) * Alpha;
				CurrentTransform.Position.y += (LayerTransform.Position.y - BindData.DefaultPosition.y) * Alpha;
				CurrentTransform.Position.z += (LayerTransform.Position.z - BindData.DefaultPosition.z) * Alpha;
				CurrentTransform.Scaling.x += (LayerTransform.Scaling.x - BindData.DefaultScaling.x) * Alpha;
				CurrentTransform.Scaling.y += (LayerTransform.Scaling.y - BindData.DefaultScaling.y) * Alpha;
				CurrentTransform.Scaling.z += (LayerTransform.Scaling.z - BindData.DefaultScaling.z) * Alpha;
				const Quaternion DeltaRotation = Quaternion::MultiplyQuaternion(LayerTransform.Rotation
					, Quaternion::Inverse(BindData.DefaultRotation));
				CurrentTransform.Rotation = Quaternion::MultiplyQuaternion(CurrentTransform.Rotation, DeltaRotation);
			}
			else
			{
				CurrentTransform.Position = Vector3::Lerp(CurrentTransform.Position, LayerTransform.Position, Alpha);
				CurrentTransform.Rotation = Quaternion::SLerp(CurrentTransform.Rotation, LayerTransform.Rotation, Alpha);
				CurrentTransform.Scaling = Vector3::Lerp(CurrentTransform.Scaling, LayerTransform.Scaling, Alpha);
			}
		}
		return TRUE;
	}

	void EAnimMontageInstance::Stop(FLOAT InBlendOutSeconds)
	{
		if (bFinished)
		{
			return;
		}
		// A stop request overrides a section transition queued for the next frame.
		PendingSectionIndex = -1;
		bPaused = FALSE;
		if (InBlendOutSeconds >= 0.0f)
		{
			BlendOutSeconds = EMath::Max(InBlendOutSeconds, 0.001f);
		}
		if ((!bStopping) && (LayerWeight > 0.0f))
		{
			bStopping = TRUE;
			ClipPlayer.Pause();
		}
		else if (LayerWeight <= 0.0f)
		{
			bFinished = TRUE;
		}
	}

	void EAnimMontageInstance::Pause()
	{
		bPaused = TRUE;
		ClipPlayer.Pause();
	}

	void EAnimMontageInstance::Resume()
	{
		if (bFinished)
		{
			return;
		}
		bPaused = FALSE;
		// A blend-out keeps the layer sampling its held pose while the weight falls; only a
		// normal playback resumes the clip clock.
		if (!bStopping)
		{
			ClipPlayer.Play();
		}
	}

	BOOL32 EAnimMontageInstance::IsPlaying()const
	{
		return (((!bFinished) && (!bStopping) && (!bPaused)) ? TRUE : FALSE);
	}

	BOOL32 EAnimMontageInstance::IsStopping()const
	{
		return bStopping;
	}

	INT32 EAnimMontageInstance::GetSectionIndex()const
	{
		return SectionIndex;
	}

	FLOAT EAnimMontageInstance::GetLayerWeight()const
	{
		return LayerWeight;
	}

	const EAnimationClipPlayer& EAnimMontageInstance::GetClipPlayer()const
	{
		return ClipPlayer;
	}

	const EAnimMontageSection* EAnimMontageInstance::GetCurrentSection()const
	{
		if ((!Montage) || (SectionIndex < 0) || (SectionIndex >= Montage->Sections.Num()))
		{
			return nullptr;
		}
		return &Montage->Sections[SectionIndex];
	}

	INT32 EAnimMontageInstance::GetLayerPriority()const
	{
		return LayerPriority;
	}

	TArray<FActiveAnimNotifyState>& EAnimMontageInstance::GetActiveNotifyStates()
	{
		return ActiveNotifyStates;
	}

	const TArray<FActiveAnimNotifyState>& EAnimMontageInstance::GetActiveNotifyStates()const
	{
		return ActiveNotifyStates;
	}

	BOOL32 EAnimMontageInstance::HasPendingSectionTransition()const
	{
		return (((!bFinished) && (!bPaused) && (PendingSectionIndex >= 0)) ? TRUE : FALSE);
	}

	BOOL32 EAnimMontageInstance::ApplyPendingSectionTransition()
	{
		if ((!Montage) || (bPaused) || (PendingSectionIndex < 0)
			|| (PendingSectionIndex >= Montage->Sections.Num())
			|| (SectionIndex < 0) || (SectionIndex >= Montage->Sections.Num()))
		{
			return FALSE;
		}

		const INT32 TargetSectionIndex = PendingSectionIndex;
		const EString PreviousSectionName = Montage->Sections[SectionIndex].Name;
		const EString NextSectionName = Montage->Sections[TargetSectionIndex].Name;
		if (!StartSection(TargetSectionIndex, TRUE))
		{
			// Keep the failed target out of the pending slot so a bad chain stops instead of
			// retrying on every frame.
			PendingSectionIndex = -1;
			Stop(BlendOutSeconds);
			return FALSE;
		}

		PE_LOG_LOG(EString("Montage '") + Montage->Name + EString("' section '") + PreviousSectionName
			+ EString("' -> '") + NextSectionName + EString("'."));
		return TRUE;
	}

	BOOL32 EAnimMontageInstance::CompileBoneMask(const ESkeleton* InSkeleton)
	{
		if ((!Montage) || (SectionIndex < 0) || (SectionIndex >= Montage->Sections.Num()) || (!InSkeleton))
		{
			return FALSE;
		}

		const EAnimMontageSection& Section = Montage->Sections[SectionIndex];
		const INT32 BoneNum = InSkeleton->GetBoneCount();
		BoneMaskWeights.SetNum(BoneNum);
		for (INT32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
		{
			BoneMaskWeights[BoneIndex] = 0.0f;
		}

		if (Section.BoneMask.BoneNames.Num() <= 0)
		{
			for (INT32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
			{
				BoneMaskWeights[BoneIndex] = EMath::Clamp(Section.BoneMask.Weight, 0.0f, 1.0f);
			}
			return TRUE;
		}

		INT32 MatchedBoneNum = 0;
		for (INT32 BoneNameIndex = 0; BoneNameIndex < Section.BoneMask.BoneNames.Num(); BoneNameIndex++)
		{
			USHORT BoneIndex = 0u;
			if (InSkeleton->GetBoneMapping().FindValue(Section.BoneMask.BoneNames[BoneNameIndex], BoneIndex)
				&& (static_cast<INT32>(BoneIndex) < BoneNum))
			{
				BoneMaskWeights[BoneIndex] = EMath::Clamp(Section.BoneMask.Weight, 0.0f, 1.0f);
				MatchedBoneNum++;
			}
		}
		if (MatchedBoneNum <= 0)
		{
			// A mask with no matching bones would silently animate nothing. Fall back to the
			// full body and make the authoring mistake visible in the log.
			PE_LOG_WARN(EString("Montage '") + Montage->Name + EString("' bone mask has no matching bones; using full body."));
			for (INT32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
			{
				BoneMaskWeights[BoneIndex] = 1.0f;
			}
		}
		return TRUE;
	}

}
