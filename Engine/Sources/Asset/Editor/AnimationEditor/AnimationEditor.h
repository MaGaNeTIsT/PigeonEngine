#pragma once

#if ENGINE_EDITOR_ONLY

#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>

#include "../PreviewScene/AssetPreviewScene.h"

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
		void ResetSelection();
		void RefreshPreviewOptions();
		void UpdatePreviewScene(const ESkeletonAnimationClip* InSelectedClip);
		void DrawAssetSummary(const ESkeletonAnimationAsset* InAsset, const ESkeletonAnimation* InAnimation) const;
		void DrawClipList(const ESkeletonAnimation* InAnimation);
		void DrawClipDetails(ESkeletonAnimationClip& InClip);
		void DrawNotifyEditor(ESkeletonAnimationClip& InClip);
		void DrawCurveList(const ESkeletonAnimationClip& InClip);
		void DrawCurveDetails(const EBoneAnimationCurve& InCurve) const;

	private:
		BOOL8							bShowEditor = FALSE;
		const ESkeletonAnimationAsset*	AnimationAsset = nullptr;
		EString							OpenedAssetFullPath;
		EString							LastOpenError;
		INT32							SelectedClipIndex = 0;
		INT32							SelectedCurveIndex = 0;
		EAssetPreviewScene				PreviewScene;
		TArray<EString>					AvailablePreviewMeshPaths;
		TArray<EString>					AvailablePreviewSkeletonPaths;
		const ESkinnedMeshAsset*		PreviewMeshAsset = nullptr;
		const ESkeletonAsset*			PreviewSkeletonAsset = nullptr;
		INT32							SelectedPreviewMeshIndex = -1;
		INT32							SelectedPreviewSkeletonIndex = -1;
		BOOL8							bPreviewAutoRotate = TRUE;
		BOOL8							bPreviewPlayAnimation = TRUE;
		FLOAT							PreviewPlaybackRate = 1.0f;
		// The binding report is rebuilt only when the selected skeleton changes; doing it
		// every frame would call Bind (and log its warnings) at the UI refresh rate.
		const ESkeletonAsset*			LastBindingSkeletonAsset = nullptr;
		BOOL8							bBindingValid = FALSE;
		INT32							BindingMissingBoneNum = 0;
	};
}

#endif