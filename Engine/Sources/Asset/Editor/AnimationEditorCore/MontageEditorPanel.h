#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include "../EditorUi.h"
#include "../PreviewScene/AssetPreviewScene.h"
#include "MontageDocument.h"
#include "PreviewAssetCatalog.h"
#include "TimelineWidget.h"
#include "TransportBar.h"

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;
	class ESkeletonAnimationAsset;
	struct ESkeletonAnimationClip;
	struct EAnimMontageSection;

	class FMontageEditorPanel
	{
	public:
		FMontageEditorPanel();
		~FMontageEditorPanel();

		void Draw(FMontageDocument& InDocument);

	private:
		void DrawToolbar(FMontageDocument& InDocument);
		void DrawLayout(FMontageDocument& InDocument, FLOAT InHeight);
		void DrawLeftPanel(FMontageDocument& InDocument);
		void DrawCenter(FMontageDocument& InDocument);
		void DrawPreviewArea(FMontageDocument& InDocument);
		void DrawTimelineArea(FMontageDocument& InDocument);
		void DrawInspector(FMontageDocument& InDocument);
		void DrawSectionInspector(FMontageDocument& InDocument, INT32 InSectionIndex);
		void DrawNotifyInspector(FMontageDocument& InDocument, INT32 InSectionIndex, INT32 InNotifyIndex);
		void DrawBottomPanel(FMontageDocument& InDocument, FLOAT InTotalHeight);
		void DrawDiagnostics(FMontageDocument& InDocument);
		void DrawSource(FMontageDocument& InDocument);

		void EnsureSelection(FMontageDocument& InDocument);
		void BindPreview(FMontageDocument& InDocument);
		void RefreshPreviewCatalog(const EString& InSkeletonName);
		BOOL32 ApplyTransport(FMontageDocument& InDocument, const FTransportInteraction& InInteraction);
		void SyncTransport(FMontageDocument& InDocument);
		void BuildTimelineTracks(FMontageDocument& InDocument, TArray<FTimelineTrack>& OutTracks)const;
		void RunMutation(FMontageDocument& InDocument
			, const TFunction<BOOL32(EString&)>& InMutation, const EString& InSuccessMessage);
		void CommitSection(FMontageDocument& InDocument, INT32 InSectionIndex
			, const EAnimMontageSection& InSection, const EString& InSuccessMessage);
		void CommitSectionNotifies(FMontageDocument& InDocument, INT32 InSectionIndex
			, const TArray<FAnimNotifyEvent>& InNotifies, const EString& InSuccessMessage);
		void AddSection(FMontageDocument& InDocument);
		void DeleteSelectedSection(FMontageDocument& InDocument);
		void AddNotify(FMontageDocument& InDocument);
		void DeleteSelectedNotify(FMontageDocument& InDocument);
		void ResetSectionBuffers();
		void ResetNotifyBuffers();
		void RestoreDocumentSnapshot(FMontageDocument& InDocument, const FEditorSnapshot& InSnapshot);
		PE_NODISCARD INT32 GetSelectedSectionIndex(const FMontageDocument& InDocument)const;
		PE_NODISCARD const EAnimMontageSection* GetSelectedSection(const FMontageDocument& InDocument)const;
		PE_NODISCARD const ESkeletonAnimationClip* GetSelectedClip(const FMontageDocument& InDocument)const;
		PE_NODISCARD FLOAT GetSelectedClipDuration(const FMontageDocument& InDocument)const;
		PE_NODISCARD EString GetDefaultSectionClip(const FMontageDocument& InDocument)const;
		PE_NODISCARD EString GetDefaultSectionSlot()const;

	private:
		FTimelineWidget			m_Timeline;
		FTransportState			m_Transport;
		FLOAT					m_PreviewHeight = 180.0f;
		FLOAT					m_TimelineHeight = 170.0f;
		BOOL8					m_bShowOutput = TRUE;
		FLOAT					m_BottomHeight = 110.0f;
		EAssetPreviewScene		m_PreviewScene;
		FPreviewAssetCatalog	m_PreviewCatalog;
		BOOL32					m_bPreviewInitialized = FALSE;
		BOOL32					m_bPreviewCatalogReady = FALSE;
		EString					m_PreviewSkeletonName;
		const FMontageDocument*	m_BoundDocument = nullptr;
		INT32					m_MeshIndex = -1;
		INT32					m_SkeletonIndex = -1;
		const ESkinnedMeshAsset* m_MeshAsset = nullptr;
		const ESkeletonAsset*	m_SkeletonAsset = nullptr;
		const ESkinnedMeshAsset* m_BoundMeshAsset = nullptr;
		const ESkeletonAsset*	m_BoundSkeletonAsset = nullptr;
		const ESkeletonAnimationAsset* m_BoundAnimationAsset = nullptr;
		INT32					m_BoundClipIndex = -1;
		INT32					m_SelectedSectionIndex = 0;
		INT32					m_SelectedNotifyIndex = -1;
		FGrowableTextBuffer		m_SectionNameBuffer;
		FGrowableTextBuffer		m_SlotNameBuffer;
		FGrowableTextBuffer		m_MaskSearchBuffer;
		EString					m_SectionBufferSource;
		FGrowableTextBuffer		m_NotifyNameBuffer;
		FGrowableTextBuffer		m_NotifyClassBuffer;
		FGrowableTextBuffer		m_NotifyAssetBuffer;
		FGrowableTextBuffer		m_NotifyPayloadBuffer;
		EString					m_NotifyBufferSource;
		FGrowableTextBuffer		m_SourceBuffer;
		const FMontageDocument* m_SourceDocument = nullptr;
		EString					m_StatusMessage;
		BOOL32					m_bStatusSucceeded = TRUE;
	};

}

#endif
