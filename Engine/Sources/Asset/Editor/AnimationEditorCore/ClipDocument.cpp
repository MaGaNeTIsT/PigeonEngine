#include "ClipDocument.h"

#if _EDITOR_ONLY

#include <AnimationAsset/AnimationAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{

	namespace
	{
		struct FJsonObjectArrayOwner
		{
			explicit FJsonObjectArrayOwner(TArray<CJsonObject*>& InObjects) : Objects(InObjects) {}
			~FJsonObjectArrayOwner()
			{
				for (INT32 Index = 0; Index < Objects.Num(); Index++)
				{
					delete Objects[Index];
				}
				Objects.Empty();
			}
			TArray<CJsonObject*>& Objects;
		};

		FLOAT GetClipDurationSeconds(const ESkeletonAnimationClip& InClip)
		{
			if (InClip.Duration <= 0.0f)
			{
				return 0.0f;
			}
			const FLOAT TicksPerSecond = (InClip.TicksPerSecond > 1e-3f)
				? InClip.TicksPerSecond
				: EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND;
			return ((TicksPerSecond > 0.0f) ? (InClip.Duration / TicksPerSecond) : 0.0f);
		}
	}

	BOOL32 FClipDocument::Initialize(const EString& InAssetPath, const EString& InAssetName
		, const ESkeletonAnimationAsset* InAsset, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("No animation asset was supplied.");
			return FALSE;
		}
		const ESkeletonAnimation* Animation = InAsset->GetStoragedResource();
		if (!Animation)
		{
			OutError = EString("Animation asset has no parsed resource.");
			return FALSE;
		}

		Kind = EAnimationDocumentKind::CLIP;
		AssetPath = InAssetPath.Replace("\\", "/");
		AssetDirectory = EPath::GetFileFolderPath(AssetPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		AssetName = InAssetName;
		DisplayName = InAssetName;
		PublishedAsset = InAsset;
		m_Asset = InAsset;
		m_DraftNotifies.SetNum(Animation->GetAnimationClipNum());
		for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			if (Clip)
			{
				m_DraftNotifies[ClipIndex] = Clip->Notifies;
			}
		}
		DraftJson = SerializeNotifies();
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		Commands.Clear();
		Selection.Clear();
		Selection.Type = EEditorSelectionType::CLIP;
		Selection.ObjectIndex = ((Animation->GetAnimationClipNum() > 0) ? 0 : -1);
		RebuildDiagnostics();
		return TRUE;
	}

	const ESkeletonAnimationAsset* FClipDocument::GetAsset()const
	{
		return m_Asset;
	}

	const TArray<TArray<FAnimNotifyEvent>>& FClipDocument::GetDraftNotifies()const
	{
		return m_DraftNotifies;
	}

	EString FClipDocument::SerializeNotifies()const
	{
		const ESkeletonAnimation* Animation = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			return EString();
		}
		CJsonObject Root;
		TArray<CJsonObject*> ClipObjects;
		for (INT32 ClipIndex = 0; ClipIndex < m_DraftNotifies.Num(); ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			CJsonObject* ClipObject = new CJsonObject();
			ClipObject->SetStringField(PE_TEXT("Name"), (Clip ? Clip->ClipName : EString()));
			TArray<CJsonObject*> NotifyObjects;
			const TArray<FAnimNotifyEvent>& Notifies = m_DraftNotifies[ClipIndex];
			for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
			{
				const FAnimNotifyEvent& Notify = Notifies[NotifyIndex];
				CJsonObject* NotifyObject = new CJsonObject();
				NotifyObject->SetStringField(PE_TEXT("Name"), Notify.NotifyName);
				NotifyObject->SetStringField(PE_TEXT("Class"), Notify.NotifyClassName);
				NotifyObject->SetStringField(PE_TEXT("Asset"), Notify.NotifyAssetName);
				NotifyObject->SetFloatField(PE_TEXT("Time"), Notify.Time);
				NotifyObject->SetStringField(PE_TEXT("Payload"), Notify.PayloadJson);
				NotifyObject->SetBoolField(PE_TEXT("State"), Notify.bIsState);
				NotifyObject->SetFloatField(PE_TEXT("Duration"), Notify.StateDuration);
				NotifyObject->SetBoolField(PE_TEXT("DurationOverride"), Notify.bStateDurationOverride);
				NotifyObject->SetBoolField(PE_TEXT("PayloadOverride"), Notify.bPayloadOverride);
				NotifyObjects.Add(NotifyObject);
			}
			ClipObject->SetObjectArrayField(PE_TEXT("Notifies"), NotifyObjects);
			for (INT32 NotifyIndex = 0; NotifyIndex < NotifyObjects.Num(); NotifyIndex++)
			{
				delete NotifyObjects[NotifyIndex];
			}
			ClipObjects.Add(ClipObject);
		}
		Root.SetObjectArrayField(PE_TEXT("Clips"), ClipObjects);
		for (INT32 ClipIndex = 0; ClipIndex < ClipObjects.Num(); ClipIndex++)
		{
			delete ClipObjects[ClipIndex];
		}
		return Root.ToString();
	}

	BOOL32 FClipDocument::ParseNotifies(const EString& InJsonText
		, TArray<TArray<FAnimNotifyEvent>>& OutNotifies, EString& OutError)const
	{
		OutError = EString();
		OutNotifies.Empty();
		const ESkeletonAnimation* Animation = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			OutError = EString("Animation asset has no parsed resource.");
			return FALSE;
		}
		CJsonObject Root(*InJsonText);
		if (!Root.IsValid())
		{
			OutError = EString("Notify JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> ClipObjects;
		if (!(Root.GetObjectArrayField(PE_TEXT("Clips"), ClipObjects)))
		{
			OutError = EString("Notify JSON has no Clips array.");
			return FALSE;
		}
		FJsonObjectArrayOwner ClipOwner(ClipObjects);
		if (ClipObjects.Num() != Animation->GetAnimationClipNum())
		{
			OutError = EString("Notify JSON clip count does not match the animation asset.");
			return FALSE;
		}
		OutNotifies.SetNum(ClipObjects.Num());
		for (INT32 ClipIndex = 0; ClipIndex < ClipObjects.Num(); ClipIndex++)
		{
			CJsonObject* ClipObject = ClipObjects[ClipIndex];
			if (!ClipObject)
			{
				continue;
			}
			TArray<CJsonObject*> NotifyObjects;
			if (ClipObject->GetObjectArrayField(PE_TEXT("Notifies"), NotifyObjects))
			{
				FJsonObjectArrayOwner NotifyOwner(NotifyObjects);
				for (INT32 NotifyIndex = 0; NotifyIndex < NotifyObjects.Num(); NotifyIndex++)
				{
					CJsonObject* NotifyObject = NotifyObjects[NotifyIndex];
					if (!NotifyObject)
					{
						continue;
					}
					FAnimNotifyEvent Notify;
					NotifyObject->GetStringField(PE_TEXT("Name"), Notify.NotifyName);
					NotifyObject->GetStringField(PE_TEXT("Class"), Notify.NotifyClassName);
					NotifyObject->GetStringField(PE_TEXT("Asset"), Notify.NotifyAssetName);
					NotifyObject->GetFloatField(PE_TEXT("Time"), Notify.Time);
					NotifyObject->GetStringField(PE_TEXT("Payload"), Notify.PayloadJson);
					NotifyObject->GetBoolField(PE_TEXT("State"), Notify.bIsState);
					NotifyObject->GetFloatField(PE_TEXT("Duration"), Notify.StateDuration);
					NotifyObject->GetBoolField(PE_TEXT("DurationOverride"), Notify.bStateDurationOverride);
					NotifyObject->GetBoolField(PE_TEXT("PayloadOverride"), Notify.bPayloadOverride);
					Notify.ResetCompiledNotify();
					Notify.Compile();
					OutNotifies[ClipIndex].Add(Notify);
				}
			}
		}
		return TRUE;
	}

	BOOL32 FClipDocument::CommitNotifies(const TArray<TArray<FAnimNotifyEvent>>& InNotifies, EString& OutError)
	{
		OutError = EString();
		const ESkeletonAnimation* Animation = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if ((!Animation) || (InNotifies.Num() != Animation->GetAnimationClipNum()))
		{
			OutError = EString("Notify draft clip count does not match the animation asset.");
			LastError = OutError;
			return FALSE;
		}
		// Draft-only edit: the live clip arrays are untouched until Apply/Save publishes a
		// fresh asset. This keeps running instances on the previous object.
		m_DraftNotifies = InNotifies;
		DraftJson = SerializeNotifies();
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FClipDocument::CommitSourceText(const EString& InJsonText, EString& OutError)
	{
		TArray<TArray<FAnimNotifyEvent>> NewNotifies;
		if (!ParseNotifies(InJsonText, NewNotifies, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_DraftNotifies = NewNotifies;
		DraftJson = InJsonText;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FClipDocument::Apply(EString& OutError)
	{
		const ESkeletonAnimationAsset* NewAsset = nullptr;
		if (!EAnimationManager::GetManagerSingleton()->ApplySkeletonAnimationAsset(
			m_Asset, m_DraftNotifies, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		AppliedJson = DraftJson;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FClipDocument::Save(EString& OutError)
	{
		const ESkeletonAnimationAsset* NewAsset = nullptr;
		if (!EAnimationManager::GetManagerSingleton()->SaveSkeletonAnimationAssetCopyOnWrite(
			m_Asset, m_DraftNotifies, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FClipDocument::Reload(EString& OutError)
	{
		const ESkeletonAnimationAsset* NewAsset = nullptr;
		if (!EAnimationManager::GetManagerSingleton()->ReloadSkeletonAnimationAsset(
			AssetDirectory, AssetName, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		const ESkeletonAnimation* Animation = (NewAsset ? NewAsset->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			OutError = EString("Reloaded animation has no resource.");
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		m_DraftNotifies.SetNum(Animation->GetAnimationClipNum());
		for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			if (Clip)
			{
				m_DraftNotifies[ClipIndex] = Clip->Notifies;
			}
		}
		DraftJson = SerializeNotifies();
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		Commands.Clear();
		Selection.Clear();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FClipDocument::Discard(EString& OutError)
	{
		return Reload(OutError);
	}

	void FClipDocument::RefreshDerived()
	{
		TArray<TArray<FAnimNotifyEvent>> NewNotifies;
		EString Error;
		if (!ParseNotifies(DraftJson, NewNotifies, Error))
		{
			LastError = Error;
			Diagnostics.Clear();
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR, Error, EString(), -1, -1
				, EString(), EEditorSelectionType::CLIP);
			return;
		}
		m_DraftNotifies = NewNotifies;
		LastError = EString();
		RebuildDiagnostics();
	}

	void FClipDocument::RebuildDiagnostics()
	{
		Diagnostics.Clear();
		const ESkeletonAnimation* Animation = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
				, EString("Animation asset has no parsed resource."), EString(), -1, -1
				, EString(), EEditorSelectionType::CLIP);
			return;
		}
		for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			if (!Clip)
			{
				continue;
			}
			const FLOAT Duration = GetClipDurationSeconds(*Clip);
			TMap<EString, INT32> NotifyNames;
			const TArray<FAnimNotifyEvent>& Notifies = m_DraftNotifies[ClipIndex];
			for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
			{
				const FAnimNotifyEvent& Notify = Notifies[NotifyIndex];
				if ((Notify.Time < 0.0f) || (Notify.Time > Duration))
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
						, EString("Notify is outside the clip range: ") + Notify.NotifyName
						, Notify.NotifyName, ClipIndex, NotifyIndex, EString()
						, EEditorSelectionType::NOTIFY);
				}
				if (Notify.NotifyClassName.Length() <= 0u)
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
						, EString("Notify has no class: ") + Notify.NotifyName
						, Notify.NotifyName, ClipIndex, NotifyIndex, EString()
						, EEditorSelectionType::NOTIFY);
				}
				else
				{
					BOOL32 bClassIsState = FALSE;
					if (EAnimNotifyRegistry::IsStateClass(Notify.NotifyClassName, bClassIsState) == FALSE)
					{
						Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
							, EString("Notify class is not registered: ") + Notify.NotifyClassName
							, Notify.NotifyName, ClipIndex, NotifyIndex, EString()
							, EEditorSelectionType::NOTIFY);
					}
					else if ((Notify.bIsState != FALSE) != (bClassIsState != FALSE))
					{
						Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
							, EString("Notify class kind does not match its State flag: ") + Notify.NotifyClassName
							, Notify.NotifyName, ClipIndex, NotifyIndex, EString()
							, EEditorSelectionType::NOTIFY);
					}
				}
				if (NotifyNames.ContainsKey(Notify.NotifyName))
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
						, EString("Duplicate notify name in clip: ") + Notify.NotifyName
						, Notify.NotifyName, ClipIndex, NotifyIndex, EString()
						, EEditorSelectionType::NOTIFY);
				}
				else
				{
					NotifyNames.Add(Notify.NotifyName, NotifyIndex);
				}
			}
		}
	}

}

#endif
