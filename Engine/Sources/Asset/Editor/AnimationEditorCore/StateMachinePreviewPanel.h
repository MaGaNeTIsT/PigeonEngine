#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include "../PreviewScene/AssetPreviewScene.h"
#include "StateMachineDocument.h"

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;

	/*
	* Draft state machine preview. It uses the normal EAnimInstance path through
	* EAssetPreviewScene, so transitions and parameter conditions evaluate exactly like they
	* do at runtime.
	*/
	class FStateMachinePreviewPanel
	{
	public:
		FStateMachinePreviewPanel();
		~FStateMachinePreviewPanel();

		void Draw(FStateMachineDocument& InDocument);
	private:
		void EnsureInitialized();
		void RefreshAssetOptions(const EString& InPreferredSkeleton);
		void RebindPreview(FStateMachineDocument& InDocument);
		void DrawControls(FStateMachineDocument& InDocument);
		void DrawParameters(FStateMachineDocument& InDocument);
		BOOL32 LoadPreviewAssets();
	private:
		EAssetPreviewScene			m_PreviewScene;
		BOOL32						m_bInitialized = FALSE;
		TArray<EString>				m_MeshPaths;
		TArray<EString>				m_SkeletonPaths;
		INT32						m_SelectedMeshIndex = -1;
		INT32						m_SelectedSkeletonIndex = -1;
		const ESkinnedMeshAsset*	m_MeshAsset = nullptr;
		const ESkeletonAsset*		m_SkeletonAsset = nullptr;
		const ESkinnedMeshAsset*	m_BoundMeshAsset = nullptr;
		const ESkeletonAsset*		m_BoundSkeletonAsset = nullptr;
		const ESkeletonAnimationAsset* m_BoundAnimationAsset = nullptr;
		const EAnimStateMachine*	m_BoundMachine = nullptr;
		const FStateMachineDocument* m_LastDocument = nullptr;
		EString						m_LastSkeletonName;
		BOOL32						m_bPlay = TRUE;
		BOOL32						m_bAutoRotate = TRUE;
		FLOAT						m_PlayRate = 1.0f;
	};

}

#endif
