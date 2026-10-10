#include "ShaderStageAsset.h"
#include <Asset/AssetLoadManager.h>

#include <IO/FileHelper.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// The stage asset registers its own chain, one link per layer: the shared game side base
	// link first, then its own entry.
	static void RegisterClassTypes()
	{
		RegisterBaseAssetClassTypes<EShaderStageData>();
		RegisterClassType<EShaderStageAsset, TBaseAsset<EShaderStageData>>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	// ---------------------------------------------------------------------------
	// EShaderStageData
	// ---------------------------------------------------------------------------

	BOOL32 EShaderStageData::IsResourceValid() const
	{
		return ((Bytecode.Num<UINT32>() > 0u) ? TRUE : FALSE);
	}

	BOOL32 EShaderStageData::InitResource()
	{
		// The cold data is created by the asset manager; nothing to initialize.
		return IsResourceValid();
	}

	void EShaderStageData::ReleaseResource()
	{
		if (Bytecode.Num<UINT32>() > 0u)
		{
			Bytecode.Empty();
		}
		if (ReflectionText.Length() > 0u)
		{
			ReflectionText = EString();
		}
	}

	// ---------------------------------------------------------------------------
	// EShaderStageAsset
	// ---------------------------------------------------------------------------

	// FNV-1a continued from a seed: the parser already hashed the reflection text, so the
	// stage identity is that hash extended with the bytecode.
	static UINT64 HashStageSource(const UINT64 InReflectionHash, const BYTE* InBytes, const UINT32 InByteSize) noexcept
	{
		UINT64 HashValue = InReflectionHash;
		for (UINT32 ByteIdx = 0u; ByteIdx < InByteSize; ByteIdx++)
		{
			HashValue ^= static_cast<UINT64>(InBytes[ByteIdx]);
			HashValue *= 1099511628211ull;
		}
		return HashValue;
	}

	// The identity of a stage's compiled input, derived from the cold backup alone: the parser's hash
	// of the reflection text extended with the bytecode. EShaderStageAsset::InitResource derives the
	// same value for the asset it builds, and the load compares the two to tell a reload that holds the
	// same content from one that has to replace the published object. Deriving it here, from the data
	// that was just read, is what lets that decision be made before any device object exists.
	static BOOL32 ComputeStageSourceHash(const EShaderStageData& InData, UINT64& OutSourceHash)
	{
		EShaderReflection Reflection;
		if (!ParseShaderReflectionText(InData.ReflectionText, Reflection))
		{
			return FALSE;
		}
		OutSourceHash = HashStageSource(Reflection.SourceHash, InData.Bytecode.GetData(), InData.Bytecode.Num<UINT32>());
		return TRUE;
	}

	EShaderStageAsset::EShaderStageAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	)
		: TBaseAsset<EShaderStageData>(InAssetPath, InAssetName
#if _EDITOR_ONLY
			, InDebugName
#endif
		)
		, Stage(EShaderStageType::SHADER_STAGE_VERTEX)
		, ShaderModel(EShaderModelType::SHADER_MODEL_6_6)
		, Reflection{}
		, bHasReflection(FALSE)
		, Revision(1u)
		, SourceHash(0u)
	{
	}

	EShaderStageAsset::~EShaderStageAsset()
	{
	}

	BOOL32 EShaderStageAsset::InitResource()
	{
		if (IsInitialized())
		{
			return TRUE;
		}
		if ((!ResourceData) || (!(ResourceData->IsResourceValid())))
		{
			PE_LOG_ERROR(EString("[ShaderStageAsset] Cannot init '") + GetAssetName() + EString("' without a cold backup."));
			return FALSE;
		}

		// Identity and reflection are what survive the cold backup, so take them first.
		Stage = ResourceData->Stage;
		ShaderModel = ResourceData->ShaderModel;
		bHasReflection = ParseShaderReflectionText(ResourceData->ReflectionText, Reflection);
		if (!bHasReflection)
		{
			PE_LOG_ERROR(EString("[ShaderStageAsset] Cannot parse the reflection of '") + GetAssetName() + EString("'."));
			return FALSE;
		}
		// The reflection json carries no shader model, it came from the manifest.
		Reflection.ShaderModel = ShaderModel;
		if (Reflection.Stage != Stage)
		{
			PE_LOG_WARN(EString("[ShaderStageAsset] '") + GetAssetName() + EString("': the reflection declares stage ")
				+ ToString(static_cast<UINT32>(Reflection.Stage)) + EString(" while the artifact was compiled as ")
				+ ToString(static_cast<UINT32>(Stage)) + EString("."));
		}

		// Identity of the compiled input, derived here because the compiler emits no
		// hash. Computed before the cold bytes are released.
		SourceHash = HashStageSource(Reflection.SourceHash, ResourceData->Bytecode.GetData(), ResourceData->Bytecode.Num<UINT32>());

#if _EDITOR_ONLY
		// Layout sanity. The compiler reports a zero layout for a variable only when it was
		// culled (that is what CompilerCulled says) or when the variable has no cbuffer
		// layout at all (a structured buffer's struct fields). A surviving constant buffer
		// field past the first one must carry a real offset, otherwise the cpu side would
		// write it on top of the first slot. Offset 0 is legitimate for the first field,
		// which is why the check starts at index 1.
		for (UINT32 CbIdx = 0u, CbNum = Reflection.ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			const EShaderReflectionConstantBuffer& ConstantBuffer = Reflection.ConstantBuffers[CbIdx];
			if (ConstantBuffer.bCompilerCulled)
			{
				continue;
			}
			for (UINT32 FieldIdx = 1u, FieldNum = ConstantBuffer.Fields.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
			{
				const EShaderReflectionField& Field = ConstantBuffer.Fields[FieldIdx];
				if ((!Field.bCompilerCulled) && (Field.ByteOffset == 0u) && (Field.ContentBytes == 0u))
				{
					PE_LOG_WARN(EString("[ShaderStageAsset] '") + GetAssetName() + EString("': field '")
						+ Field.Name.ToString() + EString("' of '") + ConstantBuffer.Name.ToString()
						+ EString("' reports no layout, but it is neither culled nor the first field."));
				}
			}
		}
		for (UINT32 CbIdx = 0u, CbNum = Reflection.ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			const EShaderReflectionConstantBuffer& ConstantBuffer = Reflection.ConstantBuffers[CbIdx];
			if (ConstantBuffer.bCompilerCulled)
			{
				continue;
			}
			for (UINT32 FieldIdx = 0u, FieldNum = ConstantBuffer.Fields.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
			{
				const EShaderReflectionField& Field = ConstantBuffer.Fields[FieldIdx];
				if (Field.bCompilerCulled)
				{
					continue;
				}
				// A field whose span runs past the shader visible buffer size means the
				// reflection and the declared layout disagree; writing it would corrupt
				// the buffer tail.
				const UINT32 SpanEnd = Field.GetSpanEndByteOffset();
				if ((SpanEnd > 0u) && (SpanEnd > ConstantBuffer.ByteSize))
				{
					PE_LOG_WARN(EString("[ShaderStageAsset] '") + GetAssetName() + EString("': field '")
						+ Field.Name.ToString() + EString("' ends at byte ") + ToString(SpanEnd)
						+ EString(" but '") + ConstantBuffer.Name.ToString() + EString("' is only ")
						+ ToString(ConstantBuffer.ByteSize) + EString(" bytes."));
				}
			}
		}
#endif	// _EDITOR_ONLY

		// The creation command runs on the render thread and may not read asset members, so
		// the cold backup and the parsed input layouts are snapshotted here, where the command
		// is enqueued. The asset keeps the backup alive for its whole lifetime.
		EShaderStageData* const ColdData = ResourceData;
		const TArray<EShaderReflectionInputLayout> InputLayouts = Reflection.InputLayouts;
		const EString StageAssetName = GetAssetName();
		if (RenderBinding.QueueCreate([ColdData, InputLayouts, StageAssetName]() -> RShaderStageResource*
		{
			return (EShaderStageAsset::CreateShaderStageResource(ColdData, InputLayouts, StageAssetName));
		}))
		{
			bIsInitialized = TRUE;
			Revision++;
			return TRUE;
		}
		return FALSE;
	}

	RShaderStageResource* EShaderStageAsset::CreateShaderStageResource(EShaderStageData* InResource
		, const TArray<EShaderReflectionInputLayout>& InInputLayouts, const EString& InAssetName)
	{
		if ((!InResource) || (!(InResource->IsResourceValid())))
		{
			return nullptr;
		}

		// The stage travels inside the cold backup, so this mapping reads only captured data,
		// never the asset.
		ERHIShaderStage RHIShaderStage = ERHIShaderStage::RHI_SHADER_STAGE_COUNT;
		switch (InResource->Stage)
		{
		case EShaderStageType::SHADER_STAGE_VERTEX:
		{
			RHIShaderStage = ERHIShaderStage::RHI_SHADER_STAGE_VERTEX;
			break;
		}
		case EShaderStageType::SHADER_STAGE_PIXEL:
		{
			RHIShaderStage = ERHIShaderStage::RHI_SHADER_STAGE_PIXEL;
			break;
		}
		case EShaderStageType::SHADER_STAGE_COMPUTE:
		{
			RHIShaderStage = ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE;
			break;
		}
		default:
		{
			PE_LOG_ERROR(EString("[ShaderStageAsset] '") + InAssetName + EString("' uses a stage the RHI cannot create yet."));
			return nullptr;
		}
		}

		RShaderStageResource* NewResource = new RShaderStageResource();
		if (!NewResource->CreateRenderResource(InResource->Bytecode.GetData()
			, static_cast<ULONG>(InResource->Bytecode.Num<UINT32>())
			, RHIShaderStage, InInputLayouts))
		{
			delete NewResource;
			NewResource = nullptr;
			return nullptr;
		}
		return NewResource;
	}

	// ---------------------------------------------------------------------------
	// EShaderStageAssetManager
	// ---------------------------------------------------------------------------

	EShaderStageAssetManager::EShaderStageAssetManager()
		: ShaderStageAssets{}
	{
	}

	EShaderStageAssetManager::~EShaderStageAssetManager()
	{
		ShutDown();
	}

	void EShaderStageAssetManager::Initialize()
	{
	}

	void EShaderStageAssetManager::ShutDown()
	{
		ReleaseAllAssets();
	}

	void EShaderStageAssetManager::Uninit()
	{
		ReleaseAllAssets();
	}

	void EShaderStageAssetManager::ReleaseAllAssets()
	{
		ShaderStageAssets.Clear();
		// The list belongs to the lifetime that recorded it, so it goes with the assets: a manager
		// reused after an Uninit + Init cycle must not inherit a name from the previous run and
		// republish the readers of an object that no longer exists.
		{
			EScopedLock ScopedLock(ReplacedNamesMutex);
			if (ReplacedStageAssetNames.Num<UINT32>() > 0u)
			{
				ReplacedStageAssetNames.Empty();
			}
		}
	}

	BOOL32 EShaderStageAssetManager::SaveShaderStagePAsset(const EString& InSavePath, const EString& InSaveName, const EShaderStageData& InStageData)
	{
		const EString FullPathName = InSavePath + InSaveName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		EShaderPAssetWriter PAssetWriter;
		if (!WriteShaderPAssetHeader(PAssetWriter, static_cast<UINT32>(EShaderPAssetKindType::SHADER_PASSET_KIND_STAGE)))
		{
			return FALSE;
		}
		PAssetWriter.WriteUInt32(static_cast<UINT32>(InStageData.Stage));
		PAssetWriter.WriteUInt32(static_cast<UINT32>(InStageData.ShaderModel));
		PAssetWriter.WriteString(InStageData.ReflectionText);
		PAssetWriter.WriteBytes(InStageData.Bytecode.GetData(), InStageData.Bytecode.Num<UINT32>());
		if (!PAssetWriter.SaveToFile(FullPathName))
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] Cannot write the stage asset: ") + FullPathName);
			return FALSE;
		}
		return TRUE;
	}

