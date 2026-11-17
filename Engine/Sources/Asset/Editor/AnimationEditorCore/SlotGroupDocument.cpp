#include "SlotGroupDocument.h"

#if ENGINE_EDITOR_ONLY

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

		BOOL32 HasGroupName(const TArray<ESkeletonSlotGroup>& InGroups, const EString& InName)
		{
			for (INT32 GroupIndex = 0; GroupIndex < InGroups.Num(); GroupIndex++)
			{
				if (InGroups[GroupIndex].Name == InName)
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL32 HasSlotName(const TArray<ESkeletonSlotGroup>& InGroups, const EString& InName)
		{
			for (INT32 GroupIndex = 0; GroupIndex < InGroups.Num(); GroupIndex++)
			{
				for (INT32 SlotIndex = 0; SlotIndex < InGroups[GroupIndex].SlotNames.Num(); SlotIndex++)
				{
					if (InGroups[GroupIndex].SlotNames[SlotIndex] == InName)
					{
						return TRUE;
					}
				}
			}
			return FALSE;
		}

		EString MakeUniqueSlotName(const TArray<ESkeletonSlotGroup>& InGroups)
		{
			INT32 TotalSlots = 0;
			for (INT32 GroupIndex = 0; GroupIndex < InGroups.Num(); GroupIndex++)
			{
				TotalSlots += InGroups[GroupIndex].SlotNames.Num();
			}
			EString Name;
			do
			{
				Name = EString("Slot") + ToString(TotalSlots++);
			} while (HasSlotName(InGroups, Name) != FALSE);
			return Name;
		}
	}

	BOOL32 FSlotGroupDocument::Initialize(const EString& InAssetPath, const EString& InAssetName
		, const ESkeletonAsset* InAsset, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("No skeleton asset was supplied.");
			return FALSE;
		}
		const ESkeleton* Skeleton = InAsset->GetStoragedResource();
		if (!Skeleton)
		{
			OutError = EString("Skeleton asset has no parsed resource.");
			return FALSE;
		}

		Kind = EAnimationDocumentKind::SLOT_GROUP;
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
		m_DraftGroups = Skeleton->GetSlotGroups();
		DraftJson = SerializeGroups(m_DraftGroups);
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		Commands.Clear();
		Selection.Clear();
		Selection.Type = EEditorSelectionType::SLOT_GROUP;
		Selection.ObjectIndex = ((m_DraftGroups.Num() > 0) ? 0 : -1);
		EString ValidationError;
		if (ValidateGroups(m_DraftGroups, ValidationError) == FALSE)
		{
			LastError = ValidationError;
		}
		RebuildDiagnostics();
		return TRUE;
	}

	const ESkeletonAsset* FSlotGroupDocument::GetAsset()const
	{
		return m_Asset;
	}

	const TArray<ESkeletonSlotGroup>& FSlotGroupDocument::GetDraftGroups()const
	{
		return m_DraftGroups;
	}

	EString FSlotGroupDocument::SerializeGroups(const TArray<ESkeletonSlotGroup>& InGroups)const
	{
		CJsonObject Root;
		TArray<CJsonObject*> GroupObjects;
		for (INT32 GroupIndex = 0; GroupIndex < InGroups.Num(); GroupIndex++)
		{
			const ESkeletonSlotGroup& Group = InGroups[GroupIndex];
			CJsonObject* GroupObject = new CJsonObject();
			GroupObject->SetStringField(PE_TEXT("Name"), Group.Name);
			GroupObject->SetStringArrayField(PE_TEXT("Slots"), Group.SlotNames);
			GroupObjects.Add(GroupObject);
		}
		Root.SetObjectArrayField(PE_TEXT("SlotGroups"), GroupObjects);
		for (INT32 GroupIndex = 0; GroupIndex < GroupObjects.Num(); GroupIndex++)
		{
			delete GroupObjects[GroupIndex];
		}
		return Root.ToString();
	}

	BOOL32 FSlotGroupDocument::ParseGroups(const EString& InJsonText
		, TArray<ESkeletonSlotGroup>& OutGroups, EString& OutError)const
	{
		OutError = EString();
		OutGroups.Empty();
		CJsonObject Root(InJsonText);
		if (!Root.IsValid())
		{
			OutError = EString("Slot group JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> GroupObjects;
		if (!Root.GetObjectArrayField(PE_TEXT("SlotGroups"), GroupObjects))
		{
			OutError = EString("Slot group JSON has no SlotGroups array.");
			return FALSE;
		}
		FJsonObjectArrayOwner Owner(GroupObjects);
		for (INT32 GroupIndex = 0; GroupIndex < GroupObjects.Num(); GroupIndex++)
		{
			CJsonObject* GroupObject = GroupObjects[GroupIndex];
			if (!GroupObject)
			{
				continue;
			}
			ESkeletonSlotGroup Group;
			GroupObject->GetStringField(PE_TEXT("Name"), Group.Name);
			GroupObject->GetStringArrayField(PE_TEXT("Slots"), Group.SlotNames);
			OutGroups.Add(Group);
		}
		return TRUE;
	}

	BOOL32 FSlotGroupDocument::ValidateGroups(const TArray<ESkeletonSlotGroup>& InGroups
		, EString& OutError)const
	{
		OutError = EString();
		if (InGroups.Num() <= 0)
		{
			OutError = EString("A skeleton needs at least one slot group.");
			return FALSE;
		}
		if (InGroups.Num() > 65535)
		{
			OutError = EString("Too many slot groups (maximum 65535).");
			return FALSE;
		}
		TMap<EString, INT32> GroupNames;
		TMap<EString, INT32> SlotNames;
		for (INT32 GroupIndex = 0; GroupIndex < InGroups.Num(); GroupIndex++)
		{
			const ESkeletonSlotGroup& Group = InGroups[GroupIndex];
			if (Group.Name.Length() <= 0u)
			{
				OutError = EString("Slot group ") + ToString(GroupIndex) + EString(" has an empty name.");
				return FALSE;
			}
			if (Group.Name.Length() >= EEngineSettings::ENGINE_ANIMATION_SLOT_GROUP_NAME_LENGTH_MAX)
			{
				OutError = EString("Slot group name is too long: ") + Group.Name;
				return FALSE;
			}
			if (GroupNames.ContainsKey(Group.Name))
			{
				OutError = EString("Duplicate slot group name: ") + Group.Name;
				return FALSE;
			}
			GroupNames.Add(Group.Name, GroupIndex);
			if (Group.SlotNames.Num() <= 0)
			{
				OutError = EString("Slot group '") + Group.Name + EString("' needs at least one slot.");
				return FALSE;
			}
			if (Group.SlotNames.Num() > 65535)
			{
				OutError = EString("Slot group '") + Group.Name + EString("' has too many slots.");
				return FALSE;
			}
			for (INT32 SlotIndex = 0; SlotIndex < Group.SlotNames.Num(); SlotIndex++)
			{
				const EString& SlotName = Group.SlotNames[SlotIndex];
				if (SlotName.Length() <= 0u)
				{
					OutError = EString("Slot group '") + Group.Name + EString("' has an empty slot name.");
					return FALSE;
				}
				if (SlotName.Length() >= EEngineSettings::ENGINE_ANIMATION_SLOT_NAME_LENGTH_MAX)
				{
					OutError = EString("Slot name is too long: ") + SlotName;
					return FALSE;
				}
				if (SlotNames.ContainsKey(SlotName))
				{
					OutError = EString("Slot name is used by more than one group: ") + SlotName;
					return FALSE;
				}
				SlotNames.Add(SlotName, SlotIndex);
			}
		}
		return TRUE;
	}

	BOOL32 FSlotGroupDocument::CommitGroups(const TArray<ESkeletonSlotGroup>& InGroups
		, EString& OutError)
	{
		if (!ValidateGroups(InGroups, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_DraftGroups = InGroups;
		DraftJson = SerializeGroups(m_DraftGroups);
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FSlotGroupDocument::CommitJsonText(const EString& InJsonText, EString& OutError)
	{
		TArray<ESkeletonSlotGroup> NewGroups;
		if (!ParseGroups(InJsonText, NewGroups, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::CommitSourceText(const EString& InJsonText, EString& OutError)
	{
		return CommitJsonText(InJsonText, OutError);
	}

	BOOL32 FSlotGroupDocument::AddGroup(EString& OutError)
	{
		TArray<ESkeletonSlotGroup> NewGroups = m_DraftGroups;
		INT32 Suffix = NewGroups.Num();
		EString NewName;
		do
		{
			NewName = EString("Group") + ToString(Suffix++);
		} while (HasGroupName(NewGroups, NewName) != FALSE);
		ESkeletonSlotGroup& NewGroup = NewGroups.AddDefaultGetRef();
		NewGroup.Name = NewName;
		NewGroup.SlotNames.Add(MakeUniqueSlotName(NewGroups));
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::DeleteGroup(INT32 InGroupIndex, EString& OutError)
	{
		if ((InGroupIndex < 0) || (InGroupIndex >= m_DraftGroups.Num()))
		{
			OutError = EString("Slot group index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		if (m_DraftGroups.Num() <= 1)
		{
			OutError = EString("A skeleton needs at least one slot group.");
			LastError = OutError;
			return FALSE;
		}
		TArray<ESkeletonSlotGroup> NewGroups = m_DraftGroups;
		NewGroups.RemoveAt(InGroupIndex);
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::MoveGroup(INT32 InGroupIndex, INT32 InDelta, EString& OutError)
	{
		if ((InGroupIndex < 0) || (InGroupIndex >= m_DraftGroups.Num()))
		{
			OutError = EString("Slot group index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		const INT32 TargetIndex = InGroupIndex + InDelta;
		if ((TargetIndex < 0) || (TargetIndex >= m_DraftGroups.Num()) || (TargetIndex == InGroupIndex))
		{
			OutError = EString("Slot group cannot move in that direction.");
			LastError = OutError;
			return FALSE;
		}
		TArray<ESkeletonSlotGroup> NewGroups = m_DraftGroups;
		const ESkeletonSlotGroup Temp = NewGroups[TargetIndex];
		NewGroups[TargetIndex] = NewGroups[InGroupIndex];
		NewGroups[InGroupIndex] = Temp;
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::RenameGroup(INT32 InGroupIndex, const EString& InName, EString& OutError)
	{
		if ((InGroupIndex < 0) || (InGroupIndex >= m_DraftGroups.Num()))
		{
			OutError = EString("Slot group index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		TArray<ESkeletonSlotGroup> NewGroups = m_DraftGroups;
		NewGroups[InGroupIndex].Name = InName;
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::AddSlot(INT32 InGroupIndex, EString& OutError)
	{
		if ((InGroupIndex < 0) || (InGroupIndex >= m_DraftGroups.Num()))
		{
			OutError = EString("Slot group index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		TArray<ESkeletonSlotGroup> NewGroups = m_DraftGroups;
		NewGroups[InGroupIndex].SlotNames.Add(MakeUniqueSlotName(NewGroups));
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::DeleteSlot(INT32 InGroupIndex, INT32 InSlotIndex, EString& OutError)
	{
		if ((InGroupIndex < 0) || (InGroupIndex >= m_DraftGroups.Num()))
		{
			OutError = EString("Slot group index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		if ((InSlotIndex < 0) || (InSlotIndex >= m_DraftGroups[InGroupIndex].SlotNames.Num()))
		{
			OutError = EString("Slot index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		if (m_DraftGroups[InGroupIndex].SlotNames.Num() <= 1)
		{
			OutError = EString("A slot group needs at least one slot.");
			LastError = OutError;
			return FALSE;
		}
		TArray<ESkeletonSlotGroup> NewGroups = m_DraftGroups;
		NewGroups[InGroupIndex].SlotNames.RemoveAt(InSlotIndex);
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::RenameSlot(INT32 InGroupIndex, INT32 InSlotIndex
		, const EString& InName, EString& OutError)
	{
		if ((InGroupIndex < 0) || (InGroupIndex >= m_DraftGroups.Num()))
		{
			OutError = EString("Slot group index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		if ((InSlotIndex < 0) || (InSlotIndex >= m_DraftGroups[InGroupIndex].SlotNames.Num()))
		{
			OutError = EString("Slot index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		TArray<ESkeletonSlotGroup> NewGroups = m_DraftGroups;
		NewGroups[InGroupIndex].SlotNames[InSlotIndex] = InName;
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::ResetDefaults(EString& OutError)
	{
		TArray<ESkeletonSlotGroup> NewGroups;
		const CHAR* DefaultGroupNames[] = { "Default", "UpperBody", "FullBody" };
		for (const CHAR* GroupName : DefaultGroupNames)
		{
			ESkeletonSlotGroup& Group = NewGroups.AddDefaultGetRef();
			Group.Name = EString(GroupName);
			Group.SlotNames.Add(EString(GroupName));
		}
		return CommitGroups(NewGroups, OutError);
	}

	BOOL32 FSlotGroupDocument::CopyGroupsToLive(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No skeleton asset is open.");
			return FALSE;
		}
		ESkeleton* Skeleton = const_cast<ESkeleton*>(m_Asset->GetStoragedResource());
		if (!Skeleton)
		{
			OutError = EString("Skeleton asset has no resource.");
			return FALSE;
		}
		Skeleton->GetSlotGroups() = m_DraftGroups;
		return TRUE;
	}

	BOOL32 FSlotGroupDocument::Apply(EString& OutError)
	{
		if (!ValidateGroups(m_DraftGroups, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		if (!CopyGroupsToLive(OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		AppliedJson = DraftJson;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FSlotGroupDocument::Save(EString& OutError)
	{
		if (!ValidateGroups(m_DraftGroups, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		if (!CopyGroupsToLive(OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		if (!ESkeletonAssetManager::GetManagerSingleton()->SaveSkeletonAsset(
			AssetDirectory, AssetName, m_Asset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FSlotGroupDocument::Reload(EString& OutError)
	{
		const ESkeletonAsset* NewAsset = nullptr;
		if (!ESkeletonAssetManager::GetManagerSingleton()->ReloadSkeletonAsset(
			AssetDirectory, AssetName, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		const ESkeleton* Skeleton = (NewAsset ? NewAsset->GetStoragedResource() : nullptr);
		if (!Skeleton)
		{
			OutError = EString("Reloaded skeleton has no resource.");
			LastError = OutError;
			return FALSE;
		}
		m_Asset = NewAsset;
		PublishedAsset = NewAsset;
		m_DraftGroups = Skeleton->GetSlotGroups();
		DraftJson = SerializeGroups(m_DraftGroups);
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		EString ValidationError;
		LastError = (ValidateGroups(m_DraftGroups, ValidationError) != FALSE)
			? EString() : ValidationError;
		Commands.Clear();
		Selection.Clear();
		Selection.Type = EEditorSelectionType::SLOT_GROUP;
		Selection.ObjectIndex = ((m_DraftGroups.Num() > 0) ? 0 : -1);
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FSlotGroupDocument::Discard(EString& OutError)
	{
		return Reload(OutError);
	}

	void FSlotGroupDocument::RefreshDerived()
	{
		EString Error;
		if (!CommitJsonText(DraftJson, Error))
		{
			LastError = Error;
			Diagnostics.Clear();
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR, Error);
		}
	}

	void FSlotGroupDocument::RebuildDiagnostics()
	{
		Diagnostics.Clear();
		if (!m_Asset)
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
				, EString("No skeleton asset is open."));
			return;
		}
		EString ValidationError;
		if (!ValidateGroups(m_DraftGroups, ValidationError))
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR, ValidationError);
			return;
		}
		INT32 SlotCount = 0;
		for (INT32 GroupIndex = 0; GroupIndex < m_DraftGroups.Num(); GroupIndex++)
		{
			SlotCount += m_DraftGroups[GroupIndex].SlotNames.Num();
		}
		Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_INFO
			, EString("Slot groups: ") + ToString(m_DraftGroups.Num())
			+ EString("  Slots: ") + ToString(SlotCount));
	}

}

#endif
