#include "NavMeshAsset.h"

#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// Version of the file the manager below writes and reads. Bump it when a field changes; the
	// loader refuses a version it does not know instead of guessing at the layout.
	PE_CONSTEXPR_STATIC UINT32 NavMeshAssetFormatVersion = 3u;

	static void RegisterClassTypes()
	{
		RegisterClassType<ENavMeshData, EObjectBase, EResourceInterface>();
		RegisterBaseAssetClassTypes<ENavMeshData>();
		RegisterClassType<ENavMeshAsset, TBaseAsset<ENavMeshData>>();
		RegisterClassType<ENavMeshAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	ENavMeshAssetManager::ENavMeshAssetManager()
	{
	}

	ENavMeshAssetManager::~ENavMeshAssetManager()
	{
	}

	// Appends one fixed size value in native byte order. The other asset families store raw
	// values as well, and a navigation mesh is not shared between machines with different
	// endianness in this engine.
	template<typename TValueType>
	static void AppendAssetValue(TArray<UINT8>& InOutData, const TValueType& InValue)
	{
		const UINT32 Offset = InOutData.Num<UINT32>();
		InOutData.SetNum(Offset + static_cast<UINT32>(sizeof(TValueType)));
		::memcpy(InOutData.GetData() + Offset, &InValue, sizeof(TValueType));
	}

	static void AppendAssetBytes(TArray<UINT8>& InOutData, const void* InBytes, UINT32 InSize)
	{
		if (InSize == 0u)
		{
			return;
		}
		const UINT32 Offset = InOutData.Num<UINT32>();
		InOutData.SetNum(Offset + InSize);
		::memcpy(InOutData.GetData() + Offset, InBytes, InSize);
	}

	// Bounds checked cursor over the file. A read either succeeds whole or leaves the cursor
	// untouched, so a truncated file cannot produce a half filled settings block.
	struct FNavMeshAssetReader
	{
		const UINT8*	Cursor = nullptr;
		UINT32			Remaining = 0u;

		BOOL32 ReadValue(void* OutValue, UINT32 InSize)
		{
			if (Remaining < InSize)
			{
				return FALSE;
			}
			::memcpy(OutValue, Cursor, InSize);
			Cursor += InSize;
			Remaining -= InSize;
			return TRUE;
		}

		template<typename TValueType>
		BOOL32 ReadValue(TValueType& OutValue)
		{
			return ReadValue(&OutValue, static_cast<UINT32>(sizeof(TValueType)));
		}

		BOOL32 ReadBytes(const UINT8*& OutBytes, UINT32 InSize)
		{
			if (Remaining < InSize)
			{
				return FALSE;
			}
			OutBytes = Cursor;
			Cursor += InSize;
			Remaining -= InSize;
			return TRUE;
		}
	};

	ENavMeshData::ENavMeshData()
	{
		POBJ_DEBUGNAME_SET(this, "NavMeshData");
	}

	ENavMeshData::~ENavMeshData()
	{
	}

	BOOL32 ENavMeshData::IsResourceValid() const
	{
		return ((Blob.Num<UINT32>() > 0u) && (Settings.ProviderName.Length() > 0u)) ? TRUE : FALSE;
	}

	BOOL32 ENavMeshData::InitResource()
	{
		return IsResourceValid();
	}

	void ENavMeshData::ReleaseResource()
	{
		Blob.Empty(0u);
		Settings = FNavMeshAssetSettings();
	}

	const FNavMeshAssetSettings& ENavMeshData::GetSettings() const
	{
		return Settings;
	}

	const TArray<UINT8>& ENavMeshData::GetBlob() const
	{
		return Blob;
	}

	void ENavMeshData::SetContents(const FNavMeshAssetSettings& InSettings, TArray<UINT8>&& InBlob)
	{
		Settings = InSettings;
		Blob = static_cast<TArray<UINT8>&&>(InBlob);
	}

	ENavMeshAsset::ENavMeshAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TBaseAsset<ENavMeshData>(InAssetPath, InAssetName
#if _EDITOR_ONLY
		, InDebugName
#endif
	)
	{
	}

	ENavMeshAsset::~ENavMeshAsset()
	{
	}

	BOOL32 ENavMeshAsset::InitResource()
	{
		if (IsInitialized())
		{
			return TRUE;
		}
		if (!IsResourceValid())
		{
			return FALSE;
		}
		bIsInitialized = TRUE;
		return TRUE;
	}

	void ENavMeshAssetManager::Initialize()
	{
	}

	void ENavMeshAssetManager::ShutDown()
	{
		ClearNavMeshAssets();
	}

	void ENavMeshAssetManager::Uninit()
	{
		ClearNavMeshAssets();
	}

	void ENavMeshAssetManager::ClearNavMeshAssets()
	{
		NavMeshAssetManager.Clear();
	}

	ENavMeshData* ENavMeshAssetManager::LoadNavMeshResource(const EString& InLoadPath, const EString& InLoadName, EString& OutError) const
	{
		OutError = EString();
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);

		void* RawBytes = nullptr;
		ULONG RawSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(FullPathName, RawBytes, RawSize))
		{
			OutError = EString(PE_TEXT("Could not read navigation mesh asset: ")) + FullPathName;
			return nullptr;
		}
		if ((!RawBytes) || (RawSize < (sizeof(UINT32) * 2u)))
		{
			delete[] static_cast<CHAR*>(RawBytes);
			OutError = EString(PE_TEXT("Navigation mesh asset is empty or truncated: ")) + FullPathName;
			return nullptr;
		}

		FNavMeshAssetReader Reader;
		Reader.Cursor = static_cast<const UINT8*>(RawBytes);
		Reader.Remaining = static_cast<UINT32>(RawSize);

		UINT32 AssetType = 0u;
		UINT32 FormatVersion = 0u;
		if ((!Reader.ReadValue(AssetType)) || (!Reader.ReadValue(FormatVersion)))
		{
			delete[] static_cast<CHAR*>(RawBytes);
			OutError = EString(PE_TEXT("Navigation mesh asset header is truncated: ")) + FullPathName;
			return nullptr;
		}
		if (AssetType != static_cast<UINT32>(EAssetType::ASSET_TYPE_NAV_MESH))
		{
			delete[] static_cast<CHAR*>(RawBytes);
			OutError = EString(PE_TEXT("File is not a navigation mesh asset: ")) + FullPathName;
			return nullptr;
		}
		if (FormatVersion != NavMeshAssetFormatVersion)
		{
			delete[] static_cast<CHAR*>(RawBytes);
			OutError = EString(PE_TEXT("Navigation mesh asset format version is not supported: ")) + FullPathName;
			return nullptr;
		}

		FNavMeshAssetSettings Settings;
		Settings.FormatVersion = FormatVersion;

		UINT32 ProviderNameLength = 0u;
		if (!Reader.ReadValue(ProviderNameLength))
		{
			delete[] static_cast<CHAR*>(RawBytes);
			OutError = EString(PE_TEXT("Navigation mesh asset provider name is truncated: ")) + FullPathName;
			return nullptr;
		}
		const UINT8* ProviderNameBytes = nullptr;
		if ((ProviderNameLength == 0u) || (!Reader.ReadBytes(ProviderNameBytes, ProviderNameLength)))
		{
			delete[] static_cast<CHAR*>(RawBytes);
			OutError = EString(PE_TEXT("Navigation mesh asset provider name is truncated: ")) + FullPathName;
			return nullptr;
		}
		Settings.ProviderName = EString(std::string(reinterpret_cast<const CHAR*>(ProviderNameBytes), static_cast<size_t>(ProviderNameLength)));

		ENavMeshBuildSettings& BuildSettings = Settings.BuildSettings;
		BOOL32 bSettingsRead = TRUE;
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.CellSize);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.CellHeight);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.AgentRadius);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.AgentHeight);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.AgentMaxClimb);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.AgentMaxSlopeAngle);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.MaxEdgeLength);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.MaxSimplificationError);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.DetailSampleDistance);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.DetailSampleMaxError);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.MinRegionArea);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.MergeRegionArea);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.MaxVertsPerPoly);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.TileSizeCells);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.MaxLayersPerTile);
		bSettingsRead = bSettingsRead && Reader.ReadValue(BuildSettings.MaxCrowdAgents);

		UINT32 FlipTriangleWinding = 0u;
		bSettingsRead = bSettingsRead && Reader.ReadValue(FlipTriangleWinding);
		BuildSettings.bFlipTriangleWinding = (FlipTriangleWinding != 0u) ? TRUE : FALSE;

		FLOAT BoundsMin[3] = { 0.0f, 0.0f, 0.0f };
		FLOAT BoundsMax[3] = { 0.0f, 0.0f, 0.0f };
		UINT32 BoundsValid = 0u;
		for (UINT32 Axis = 0u; Axis < 3u; Axis++)
		{
			bSettingsRead = bSettingsRead && Reader.ReadValue(BoundsMin[Axis]);
		}
		for (UINT32 Axis = 0u; Axis < 3u; Axis++)
		{
			bSettingsRead = bSettingsRead && Reader.ReadValue(BoundsMax[Axis]);
		}
		bSettingsRead = bSettingsRead && Reader.ReadValue(BoundsValid);
		bSettingsRead = bSettingsRead && Reader.ReadValue(Settings.PolygonCount);
		bSettingsRead = bSettingsRead && Reader.ReadValue(Settings.TileCount);

		UINT32 BlobSize = 0u;
		bSettingsRead = bSettingsRead && Reader.ReadValue(BlobSize);
		const UINT8* BlobBytes = nullptr;
		bSettingsRead = bSettingsRead && Reader.ReadBytes(BlobBytes, BlobSize);

		if (!bSettingsRead)
		{
			delete[] static_cast<CHAR*>(RawBytes);
			OutError = EString(PE_TEXT("Navigation mesh asset payload is truncated: ")) + FullPathName;
			return nullptr;
		}

		Settings.Bounds = EBoundAABB(
			Vector3(BoundsMin[0], BoundsMin[1], BoundsMin[2]),
			Vector3(BoundsMax[0], BoundsMax[1], BoundsMax[2]),
			(BoundsValid != 0u) ? TRUE : FALSE);

		TArray<UINT8> Blob;
		Blob.SetNum(BlobSize);
		if (BlobSize > 0u)
		{
			::memcpy(Blob.GetData(), BlobBytes, BlobSize);
		}

		delete[] static_cast<CHAR*>(RawBytes);

		ENavMeshData* NewResource = new ENavMeshData();
		NewResource->SetContents(Settings, static_cast<TArray<UINT8>&&>(Blob));
		return NewResource;
	}

	BOOL32 ENavMeshAssetManager::SaveNavMeshResource(const EString& InSavePath, const EString& InSaveName
		, const FNavMeshAssetSettings& InSettings, const TArray<UINT8>& InBlob) const
	{
		const EString FullPathName = MakeAssetFullPathName(InSavePath, InSaveName);

		TArray<UINT8> Output;
		Output.Reserve(InBlob.Num<UINT32>() + 256u);
		AppendAssetValue(Output, static_cast<UINT32>(EAssetType::ASSET_TYPE_NAV_MESH));
		AppendAssetValue(Output, NavMeshAssetFormatVersion);

		const EString& ProviderName = InSettings.ProviderName;
		const UINT32 ProviderNameLength = ProviderName.Length();
		AppendAssetValue(Output, ProviderNameLength);
		AppendAssetBytes(Output, *ProviderName, ProviderNameLength);

		const ENavMeshBuildSettings& BuildSettings = InSettings.BuildSettings;
		AppendAssetValue(Output, BuildSettings.CellSize);
		AppendAssetValue(Output, BuildSettings.CellHeight);
		AppendAssetValue(Output, BuildSettings.AgentRadius);
		AppendAssetValue(Output, BuildSettings.AgentHeight);
		AppendAssetValue(Output, BuildSettings.AgentMaxClimb);
		AppendAssetValue(Output, BuildSettings.AgentMaxSlopeAngle);
		AppendAssetValue(Output, BuildSettings.MaxEdgeLength);
		AppendAssetValue(Output, BuildSettings.MaxSimplificationError);
		AppendAssetValue(Output, BuildSettings.DetailSampleDistance);
		AppendAssetValue(Output, BuildSettings.DetailSampleMaxError);
		AppendAssetValue(Output, BuildSettings.MinRegionArea);
		AppendAssetValue(Output, BuildSettings.MergeRegionArea);
		AppendAssetValue(Output, BuildSettings.MaxVertsPerPoly);
		AppendAssetValue(Output, BuildSettings.TileSizeCells);
		AppendAssetValue(Output, BuildSettings.MaxLayersPerTile);
		AppendAssetValue(Output, BuildSettings.MaxCrowdAgents);
		AppendAssetValue(Output, static_cast<UINT32>(BuildSettings.bFlipTriangleWinding ? 1u : 0u));

		AppendAssetValue(Output, InSettings.Bounds.AABBMin.x);
		AppendAssetValue(Output, InSettings.Bounds.AABBMin.y);
		AppendAssetValue(Output, InSettings.Bounds.AABBMin.z);
		AppendAssetValue(Output, InSettings.Bounds.AABBMax.x);
		AppendAssetValue(Output, InSettings.Bounds.AABBMax.y);
		AppendAssetValue(Output, InSettings.Bounds.AABBMax.z);
		AppendAssetValue(Output, static_cast<UINT32>(InSettings.Bounds.IsValid ? 1u : 0u));

		AppendAssetValue(Output, InSettings.PolygonCount);
		AppendAssetValue(Output, InSettings.TileCount);
		AppendAssetValue(Output, InBlob.Num<UINT32>());
		AppendAssetBytes(Output, InBlob.GetData(), InBlob.Num<UINT32>());

		// Atomic: a failed write leaves the previous file in place instead of truncating it, which
		// matters because a bake is expensive to redo.
		return EFileHelper::SaveBytesToFileAtomic(FullPathName, Output.GetData(), Output.Num<ULONG>());
	}

	const ENavMeshAsset* ENavMeshAssetManager::FindNavMeshAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		return NavMeshAssetManager.Find(FullPathName);
	}

	BOOL32 ENavMeshAssetManager::LoadNavMeshAsset(const EString& InLoadPath, const EString& InLoadName, const ENavMeshAsset*& OutAsset)
	{
		OutAsset = nullptr;
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (ENavMeshAsset* CachedAsset = NavMeshAssetManager.Find(FullPathName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}

		EString Error;
		ENavMeshData* LoadedResource = LoadNavMeshResource(InLoadPath, InLoadName, Error);
		if ((!LoadedResource) || (!LoadedResource->IsResourceValid()))
		{
			delete LoadedResource;
			if (Error.Length() > 0u)
			{
				PE_LOG_WARN(EString(PE_TEXT("Navigation: ")) + Error);
			}
			return FALSE;
		}

		ENavMeshAsset* NewAsset = new ENavMeshAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(NewAsset->StorageResourceInternal([LoadedResource]() -> ENavMeshData*
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
		if (NavMeshAssetManager.Add(FullPathName, NewAsset, TRUE) == 0u)
		{
			delete NewAsset;
			return FALSE;
		}
		OutAsset = NewAsset;
		return TRUE;
	}

	BOOL32 ENavMeshAssetManager::RequireLoadNavMeshAsset(const EString& InLoadPath, const EString& InLoadName
		, const ENavMeshAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete)
	{
		const EString FullPathName = MakeAssetFullPathName(InLoadPath, InLoadName);
		if (ENavMeshAsset* CachedAsset = NavMeshAssetManager.Find(FullPathName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}
		OutAsset = nullptr;
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("NavMesh"), FullPathName),
			[InLoadPath, InLoadName]() -> BOOL32
			{
				const ENavMeshAsset* LoadedAsset = nullptr;
				return ENavMeshAssetManager::GetManagerSingleton()->LoadNavMeshAsset(InLoadPath, InLoadName, LoadedAsset);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}

	BOOL32 ENavMeshAssetManager::CreateNavMeshAsset(const EString& InAssetName, const FNavMeshAssetSettings& InSettings
		, const TArray<UINT8>& InBlob, const EString& InSavePath
		, const ENavMeshAsset*& OutAsset, EString& OutError)
	{
		OutAsset = nullptr;
		OutError = EString();
		if (InAssetName.Length() <= 0u)
		{
			OutError = EString(PE_TEXT("Navigation mesh asset name is empty."));
			return FALSE;
		}
		if (InBlob.Num<UINT32>() == 0u)
		{
			OutError = EString(PE_TEXT("Navigation mesh data is empty; bake before saving."));
			return FALSE;
		}
		if (InSettings.ProviderName.Length() <= 0u)
		{
			OutError = EString(PE_TEXT("Navigation mesh asset does not record which backend produced it."));
			return FALSE;
		}

		const EString FullPathName = MakeAssetFullPathName(InSavePath, InAssetName);
		if (EFileHelper::IsFileExists(FullPathName))
		{
			OutError = EString(PE_TEXT("Navigation mesh asset already exists: ")) + FullPathName;
			return FALSE;
		}
		if (!SaveNavMeshResource(InSavePath, InAssetName, InSettings, InBlob))
		{
			OutError = EString(PE_TEXT("Failed to write navigation mesh asset: ")) + FullPathName;
			return FALSE;
		}

		const ENavMeshAsset* LoadedAsset = nullptr;
		if ((!LoadNavMeshAsset(InSavePath, InAssetName, LoadedAsset)) || (!LoadedAsset))
		{
			OutError = EString(PE_TEXT("Saved navigation mesh asset could not be loaded back: ")) + FullPathName;
			return FALSE;
		}
		OutAsset = LoadedAsset;
		PE_LOG_LOG(EString(PE_TEXT("Navigation: saved "))
			+ EString::FromInt(InBlob.Num<UINT32>()) + EString(PE_TEXT(" bytes to "))
			+ LoadedAsset->GetAssetFullPathName());
		return TRUE;
	}

}  // namespace PigeonEngine