#if _EDITOR_ONLY

	BOOL32 EShaderStageAssetManager::ImportShaderStageAsset(const EString& InAssetName
		, const EString& InImportBytecodeFullPath, const EString& InImportReflectionFullPath
		, const EShaderStageType InStage, const EShaderModelType InShaderModel
		, const EString& InSavePath)
	{
		void* RawBytecode = nullptr;
		ULONG RawBytecodeSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(InImportBytecodeFullPath, RawBytecode, RawBytecodeSize))
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] Cannot read the bytecode to import: ") + InImportBytecodeFullPath);
			return FALSE;
		}

		EShaderStageData StageData;
		StageData.Stage = InStage;
		StageData.ShaderModel = InShaderModel;
		StageData.Bytecode.SetNum(static_cast<UINT32>(RawBytecodeSize));
		::memcpy_s(StageData.Bytecode.GetData(), static_cast<rsize_t>(RawBytecodeSize), RawBytecode, static_cast<rsize_t>(RawBytecodeSize));
		delete[] static_cast<BYTE*>(RawBytecode);
		RawBytecode = nullptr;

		if (!EFileHelper::ReadFileAsString(InImportReflectionFullPath, StageData.ReflectionText))
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] Cannot read the reflection to import: ") + InImportReflectionFullPath);
			return FALSE;
		}

		if (!SaveShaderStagePAsset(InSavePath, InAssetName, StageData))
		{
			return FALSE;
		}

		PE_LOG_LOG(EString("[ShaderStageAssetManager] Imported stage asset '") + InAssetName
			+ EString("' (") + ToString(static_cast<UINT32>(RawBytecodeSize)) + EString(" bytes)."));
		return TRUE;
	}

