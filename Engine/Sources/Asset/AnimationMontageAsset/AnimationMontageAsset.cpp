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
			OutMontage.Sections = InMontage.Sections;
			OutMontage.SectionMapping = InMontage.SectionMapping;
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

	BOOL32 EAnimMontageManager::CreateMontageAsset(const EString& InAssetName, const EString& InJsonText
		, const EString& InSavePath, EString& OutError)
	{
		OutError = EString();
		if (InAssetName.Length() <= 0u)
		{
			OutError = EString("Montage asset name is empty.");
			return FALSE;
		}
		if (InJsonText.Length() <= 0u)
		{
			OutError = EString("Montage JSON is empty.");
			return FALSE;
		}

		EAnimMontage TempMontage;
		if (!BuildMontage(TempMontage, InJsonText, OutError))
		{
			return FALSE;
		}
		if (TempMontage.Name.Length() <= 0u)
		{
			TempMontage.Name = InAssetName;
		}

		const EString FullPathName = MakeAssetFullPathName(InSavePath, InAssetName);
		if (EFileHelper::IsFileExists(FullPathName))
		{
			OutError = EString("Montage asset already exists: ") + FullPathName;
			return FALSE;
		}
		if (!SaveMontageResource(InSavePath, InAssetName, InJsonText))
		{
			OutError = EString("Failed to write montage asset: ") + FullPathName;
			return FALSE;
		}
		const EAnimMontageAsset* LoadedAsset = nullptr;
		if (!LoadMontageAsset(InSavePath, InAssetName, LoadedAsset) || (!LoadedAsset))
		{
			OutError = EString("Created montage could not be loaded: ") + FullPathName;
			return FALSE;
		}
		return TRUE;
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
	BOOL32 EAnimMontageManager::ReloadMontageAsset(const EString& InLoadPath, const EString& InLoadName
		, const EAnimMontageAsset*& OutAsset, EString& OutError)
	{
		OutAsset = nullptr;
		OutError = EString();
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);

		EString LoadError;
		EAnimMontage* LoadedResource = LoadMontageResource(InLoadPath, InLoadName, LoadError);
		if ((!LoadedResource) || (!LoadedResource->IsResourceValid()))
		{
			delete LoadedResource;
			OutError = (LoadError.Length() > 0u)
				? LoadError
				: (EString(PE_TEXT("Failed to load montage asset from disk: ")) + FullPathName);
			return FALSE;
		}

		EAnimMontageAsset* FreshAsset = new EAnimMontageAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(FreshAsset->StorageResourceInternal([LoadedResource]() -> EAnimMontage*
			{
				return LoadedResource;
			})))
		{
			delete FreshAsset;
			OutError = EString(PE_TEXT("Failed to build reloaded montage asset: ")) + FullPathName;
			return FALSE;
		}
		if (!(FreshAsset->InitResource()))
		{
			delete FreshAsset;
			OutError = EString(PE_TEXT("Montage asset has no valid resource: ")) + FullPathName;
			return FALSE;
		}

		EAnimMontageAsset* OldAsset = MontageAssetManager.Detach(FullPathName);
		if (MontageAssetManager.Add(FullPathName, FreshAsset, FALSE) == 0u)
		{
			if (OldAsset && (MontageAssetManager.Add(FullPathName, OldAsset, FALSE) == 0u))
			{
				MontageAssetManager.RetireDetached(OldAsset);
			}
			delete FreshAsset;
			OutError = EString(PE_TEXT("Failed to publish reloaded montage asset: ")) + FullPathName;
			return FALSE;
		}
		MontageAssetManager.RetireDetached(OldAsset);
		OutAsset = FreshAsset;
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

	EAnimMontage* EAnimMontageManager::BuildTransientMontage(const EString& InJsonText, EString& OutError)
	{
		EAnimMontage* Montage = new EAnimMontage();
		if (!BuildMontage(*Montage, InJsonText, OutError))
		{
			delete Montage;
			return nullptr;
		}
		return Montage;
	}

	BOOL32 EAnimMontageManager::BuildAndPublishMontageAsset(const EAnimMontageAsset* InAsset
		, const EString& InJsonText, BOOL32 bInSaveToDisk
		, const EAnimMontageAsset*& OutAsset, EString& OutError)
	{
		OutAsset = nullptr;
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("Null montage asset.");
			return FALSE;
		}
		EAnimMontage* NewResource = new EAnimMontage();
		if (!BuildMontage(*NewResource, InJsonText, OutError))
		{
			delete NewResource;
			return FALSE;
		}
		if (NewResource->Name.Length() <= 0u)
		{
			NewResource->Name = InAsset->GetAssetName();
		}

		EAnimMontageAsset* FreshAsset = new EAnimMontageAsset(InAsset->GetAssetPath()
			, InAsset->GetAssetName()
#if _EDITOR_ONLY
			, InAsset->GetAssetName()
#endif
		);
		if (!(FreshAsset->StorageResourceInternal([NewResource]() -> EAnimMontage*
			{
				return NewResource;
			})))
		{
			delete NewResource;
			delete FreshAsset;
			OutError = EString("Failed to build the new montage resource.");
			return FALSE;
		}
		if (!(FreshAsset->InitResource()))
		{
			delete FreshAsset;
			OutError = EString("Montage asset has no valid resource.");
			return FALSE;
		}
		if (bInSaveToDisk != FALSE)
		{
			if (!SaveMontageResource(FreshAsset->GetAssetPath(), FreshAsset->GetAssetName(), InJsonText))
			{
				delete FreshAsset;
				OutError = EString(PE_TEXT("Failed to write montage asset: "))
					+ InAsset->GetAssetPath() + InAsset->GetAssetName();
				return FALSE;
			}
		}

		const EString FullPathName = FreshAsset->GetAssetFullPathName();
		EAnimMontageAsset* OldAsset = MontageAssetManager.Detach(FullPathName);
		if (MontageAssetManager.Add(FullPathName, FreshAsset, FALSE) == 0u)
		{
			if (OldAsset && (MontageAssetManager.Add(FullPathName, OldAsset, FALSE) == 0u))
			{
				MontageAssetManager.RetireDetached(OldAsset);
			}
			delete FreshAsset;
			OutError = EString(PE_TEXT("Failed to publish montage asset: ")) + FullPathName;
			return FALSE;
		}
		MontageAssetManager.RetireDetached(OldAsset);
		OutAsset = FreshAsset;
		return TRUE;
	}

	BOOL32 EAnimMontageManager::ApplyMontageAsset(const EAnimMontageAsset* InAsset
		, const EString& InJsonText, const EAnimMontageAsset*& OutAsset, EString& OutError)
	{
		return BuildAndPublishMontageAsset(InAsset, InJsonText, FALSE, OutAsset, OutError);
	}

	BOOL32 EAnimMontageManager::SaveMontageAssetCopyOnWrite(const EAnimMontageAsset* InAsset
		, const EString& InJsonText, const EAnimMontageAsset*& OutAsset, EString& OutError)
	{
		return BuildAndPublishMontageAsset(InAsset, InJsonText, TRUE, OutAsset, OutError);
	}

	BOOL32 EAnimMontageManager::SaveMontageAsset(const EAnimMontageAsset* InAsset
		, const EString& InJsonText, EString& OutError)
	{
		const EAnimMontageAsset* PublishedAsset = nullptr;
		return SaveMontageAssetCopyOnWrite(InAsset, InJsonText, PublishedAsset, OutError);
	}

	void EAnimMontageManager::GetLoadedAssets(TArray<const EAnimMontageAsset*>& OutAssets)
	{
		OutAssets.Empty();
		TArray<EAnimMontageAsset*> LoadedAssets;
		MontageAssetManager.GenerateValueArray(LoadedAssets);
		for (INT32 AssetIndex = 0; AssetIndex < LoadedAssets.Num(); AssetIndex++)
		{
			if (LoadedAssets[AssetIndex])
			{
				OutAssets.Add(LoadedAssets[AssetIndex]);
			}
		}
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

		const BOOL32 Result = EFileHelper::SaveBytesToFileAtomic(FullPathName, OutputMem, OutputSize);
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

		Root.GetStringField(PE_TEXT("Name"), OutMontage.Name);
		Root.GetStringField(PE_TEXT("Skeleton"), OutMontage.SkeletonName);
		Root.GetStringField(PE_TEXT("Animation"), OutMontage.AnimationAssetName);

		{
			TArray<CJsonObject*> SectionObjects;
			if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), SectionObjects)))
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
				SectionObject->GetStringField(PE_TEXT("Name"), Section.Name);
				SectionObject->GetStringField(PE_TEXT("Clip"), Section.ClipName);
				SectionObject->GetStringField(PE_TEXT("Slot"), Section.SlotName);
				SectionObject->GetStringField(PE_TEXT("Next"), Section.NextSectionName);
				EString LoopText;
				SectionObject->GetStringField(PE_TEXT("Loop"), LoopText);
				Section.LoopMode = ParseLoopMode(LoopText);
				FLOAT PlayRate = 1.0f;
				if (SectionObject->GetFloatField(PE_TEXT("Rate"), PlayRate))
				{
					Section.PlayRate = PlayRate;
				}
				FLOAT BlendIn = 0.1f;
				if (SectionObject->GetFloatField(PE_TEXT("BlendIn"), BlendIn))
				{
					Section.BlendInSeconds = BlendIn;
				}
				FLOAT BlendOut = 0.2f;
				if (SectionObject->GetFloatField(PE_TEXT("BlendOut"), BlendOut))
				{
					Section.BlendOutSeconds = BlendOut;
				}
				BOOL8 bAdditive = FALSE;
				SectionObject->GetBoolField(PE_TEXT("Additive"), bAdditive);
				Section.bAdditive = ((bAdditive != FALSE) ? TRUE : FALSE);

				CJsonObject MaskObject;
				if (SectionObject->GetObjectField(PE_TEXT("BoneMask"), MaskObject))
				{
					MaskObject.GetStringField(PE_TEXT("Name"), Section.BoneMask.Name);
					MaskObject.GetStringArrayField(PE_TEXT("Bones"), Section.BoneMask.BoneNames);
					FLOAT MaskWeight = 1.0f;
					if (MaskObject.GetFloatField(PE_TEXT("Weight"), MaskWeight))
					{
						Section.BoneMask.Weight = MaskWeight;
					}
				}

				TArray<CJsonObject*> NotifyObjects;
				const BOOL8 bHasNotifyArray = SectionObject->GetObjectArrayField(PE_TEXT("Notifies"), NotifyObjects);
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
					NotifyObject->GetStringField(PE_TEXT("Name"), Notify.NotifyName);
					NotifyObject->GetStringField(PE_TEXT("Class"), Notify.NotifyClassName);
					NotifyObject->GetStringField(PE_TEXT("NotifyAsset"), Notify.NotifyAssetName);
					FLOAT NotifyTime = 0.0f;
					if (NotifyObject->GetFloatField(PE_TEXT("Time"), NotifyTime))
					{
						Notify.Time = EMath::Max(NotifyTime, 0.0f);
					}
					NotifyObject->GetStringField(PE_TEXT("Payload"), Notify.PayloadJson);
					if (Notify.PayloadJson.Length() <= 0u)
					{
						Notify.PayloadJson = EString(PE_TEXT("{}"));
					}
					BOOL8 bIsState = FALSE;
					NotifyObject->GetBoolField(PE_TEXT("State"), bIsState);
					Notify.bIsState = ((bIsState != FALSE) ? TRUE : FALSE);
					FLOAT StateDuration = 0.0f;
					const BOOL32 bHasDuration = (NotifyObject->HasField(PE_TEXT("Duration")) ? TRUE : FALSE);
					if (NotifyObject->GetFloatField(PE_TEXT("Duration"), StateDuration))
					{
						Notify.StateDuration = EMath::Max(StateDuration, 0.001f);
					}
					Notify.bStateDurationOverride = bHasDuration;

					const BOOL32 bHasPayload = (NotifyObject->HasField(PE_TEXT("Payload")) ? TRUE : FALSE);
					if (bHasPayload)
					{
						if (!NotifyObject->GetStringField(PE_TEXT("Payload"), Notify.PayloadJson))
						{
							CJsonObject PayloadObject;
							if (NotifyObject->GetObjectField(PE_TEXT("Payload"), PayloadObject))
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
