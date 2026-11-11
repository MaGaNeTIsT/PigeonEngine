#include "MontageDocument.h"

#if _EDITOR_ONLY

#include <AnimationMontageAsset/AnimationMontageAsset.h>
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

		const CHAR* GetLoopModeText(const EAnimationLoopMode InLoopMode)
		{
			switch (InLoopMode)
			{
			case ANIMATION_LOOP_ONCE:
				return "Once";
			case ANIMATION_LOOP_PINGPONG:
				return "PingPong";
			case ANIMATION_LOOP_CLAMP:
				return "Clamp";
			case ANIMATION_LOOP_REPEAT:
			default:
				return "Repeat";
			}
		}

		EAnimationLoopMode ParseLoopModeText(const EString& InText)
		{
			if (InText == EString("Once"))
			{
				return ANIMATION_LOOP_ONCE;
			}
			if (InText == EString("PingPong"))
			{
				return ANIMATION_LOOP_PINGPONG;
			}
			if (InText == EString("Clamp"))
			{
				return ANIMATION_LOOP_CLAMP;
			}
			return ANIMATION_LOOP_REPEAT;
		}

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

		EString GetJsonStringField(CJsonObject& InObject, const EString& InFieldName)
		{
			EString Result;
			InObject.GetStringField(InFieldName, Result);
			return Result;
		}

		BOOL32 JsonSectionNameExists(const TArray<CJsonObject*>& InSections, const EString& InName)
		{
			for (INT32 SectionIndex = 0; SectionIndex < InSections.Num(); SectionIndex++)
			{
				if ((InSections[SectionIndex])
					&& (GetJsonStringField(*InSections[SectionIndex], PE_TEXT("Name")) == InName))
				{
					return TRUE;
				}
			}
			return FALSE;
		}
	}

	FMontageDocument::~FMontageDocument()
	{
		ReleaseDraftMontage();
	}

	BOOL32 FMontageDocument::Initialize(const EString& InAssetPath, const EString& InAssetName
		, const EAnimMontageAsset* InAsset, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("No montage asset was supplied.");
			return FALSE;
		}
		const EAnimMontage* Montage = InAsset->GetStoragedResource();
		if (!Montage)
		{
			OutError = EString("Montage asset has no parsed resource.");
			return FALSE;
		}

		Kind = EAnimationDocumentKind::MONTAGE;
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
		DraftJson = Montage->SourceJson;
		if (DraftJson.Length() <= 0u)
		{
			OutError = EString("Montage asset has no source JSON.");
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
		Selection.Type = EEditorSelectionType::SECTION;
		Selection.ObjectIndex = ((m_DraftMontage && (m_DraftMontage->Sections.Num() > 0)) ? 0 : -1);
		RebuildDiagnostics();
		return TRUE;
	}

	const EAnimMontage* FMontageDocument::GetDraftMontage()const
	{
		return m_DraftMontage;
	}

	const EAnimMontageAsset* FMontageDocument::GetMontageAsset()const
	{
		return m_Asset;
	}

	const ESkeletonAnimationAsset* FMontageDocument::GetAnimationAsset()const
	{
		return m_AnimationAsset;
	}

	const ESkeletonAnimationClip* FMontageDocument::GetSectionClip(INT32 InSectionIndex)const
	{
		if ((!m_DraftMontage) || (InSectionIndex < 0) || (InSectionIndex >= m_DraftMontage->Sections.Num()))
		{
			return nullptr;
		}
		const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			return nullptr;
		}
		return Animation->GetAnimationClip(m_DraftMontage->Sections[InSectionIndex].ClipName);
	}

	INT32 FMontageDocument::GetSectionClipIndex(INT32 InSectionIndex)const
	{
		if ((!m_DraftMontage) || (InSectionIndex < 0) || (InSectionIndex >= m_DraftMontage->Sections.Num()))
		{
			return -1;
		}
		const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			return -1;
		}
		const EString& ClipName = m_DraftMontage->Sections[InSectionIndex].ClipName;
		for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			if (Clip && (Clip->ClipName == ClipName))
			{
				return ClipIndex;
			}
		}
		return -1;
	}

	FLOAT FMontageDocument::GetSectionClipDuration(INT32 InSectionIndex)const
	{
		const ESkeletonAnimationClip* Clip = GetSectionClip(InSectionIndex);
		return (Clip ? GetClipDurationSeconds(*Clip) : 0.0f);
	}

	BOOL32 FMontageDocument::CommitSourceText(const EString& InJsonText, EString& OutError)
	{
		return CommitJsonText(InJsonText, OutError);
	}

	BOOL32 FMontageDocument::CommitJsonText(const EString& InJsonText, EString& OutError)
	{
		OutError = EString();
		EAnimMontage* NewMontage = EAnimMontageManager::GetManagerSingleton()
			->BuildTransientMontage(InJsonText, OutError);
		if (!NewMontage)
		{
			LastError = OutError;
			return FALSE;
		}
		if (NewMontage->Name.Length() <= 0u)
		{
			NewMontage->Name = AssetName;
		}

		ReleaseDraftMontage();
		m_DraftMontage = NewMontage;
		DraftJson = InJsonText;
		LastError = EString();
		SyncAnimationAsset();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FMontageDocument::MutateJson(const TFunction<void(CJsonObject&)>& InMutator, EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("Montage JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		InMutator(Root);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FMontageDocument::UpdateSectionObject(INT32 InSectionIndex
		, const TFunction<void(CJsonObject&)>& InMutator, EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("Montage JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Sections;
		if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), Sections)))
		{
			OutError = EString("Montage JSON has no Sections array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner Owner(Sections);
		if ((InSectionIndex < 0) || (InSectionIndex >= Sections.Num()) || (!Sections[InSectionIndex]))
		{
			OutError = EString("Montage section index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		InMutator(*Sections[InSectionIndex]);
		Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FMontageDocument::AddSection(const EString& InClipName, const EString& InSlotName
		, EString& OutError)
	{
		OutError = EString();
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("Montage JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Sections;
		if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), Sections)))
		{
			OutError = EString("Montage JSON has no Sections array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner Owner(Sections);

		EString NewName;
		INT32 Suffix = Sections.Num();
		do
		{
			NewName = EString("Section") + ToString(Suffix++);
		} while (JsonSectionNameExists(Sections, NewName) != FALSE);

		EString DefaultClip = InClipName;
		EString DefaultSlot = InSlotName;
		for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
		{
			if (!Sections[SectionIndex])
			{
				continue;
			}
			if (DefaultClip.Length() <= 0u)
			{
				DefaultClip = GetJsonStringField(*Sections[SectionIndex], PE_TEXT("Clip"));
			}
			if (DefaultSlot.Length() <= 0u)
			{
				DefaultSlot = GetJsonStringField(*Sections[SectionIndex], PE_TEXT("Slot"));
			}
		}
		if (DefaultClip.Length() <= 0u)
		{
			const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
			if (Animation && (Animation->GetAnimationClipNum() > 0) && Animation->GetAnimationClip(0))
			{
				DefaultClip = Animation->GetAnimationClip(0)->ClipName;
			}
		}
		if (DefaultSlot.Length() <= 0u)
		{
			DefaultSlot = EString("DefaultSlot");
		}

		CJsonObject* NewSection = new CJsonObject();
		NewSection->SetStringField(PE_TEXT("Name"), NewName);
		NewSection->SetStringField(PE_TEXT("Clip"), DefaultClip);
		NewSection->SetStringField(PE_TEXT("Slot"), DefaultSlot);
		NewSection->SetStringField(PE_TEXT("Next"), EString());
		NewSection->SetStringField(PE_TEXT("Loop"), EString("Once"));
		NewSection->SetFloatField(PE_TEXT("Rate"), 1.0f);
		NewSection->SetFloatField(PE_TEXT("BlendIn"), 0.1f);
		NewSection->SetFloatField(PE_TEXT("BlendOut"), 0.2f);
		NewSection->SetBoolField(PE_TEXT("Additive"), FALSE);
		Sections.Add(NewSection);
		Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		Selection.Type = EEditorSelectionType::SECTION;
		Selection.ObjectIndex = Sections.Num() - 1;
		return TRUE;
	}

	BOOL32 FMontageDocument::DeleteSection(INT32 InSectionIndex, EString& OutError)
	{
		OutError = EString();
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("Montage JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Sections;
		if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), Sections)))
		{
			OutError = EString("Montage JSON has no Sections array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner Owner(Sections);
		if ((InSectionIndex < 0) || (InSectionIndex >= Sections.Num()) || (!Sections[InSectionIndex]))
		{
			OutError = EString("Montage section index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		if (Sections.Num() <= 1)
		{
			OutError = EString("A montage needs at least one section.");
			LastError = OutError;
			return FALSE;
		}

		const EString RemovedName = GetJsonStringField(*Sections[InSectionIndex], PE_TEXT("Name"));
		CJsonObject* RemovedSection = Sections[InSectionIndex];
		Sections.RemoveAt(InSectionIndex);
		delete RemovedSection;
		for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
		{
			if (!Sections[SectionIndex])
			{
				continue;
			}
			if (GetJsonStringField(*Sections[SectionIndex], PE_TEXT("Next")) == RemovedName)
			{
				Sections[SectionIndex]->SetStringField(PE_TEXT("Next"), EString());
			}
		}
		Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		Selection.Type = EEditorSelectionType::SECTION;
		Selection.ObjectIndex = EMath::Clamp(InSectionIndex, 0, Sections.Num() - 1);
		return TRUE;
	}

	BOOL32 FMontageDocument::UpdateSection(INT32 InSectionIndex
		, const EAnimMontageSection& InSection, EString& OutError)
	{
		OutError = EString();
		if (InSection.Name.Length() <= 0u)
		{
			OutError = EString("A montage section needs a name.");
			LastError = OutError;
			return FALSE;
		}
		if (InSection.ClipName.Length() <= 0u)
		{
			OutError = EString("A montage section needs a clip.");
			LastError = OutError;
			return FALSE;
		}
		if (InSection.SlotName.Length() <= 0u)
		{
			OutError = EString("A montage section needs a slot.");
			LastError = OutError;
			return FALSE;
		}

		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("Montage JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Sections;
		if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), Sections)))
		{
			OutError = EString("Montage JSON has no Sections array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner Owner(Sections);
		if ((InSectionIndex < 0) || (InSectionIndex >= Sections.Num()) || (!Sections[InSectionIndex]))
		{
			OutError = EString("Montage section index is out of range.");
			LastError = OutError;
			return FALSE;
		}

		const EString OldName = GetJsonStringField(*Sections[InSectionIndex], PE_TEXT("Name"));
		for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
		{
			if ((SectionIndex != InSectionIndex) && Sections[SectionIndex]
				&& (GetJsonStringField(*Sections[SectionIndex], PE_TEXT("Name")) == InSection.Name))
			{
				OutError = EString("A montage section named '") + InSection.Name + EString("' already exists.");
				LastError = OutError;
				return FALSE;
			}
		}

		CJsonObject* SectionObject = Sections[InSectionIndex];
		SectionObject->SetStringField(PE_TEXT("Name"), InSection.Name);
		SectionObject->SetStringField(PE_TEXT("Clip"), InSection.ClipName);
		SectionObject->SetStringField(PE_TEXT("Slot"), InSection.SlotName);
		SectionObject->SetStringField(PE_TEXT("Next"), InSection.NextSectionName);
		SectionObject->SetStringField(PE_TEXT("Loop"), EString(GetLoopModeText(InSection.LoopMode)));
		SectionObject->SetFloatField(PE_TEXT("Rate"), InSection.PlayRate);
		SectionObject->SetFloatField(PE_TEXT("BlendIn"), InSection.BlendInSeconds);
		SectionObject->SetFloatField(PE_TEXT("BlendOut"), InSection.BlendOutSeconds);
		SectionObject->SetBoolField(PE_TEXT("Additive"), InSection.bAdditive);

		if ((OldName.Length() > 0u) && (OldName != InSection.Name))
		{
			for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
			{
				if (!Sections[SectionIndex])
				{
					continue;
				}
				if (GetJsonStringField(*Sections[SectionIndex], PE_TEXT("Next")) == OldName)
				{
					Sections[SectionIndex]->SetStringField(PE_TEXT("Next"), InSection.Name);
				}
			}
		}
		Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FMontageDocument::SetSectionNotifies(INT32 InSectionIndex
		, const TArray<FAnimNotifyEvent>& InNotifies, EString& OutError)
	{
		return UpdateSectionObject(InSectionIndex, [&InNotifies](CJsonObject& InSection)
			{
				TArray<CJsonObject*> NotifyObjects;
				for (INT32 NotifyIndex = 0; NotifyIndex < InNotifies.Num(); NotifyIndex++)
				{
					const FAnimNotifyEvent& Notify = InNotifies[NotifyIndex];
					CJsonObject* NotifyObject = new CJsonObject();
					NotifyObject->SetStringField(PE_TEXT("Name"), Notify.NotifyName);
					NotifyObject->SetStringField(PE_TEXT("Class"), Notify.NotifyClassName);
					NotifyObject->SetStringField(PE_TEXT("NotifyAsset"), Notify.NotifyAssetName);
					NotifyObject->SetFloatField(PE_TEXT("Time"), Notify.Time);
					NotifyObject->SetStringField(PE_TEXT("Payload"), Notify.PayloadJson);
					NotifyObject->SetBoolField(PE_TEXT("State"), Notify.bIsState);
					NotifyObject->SetFloatField(PE_TEXT("Duration"), Notify.StateDuration);
					NotifyObjects.Add(NotifyObject);
				}
				InSection.SetObjectArrayField(PE_TEXT("Notifies"), NotifyObjects);
				for (INT32 NotifyIndex = 0; NotifyIndex < NotifyObjects.Num(); NotifyIndex++)
				{
					delete NotifyObjects[NotifyIndex];
				}
			}, OutError);
	}

	BOOL32 FMontageDocument::SetSectionBoneMask(INT32 InSectionIndex
		, const ESkeletonBoneMask& InMask, EString& OutError)
	{
		return UpdateSectionObject(InSectionIndex, [&InMask](CJsonObject& InSection)
			{
				CJsonObject MaskObject;
				InSection.GetObjectField(PE_TEXT("BoneMask"), MaskObject);
				MaskObject.SetStringField(PE_TEXT("Name"), InMask.Name);
				MaskObject.SetStringArrayField(PE_TEXT("Bones"), InMask.BoneNames);
				MaskObject.SetFloatField(PE_TEXT("Weight"), InMask.Weight);
				InSection.SetObjectField(PE_TEXT("BoneMask"), &MaskObject);
			}, OutError);
	}

	BOOL32 FMontageDocument::Apply(EString& OutError)
	{
		const EAnimMontageAsset* NewAsset = nullptr;
		if (!EAnimMontageManager::GetManagerSingleton()->ApplyMontageAsset(
			m_Asset, DraftJson, NewAsset, OutError))
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

	BOOL32 FMontageDocument::Save(EString& OutError)
	{
		const EAnimMontageAsset* NewAsset = nullptr;
		if (!EAnimMontageManager::GetManagerSingleton()->SaveMontageAssetCopyOnWrite(
			m_Asset, DraftJson, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		AppliedJson = DraftJson;
		BaselineJson = DraftJson;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FMontageDocument::Reload(EString& OutError)
	{
		const EAnimMontageAsset* NewAsset = nullptr;
		if (!EAnimMontageManager::GetManagerSingleton()->ReloadMontageAsset(
			AssetDirectory, AssetName, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		const EAnimMontage* Montage = (NewAsset ? NewAsset->GetStoragedResource() : nullptr);
		if (!Montage)
		{
			OutError = EString("Reloaded montage has no resource.");
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		DraftJson = Montage->SourceJson;
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		if (!CommitJsonText(DraftJson, OutError))
		{
			return FALSE;
		}
		Commands.Clear();
		Selection.Clear();
		Selection.Type = EEditorSelectionType::SECTION;
		Selection.ObjectIndex = ((m_DraftMontage && (m_DraftMontage->Sections.Num() > 0)) ? 0 : -1);
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FMontageDocument::Discard(EString& OutError)
	{
		return Reload(OutError);
	}

	void FMontageDocument::RefreshDerived()
	{
		EString Error;
		if (!CommitJsonText(DraftJson, Error))
		{
			LastError = Error;
			Diagnostics.Clear();
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR, Error, EString(), -1, -1
				, EString(), EEditorSelectionType::SECTION);
		}
	}

	void FMontageDocument::ReleaseDraftMontage()
	{
		if (m_DraftMontage)
		{
			delete m_DraftMontage;
			m_DraftMontage = nullptr;
		}
	}

	void FMontageDocument::SyncAnimationAsset()
	{
		m_AnimationAsset = nullptr;
		const EString AnimationName = (m_DraftMontage ? m_DraftMontage->AnimationAssetName : EString());
		if (AnimationName.Length() <= 0u)
		{
			return;
		}
		const ESkeletonAnimationAsset* LoadedAnimation = nullptr;
		if (EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
			EBaseSettings::ENGINE_ANIMATION_PATH, AnimationName, LoadedAnimation))
		{
			m_AnimationAsset = LoadedAnimation;
		}
	}

	void FMontageDocument::RebuildDiagnostics()
	{
		Diagnostics.Clear();
		if (!m_DraftMontage)
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
				, EString("Montage draft is not available."));
			return;
		}

		const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
		if ((m_DraftMontage->AnimationAssetName.Length() > 0u) && (!Animation))
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
				, EString("Montage animation asset is not loaded: ") + m_DraftMontage->AnimationAssetName
				, EString(), -1, -1, EString(), EEditorSelectionType::NONE);
		}

		for (INT32 SectionIndex = 0; SectionIndex < m_DraftMontage->Sections.Num(); SectionIndex++)
		{
			const EAnimMontageSection& Section = m_DraftMontage->Sections[SectionIndex];
			if (Section.ClipName.Length() <= 0u)
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
					, EString("Section has no clip: ") + Section.Name
					, Section.Name, SectionIndex, -1, EString(), EEditorSelectionType::SECTION);
			}
			else if (Animation && (!Animation->GetAnimationClip(Section.ClipName)))
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
					, EString("Section clip is not in the animation asset: ") + Section.ClipName
					, Section.Name, SectionIndex, -1, EString(), EEditorSelectionType::SECTION);
			}
			if (Section.SlotName.Length() <= 0u)
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
					, EString("Section has no slot: ") + Section.Name
					, Section.Name, SectionIndex, -1, EString(), EEditorSelectionType::SECTION);
			}
			if ((Section.NextSectionName.Length() > 0u)
				&& (m_DraftMontage->FindSection(Section.NextSectionName) < 0))
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
					, EString("Section next target does not exist: ") + Section.NextSectionName
					, Section.Name, SectionIndex, -1, EString(), EEditorSelectionType::SECTION);
			}

			const ESkeletonAnimationClip* Clip = (Animation
				? Animation->GetAnimationClip(Section.ClipName) : nullptr);
			const FLOAT ClipDuration = (Clip ? GetClipDurationSeconds(*Clip) : 0.0f);
			TMap<EString, INT32> NotifyNames;
			for (INT32 NotifyIndex = 0; NotifyIndex < Section.Notifies.Num(); NotifyIndex++)
			{
				const FAnimNotifyEvent& Notify = Section.Notifies[NotifyIndex];
				if ((Clip) && ((Notify.Time < 0.0f) || (Notify.Time > ClipDuration)))
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
						, EString("Notify is outside the section clip range: ") + Notify.NotifyName
						, Notify.NotifyName, SectionIndex, NotifyIndex, EString()
						, EEditorSelectionType::NOTIFY);
				}
				if (Notify.NotifyClassName.Length() <= 0u)
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
						, EString("Notify has no class: ") + Notify.NotifyName
						, Notify.NotifyName, SectionIndex, NotifyIndex, EString()
						, EEditorSelectionType::NOTIFY);
				}
				else
				{
					BOOL32 bClassIsState = FALSE;
					if (EAnimNotifyRegistry::IsStateClass(Notify.NotifyClassName, bClassIsState) == FALSE)
					{
						Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
							, EString("Notify class is not registered: ") + Notify.NotifyClassName
							, Notify.NotifyName, SectionIndex, NotifyIndex, EString()
							, EEditorSelectionType::NOTIFY);
					}
					else if ((Notify.bIsState != FALSE) != (bClassIsState != FALSE))
					{
						Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
							, EString("Notify class kind does not match its State flag: ") + Notify.NotifyClassName
							, Notify.NotifyName, SectionIndex, NotifyIndex, EString()
							, EEditorSelectionType::NOTIFY);
					}
				}
				if (NotifyNames.ContainsKey(Notify.NotifyName))
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
						, EString("Duplicate notify name in section: ") + Notify.NotifyName
						, Notify.NotifyName, SectionIndex, NotifyIndex, EString()
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
