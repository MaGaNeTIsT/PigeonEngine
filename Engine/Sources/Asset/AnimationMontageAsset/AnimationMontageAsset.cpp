#include "AnimationMontageAsset.h"

#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <Editor/EditorLogManager.h>
#include <IO/FileHelper.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{

	namespace
	{
		constexpr UINT32 MaxMontageJsonLength = 16u * 1024u * 1024u;

		EAnimationLoopMode ParseLoopMode(const EString& InText)
		{
			if ((InText == EString("Once")) || (InText == EString("ONCE")) || (InText == EString("once")))
			{
				return ANIMATION_LOOP_ONCE;
			}
			if ((InText == EString("PingPong")) || (InText == EString("PINGPONG")) || (InText == EString("pingpong")))
			{
				return ANIMATION_LOOP_PINGPONG;
			}
			if ((InText == EString("Clamp")) || (InText == EString("CLAMP")) || (InText == EString("clamp")))
			{
				return ANIMATION_LOOP_CLAMP;
			}
			return ANIMATION_LOOP_REPEAT;
		}

		struct FJsonObjectArrayGuard
		{
			explicit FJsonObjectArrayGuard(TArray<CJsonObject*>& InObjects) : Objects(InObjects) {}
			~FJsonObjectArrayGuard()
			{
				for (INT32 Index = 0; Index < Objects.Num(); Index++)
				{
					delete Objects[Index];
				}
				Objects.Empty();
			}
			TArray<CJsonObject*>& Objects;
		};

		void CopyMontageData(EAnimMontage& OutMontage, const EAnimMontage& InMontage)
		{
			OutMontage.Name = InMontage.Name;
			OutMontage.SkeletonName = InMontage.SkeletonName;
			OutMontage.AnimationAssetName = InMontage.AnimationAssetName;
			OutMontage.SourceJson = InMontage.SourceJson;
			OutMontage.SlotGroups = InMontage.SlotGroups;
			OutMontage.Sections = InMontage.Sections;
			OutMontage.SectionMapping = InMontage.SectionMapping;
			OutMontage.SlotGroupMapping = InMontage.SlotGroupMapping;
		}
	}

	EAnimMontageAsset::EAnimMontageAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<EAnimMontage>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}

	EAnimMontageAsset::~EAnimMontageAsset()
	{
	}

	BOOL32 EAnimMontageAsset::InitResource()
	{
		if (IsInitialized())
		{
			return TRUE;
		}
		bIsInitialized = TRUE;
		return TRUE;
	}

	EAnimMontageManager::EAnimMontageManager()
	{
#if _EDITOR_ONLY
		DebugName = "AnimationMontageManager";
#endif
	}

	EAnimMontageManager::~EAnimMontageManager()
	{
	}

	void EAnimMontageManager::Initialize()
	{
	}

	void EAnimMontageManager::ShutDown()
	{
		ReleaseAllAssets();
	}

	void EAnimMontageManager::Uninit()
	{
		ReleaseAllAssets();
	}

	void EAnimMontageManager::ReleaseAllAssets()
	{
		ClearMontages();
	}

	void EAnimMontageManager::ClearMontages()
	{
		MontageAssetManager.Clear();
	}

#if _EDITOR_ONLY
	BOOL32 EAnimMontageManager::ImportMontage(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath)
	{
		EString JsonText;
		if (!(EFileHelper::ReadFileAsString(InJsonFullPathName, JsonText)))
		{
			PE_LOG_WARN(EString("Montage importer could not read '") + InJsonFullPathName + EString("'."));
			return FALSE;
		}

		EAnimMontage TempMontage;
		EString Error;
		if (!BuildMontage(TempMontage, JsonText, Error))
		{
			PE_LOG_WARN(EString("Montage importer rejected '") + InJsonFullPathName + EString("': ") + Error);
			return FALSE;
		}
		if (TempMontage.Name.Length() <= 0u)
		{
			TempMontage.Name = InAssetName;
		}
		return SaveMontageResource(InSavePath, InAssetName, JsonText);
	}
