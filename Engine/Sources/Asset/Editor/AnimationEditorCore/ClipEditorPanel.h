#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include "../EditorUi.h"
#include "../PreviewScene/AssetPreviewScene.h"
#include "ClipDocument.h"
#include "PreviewAssetCatalog.h"
#include "TimelineWidget.h"
#include "TransportBar.h"

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;
	struct ESkeletonAnimationClip;
	struct EBoneAnimationCurve;

	class FClipEditorPanel
	{
	public:
		FClipEditorPanel();
		~FClipEditorPanel();

		void Draw(FClipDocument& InDocument);
	private:
		void DrawToolbar(FClipDocument& InDocument);
		void DrawLayout(FClipDocument& InDocument, FLOAT InHeight);
		void DrawLeftPanel(FClipDocument& InDocument);
		void DrawCenter(FClipDocument& InDocument);
		void DrawPreviewArea(FClipDocument& InDocument);
		void DrawTimelineArea(FClipDocument& InDocument);
		void DrawCurveArea(FClipDocument& InDocument);
		void DrawInspector(FClipDocument& InDocument);
		void DrawNotifyInspector(FClipDocument& InDocument, INT32 InNotifyIndex);
		void DrawClipInspector(FClipDocument& InDocument);
		void DrawBottomPanel(FClipDocument& InDocument, FLOAT InTotalHeight);
		void DrawDiagnostics(FClipDocument& InDocument);
		void DrawSource(FClipDocument& InDocument);

		void BindPreview(FClipDocument& InDocument);
		void RefreshPreviewCatalog(const EString& InSkeletonName);
		void ApplyTransport(FClipDocument& InDocument, const FTransportInteraction& InInteraction);
		void SyncTransport(FClipDocument& InDocument);
		void BuildTimelineTracks(FClipDocument& InDocument, TArray<FTimelineTrack>& OutTracks)const;
		void RunMutation(FClipDocument& InDocument
			, const TFunction<BOOL32(EString&)>& InMutation, const EString& InSuccessMessage);
		void CommitSelectedNotify(FClipDocument& InDocument, const FAnimNotifyEvent& InNotify
			, const EString& InSuccessMessage);
		void AddNotify(FClipDocument& InDocument);
		void DeleteSelectedNotify(FClipDocument& InDocument);
		void ResetNotifyBuffers();
		void EnsureSelection(FClipDocument& InDocument);
		PE_NODISCARD ESkeletonAnimationClip* GetSelectedClip(FClipDocument& InDocument)const;
		PE_NODISCARD FLOAT GetSelectedClipDuration(FClipDocument& InDocument)const;

	private:
		FTimelineWidget			m_Timeline;
		FTransportState			m_Transport;
		FLOAT					m_PreviewHeight = 150.0f;
		FLOAT					m_TimelineHeight = 150.0f;
		BOOL8					m_bShowOutput = TRUE;
		FLOAT					m_BottomHeight = 110.0f;
		EAssetPreviewScene		m_PreviewScene;
		FPreviewAssetCatalog	m_PreviewCatalog;
		BOOL32					m_bPreviewInitialized = FALSE;
		BOOL32					m_bPreviewCatalogReady = FALSE;
		EString					m_PreviewSkeletonName;
		INT32					m_MeshIndex = -1;
		INT32					m_SkeletonIndex = -1;
		const ESkinnedMeshAsset* m_MeshAsset = nullptr;
		const ESkeletonAsset*	m_SkeletonAsset = nullptr;
		const ESkinnedMeshAsset* m_BoundMeshAsset = nullptr;
		const ESkeletonAsset*	m_BoundSkeletonAsset = nullptr;
		const ESkeletonAnimationAsset* m_BoundAnimationAsset = nullptr;
		INT32					m_BoundClipIndex = -1;
		INT32					m_SelectedClipIndex = 0;
		INT32					m_SelectedCurveIndex = 0;
		INT32					m_SelectedNotifyIndex = -1;
		FGrowableTextBuffer		m_NotifyNameBuffer;
		FGrowableTextBuffer		m_NotifyClassBuffer;
		FGrowableTextBuffer		m_NotifyAssetBuffer;
		FGrowableTextBuffer		m_NotifyPayloadBuffer;
		EString					m_NotifyBufferSource;
		FGrowableTextBuffer		m_SourceBuffer;
		const FClipDocument*	m_SourceDocument = nullptr;
		EString					m_StatusMessage;
		BOOL32					m_bStatusSucceeded = TRUE;
	};

}

#endif
