#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>
#include "../EditorDocument.h"

namespace PigeonEngine
{

	class CJsonObject;
	class EAnimMontage;
	class EAnimMontageAsset;
	struct EAnimMontageSection;
	class ESkeletonAsset;
	class ESkeletonAnimationAsset;

	/*
	* First-pass montage editor: slot groups, sections, chaining and notifies are authored as
	* JSON, validated on Apply, and saved back into the same PAsset. The read-only summary
	* makes the current chain and event counts visible without parsing the JSON by eye. A
	* timeline view can replace this window later without changing the asset or runtime.
	*/
	class EAnimMontageEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenMontageAsset(const EString& InAssetFullPath);

	private:
		void OpenMontageAssetNow(const EString& InAssetFullPath);
		BOOL32 SaveMontageAsset(EString& OutError);
		BOOL32 ReloadMontageAssetNow(EString& OutError);
		void CaptureBaseline();
		BOOL32 DiscardChanges();
		void HandleDocumentAction();
		void LoadJsonBufferFromAsset();
		void ReplaceJsonBuffer(const EString& InJsonText);
		// Rebuilds the live montage from InJsonText; on success the text buffer is replaced and
		// the document is marked dirty.
		BOOL32 CommitJsonText(const EString& InJsonText);
		// Rewrites one section object in the JSON buffer and commits it. Existing boneMask and
		// notifies fields are preserved because only the mutated fields are changed in place.
		BOOL32 UpdateMontageSection(INT32 InSectionIndex, const TFunction<void(CJsonObject&)>& InMutator);
		BOOL32 AddMontageSection();
		BOOL32 RemoveMontageSection(INT32 InSectionIndex);
		BOOL32 UpdateBoneMask(INT32 InSectionIndex, const TArray<EString>& InBones, FLOAT InWeight
			, const EString& InMaskName);
		BOOL32 UpdateMontageNotify(INT32 InSectionIndex, INT32 InNotifyIndex
			, const TFunction<void(CJsonObject&)>& InMutator);
		BOOL32 AddMontageNotify(INT32 InSectionIndex);
		BOOL32 RemoveMontageNotify(INT32 InSectionIndex, INT32 InNotifyIndex);
		// Returns TRUE when a notify change was committed; the caller must stop touching the
		// live montage for this frame.
		BOOL32 DrawSectionNotifies(const EAnimMontageSection& InSection, INT32 InSectionIndex);
		void DrawEditableSections(const EAnimMontage& InMontage);
		void RefreshSkeletonAsset(const EAnimMontage& InMontage);
		void DrawTimeline(const EAnimMontage& InMontage);
		void DrawCurvePreview(const EAnimMontage& InMontage);

	private:
		BOOL8							m_ShowEditor = FALSE;
		const EAnimMontageAsset*		m_Asset = nullptr;
		// Loaded on demand for clip durations; the timeline still works without it.
		const ESkeletonAnimationAsset*	m_AnimationAsset = nullptr;
		// Slot names for the section editor come from the skeleton, not the legacy slotGroups
		// embedded in old montage assets.
		const ESkeletonAsset*			m_SkeletonAsset = nullptr;
		EString							m_OpenedAssetFullPath;
		EString							m_LastError;
		EString							m_StatusMessage;
		BOOL8							m_bStatusSucceeded = TRUE;
		FEditorDocumentState			m_Document;
		// Last applied/saved JSON. Discard rebuilds the live montage from this text.
		EString							m_BaselineJson;
		TArray<CHAR>					m_JsonBuffer;
		INT32							m_SelectedCurveSectionIndex = 0;
		INT32							m_SelectedCurveIndex = 0;
	};

}

#endif
