#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Animation/AnimationNotify.h>
#include "AnimationDocument.h"

namespace PigeonEngine
{

	class ESkeletonAnimationAsset;
	struct ESkeletonAnimationClip;

	/*
	* Clip document. Animation clips are binary assets, so only the notify lists are stored
	* as an editable draft. A commit applies the draft to the live clip arrays and updates the
	* JSON snapshot used by undo/redo; Save writes the asset back to disk.
	*/
	class FClipDocument final : public FAnimationDocument
	{
	public:
		BOOL32 Initialize(const EString& InAssetPath, const EString& InAssetName
			, const ESkeletonAnimationAsset* InAsset, EString& OutError);

		PE_NODISCARD const ESkeletonAnimationAsset* GetAsset()const;
		PE_NODISCARD const TArray<TArray<FAnimNotifyEvent>>& GetDraftNotifies()const;

		BOOL32 CommitNotifies(const TArray<TArray<FAnimNotifyEvent>>& InNotifies, EString& OutError);
		BOOL32 CommitSourceText(const EString& InJsonText, EString& OutError);

		virtual BOOL32 Apply(EString& OutError) override;
		virtual BOOL32 Save(EString& OutError) override;
		virtual BOOL32 Reload(EString& OutError) override;
		virtual BOOL32 Discard(EString& OutError) override;
		virtual void RefreshDerived() override;

	private:
		BOOL32 ParseNotifies(const EString& InJsonText
			, TArray<TArray<FAnimNotifyEvent>>& OutNotifies, EString& OutError)const;
		EString SerializeNotifies()const;
		void RebuildDiagnostics();

	private:
		const ESkeletonAnimationAsset* m_Asset = nullptr;
		TArray<TArray<FAnimNotifyEvent>> m_DraftNotifies;
	};

}

#endif
