#pragma once

#if _EDITOR_ONLY

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
		void DrawClipDetails(const ESkeletonAnimationClip& InClip);
		void DrawCurveList(const ESkeletonAnimationClip& InClip);
		void DrawCurveDetails(const EBoneAnimationCurve& InCurve) const;

	private:
		BOOL8							m_ShowEditor = FALSE;
		const ESkeletonAnimationAsset*	m_AnimationAsset = nullptr;
		EString							m_OpenedAssetFullPath;
		EString							m_LastOpenError;
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
	};
}

#endif