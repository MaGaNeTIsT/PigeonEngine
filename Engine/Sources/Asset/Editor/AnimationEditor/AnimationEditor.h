#pragma once

#if _EDITOR_ONLY

#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>

#include "../PreviewScene/AssetPreviewScene.h"
#include "../EditorDocument.h"
#include <Animation/AnimationNotify.h>

namespace PigeonEngine
{
	class ESkeletonAnimationAsset;
	class ESkeletonAnimation;
	class ESkinnedMeshAsset;
	class ESkeletonAsset;
	struct ESkeletonAnimationClip;
	struct EBoneAnimationCurve;

	class EAnimationEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenAnimationAsset(const EString& InAssetFullPath);

	private:
		void OpenAnimationAssetNow(const EString& InAssetFullPath);
		BOOL32 SaveAnimationAsset(EString& OutError);
		BOOL32 ReloadAnimationAssetNow(EString& OutError);
		void CaptureBaseline();
		void DiscardChanges();
		void HandleDocumentAction();
		void ResetSelection();
		void RefreshPreviewOptions();
		void UpdatePreviewScene();
		void DrawAssetSummary(const ESkeletonAnimationAsset* InAsset, const ESkeletonAnimation* InAnimation) const;
		void DrawClipList(const ESkeletonAnimation* InAnimation);
		void DrawClipDetails(ESkeletonAnimationClip& InClip);
		void DrawNotifyEditor(ESkeletonAnimationClip& InClip);
		void DrawCurveList(const ESkeletonAnimationClip& InClip);
		void DrawCurveDetails(const EBoneAnimationCurve& InCurve) const;

	private:
		BOOL8							m_ShowEditor = FALSE;
		const ESkeletonAnimationAsset*	m_AnimationAsset = nullptr;
		EString							m_OpenedAssetFullPath;
		EString							m_LastOpenError;
		EString							m_StatusMessage;
		BOOL8							m_bStatusSucceeded = TRUE;
		FEditorDocumentState			m_Document;
		// One snapshot per clip of the notify lists as they were when the asset was opened or
		// last saved. Notify editing is the only live mutation this editor performs, so this
		// is all Discard has to restore.
		TArray<TArray<FAnimNotifyEvent>>	m_BaselineNotifies;
		INT32							m_SelectedClipIndex = 0;
		INT32							m_SelectedCurveIndex = 0;
		EAssetPreviewScene				m_PreviewScene;
		TArray<EString>					m_AvailablePreviewMeshPaths;
		TArray<EString>					m_AvailablePreviewSkeletonPaths;
		const ESkinnedMeshAsset*		m_PreviewMeshAsset = nullptr;
		const ESkeletonAsset*			m_PreviewSkeletonAsset = nullptr;
		INT32							m_SelectedPreviewMeshIndex = -1;
		INT32							m_SelectedPreviewSkeletonIndex = -1;
		BOOL8							m_PreviewAutoRotate = TRUE;
		BOOL8							m_PreviewPlayAnimation = TRUE;
		FLOAT							m_PreviewPlaybackRate = 1.0f;
		EAnimationLoopMode				m_PreviewLoopMode = ANIMATION_LOOP_REPEAT;
		// The binding report is rebuilt only when the selected skeleton changes; doing it
		// every frame would call Bind (and log its warnings) at the UI refresh rate.
		const ESkeletonAsset*			m_LastBindingSkeletonAsset = nullptr;
		BOOL8							m_BindingValid = FALSE;
		INT32							m_BindingMissingBoneNum = 0;
	};
}

#endif