#endif

	BOOL32 EAnimMontageManager::LoadMontageAsset(const EString& InLoadPath, const EString& InLoadName
		, const EAnimMontageAsset*& OutAsset)
	{
		OutAsset = nullptr;
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (EAnimMontageAsset* CachedAsset = MontageAssetManager.Find(FullPathName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}

		EString Error;
		EAnimMontage* LoadedResource = LoadMontageResource(InLoadPath, InLoadName, Error);
		if ((!LoadedResource) || (!LoadedResource->IsResourceValid()))
		{
			delete LoadedResource;
			return FALSE;
		}

		EAnimMontageAsset* NewAsset = new EAnimMontageAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(NewAsset->StorageResourceInternal([LoadedResource]() -> EAnimMontage*
			{
				return LoadedResource;
			})))
		{
			delete NewAsset;
			return FALSE;
		}
		if (!(NewAsset->InitResource()))
		{
			delete NewAsset;
			return FALSE;
		}
		if (MontageAssetManager.Add(FullPathName, NewAsset, TRUE) == 0u)
		{
			delete NewAsset;
			return FALSE;
		}
		OutAsset = NewAsset;
		return TRUE;
	}

	BOOL32 EAnimMontageManager::RequireLoadMontageAsset(const EString& InLoadPath, const EString& InLoadName
		, const EAnimMontageAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete)
	{
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (EAnimMontageAsset* CachedAsset = MontageAssetManager.Find(FullPathName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}
		OutAsset = nullptr;
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("AnimMontage"), FullPathName),
			[InLoadPath, InLoadName]() -> BOOL32
			{
				const EAnimMontageAsset* LoadedAsset = nullptr;
				return EAnimMontageManager::GetManagerSingleton()->LoadMontageAsset(InLoadPath, InLoadName, LoadedAsset);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}

	BOOL32 EAnimMontageManager::RebuildMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("Null montage asset.");
			return FALSE;
		}
		EAnimMontage TempMontage;
		if (!BuildMontage(TempMontage, InJsonText, OutError))
		{
			return FALSE;
		}
		if (TempMontage.Name.Length() <= 0u)
		{
			TempMontage.Name = InAsset->GetAssetName();
		}

		EAnimMontage* Montage = const_cast<EAnimMontage*>(InAsset->GetStoragedResource());
		if (!Montage)
		{
			OutError = EString("Montage asset has no resource.");
			return FALSE;
		}
		CopyMontageData(*Montage, TempMontage);
		return TRUE;
	}

	BOOL32 EAnimMontageManager::SaveMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText, EString& OutError)
	{
		if (!RebuildMontageAsset(InAsset, InJsonText, OutError))
		{
			return FALSE;
		}
		return SaveMontageResource(InAsset->GetAssetPath(), InAsset->GetAssetName(), InJsonText);
	}

	void EAnimMontageManager::RefreshNotifyReferences()
	{
		TArray<EAnimMontageAsset*> LoadedAssets;
		MontageAssetManager.GenerateValueArray(LoadedAssets);
		for (INT32 AssetIndex = 0; AssetIndex < LoadedAssets.Num(); AssetIndex++)
		{
			EAnimMontage* Montage = (LoadedAssets[AssetIndex]
				? const_cast<EAnimMontage*>(LoadedAssets[AssetIndex]->GetStoragedResource()) : nullptr);
			if (!Montage)
			{
				continue;
			}
			for (INT32 SectionIndex = 0; SectionIndex < Montage->Sections.Num(); SectionIndex++)
			{
				TArray<FAnimNotifyEvent>& Notifies = Montage->Sections[SectionIndex].Notifies;
				for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
				{
					FAnimNotifyEvent& Notify = Notifies[NotifyIndex];
					if (Notify.NotifyAssetName.Length() <= 0u)
					{
						continue;
					}
					const EAnimNotifyDefinition* Definition = nullptr;
					if ((EAnimNotifyManager::GetManagerSingleton()->ResolveNotifyDefinition(Notify.NotifyAssetName, Definition))
						&& Definition)
					{
						ApplyNotifyDefinitionToEvent(Notify, *Definition);
					}
					else
					{
						PE_LOG_WARN(EString("Montage '") + Montage->Name + EString("' notify '")
							+ Notify.NotifyName + EString("' could not resolve notify asset '")
							+ Notify.NotifyAssetName + EString("'."));
					}
				}
			}
		}
	}

	EAnimMontageAsset* EAnimMontageManager::LoadMontageAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		const EAnimMontageAsset* LoadedAsset = nullptr;
		if (!LoadMontageAsset(InLoadPath, InLoadName, LoadedAsset))
		{
			return nullptr;
		}
		return const_cast<EAnimMontageAsset*>(LoadedAsset);
	}

	EAnimMontage* EAnimMontageManager::LoadMontageResource(const EString& InLoadPath, const EString& InLoadName, EString& OutError)
	{
		OutError = EString();
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		void* ReadFileMem = nullptr;
		ULONG ReadFileSize = 0u;
		if (!(EFileHelper::ReadFileAsBinary(FullPathName, ReadFileMem, ReadFileSize)) || (!ReadFileMem))
		{
			OutError = EString("Could not read montage PAsset '") + FullPathName + EString("'.");
			return nullptr;
		}

		const BYTE* Cursor = static_cast<const BYTE*>(ReadFileMem);
		LONGLONG Remaining = static_cast<LONGLONG>(ReadFileSize);
		if (Remaining < static_cast<LONGLONG>(sizeof(UINT32) * 2u))
		{
			OutError = EString("Montage PAsset header is truncated.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}
		const UINT32 AssetType = *reinterpret_cast<const UINT32*>(Cursor);
		Cursor += sizeof(UINT32);
		Remaining -= sizeof(UINT32);
		if (AssetType != static_cast<UINT32>(EAssetType::ASSET_TYPE_ANIM_MONTAGE))
		{
			OutError = EString("Asset is not an animation montage.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}
		const UINT32 JsonLength = *reinterpret_cast<const UINT32*>(Cursor);
		Cursor += sizeof(UINT32);
		Remaining -= sizeof(UINT32);
		if ((JsonLength == 0u) || (JsonLength > MaxMontageJsonLength) || (Remaining < static_cast<LONGLONG>(JsonLength)))
		{
			OutError = EString("Montage JSON length is invalid.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}

		TArray<CHAR> JsonBuffer;
		JsonBuffer.SetNum(static_cast<INT32>(JsonLength) + 1);
		::memcpy_s(JsonBuffer.GetData(), JsonLength, Cursor, JsonLength);
		JsonBuffer[static_cast<INT32>(JsonLength)] = '\0';
		const EString JsonText(JsonBuffer.GetData());
		delete[] static_cast<CHAR*>(ReadFileMem);

		EAnimMontage* LoadedResource = new EAnimMontage();
		if (!BuildMontage(*LoadedResource, JsonText, OutError))
		{
			delete LoadedResource;
			return nullptr;
		}
		if (LoadedResource->Name.Length() <= 0u)
		{
			LoadedResource->Name = InLoadName;
		}
		return LoadedResource;
	}

	BOOL32 EAnimMontageManager::SaveMontageResource(const EString& InSavePath, const EString& InSaveName, const EString& InJsonText)
	{
		const EString FullPathName = MakeAssetFullPathName(InSavePath, InSaveName);
		const UINT32 JsonLength = static_cast<UINT32>(InJsonText.Length() + 1u);
		const ULONG OutputSize = sizeof(UINT32) + sizeof(UINT32) + JsonLength;
		BYTE* OutputMem = new BYTE[OutputSize];

		BYTE* Cursor = OutputMem;
		*reinterpret_cast<UINT32*>(Cursor) = static_cast<UINT32>(EAssetType::ASSET_TYPE_ANIM_MONTAGE);
		Cursor += sizeof(UINT32);
		*reinterpret_cast<UINT32*>(Cursor) = JsonLength;
		Cursor += sizeof(UINT32);
		::memcpy_s(Cursor, JsonLength, *InJsonText, JsonLength);

		const BOOL32 Result = EFileHelper::SaveBytesToFile(FullPathName, OutputMem, OutputSize);
		delete[] OutputMem;
		return Result;
	}

	BOOL32 EAnimMontageManager::BuildMontage(EAnimMontage& OutMontage, const EString& InJsonText, EString& OutError)
	{
		OutMontage.Reset();
		OutError = EString();
		if (InJsonText.Length() <= 0u)
		{
			OutError = EString("Montage JSON is empty.");
			return FALSE;
		}

		CJsonObject Root(*InJsonText);
		if (!Root.IsValid())
		{
			OutError = EString("Montage JSON is not a valid object.");
			return FALSE;
		}

		Root.GetStringField(PE_TEXT("name"), OutMontage.Name);
		Root.GetStringField(PE_TEXT("skeleton"), OutMontage.SkeletonName);
		Root.GetStringField(PE_TEXT("animation"), OutMontage.AnimationAssetName);

		{
			TArray<CJsonObject*> GroupObjects;
			Root.GetObjectArrayField(PE_TEXT("slotGroups"), GroupObjects);
			FJsonObjectArrayGuard GroupGuard(GroupObjects);
			for (INT32 GroupIndex = 0; GroupIndex < GroupObjects.Num(); GroupIndex++)
			{
				CJsonObject* GroupObject = GroupObjects[GroupIndex];
				if (!GroupObject)
				{
					continue;
				}
				EAnimMontageSlotGroup Group;
				GroupObject->GetStringField(PE_TEXT("name"), Group.Name);
				GroupObject->GetStringArrayField(PE_TEXT("slots"), Group.SlotNames);
				if (Group.Name.Length() <= 0u || Group.SlotNames.Num() <= 0)
				{
					OutError = EString("Every montage slot group needs a name and at least one slot.");
					return FALSE;
				}
				OutMontage.SlotGroups.Add(Group);
			}
		}

		{
			TArray<CJsonObject*> SectionObjects;
			if (!(Root.GetObjectArrayField(PE_TEXT("sections"), SectionObjects)))
			{
				for (INT32 SectionIndex = 0; SectionIndex < SectionObjects.Num(); SectionIndex++)
				{
					delete SectionObjects[SectionIndex];
				}
				OutError = EString("Montage JSON has no sections array.");
				return FALSE;
			}
			FJsonObjectArrayGuard SectionGuard(SectionObjects);
			for (INT32 SectionIndex = 0; SectionIndex < SectionObjects.Num(); SectionIndex++)
			{
				CJsonObject* SectionObject = SectionObjects[SectionIndex];
				if (!SectionObject)
				{
					continue;
				}
				EAnimMontageSection Section;
				SectionObject->GetStringField(PE_TEXT("name"), Section.Name);
				SectionObject->GetStringField(PE_TEXT("clip"), Section.ClipName);
				SectionObject->GetStringField(PE_TEXT("slot"), Section.SlotName);
				SectionObject->GetStringField(PE_TEXT("next"), Section.NextSectionName);
				EString LoopText;
				SectionObject->GetStringField(PE_TEXT("loop"), LoopText);
				Section.LoopMode = ParseLoopMode(LoopText);
				FLOAT PlayRate = 1.0f;
				if (SectionObject->GetFloatField(PE_TEXT("rate"), PlayRate))
				{
					Section.PlayRate = PlayRate;
				}
				FLOAT BlendIn = 0.1f;
				if (SectionObject->GetFloatField(PE_TEXT("blendIn"), BlendIn))
				{
					Section.BlendInSeconds = BlendIn;
				}
				FLOAT BlendOut = 0.2f;
				if (SectionObject->GetFloatField(PE_TEXT("blendOut"), BlendOut))
				{
					Section.BlendOutSeconds = BlendOut;
				}

				CJsonObject MaskObject;
				if (SectionObject->GetObjectField(PE_TEXT("boneMask"), MaskObject))
				{
					MaskObject.GetStringField(PE_TEXT("name"), Section.BoneMask.Name);
					MaskObject.GetStringArrayField(PE_TEXT("bones"), Section.BoneMask.BoneNames);
					FLOAT MaskWeight = 1.0f;
					if (MaskObject.GetFloatField(PE_TEXT("weight"), MaskWeight))
					{
						Section.BoneMask.Weight = MaskWeight;
					}
				}

				TArray<CJsonObject*> NotifyObjects;
				const BOOL8 bHasNotifyArray = SectionObject->GetObjectArrayField(PE_TEXT("notifies"), NotifyObjects);
				FJsonObjectArrayGuard NotifyGuard(NotifyObjects);
				if ((!bHasNotifyArray) && (NotifyObjects.Num() > 0))
				{
					OutError = EString("Montage section 'notifies' must be an array of objects.");
					return FALSE;
				}
				for (INT32 NotifyIndex = 0; NotifyIndex < NotifyObjects.Num(); NotifyIndex++)
				{
					CJsonObject* NotifyObject = NotifyObjects[NotifyIndex];
					if (!NotifyObject)
					{
						continue;
					}
					FAnimNotifyEvent Notify;
					Notify.NotifyName = EString(PE_TEXT("Notify")) + ToString(NotifyIndex);
					NotifyObject->GetStringField(PE_TEXT("name"), Notify.NotifyName);
					NotifyObject->GetStringField(PE_TEXT("class"), Notify.NotifyClassName);
					NotifyObject->GetStringField(PE_TEXT("notifyAsset"), Notify.NotifyAssetName);
					FLOAT NotifyTime = 0.0f;
					if (NotifyObject->GetFloatField(PE_TEXT("time"), NotifyTime))
					{
						Notify.Time = EMath::Max(NotifyTime, 0.0f);
					}
					NotifyObject->GetStringField(PE_TEXT("payload"), Notify.PayloadJson);
					if (Notify.PayloadJson.Length() <= 0u)
					{
						Notify.PayloadJson = EString(PE_TEXT("{}"));
					}
					BOOL8 bIsState = FALSE;
					NotifyObject->GetBoolField(PE_TEXT("state"), bIsState);
					Notify.bIsState = ((bIsState != FALSE) ? TRUE : FALSE);
					FLOAT StateDuration = 0.0f;
					const BOOL32 bHasDuration = (NotifyObject->HasField(PE_TEXT("duration")) ? TRUE : FALSE);
					if (NotifyObject->GetFloatField(PE_TEXT("duration"), StateDuration))
					{
						Notify.StateDuration = EMath::Max(StateDuration, 0.001f);
					}
					Notify.bStateDurationOverride = bHasDuration;

					const BOOL32 bHasPayload = (NotifyObject->HasField(PE_TEXT("payload")) ? TRUE : FALSE);
					if (bHasPayload)
					{
						if (!NotifyObject->GetStringField(PE_TEXT("payload"), Notify.PayloadJson))
						{
							CJsonObject PayloadObject;
							if (NotifyObject->GetObjectField(PE_TEXT("payload"), PayloadObject))
							{
								Notify.PayloadJson = PayloadObject.ToString();
							}
						}
					}
					Notify.bPayloadOverride = bHasPayload;

					// A standalone notify asset supplies class, state kind and default data.
					// Per-event duration/payload stay as optional overrides.
					if (Notify.NotifyAssetName.Length() > 0u)
					{
						const EAnimNotifyDefinition* Definition = nullptr;
						if ((EAnimNotifyManager::GetManagerSingleton()->ResolveNotifyDefinition(Notify.NotifyAssetName, Definition))
							&& Definition)
						{
							ApplyNotifyDefinitionToEvent(Notify, *Definition);
						}
						else
						{
							PE_LOG_WARN(EString("Montage '") + OutMontage.Name + EString("' notify '")
								+ Notify.NotifyName + EString("' could not resolve notify asset '")
								+ Notify.NotifyAssetName + EString("'."));
						}
					}

					if (Notify.bIsState && (Notify.StateDuration <= 0.0f))
					{
						// A state with no duration would only fire for the single frame the
						// playhead lands exactly on it; use a visible default instead.
						Notify.StateDuration = 0.2f;
					}

					// Compile here so playback never allocates a notify object. A resolved
					// definition already provides the shared cached object, so Compile() is a
					// no-op in that case.
					Notify.Compile();
					Section.Notifies.Add(Notify);
				}

				if ((Section.Name.Length() <= 0u) || (Section.ClipName.Length() <= 0u) || (Section.SlotName.Length() <= 0u))
				{
					OutError = EString("Every montage section needs a name, clip and slot.");
					return FALSE;
				}
				OutMontage.Sections.Add(Section);
			}
		}

		if (!OutMontage.RebuildMappings())
		{
			OutError = EString("Montage section or slot group names are empty or duplicated.");
			return FALSE;
		}
		OutMontage.SourceJson = InJsonText;
		if (!OutMontage.IsValid())
		{
			OutError = EString("Montage graph is invalid.");
			return FALSE;
		}
		return TRUE;
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<EAnimMontage, EObjectBase, EResourceInterface>();
		RegisterBaseAssetClassTypes<EAnimMontage>();
		RegisterClassType<EAnimMontageAsset, TBaseAsset<EAnimMontage>>();
		RegisterClassType<EAnimMontageManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

}
