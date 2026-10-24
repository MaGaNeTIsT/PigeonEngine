#include "AnimationNotifyAsset.h"

#include <Editor/EditorLogManager.h>
#include <IO/FileHelper.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{

	namespace
	{
		constexpr UINT32 MaxNotifyJsonLength = 4u * 1024u * 1024u;

		void CopyNotifyDefinition(EAnimNotifyDefinition& OutDefinition, const EAnimNotifyDefinition& InDefinition)
		{
			OutDefinition.Name = InDefinition.Name;
			OutDefinition.Description = InDefinition.Description;
			OutDefinition.NotifyClassName = InDefinition.NotifyClassName;
			OutDefinition.bIsState = InDefinition.bIsState;
			OutDefinition.DefaultDuration = InDefinition.DefaultDuration;
			OutDefinition.PayloadJson = InDefinition.PayloadJson;
			OutDefinition.SourceJson = InDefinition.SourceJson;
			OutDefinition.CachedNotify = InDefinition.CachedNotify;
		}
	}

	EAnimNotifyDefinition::EAnimNotifyDefinition()
	{
	}

	EAnimNotifyDefinition::~EAnimNotifyDefinition()
	{
	}

	BOOL32 EAnimNotifyDefinition::IsResourceValid()const
	{
		return IsValid();
	}

	BOOL32 EAnimNotifyDefinition::InitResource()
	{
		return TRUE;
	}

	void EAnimNotifyDefinition::ReleaseResource()
	{
		Reset();
	}

	void EAnimNotifyDefinition::Reset()
	{
		Name = EString();
		Description = EString();
		NotifyClassName = EString();
		bIsState = FALSE;
		DefaultDuration = 0.0f;
		PayloadJson = EString();
		SourceJson = EString();
		CachedNotify.reset();
	}

	BOOL32 EAnimNotifyDefinition::IsValid()const
	{
		if ((NotifyClassName.Length() <= 0u) || (!CachedNotify))
		{
			return FALSE;
		}
		if (CachedNotify->IsStateNotify() != (bIsState != FALSE))
		{
			return FALSE;
		}
		if (bIsState && (DefaultDuration <= 0.0f))
		{
			return FALSE;
		}
		return TRUE;
	}

	EAnimNotifyAsset::EAnimNotifyAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<EAnimNotifyDefinition>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}

	EAnimNotifyAsset::~EAnimNotifyAsset()
	{
	}

	BOOL32 EAnimNotifyAsset::InitResource()
	{
		if (IsInitialized())
		{
			return TRUE;
		}
		bIsInitialized = TRUE;
		return TRUE;
	}

	EAnimNotifyManager::EAnimNotifyManager()
	{
#if _EDITOR_ONLY
		DebugName = "AnimationNotifyManager";
#endif
	}

	EAnimNotifyManager::~EAnimNotifyManager()
	{
	}

	void EAnimNotifyManager::Initialize()
	{
	}

	void EAnimNotifyManager::ShutDown()
	{
		ReleaseAllAssets();
	}

	void EAnimNotifyManager::Uninit()
	{
		ReleaseAllAssets();
	}

	void EAnimNotifyManager::ReleaseAllAssets()
	{
		ClearNotifies();
	}

	void EAnimNotifyManager::ClearNotifies()
	{
		NotifyAssetManager.Clear();
	}

#if _EDITOR_ONLY
	BOOL32 EAnimNotifyManager::ImportNotify(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath)
	{
		EString JsonText;
		if (!(EFileHelper::ReadFileAsString(InJsonFullPathName, JsonText)))
		{
			PE_LOG_WARN(EString("Notify importer could not read '") + InJsonFullPathName + EString("'."));
			return FALSE;
		}

		EAnimNotifyDefinition TempDefinition;
		EString Error;
		if (!BuildNotifyDefinition(TempDefinition, JsonText, Error))
		{
			PE_LOG_WARN(EString("Notify importer rejected '") + InJsonFullPathName + EString("': ") + Error);
			return FALSE;
		}
		if (TempDefinition.Name.Length() <= 0u)
		{
			TempDefinition.Name = InAssetName;
		}
		return SaveNotifyResource(InSavePath, InAssetName, JsonText);
	}
