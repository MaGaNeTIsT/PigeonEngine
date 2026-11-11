#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include "AnimationDocument.h"

namespace PigeonEngine
{

	class ESkeletonAsset;
	class CJsonObject;

	/*
	* Editable ordered slot groups for one skeleton. The skeleton asset is binary, so the
	* document keeps an in-memory draft plus a small JSON representation used by the shared
	* snapshot command stack and the Source tab. Apply writes the draft into the live
	* skeleton resource; Save also calls ESkeletonAssetManager::SaveSkeletonAsset.
	*/
	class FSlotGroupDocument final : public FAnimationDocument
	{
	public:
		BOOL32 Initialize(const EString& InAssetPath, const EString& InAssetName
			, const ESkeletonAsset* InAsset, EString& OutError);

		PE_NODISCARD const ESkeletonAsset* GetAsset()const;
		PE_NODISCARD const TArray<ESkeletonSlotGroup>& GetDraftGroups()const;

		BOOL32 CommitSourceText(const EString& InJsonText, EString& OutError);
		BOOL32 AddGroup(EString& OutError);
		BOOL32 DeleteGroup(INT32 InGroupIndex, EString& OutError);
		BOOL32 MoveGroup(INT32 InGroupIndex, INT32 InDelta, EString& OutError);
		BOOL32 RenameGroup(INT32 InGroupIndex, const EString& InName, EString& OutError);
		BOOL32 AddSlot(INT32 InGroupIndex, EString& OutError);
		BOOL32 DeleteSlot(INT32 InGroupIndex, INT32 InSlotIndex, EString& OutError);
		BOOL32 RenameSlot(INT32 InGroupIndex, INT32 InSlotIndex, const EString& InName, EString& OutError);
		BOOL32 ResetDefaults(EString& OutError);

		virtual BOOL32 Apply(EString& OutError) override;
		virtual BOOL32 Save(EString& OutError) override;
		virtual BOOL32 Reload(EString& OutError) override;
		virtual BOOL32 Discard(EString& OutError) override;
		virtual void RefreshDerived() override;

	private:
		BOOL32 CommitGroups(const TArray<ESkeletonSlotGroup>& InGroups, EString& OutError);
		BOOL32 CommitJsonText(const EString& InJsonText, EString& OutError);
		BOOL32 ParseGroups(const EString& InJsonText, TArray<ESkeletonSlotGroup>& OutGroups, EString& OutError)const;
		EString SerializeGroups(const TArray<ESkeletonSlotGroup>& InGroups)const;
		BOOL32 ValidateGroups(const TArray<ESkeletonSlotGroup>& InGroups, EString& OutError)const;
		BOOL32 CopyGroupsToLive(EString& OutError);
		void RebuildDiagnostics();

	private:
		const ESkeletonAsset*	m_Asset = nullptr;
		TArray<ESkeletonSlotGroup>	m_DraftGroups;
	};

}

#endif
