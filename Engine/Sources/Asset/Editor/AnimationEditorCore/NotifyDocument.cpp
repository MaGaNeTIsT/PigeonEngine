#include "NotifyDocument.h"

#if _EDITOR_ONLY

#include <AnimationAsset/AnimationAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{

	FNotifyDocument::~FNotifyDocument()
	{
		ReleaseDraftDefinition();
	}

	BOOL32 FNotifyDocument::Initialize(const EString& InAssetPath, const EString& InAssetName
		, const EAnimNotifyAsset* InAsset, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("No notify asset was supplied.");
			return FALSE;
		}
		const EAnimNotifyDefinition* Definition = InAsset->GetStoragedResource();
		if (!Definition)
		{
			OutError = EString("Notify asset has no parsed resource.");
			return FALSE;
		}

		Kind = EAnimationDocumentKind::NOTIFY;
		AssetPath = InAssetPath.Replace("\\", "/");
		AssetDirectory = EPath::GetFileFolderPath(AssetPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		AssetName = InAssetName;
		DisplayName = InAssetName;
		m_Asset = InAsset;
		PublishedAsset = InAsset;
		DraftJson = Definition->SourceJson;
		if (DraftJson.Length() <= 0u)
		{
			OutError = EString("Notify asset has no source JSON.");
			return FALSE;
		}
		if (!CommitJsonText(DraftJson, OutError))
		{
			return FALSE;
		}
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		Commands.Clear();
		Selection.Clear();
		RebuildDiagnostics();
		return TRUE;
	}

	const EAnimNotifyAsset* FNotifyDocument::GetAsset()const
	{
		return m_Asset;
	}

	const EAnimNotifyDefinition* FNotifyDocument::GetDraftDefinition()const
	{
		return m_DraftDefinition;
	}

	BOOL32 FNotifyDocument::CommitSourceText(const EString& InJsonText, EString& OutError)
	{
		return CommitJsonText(InJsonText, OutError);
	}

	BOOL32 FNotifyDocument::CommitJsonText(const EString& InJsonText, EString& OutError)
	{
		OutError = EString();
		EAnimNotifyDefinition* NewDefinition = EAnimNotifyManager::GetManagerSingleton()
			->BuildTransientNotifyDefinition(InJsonText, OutError);
		if (!NewDefinition)
		{
			LastError = OutError;
			return FALSE;
		}
		ReleaseDraftDefinition();
		m_DraftDefinition = NewDefinition;
		DraftJson = InJsonText;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FNotifyDocument::UpdateDefinition(const EString& InName, const EString& InDescription
		, const EString& InClassName, const EString& InPayloadJson
		, FLOAT InDefaultDuration, EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("Notify JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		BOOL32 bIsState = FALSE;
		if ((InClassName.Length() > 0u)
			&& (EAnimNotifyRegistry::IsStateClass(InClassName, bIsState) == FALSE))
		{
			OutError = EString("Notify class is not registered: ") + InClassName;
			LastError = OutError;
			return FALSE;
		}

		Root.SetStringField(PE_TEXT("Name"), InName);
		Root.SetStringField(PE_TEXT("Description"), InDescription);
		Root.SetStringField(PE_TEXT("Class"), InClassName);
		Root.SetStringField(PE_TEXT("Payload"), InPayloadJson);
		Root.SetBoolField(PE_TEXT("State"), bIsState);
		Root.SetFloatField(PE_TEXT("Duration")
			, (bIsState != FALSE) ? EMath::Max(InDefaultDuration, 0.001f) : 0.0f);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FNotifyDocument::Apply(EString& OutError)
	{
		const EAnimNotifyAsset* NewAsset = nullptr;
		if (!EAnimNotifyManager::GetManagerSingleton()->ApplyNotifyAsset(
			m_Asset, DraftJson, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		AppliedJson = DraftJson;
		EAnimationManager::GetManagerSingleton()->RefreshNotifyReferences();
		EAnimMontageManager::GetManagerSingleton()->RefreshNotifyReferences();
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FNotifyDocument::Save(EString& OutError)
	{
		const EAnimNotifyAsset* NewAsset = nullptr;
		if (!EAnimNotifyManager::GetManagerSingleton()->SaveNotifyAssetCopyOnWrite(
			m_Asset, DraftJson, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		AppliedJson = DraftJson;
		BaselineJson = DraftJson;
		EAnimationManager::GetManagerSingleton()->RefreshNotifyReferences();
		EAnimMontageManager::GetManagerSingleton()->RefreshNotifyReferences();
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FNotifyDocument::Reload(EString& OutError)
	{
		const EAnimNotifyAsset* NewAsset = nullptr;
		if (!EAnimNotifyManager::GetManagerSingleton()->ReloadNotifyAsset(
			AssetDirectory, AssetName, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		const EAnimNotifyDefinition* Definition = (NewAsset ? NewAsset->GetStoragedResource() : nullptr);
		if (!Definition)
		{
			OutError = EString("Reloaded notify has no resource.");
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		DraftJson = Definition->SourceJson;
		if (!CommitJsonText(DraftJson, OutError))
		{
			return FALSE;
		}
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		Commands.Clear();
		Selection.Clear();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FNotifyDocument::Discard(EString& OutError)
	{
		return Reload(OutError);
	}

	void FNotifyDocument::RefreshDerived()
	{
		EString Error;
		if (!CommitJsonText(DraftJson, Error))
		{
			LastError = Error;
			Diagnostics.Clear();
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR, Error);
		}
	}

	void FNotifyDocument::ReleaseDraftDefinition()
	{
		if (m_DraftDefinition)
		{
			delete m_DraftDefinition;
			m_DraftDefinition = nullptr;
		}
	}

	void FNotifyDocument::RebuildDiagnostics()
	{
		Diagnostics.Clear();
		if (!m_DraftDefinition)
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
				, (LastError.Length() > 0u) ? LastError : EString("Notify definition is not valid."));
			return;
		}
		if (m_DraftDefinition->Name.Length() <= 0u)
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
				, EString("Notify definition has no name."));
		}
		const CHAR* KindText = ((m_DraftDefinition->bIsState != FALSE) ? "State" : "Instant");
		Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_INFO
			, EString("Class '") + m_DraftDefinition->NotifyClassName + EString("' (") + EString(KindText) + EString(")."));
	}

}

#endif