#endif	// _EDITOR_ONLY

	BOOL32 EShaderStageAssetManager::LoadShaderStageAsset(const EString& InLoadPath, const EString& InLoadName
		, const EShaderStageAsset*& OutAsset, const BOOL32 InIsReload)
	{
		OutAsset = nullptr;
		const EName AssetName(InLoadName);
		if (AssetName.IsNone())
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] Cannot load a stage asset without a name."));
			return FALSE;
		}

		if (!InIsReload)
		{
			EShaderStageAsset* CachedAsset = ShaderStageAssets.Find(AssetName);
			if (CachedAsset)
			{
				OutAsset = CachedAsset;
				return TRUE;
			}
		}

		const EString FullPathName = InLoadPath + InLoadName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		EShaderPAssetReader PAssetReader;
		if (!PAssetReader.OpenFromFile(FullPathName))
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] Cannot read the stage asset: ") + FullPathName);
			return FALSE;
		}

		UINT32 PAssetKind = 0u;
		if (!ReadShaderPAssetHeader(PAssetReader, PAssetKind))
		{
			return FALSE;
		}
		if (PAssetKind != static_cast<UINT32>(EShaderPAssetKindType::SHADER_PASSET_KIND_STAGE))
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] '") + FullPathName + EString("' does not hold a stage asset."));
			return FALSE;
		}

		EShaderStageData* StageData = new EShaderStageData();
		UINT32 StageValue = 0u;
		UINT32 ShaderModelValue = 0u;
		if ((!PAssetReader.ReadUInt32(StageValue))
			|| (!PAssetReader.ReadUInt32(ShaderModelValue))
			|| (!PAssetReader.ReadString(StageData->ReflectionText))
			|| (!PAssetReader.ReadBytesCopy(StageData->Bytecode)))
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] The stage asset is truncated: ") + FullPathName);
			delete StageData;
			StageData = nullptr;
			return FALSE;
		}
		StageData->Stage = static_cast<EShaderStageType>(static_cast<UINT8>(StageValue));
		StageData->ShaderModel = static_cast<EShaderModelType>(static_cast<UINT8>(ShaderModelValue));

		if (InIsReload)
		{
			/*
			* A reload whose compiled content did not change is not a replacement, and treating it as one
			* is what breaks readers this load knows nothing about: a stage asset is shared, so every
			* shader asset that references this name would be left composed against the released object.
			* The source hash is the identity of that content, and the decision is made here, on the cold
			* backup, before an asset object exists - an object built from unchanged content would create
			* a device shader only to be thrown away again.
			*/
			EShaderStageAsset* CachedAsset = ShaderStageAssets.Find(AssetName);
			UINT64 NewSourceHash = 0u;
			if ((CachedAsset) && (ComputeStageSourceHash(*StageData, NewSourceHash))
				// The hash covers the compiled content; the stage and the model say what the artifact is.
				// A difference in any of the three means this is not the object the file describes.
				&& (CachedAsset->GetSourceHash() == NewSourceHash)
				&& (CachedAsset->GetStage() == StageData->Stage)
				&& (CachedAsset->GetShaderModel() == StageData->ShaderModel))
			{
				// The published object is still exactly what the file says, so it stays and the data
				// just read is dropped whole. Nothing else refers to it: no asset took ownership.
				delete StageData;
				StageData = nullptr;
				OutAsset = CachedAsset;
#if _EDITOR_ONLY
				PE_LOG_LOG(EString("[ShaderStageAssetManager] Kept the loaded stage asset '") + InLoadName
					+ EString("': the reload holds the same compiled content (source hash ")
					+ ToString(NewSourceHash) + EString(")."));
#endif	// _EDITOR_ONLY
				return TRUE;
			}
			if (CachedAsset)
			{
				// An object is about to be swapped out under this name, and its readers are not only
				// the shader asset this load is composing: the name is recorded so the caller can move
				// every other shader asset that references it onto the new object.
				RecordReplacedStageAssetName(InLoadName);
			}
		}

		EShaderStageAsset* NewAsset = new EShaderStageAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		if (!(NewAsset->StorageResourceInternal([StageData]()->EShaderStageData*
		{
			return StageData;
		})))
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] Cannot store the cold backup of '") + InLoadName + EString("'."));
			delete StageData;
			StageData = nullptr;
			delete NewAsset;
			NewAsset = nullptr;
			return FALSE;
		}
		if (!NewAsset->InitResource())
		{
			PE_LOG_ERROR(EString("[ShaderStageAssetManager] Cannot init the stage asset '") + InLoadName + EString("'."));
			delete NewAsset;
			NewAsset = nullptr;
			return FALSE;
		}

		ShaderStageAssets.Add(AssetName, NewAsset, (InIsReload ? TRUE : FALSE));
		OutAsset = NewAsset;

