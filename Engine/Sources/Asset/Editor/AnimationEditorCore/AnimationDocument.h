#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Map.h>
#include <imgui.h>
#include "EditorCommandStack.h"
#include "EditorDiagnostics.h"
#include "EditorSelection.h"

namespace PigeonEngine
{

	class EAssetBase;

	enum class EAnimationDocumentKind : UINT8
	{
		NONE = 0,
		CLIP,
		MONTAGE,
		STATE_MACHINE,
		NOTIFY,
		SLOT_GROUP
	};

	/*
	* Shared document state for one opened animation asset. The draft is the only editable
	* source; baseline is what disk holds and applied is what the live asset holds. Applying
	* publishes a new asset object, so the two can differ while the editor is open.
	*/
	class FAnimationDocument
	{
	public:
		FAnimationDocument() = default;
		virtual ~FAnimationDocument() = default;

	public:
		EAnimationDocumentKind	Kind = EAnimationDocumentKind::NONE;
		EString					AssetPath;
		EString					AssetDirectory;
		EString					AssetName;
		EString					DisplayName;
		const EAssetBase*		PublishedAsset = nullptr;
		EString					DraftJson;
		EString					BaselineJson;
		EString					AppliedJson;
		TMap<EString, ImVec2>	NodePositions;
		FEditorSelection		Selection;
		FEditorDiagnostics		Diagnostics;
		FEditorCommandStack		Commands;
		EString					LastError;
		BOOL32					bLoading = FALSE;
		BOOL32					bClosePending = FALSE;

	public:
		PE_NODISCARD BOOL32 IsDirty()const;
		PE_NODISCARD BOOL32 IsApplied()const;
		PE_NODISCARD FEditorSnapshot CaptureSnapshot()const;
		void RestoreSnapshot(const FEditorSnapshot& InSnapshot);
		virtual BOOL32 Apply(EString& OutError) = 0;
		virtual BOOL32 Save(EString& OutError) = 0;
		virtual BOOL32 Reload(EString& OutError) = 0;
		// Reverts live/draft state to the last saved version. Closing with "Discard" uses
		// this so a live-edit document cannot leak unsaved changes into the running asset.
		virtual BOOL32 Discard(EString& OutError) = 0;
		virtual void RefreshDerived() = 0;
	};

}

#endif
