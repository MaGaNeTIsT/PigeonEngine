#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Animation/AnimationMontage.h>
#include "AnimationDocument.h"

namespace PigeonEngine
{

	class EAnimMontageAsset;
	class ESkeletonAnimationAsset;
	struct ESkeletonAnimationClip;
	class CJsonObject;

	/*
	* Editable montage document. The draft JSON is the only editable source; every structured
	* edit builds a transient EAnimMontage first, mirrors the StateMachine/Clip documents, and
	* the live asset is replaced only by Apply/Save. This keeps a bad section rename or a
	* malformed notify edit from breaking the asset other systems are currently using.
	*/
	class FMontageDocument final : public FAnimationDocument
	{
	public:
		~FMontageDocument() override;

	public:
		BOOL32 Initialize(const EString& InAssetPath, const EString& InAssetName
			, const EAnimMontageAsset* InAsset, EString& OutError);

		PE_NODISCARD const EAnimMontage* GetDraftMontage()const;
		PE_NODISCARD const EAnimMontageAsset* GetMontageAsset()const;
		PE_NODISCARD const ESkeletonAnimationAsset* GetAnimationAsset()const;
		PE_NODISCARD const ESkeletonAnimationClip* GetSectionClip(INT32 InSectionIndex)const;
		PE_NODISCARD INT32 GetSectionClipIndex(INT32 InSectionIndex)const;
		PE_NODISCARD FLOAT GetSectionClipDuration(INT32 InSectionIndex)const;

		BOOL32 CommitSourceText(const EString& InJsonText, EString& OutError);

		BOOL32 AddSection(const EString& InClipName, const EString& InSlotName, EString& OutError);
		BOOL32 DeleteSection(INT32 InSectionIndex, EString& OutError);
		// Writes the editable section fields (Name/Clip/Slot/Next/Loop/Rate/Blend/Additive) and
		// preserves BoneMask/Notifies in the JSON. A rename remaps other sections' Next.
		BOOL32 UpdateSection(INT32 InSectionIndex, const EAnimMontageSection& InSection, EString& OutError);
		BOOL32 SetSectionNotifies(INT32 InSectionIndex
			, const TArray<FAnimNotifyEvent>& InNotifies, EString& OutError);
		BOOL32 SetSectionBoneMask(INT32 InSectionIndex, const ESkeletonBoneMask& InMask, EString& OutError);

		virtual BOOL32 Apply(EString& OutError) override;
		virtual BOOL32 Save(EString& OutError) override;
		virtual BOOL32 Reload(EString& OutError) override;
		virtual BOOL32 Discard(EString& OutError) override;
		virtual void RefreshDerived() override;

	private:
		BOOL32 CommitJsonText(const EString& InJsonText, EString& OutError);
		BOOL32 MutateJson(const TFunction<void(CJsonObject&)>& InMutator, EString& OutError);
		BOOL32 UpdateSectionObject(INT32 InSectionIndex
			, const TFunction<void(CJsonObject&)>& InMutator, EString& OutError);
		void ReleaseDraftMontage();
		void SyncAnimationAsset();
		void RebuildDiagnostics();

	private:
		const EAnimMontageAsset*	m_Asset = nullptr;
		EAnimMontage*				m_DraftMontage = nullptr;
		const ESkeletonAnimationAsset*	m_AnimationAsset = nullptr;
	};

}

#endif