#if _EDITOR_ONLY
		PE_LOG_LOG(EString("[ShaderStageAssetManager] Loaded stage asset '") + InLoadName
			+ EString("' (revision ") + ToString(NewAsset->GetRevision())
			+ EString(", cbuffers ") + ToString(NewAsset->GetReflection().ConstantBuffers.Num<UINT32>())
			+ EString(", culled fields ") + ToString(NewAsset->GetReflection().GetCulledFieldNum())
			// Culled resources are declared but absent from the compiled shader; they are
			// reported here so a "parameter does nothing" report can be checked against
			// this number first.
			+ EString(", culled resources ") + ToString(NewAsset->GetReflection().GetCulledResourceNum())
			+ EString(", source hash ") + ToString(NewAsset->GetSourceHash())
			+ EString(")."));
#endif	// _EDITOR_ONLY

		return TRUE;
	}
	BOOL32 EShaderStageAssetManager::RequireLoadShaderStageAsset(const EString& InLoadPath, const EString& InLoadName
		, const EShaderStageAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete)
	{
		OutAsset = nullptr;
		const EName AssetName(InLoadName);
		if (AssetName.IsNone())
		{
			return FALSE;
		}
		if (EShaderStageAsset* CachedAsset = ShaderStageAssets.Find(AssetName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}
		// The cache is keyed by name alone, but the request is keyed by the full path: two stage
		// assets of the same name in different directories are different files, and only one of
		// them can be the cached one.
		const EString FullPathName = InLoadPath + InLoadName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("ShaderStage"), FullPathName),
			[InLoadPath, InLoadName]() -> BOOL32
			{
				const EShaderStageAsset* LoadedAsset = nullptr;
				return EShaderStageAssetManager::GetManagerSingleton()->LoadShaderStageAsset(InLoadPath, InLoadName, LoadedAsset, FALSE);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}

	EShaderStageAsset* EShaderStageAssetManager::FindShaderStageAsset(const EName& InAssetName)
	{
		return ShaderStageAssets.Find(InAssetName);
	}

	const EShaderStageAsset* EShaderStageAssetManager::FindShaderStageAsset(const EName& InAssetName) const
	{
		EShaderStageAsset* TempAsset = const_cast<EShaderStageAssetManager*>(this)->ShaderStageAssets.Find(InAssetName);
		return TempAsset;
	}

	UINT32 EShaderStageAssetManager::GetShaderStageAssetNum() const
	{
		return ShaderStageAssets.Size();
	}

	void EShaderStageAssetManager::RecordReplacedStageAssetName(const EString& InAssetName)
	{
		EScopedLock ScopedLock(ReplacedNamesMutex);
		ReplacedStageAssetNames.Add(InAssetName);
	}

	void EShaderStageAssetManager::TakeReplacedStageAssetNames(TArray<EString>& OutNames)
	{
		EScopedLock ScopedLock(ReplacedNamesMutex);
		for (UINT32 NameIndex = 0u, NameNum = ReplacedStageAssetNames.Num<UINT32>(); NameIndex < NameNum; NameIndex++)
		{
			OutNames.Add(ReplacedStageAssetNames[NameIndex]);
		}
		if (ReplacedStageAssetNames.Num<UINT32>() > 0u)
		{
			ReplacedStageAssetNames.Empty();
		}
	}

} // namespace PigeonEngine