#endif

	BOOL32 EAnimNotifyManager::LoadNotifyAsset(const EString& InLoadPath, const EString& InLoadName, const EAnimNotifyAsset*& OutAsset)
	{
		OutAsset = nullptr;
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (EAnimNotifyAsset* CachedAsset = NotifyAssetManager.Find(FullPathName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}

		EString Error;
		EAnimNotifyDefinition* LoadedResource = LoadNotifyResource(InLoadPath, InLoadName, Error);
		if ((!LoadedResource) || (!LoadedResource->IsResourceValid()))
		{
			delete LoadedResource;
			return FALSE;
		}

		EAnimNotifyAsset* NewAsset = new EAnimNotifyAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(NewAsset->StorageResourceInternal([LoadedResource]() -> EAnimNotifyDefinition*
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
		if (NotifyAssetManager.Add(FullPathName, NewAsset, TRUE) == 0u)
		{
			delete NewAsset;
			return FALSE;
		}
		OutAsset = NewAsset;
		return TRUE;
	}

	BOOL32 EAnimNotifyManager::ResolveNotifyDefinition(const EString& InNotifyName, const EAnimNotifyDefinition*& OutDefinition)
	{
		OutDefinition = nullptr;
		if (InNotifyName.Length() <= 0u)
		{
			return FALSE;
		}
		const EAnimNotifyAsset* NotifyAsset = nullptr;
		if ((!LoadNotifyAsset(EBaseSettings::ENGINE_NOTIFY_PATH, InNotifyName, NotifyAsset)) || (!NotifyAsset))
		{
			return FALSE;
		}
		OutDefinition = NotifyAsset->GetStoragedResource();
		return ((OutDefinition != nullptr) ? TRUE : FALSE);
	}

	BOOL32 EAnimNotifyManager::RebuildNotifyAsset(const EAnimNotifyAsset* InAsset, const EString& InJsonText, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("Null notify asset.");
			return FALSE;
		}
		EAnimNotifyDefinition TempDefinition;
		if (!BuildNotifyDefinition(TempDefinition, InJsonText, OutError))
		{
			return FALSE;
		}
		if (TempDefinition.Name.Length() <= 0u)
		{
			TempDefinition.Name = InAsset->GetAssetName();
		}

		EAnimNotifyDefinition* Definition = const_cast<EAnimNotifyDefinition*>(InAsset->GetStoragedResource());
		if (!Definition)
		{
			OutError = EString("Notify asset has no resource.");
			return FALSE;
		}
		CopyNotifyDefinition(*Definition, TempDefinition);
		return TRUE;
	}

	BOOL32 EAnimNotifyManager::SaveNotifyAsset(const EAnimNotifyAsset* InAsset, const EString& InJsonText, EString& OutError)
	{
		if (!RebuildNotifyAsset(InAsset, InJsonText, OutError))
		{
			return FALSE;
		}
		return SaveNotifyResource(InAsset->GetAssetPath(), InAsset->GetAssetName(), InJsonText);
	}

	EAnimNotifyAsset* EAnimNotifyManager::LoadNotifyAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		const EAnimNotifyAsset* LoadedAsset = nullptr;
		if (!LoadNotifyAsset(InLoadPath, InLoadName, LoadedAsset))
		{
			return nullptr;
		}
		return const_cast<EAnimNotifyAsset*>(LoadedAsset);
	}

	EAnimNotifyDefinition* EAnimNotifyManager::LoadNotifyResource(const EString& InLoadPath, const EString& InLoadName, EString& OutError)
	{
		OutError = EString();
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		void* ReadFileMem = nullptr;
		ULONG ReadFileSize = 0u;
		if (!(EFileHelper::ReadFileAsBinary(FullPathName, ReadFileMem, ReadFileSize)) || (!ReadFileMem))
		{
			OutError = EString("Could not read notify PAsset '") + FullPathName + EString("'.");
			return nullptr;
		}

		const BYTE* Cursor = static_cast<const BYTE*>(ReadFileMem);
		LONGLONG Remaining = static_cast<LONGLONG>(ReadFileSize);
		if (Remaining < static_cast<LONGLONG>(sizeof(UINT32) * 2u))
		{
			OutError = EString("Notify PAsset header is truncated.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}
		const UINT32 AssetType = *reinterpret_cast<const UINT32*>(Cursor);
		Cursor += sizeof(UINT32);
		Remaining -= sizeof(UINT32);
		if (AssetType != static_cast<UINT32>(EAssetType::ASSET_TYPE_ANIM_NOTIFY))
		{
			OutError = EString("Asset is not an animation notify.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}
		const UINT32 JsonLength = *reinterpret_cast<const UINT32*>(Cursor);
		Cursor += sizeof(UINT32);
		Remaining -= sizeof(UINT32);
		if ((JsonLength == 0u) || (JsonLength > MaxNotifyJsonLength) || (Remaining < static_cast<LONGLONG>(JsonLength)))
		{
			OutError = EString("Notify JSON length is invalid.");
			delete[] static_cast<CHAR*>(ReadFileMem);
			return nullptr;
		}

		TArray<CHAR> JsonBuffer;
		JsonBuffer.SetNum(static_cast<INT32>(JsonLength) + 1);
		::memcpy_s(JsonBuffer.GetData(), JsonLength, Cursor, JsonLength);
		JsonBuffer[static_cast<INT32>(JsonLength)] = '\0';
		const EString JsonText(JsonBuffer.GetData());
		delete[] static_cast<CHAR*>(ReadFileMem);

		EAnimNotifyDefinition* LoadedDefinition = new EAnimNotifyDefinition();
		if (!BuildNotifyDefinition(*LoadedDefinition, JsonText, OutError))
		{
			delete LoadedDefinition;
			return nullptr;
		}
		if (LoadedDefinition->Name.Length() <= 0u)
		{
			LoadedDefinition->Name = InLoadName;
		}
		return LoadedDefinition;
	}

	BOOL32 EAnimNotifyManager::SaveNotifyResource(const EString& InSavePath, const EString& InSaveName, const EString& InJsonText)
	{
		const EString FullPathName = MakeAssetFullPathName(InSavePath, InSaveName);
		const UINT32 JsonLength = static_cast<UINT32>(InJsonText.Length() + 1u);
		const ULONG OutputSize = sizeof(UINT32) + sizeof(UINT32) + JsonLength;
		BYTE* OutputMem = new BYTE[OutputSize];

		BYTE* Cursor = OutputMem;
		*reinterpret_cast<UINT32*>(Cursor) = static_cast<UINT32>(EAssetType::ASSET_TYPE_ANIM_NOTIFY);
		Cursor += sizeof(UINT32);
		*reinterpret_cast<UINT32*>(Cursor) = JsonLength;
		Cursor += sizeof(UINT32);
		::memcpy_s(Cursor, JsonLength, *InJsonText, JsonLength);

		const BOOL32 Result = EFileHelper::SaveBytesToFile(FullPathName, OutputMem, OutputSize);
		delete[] OutputMem;
		return Result;
	}

	BOOL32 EAnimNotifyManager::BuildNotifyDefinition(EAnimNotifyDefinition& OutDefinition, const EString& InJsonText, EString& OutError)
	{
		OutDefinition.Reset();
		OutError = EString();
		if (InJsonText.Length() <= 0u)
		{
			OutError = EString("Notify JSON is empty.");
			return FALSE;
		}

		CJsonObject Root(*InJsonText);
		if (!Root.IsValid())
		{
			OutError = EString("Notify JSON is not a valid object.");
			return FALSE;
		}

		Root.GetStringField(PE_TEXT("name"), OutDefinition.Name);
		Root.GetStringField(PE_TEXT("description"), OutDefinition.Description);
		Root.GetStringField(PE_TEXT("class"), OutDefinition.NotifyClassName);
		// Payload is authored either as a JSON string (free-form game data) or as an object
		// that is serialized back to text for the notify class.
		if (!Root.GetStringField(PE_TEXT("payload"), OutDefinition.PayloadJson))
		{
			CJsonObject PayloadObject;
			if (Root.GetObjectField(PE_TEXT("payload"), PayloadObject))
			{
				OutDefinition.PayloadJson = PayloadObject.ToString();
			}
		}
		if (OutDefinition.PayloadJson.Length() <= 0u)
		{
			OutDefinition.PayloadJson = EString(PE_TEXT("{}"));
		}

		BOOL8 bIsState = FALSE;
		Root.GetBoolField(PE_TEXT("state"), bIsState);
		OutDefinition.bIsState = ((bIsState != FALSE) ? TRUE : FALSE);

		FLOAT Duration = 0.0f;
		if (Root.GetFloatField(PE_TEXT("duration"), Duration))
		{
			OutDefinition.DefaultDuration = EMath::Max(Duration, 0.001f);
		}
		if (OutDefinition.bIsState && (OutDefinition.DefaultDuration <= 0.0f))
		{
			OutDefinition.DefaultDuration = 0.2f;
		}
		if (!OutDefinition.bIsState)
		{
			OutDefinition.DefaultDuration = 0.0f;
		}

		if (OutDefinition.NotifyClassName.Length() <= 0u)
		{
			OutError = EString("Notify definition needs a class name.");
			return FALSE;
		}
		OutDefinition.CachedNotify = EAnimNotifyRegistry::Create(OutDefinition.NotifyClassName);
		if (!OutDefinition.CachedNotify)
		{
			OutError = EString("Notify class '") + OutDefinition.NotifyClassName + EString("' is not registered.");
			OutDefinition.Reset();
			return FALSE;
		}
		if (OutDefinition.CachedNotify->IsStateNotify() != (OutDefinition.bIsState != FALSE))
		{
			OutError = EString("Notify class '") + OutDefinition.NotifyClassName
				+ EString("' does not match the definition's state kind.");
			OutDefinition.Reset();
			return FALSE;
		}

		OutDefinition.SourceJson = InJsonText;
		if (!OutDefinition.IsValid())
		{
			OutError = EString("Notify definition is invalid.");
			return FALSE;
		}
		return TRUE;
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<EAnimNotifyDefinition, EObjectBase, EResourceInterface>();
		RegisterBaseAssetClassTypes<EAnimNotifyDefinition>();
		RegisterClassType<EAnimNotifyAsset, TBaseAsset<EAnimNotifyDefinition>>();
		RegisterClassType<EAnimNotifyManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

}
