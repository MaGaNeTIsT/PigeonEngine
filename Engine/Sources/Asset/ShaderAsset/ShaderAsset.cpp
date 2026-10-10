#include "ShaderAsset.h"
#include <Asset/AssetLoadManager.h>

#include <RapidJson/Headers/JsonObject.h>
#include <IO/FileHelper.h>
#include <Editor/EditorLogManager.h>
#include <Config/ErrorCaption.h>
#include <Shader/EngineShaderInstanceCache.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// Path helpers - artifact names inside a manifest are relative to the manifest
	// itself, so the engine never rebuilds a file name.
	// ---------------------------------------------------------------------------

	static EString ParentDirectoryOf(const EString& InFilePath)
	{
		const INT32 SlashPos = InFilePath.RightFind(EString("/"));
		if (SlashPos >= 0)
		{
			return InFilePath.Left(static_cast<UINT32>(SlashPos) + 1u);
		}
		return EString();
	}

	static EString FileNameStemOf(const EString& InFileName)
	{
		const INT32 DotPos = InFileName.RightFind(EString("."));
		if (DotPos > 0)
		{
			return InFileName.Left(static_cast<UINT32>(DotPos));
		}
		return InFileName;
	}

	// A shader asset derives from the game side asset base, so it registers like any other
	// asset: the shared TBaseAsset link first, then its own entry in the chain.
	static void RegisterClassTypes()
	{
		RegisterBaseAssetClassTypes<EShaderAssetData>();
		RegisterClassType<EShaderAsset, TBaseAsset<EShaderAssetData>>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	// ---------------------------------------------------------------------------
	// Manifest
	// ---------------------------------------------------------------------------

	const EShaderManifestVariant* EShaderManifest::FindVariant(const EName& InVariantName) const
	{
		for (UINT32 VariantIdx = 0u, VariantNum = Variants.Num<UINT32>(); VariantIdx < VariantNum; VariantIdx++)
		{
			if (Variants[VariantIdx].Name == InVariantName)
			{
				return (&(Variants[VariantIdx]));
			}
		}
		return nullptr;
	}

	static BOOL32 ParseManifestVariants(CJsonObject& InJson, EShaderManifest& OutManifest);

	BOOL32 ParseShaderManifestText(const EString& InManifestJsonText, EShaderManifest& OutManifest)
	{
		if (InManifestJsonText.Length() == 0u)
		{
			return FALSE;
		}

		OutManifest = EShaderManifest();
		CJsonObject ManifestJson(InManifestJsonText);

		EString ValueStr;
		if (ManifestJson.GetStringField(EString("Name"), ValueStr))
		{
			OutManifest.ShaderName = EName(ValueStr);
		}
		else
		{
			PE_LOG_ERROR(EString("[ShaderManifest] The manifest does not declare a shader name."));
			return FALSE;
		}
		if (ManifestJson.GetStringField(EString("Entry"), ValueStr))
		{
			OutManifest.EntryPoint = EName(ValueStr);
		}
		if (!ManifestJson.GetStringField(EString("Stage"), ValueStr))
		{
			PE_LOG_ERROR(EString("[ShaderManifest] The manifest does not declare a stage."));
			return FALSE;
		}
		if (!ParseShaderStageFromString(ValueStr, OutManifest.Stage))
		{
			PE_LOG_ERROR(EString("[ShaderManifest] Unknown stage '") + ValueStr + EString("'."));
			return FALSE;
		}
		ManifestJson.GetStringField(EString("StageSuffix"), OutManifest.StageSuffix);
		ManifestJson.GetStringField(EString("EntryJson"), OutManifest.EntryJsonPath);
		if (!ManifestJson.GetStringField(EString("ShaderModel"), ValueStr))
		{
			PE_LOG_ERROR(EString("[ShaderManifest] The manifest does not declare a shader model."));
			return FALSE;
		}
		if (!ParseShaderModelFromString(ValueStr, OutManifest.ShaderModel))
		{
			PE_LOG_ERROR(EString("[ShaderManifest] Unknown shader model '") + ValueStr + EString("'."));
			return FALSE;
		}

		return ParseManifestVariants(ManifestJson, OutManifest);
	}

	static BOOL32 ParseManifestVariants(CJsonObject& InJson, EShaderManifest& OutManifest)
	{
		TArray<CJsonObject*> VariantArray;
		if (!InJson.GetObjectArrayField(EString("Variants"), VariantArray))
		{
			PE_LOG_ERROR(EString("[ShaderManifest] The manifest declares no variants."));
			return FALSE;
		}

		for (UINT32 VariantIdx = 0u, VariantNum = VariantArray.Num<UINT32>(); VariantIdx < VariantNum; VariantIdx++)
		{
			CJsonObject* VariantJson = VariantArray[VariantIdx];
			if (!VariantJson)
			{
				continue;
			}
			EShaderManifestVariant Variant;

			EString VariantName;
			if (!VariantJson->GetStringField(EString("Name"), VariantName))
			{
				PE_LOG_ERROR(EString("[ShaderManifest] A variant entry has no name, it is skipped."));
				delete VariantJson;
				continue;
			}
			Variant.Name = EName(VariantName);

			VariantJson->GetUIntField(EString("Index"), Variant.Index);
			VariantJson->GetStringField(EString("DefHash"), Variant.DefineHash);
			VariantJson->GetStringField(EString("RplHash"), Variant.ReplaceHash);
			VariantJson->GetStringField(EString("Binary"), Variant.BinaryFile);
			VariantJson->GetStringField(EString("Reflection"), Variant.ReflectionFile);
			VariantJson->GetStringField(EString("Pdb"), Variant.PdbFile);
			VariantJson->GetStringField(EString("Errors"), Variant.Errors);

			BOOL8 bSuccess = FALSE;
			if (VariantJson->GetBoolField(EString("Success"), bSuccess))
			{
				Variant.bSuccess = (bSuccess ? TRUE : FALSE);
			}

			TArray<CJsonObject*> DefineArray;
			if (VariantJson->GetObjectArrayField(EString("Defines"), DefineArray))
			{
				for (UINT32 DefIdx = 0u, DefNum = DefineArray.Num<UINT32>(); DefIdx < DefNum; DefIdx++)
				{
					CJsonObject* DefineJson = DefineArray[DefIdx];
					if (!DefineJson)
					{
						continue;
					}
					EShaderVariantDefine Define;
					EString DefineName;
					if (DefineJson->GetStringField(EString("Name"), DefineName))
					{
						Define.Name = EName(DefineName);
						DefineJson->GetIntField(EString("Value"), Define.Value);
						Variant.Defines.Add(Define);
					}
					delete DefineJson;
				}
			}

			OutManifest.Variants.Add(Variant);
			delete VariantJson;
		}
		return TRUE;
	}

	BOOL32 ParseShaderManifestJson(const EString& InManifestJsonPath, EShaderManifest& OutManifest)
	{
		EString ManifestText;
		if (!EFileHelper::ReadFileAsString(InManifestJsonPath, ManifestText))
		{
			PE_LOG_ERROR(EString("[ShaderManifest] Cannot read manifest: ") + InManifestJsonPath);
			return FALSE;
		}
		if (!ParseShaderManifestText(ManifestText, OutManifest))
		{
			PE_LOG_ERROR(EString("[ShaderManifest] Cannot parse manifest: ") + InManifestJsonPath);
			return FALSE;
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Cold backup
	// ---------------------------------------------------------------------------

	const EShaderAssetStageRef* EShaderAssetVariantData::FindStageRef(const EShaderStageType InStage) const
	{
		for (UINT32 RefIdx = 0u, RefNum = StageRefs.Num<UINT32>(); RefIdx < RefNum; RefIdx++)
		{
			if (StageRefs[RefIdx].Stage == InStage)
			{
				return (&(StageRefs[RefIdx]));
			}
		}
		return nullptr;
	}

	EShaderAssetVariantData* EShaderAssetData::FindVariant(const EName& InVariantName)
	{
		for (UINT32 VariantIdx = 0u, VariantNum = Variants.Num<UINT32>(); VariantIdx < VariantNum; VariantIdx++)
		{
			if (Variants[VariantIdx].Name == InVariantName)
			{
				return (&(Variants[VariantIdx]));
			}
		}
		return nullptr;
	}

	const EShaderAssetVariantData* EShaderAssetData::FindVariant(const EName& InVariantName) const
	{
		for (UINT32 VariantIdx = 0u, VariantNum = Variants.Num<UINT32>(); VariantIdx < VariantNum; VariantIdx++)
		{
			if (Variants[VariantIdx].Name == InVariantName)
			{
				return (&(Variants[VariantIdx]));
			}
		}
		return nullptr;
	}

	// A cold backup carries no GPU payload, so "valid" only means the read produced a named
	// asset; there is nothing to build and nothing to release beyond dropping the copy.
	BOOL32 EShaderAssetData::IsResourceValid() const
	{
		return ((!ShaderName.IsNone()) ? TRUE : FALSE);
	}

	BOOL32 EShaderAssetData::InitResource()
	{
		return TRUE;
	}

	void EShaderAssetData::ReleaseResource()
	{
		if (Variants.Num<UINT32>() > 0u)
		{
			Variants.Empty();
		}
	}

	// ---------------------------------------------------------------------------
	// EShaderAsset
	// ---------------------------------------------------------------------------

	EShaderAsset::EShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	)
		: TBaseAsset<EShaderAssetData>(InAssetPath, InAssetName
#if _EDITOR_ONLY
			, InDebugName
#endif
		)
		, Name(InAssetName)
		, EntryJsonPath()
		, Revision(1u)
		, Programs{}
		, DefaultParameterSets{}
	{
	}

	EShaderAsset::~EShaderAsset()
	{
		for (UINT32 ProgramIdx = 0u, ProgramNum = Programs.Num<UINT32>(); ProgramIdx < ProgramNum; ProgramIdx++)
		{
			if (Programs[ProgramIdx])
			{
				delete Programs[ProgramIdx];
				Programs[ProgramIdx] = nullptr;
			}
		}
		if (Programs.Num<UINT32>() > 0u)
		{
			Programs.Empty();
		}

		// Default parameter sets are owned here; instances only point their parent at
		// them, so they must be gone before the asset is.
		TArray<EShaderParameterSet*> ParameterSets;
		DefaultParameterSets.GenerateValueArray(ParameterSets);
		for (UINT32 SetIdx = 0u, SetNum = ParameterSets.Num<UINT32>(); SetIdx < SetNum; SetIdx++)
		{
			if (ParameterSets[SetIdx])
			{
				delete ParameterSets[SetIdx];
				ParameterSets[SetIdx] = nullptr;
			}
		}
		if (DefaultParameterSets.Num<UINT32>() > 0u)
		{
			DefaultParameterSets.Empty();
		}
	}

	BOOL32 EShaderAsset::InitResource()
	{
		if (!ResourceData)
		{
			PE_LOG_ERROR(EString("[ShaderAsset] '") + Name.ToString() + EString("' has no cold backup to restore."));
			return FALSE;
		}
		if (!ComposeFromData(*ResourceData, GetAssetPath(), FALSE))
		{
			return FALSE;
		}
		bIsInitialized = TRUE;
		return TRUE;
	}

	UINT32 EShaderAsset::GetStageMask() const
	{
		UINT32 StageMaskValue = SHADER_STAGE_MASK_NONE;
		for (UINT32 ProgramIdx = 0u, ProgramNum = Programs.Num<UINT32>(); ProgramIdx < ProgramNum; ProgramIdx++)
		{
			if (Programs[ProgramIdx])
			{
				StageMaskValue |= Programs[ProgramIdx]->GetStageMask();
			}
		}
		return StageMaskValue;
	}

	BOOL32 EShaderAsset::HasStage(const EShaderStageType InStage) const
	{
		UINT32 StageMaskBit = SHADER_STAGE_MASK_NONE;
		if (!GetShaderStageMaskBit(InStage, StageMaskBit))
		{
			return FALSE;
		}
		return (((GetStageMask() & StageMaskBit) != 0u) ? TRUE : FALSE);
	}

	UINT32 EShaderAsset::GetCulledFieldNum() const
	{
		UINT32 CulledNum = 0u;
		for (UINT32 ProgramIdx = 0u, ProgramNum = Programs.Num<UINT32>(); ProgramIdx < ProgramNum; ProgramIdx++)
		{
			if (Programs[ProgramIdx])
			{
				CulledNum += Programs[ProgramIdx]->GetCulledFieldNum();
			}
		}
		return CulledNum;
	}

	UINT32 EShaderAsset::GetExposedFieldNum() const
	{
		UINT32 ExposedNum = 0u;
		for (UINT32 ProgramIdx = 0u, ProgramNum = Programs.Num<UINT32>(); ProgramIdx < ProgramNum; ProgramIdx++)
		{
			if (Programs[ProgramIdx])
			{
				ExposedNum += Programs[ProgramIdx]->GetExposedFieldNum();
			}
		}
		return ExposedNum;
	}

	EShaderProgram* EShaderAsset::FindProgram(const EName& InVariantName)
	{
		for (UINT32 ProgramIdx = 0u, ProgramNum = Programs.Num<UINT32>(); ProgramIdx < ProgramNum; ProgramIdx++)
		{
			if (Programs[ProgramIdx] && (Programs[ProgramIdx]->GetVariantName() == InVariantName))
			{
				return Programs[ProgramIdx];
			}
		}
		return nullptr;
	}

	const EShaderProgram* EShaderAsset::FindProgram(const EName& InVariantName) const
	{
		for (UINT32 ProgramIdx = 0u, ProgramNum = Programs.Num<UINT32>(); ProgramIdx < ProgramNum; ProgramIdx++)
		{
			if (Programs[ProgramIdx] && (Programs[ProgramIdx]->GetVariantName() == InVariantName))
			{
				return Programs[ProgramIdx];
			}
		}
		return nullptr;
	}

	const EShaderProgram* EShaderAsset::GetProgramByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= Programs.Num<UINT32>())
		{
			return nullptr;
		}
		return Programs[InIndex];
	}

	EShaderProgram* EShaderAsset::EnsureProgram(const EName& InVariantName, const UINT32 InVariantIndex)
	{
		EShaderProgram* ExistingProgram = FindProgram(InVariantName);
		if (ExistingProgram)
		{
			return ExistingProgram;
		}
		EShaderProgram* NewProgram = new EShaderProgram(Name, InVariantName, InVariantIndex);
		Programs.Add(NewProgram);
		return NewProgram;
	}

	BOOL32 EShaderAsset::ComposeFromData(const EShaderAssetData& InAssetData, const EString& InAssetDirectory, const BOOL32 InIsReload)
	{
		// The entry json path travels inside the backup, and both entry points pass data that
		// carries it, so this covers a cold start and an import alike.
		EntryJsonPath = InAssetData.EntryJsonPath;

		EShaderStageAssetManager* StageAssetManager = EShaderStageAssetManager::GetManagerSingleton();

		UINT32 ComposedStageNum = 0u;
		for (UINT32 VariantIdx = 0u, VariantNum = InAssetData.Variants.Num<UINT32>(); VariantIdx < VariantNum; VariantIdx++)
		{
			const EShaderAssetVariantData& VariantData = InAssetData.Variants[VariantIdx];
			EShaderProgram* Program = EnsureProgram(VariantData.Name, VariantData.VariantIndex);
			if (!Program)
			{
				continue;
			}
			Program->SetVariantSource(VariantData.Defines, VariantData.DefineHash, VariantData.ReplaceHash);

			for (UINT32 RefIdx = 0u, RefNum = VariantData.StageRefs.Num<UINT32>(); RefIdx < RefNum; RefIdx++)
			{
				const EShaderAssetStageRef& StageRef = VariantData.StageRefs[RefIdx];
				const EShaderStageAsset* StageAsset = nullptr;
				if (!StageAssetManager->LoadShaderStageAsset(InAssetDirectory, StageRef.StageAssetName, StageAsset, InIsReload))
				{
					PE_LOG_ERROR(EString("[ShaderAsset] Variant '") + VariantData.Name.ToString()
						+ EString("': cannot load the stage asset '") + StageRef.StageAssetName + EString("'."));
					continue;
				}

				EShaderStageEntry* StageEntry = Program->EnsureStageEntry(StageRef.Stage);
				if (!StageEntry)
				{
					continue;
				}
				StageEntry->StageAsset = StageAsset;
				// Copied while the asset is alive, because it is the borrowed half of the entry: a
				// report of "this stage has no render resource" runs in the state where the object
				// behind the pointer may already have been replaced and released (see
				// EShaderStageEntry::StageAssetName).
				StageEntry->StageAssetName = StageAsset->GetAssetName();
				// The reflection (and with it the input layout) travels inside the stage
				// asset, so composing a stage is copying it into the entry.
				StageEntry->Reflection = StageAsset->GetReflection();
				StageEntry->BindingPlan.BuildFromReflection(StageEntry->Reflection, Program->IsBindlessVariant());
				StageEntry->bValid = ((StageAsset->IsValid()) ? TRUE : FALSE);
				if (StageEntry->bValid)
				{
					ComposedStageNum++;
				}
			}
		}

		BumpRevision();
		if (ComposedStageNum == 0u)
		{
			PE_LOG_WARN(EString("[ShaderAsset] '") + Name.ToString() + EString("' composed no usable stage."));
		}
		return TRUE;
	}

	EShaderParameterSet* EShaderAsset::GetOrCreateDefaultParameterSet(const EName& InVariantName)
	{
		EShaderParameterSet* ExistingSet = nullptr;
		if (DefaultParameterSets.FindValue(InVariantName, ExistingSet))
		{
			return ExistingSet;
		}

		const EShaderProgram* Program = FindProgram(InVariantName);
		if (!Program)
		{
			return nullptr;
		}

		EShaderParameterSet* NewSet = new EShaderParameterSet();
		NewSet->InitializeFromProgram(*Program);
		DefaultParameterSets.Add(InVariantName, NewSet);
		return NewSet;
	}

	const EShaderParameterSet* EShaderAsset::FindDefaultParameterSet(const EName& InVariantName) const
	{
		EShaderParameterSet* FoundSet = nullptr;
		EShaderAsset* MutableThis = const_cast<EShaderAsset*>(this);
		if (MutableThis->DefaultParameterSets.FindValue(InVariantName, FoundSet))
		{
			return FoundSet;
		}
		return nullptr;
	}

	// ---------------------------------------------------------------------------
	// EShaderAssetManager - cold backup IO
	// ---------------------------------------------------------------------------

	EShaderAssetManager::EShaderAssetManager()
		: ShaderAssets{}
	{
	}

	EShaderAssetManager::~EShaderAssetManager()
	{
		ShutDown();
	}

	void EShaderAssetManager::Initialize()
	{
	}

	void EShaderAssetManager::ShutDown()
	{
		ReleaseAllAssets();
	}

	void EShaderAssetManager::Uninit()
	{
		ReleaseAllAssets();
	}

	void EShaderAssetManager::ReleaseAllAssets()
	{
		ShaderAssets.Clear();
	}

	BOOL32 EShaderAssetManager::SaveShaderAssetData(const EString& InFullPathName, const EShaderAssetData& InAssetData)
	{
		EShaderPAssetWriter PAssetWriter;
		if (!WriteShaderPAssetHeader(PAssetWriter, static_cast<UINT32>(EShaderPAssetKindType::SHADER_PASSET_KIND_SHADER)))
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot write the shader asset header: ") + InFullPathName);
			return FALSE;
		}
		PAssetWriter.WriteString(InAssetData.ShaderName.ToString());
		PAssetWriter.WriteString(InAssetData.EntryJsonPath);
		PAssetWriter.WriteUInt32(InAssetData.Variants.Num<UINT32>());
		for (UINT32 VariantIdx = 0u, VariantNum = InAssetData.Variants.Num<UINT32>(); VariantIdx < VariantNum; VariantIdx++)
		{
			const EShaderAssetVariantData& VariantData = InAssetData.Variants[VariantIdx];
			PAssetWriter.WriteString(VariantData.Name.ToString());
			PAssetWriter.WriteUInt32(VariantData.VariantIndex);
			PAssetWriter.WriteString(VariantData.DefineHash);
			PAssetWriter.WriteString(VariantData.ReplaceHash);
			PAssetWriter.WriteUInt32(VariantData.Defines.Num<UINT32>());
			for (UINT32 DefIdx = 0u, DefNum = VariantData.Defines.Num<UINT32>(); DefIdx < DefNum; DefIdx++)
			{
				PAssetWriter.WriteString(VariantData.Defines[DefIdx].Name.ToString());
				PAssetWriter.WriteInt32(VariantData.Defines[DefIdx].Value);
			}
			PAssetWriter.WriteUInt32(VariantData.StageRefs.Num<UINT32>());
			for (UINT32 RefIdx = 0u, RefNum = VariantData.StageRefs.Num<UINT32>(); RefIdx < RefNum; RefIdx++)
			{
				const EShaderAssetStageRef& StageRef = VariantData.StageRefs[RefIdx];
				PAssetWriter.WriteUInt32(static_cast<UINT32>(StageRef.Stage));
				PAssetWriter.WriteUInt32(static_cast<UINT32>(StageRef.ShaderModel));
				PAssetWriter.WriteString(StageRef.StageAssetName);
			}
		}
		if (!PAssetWriter.SaveToFile(InFullPathName))
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot write the shader asset: ") + InFullPathName);
			return FALSE;
		}
		return TRUE;
	}

	BOOL32 EShaderAssetManager::LoadShaderAssetData(const EString& InFullPathName, EShaderAssetData& OutAssetData)
	{
		EShaderPAssetReader PAssetReader;
		if (!PAssetReader.OpenFromFile(InFullPathName))
		{
			return FALSE;
		}
		UINT32 PAssetKind = 0u;
		if (!ReadShaderPAssetHeader(PAssetReader, PAssetKind))
		{
			return FALSE;
		}
		if (PAssetKind != static_cast<UINT32>(EShaderPAssetKindType::SHADER_PASSET_KIND_SHADER))
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] '") + InFullPathName + EString("' does not hold a shader asset."));
			return FALSE;
		}

		EString ShaderNameStr;
		UINT32 VariantNum = 0u;
		if ((!PAssetReader.ReadString(ShaderNameStr))
			|| (!PAssetReader.ReadString(OutAssetData.EntryJsonPath))
			|| (!PAssetReader.ReadUInt32(VariantNum)))
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] The shader asset header is truncated: ") + InFullPathName);
			return FALSE;
		}
		OutAssetData.ShaderName = EName(ShaderNameStr);

		for (UINT32 VariantIdx = 0u; VariantIdx < VariantNum; VariantIdx++)
		{
			EShaderAssetVariantData VariantData;
			EString VariantNameStr;
			UINT32 DefineNum = 0u;
			UINT32 StageRefNum = 0u;
			if ((!PAssetReader.ReadString(VariantNameStr))
				|| (!PAssetReader.ReadUInt32(VariantData.VariantIndex))
				|| (!PAssetReader.ReadString(VariantData.DefineHash))
				|| (!PAssetReader.ReadString(VariantData.ReplaceHash))
				|| (!PAssetReader.ReadUInt32(DefineNum)))
			{
				PE_LOG_ERROR(EString("[ShaderAssetManager] A variant entry is truncated: ") + InFullPathName);
				return FALSE;
			}
			VariantData.Name = EName(VariantNameStr);
			for (UINT32 DefIdx = 0u; DefIdx < DefineNum; DefIdx++)
			{
				EShaderVariantDefine Define;
				EString DefineNameStr;
				if ((!PAssetReader.ReadString(DefineNameStr)) || (!PAssetReader.ReadInt32(Define.Value)))
				{
					PE_LOG_ERROR(EString("[ShaderAssetManager] A variant define is truncated: ") + InFullPathName);
					return FALSE;
				}
				Define.Name = EName(DefineNameStr);
				VariantData.Defines.Add(Define);
			}
			if (!PAssetReader.ReadUInt32(StageRefNum))
			{
				PE_LOG_ERROR(EString("[ShaderAssetManager] A variant stage list is truncated: ") + InFullPathName);
				return FALSE;
			}
			for (UINT32 RefIdx = 0u; RefIdx < StageRefNum; RefIdx++)
			{
				EShaderAssetStageRef StageRef;
				UINT32 StageValue = 0u;
				UINT32 ShaderModelValue = 0u;
				if ((!PAssetReader.ReadUInt32(StageValue))
					|| (!PAssetReader.ReadUInt32(ShaderModelValue))
					|| (!PAssetReader.ReadString(StageRef.StageAssetName)))
				{
					PE_LOG_ERROR(EString("[ShaderAssetManager] A stage reference is truncated: ") + InFullPathName);
					return FALSE;
				}
				StageRef.Stage = static_cast<EShaderStageType>(static_cast<UINT8>(StageValue));
				StageRef.ShaderModel = static_cast<EShaderModelType>(static_cast<UINT8>(ShaderModelValue));
				VariantData.StageRefs.Add(StageRef);
			}
			OutAssetData.Variants.Add(VariantData);
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// EShaderAssetManager - load
	// ---------------------------------------------------------------------------

	BOOL32 EShaderAssetManager::LoadShaderAsset(const EString& InLoadPath, const EString& InLoadName
		, const EShaderAsset*& OutAsset, const BOOL32 InIsReload)
	{
		OutAsset = nullptr;
		const EName AssetName(InLoadName);
		if (AssetName.IsNone())
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot load a shader asset without a name."));
			return FALSE;
		}

		if (!InIsReload)
		{
			EShaderAsset* CachedAsset = ShaderAssets.Find(AssetName);
			if (CachedAsset)
			{
				OutAsset = CachedAsset;
				return TRUE;
			}
		}

		const EString FullPathName = InLoadPath + InLoadName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		EShaderAssetData AssetData;
		if (!LoadShaderAssetData(FullPathName, AssetData))
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot read the shader asset: ") + FullPathName);
			return FALSE;
		}

		EShaderAsset* Asset = new EShaderAsset(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, EString(PE_TEXT("ShaderAsset")) + InLoadName
#endif
		);
		// Keep the cold backup: it is what InitResource restores from, and keeping it also means a
		// hot reload can re-compose later without re-reading the file.
		auto StoreColdBackup = [&AssetData]() -> EShaderAssetData*
		{
			return (new EShaderAssetData(AssetData));
		};
		if (!Asset->StorageResourceInternal(StoreColdBackup))
		{
			delete Asset;
			return FALSE;
		}
		/*
		* A new object every time, on purpose, and this is where that decision lives.
		*
		* A published asset belongs to every reader that holds a pointer into it: a draw walks its
		* programs, their stage entries, their binding plans and the reflection inside them. A
		* reload that rewrote any of that in place would rewrite it while a draw is walking it,
		* which is a race no lock fixes without turning every read into a locked read. Building a
		* new object and publishing it instead makes a reload a pointer swap: whoever is told about
		* it moves to the new object, whoever is still inside the old one finishes against the old
		* one, and the old one is freed by the retirement once the render thread has drained a
		* frame that started after the swap.
		*
		* InIsReload belongs to the call rather than to the asset: a cold start restores from the
		* backup it just stored, a reload composes from the same data and reloads the stage assets
		* it references, because the import that got here rewrote those too.
		*/
		const BOOL32 bComposed = (InIsReload
			? Asset->ComposeFromData(AssetData, InLoadPath, TRUE)
			: Asset->InitResource());
		if (!bComposed)
		{
			Asset->UninitResource();
			delete Asset;
			return FALSE;
		}
		// Published last, so a reader that finds it in the table finds a finished asset. An asset
		// this replaces leaves the table here and is parked by the table, not freed.
		if (ShaderAssets.Add(AssetName, Asset, (InIsReload ? TRUE : FALSE)) == 0u)
		{
#if _EDITOR_ONLY
			PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[ShaderAssetManager] The loaded shader could not be added to the cache.")));
#endif
			Asset->UninitResource();
			delete Asset;
			return FALSE;
		}

		OutAsset = Asset;

		if (InIsReload)
		{
			/*
			* The compose above reloaded the stage assets this shader references, and a stage asset is
			* shared: replacing one under its own name leaves every other shader asset that references it
			* composed against a released object. Moving those onto the new objects is this call's job -
			* it is the one that replaced them - and it has to happen now, before the retirement frees
			* what they point at. A reload that replaced nothing (the compiled content was unchanged)
			* drains an empty list and does nothing.
			*/
			TArray<EString> ReplacedStageAssets;
			EShaderStageAssetManager::GetManagerSingleton()->TakeReplacedStageAssetNames(ReplacedStageAssets);
			if ((ReplacedStageAssets.Num<UINT32>() > 0u)
				&& (!RepublishShadersReferencingStages(ReplacedStageAssets, AssetName)))
			{
				// The load itself succeeded, so the asset stays published; what failed is the move of a
				// reader onto the new stage objects, and that reader keeps pointers the retirement is
				// about to free. Per asset reasons are in the log above - this is the break.
				PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[ShaderAssetManager] A shader asset referencing a replaced stage asset could not be republished.")));
			}
		}
#if _EDITOR_ONLY
		PE_LOG_LOG(EString("[ShaderAssetManager] Loaded '") + InLoadName
			+ EString("' (revision ") + ToString(Asset->GetRevision())
			+ EString(", variants ") + ToString(Asset->GetProgramNum())
			+ EString(", culled fields ") + ToString(Asset->GetCulledFieldNum())
			+ EString(", exposed fields ") + ToString(Asset->GetExposedFieldNum()) + EString(")."));
#endif	// _EDITOR_ONLY
		return TRUE;
	}

	// Does this asset's cold backup reference any of the names? The backup is what a compose reads, so
	// it is also what decides whether a replacement behind one of those names concerns this asset. The
	// reference is by name there on purpose: a stage asset's identity is its name, and the pointer an
	// entry holds is resolved while composing, never stored in the backup.
	static BOOL32 DoesShaderReferenceStageAssets(const EShaderAsset& InAsset, const TArray<EString>& InStageAssetNames)
	{
		const EShaderAssetData* AssetData = InAsset.GetStoragedResource();
		if (!AssetData)
		{
			return FALSE;
		}
		for (UINT32 VariantIndex = 0u, VariantNum = AssetData->Variants.Num<UINT32>(); VariantIndex < VariantNum; VariantIndex++)
		{
			const TArray<EShaderAssetStageRef>& StageRefs = AssetData->Variants[VariantIndex].StageRefs;
			for (UINT32 RefIndex = 0u, RefNum = StageRefs.Num<UINT32>(); RefIndex < RefNum; RefIndex++)
			{
				if (InStageAssetNames.Contains(StageRefs[RefIndex].StageAssetName))
				{
					return TRUE;
				}
			}
		}
		return FALSE;
	}

	BOOL32 EShaderAssetManager::RepublishShadersReferencingStages(const TArray<EString>& InReplacedStageAssets, const EName& InSkipAssetName)
	{
		// Snapshot the table before rebuilding: a rebuild publishes into it, so the walk must not read
		// a table this loop writes.
		TArray<EShaderAsset*> CachedAssets;
		ShaderAssets.GenerateValueArray(CachedAssets);

		BOOL32 bAllRepublished = TRUE;
		TArray<EName> RepublishedNames;
		for (UINT32 AssetIndex = 0u, AssetNum = CachedAssets.Num<UINT32>(); AssetIndex < AssetNum; AssetIndex++)
		{
			EShaderAsset* CachedAsset = CachedAssets[AssetIndex];
			if ((!CachedAsset) || (CachedAsset->GetName() == InSkipAssetName))
			{
				continue;
			}
			if (!DoesShaderReferenceStageAssets(*CachedAsset, InReplacedStageAssets))
			{
				continue;
			}
			// One asset can reference two of the replaced names, and it only has to be rebuilt once.
			const EName CachedName = CachedAsset->GetName();
			if (RepublishedNames.Contains(CachedName))
			{
				continue;
			}
			RepublishedNames.Add(CachedName);
			if (!RebuildAndRepublishShaderAsset(*CachedAsset))
			{
				bAllRepublished = FALSE;
			}
		}
		return bAllRepublished;
	}

	BOOL32 EShaderAssetManager::RebuildAndRepublishShaderAsset(EShaderAsset& InAsset)
	{
		const EName AssetName = InAsset.GetName();
		const EString AssetNameString = AssetName.ToString();
		const EShaderAssetData* ColdData = InAsset.GetStoragedResource();
		if (!ColdData)
		{
			// Cannot happen for a published shader asset, which stores its backup before it publishes.
			// Reported rather than skipped, because the state it leaves is the one this step exists to
			// prevent: the asset keeps a pointer to a stage object that was replaced.
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot republish '") + AssetNameString
				+ EString("': it has no cold backup to rebuild from."));
			return FALSE;
		}

		// The backup is copied for the new object, while the published one still holds it: a rebuild is
		// a new asset rather than a re-read, so nothing here touches the disk.
		auto StoreColdBackup = [ColdData]() -> EShaderAssetData*
		{
			return (new EShaderAssetData(*ColdData));
		};
		EShaderAsset* NewAsset = new EShaderAsset(InAsset.GetAssetPath(), AssetNameString
#if _EDITOR_ONLY
			, EString(PE_TEXT("ShaderAsset")) + AssetNameString
#endif
		);
		if (!NewAsset->StorageResourceInternal(StoreColdBackup))
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot store the rebuilt cold backup of '") + AssetNameString + EString("'."));
			delete NewAsset;
			NewAsset = nullptr;
			return FALSE;
		}
		// Composed through InitResource, which restores from the backup just stored and does not reload
		// the stage assets: this compose resolves them from the cache, so the objects it picks up are
		// the current ones and nothing is replaced here - which is also what keeps this from cascading
		// into another republish.
		if (!NewAsset->InitResource())
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot compose the rebuild of '") + AssetNameString + EString("'."));
			NewAsset->UninitResource();
			delete NewAsset;
			NewAsset = nullptr;
			return FALSE;
		}
		if (ShaderAssets.Add(AssetName, NewAsset, TRUE) == 0u)
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] The rebuild of '") + AssetNameString + EString("' could not be published."));
			NewAsset->UninitResource();
			delete NewAsset;
			NewAsset = nullptr;
			return FALSE;
		}

		// The readers of the name that was just published: re-pointing the instances and telling the
		// render thread to drop what it derived from the previous object is exactly the step a reload of
		// this name runs. Leaving it out would leave the pointers this rebuild exists to move.
		REngineShaderInstanceCache::GetManagerSingleton()->PublishShaderAssetSwap(AssetName);
#if _EDITOR_ONLY
		PE_LOG_LOG(EString("[ShaderAssetManager] Republished '") + AssetNameString
			+ EString("': a stage asset it references was replaced."));
#endif	// _EDITOR_ONLY
		return TRUE;
	}
	BOOL32 EShaderAssetManager::RequireLoadShaderAsset(const EString& InLoadPath, const EString& InLoadName
		, const EShaderAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete)
	{
		OutAsset = nullptr;
		const EName AssetName(InLoadName);
		if (AssetName.IsNone())
		{
			return FALSE;
		}
		if (EShaderAsset* CachedAsset = ShaderAssets.Find(AssetName))
		{
			OutAsset = CachedAsset;
			return TRUE;
		}
		// The cache is keyed by name alone, but the request is keyed by the full path: two shader
		// assets of the same name in different directories are different files, and only one of
		// them can be the cached one.
		const EString FullPathName = InLoadPath + InLoadName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		EAssetLoadManager::GetManagerSingleton()->RequestLoad(
			MakeAssetLoadRequestKey(PE_TEXT("Shader"), FullPathName),
			[InLoadPath, InLoadName]() -> BOOL32
			{
				const EShaderAsset* LoadedAsset = nullptr;
				return EShaderAssetManager::GetManagerSingleton()->LoadShaderAsset(InLoadPath, InLoadName, LoadedAsset, FALSE);
			},
			static_cast<EAssetLoadCompleteType&&>(InOnLoadComplete));
		return FALSE;
	}

	EShaderAsset* EShaderAssetManager::FindShaderAsset(const EName& InAssetName)
	{
		return ShaderAssets.Find(InAssetName);
	}

	const EShaderAsset* EShaderAssetManager::FindShaderAsset(const EName& InAssetName) const
	{
		EShaderAsset* TempAsset = const_cast<EShaderAssetManager*>(this)->ShaderAssets.Find(InAssetName);
		return TempAsset;
	}

	UINT32 EShaderAssetManager::GetShaderAssetNum() const
	{
		return ShaderAssets.Size();
	}

	const EShaderProgram* EShaderAssetManager::FindProgram(const EName& InAssetName, const EName& InVariantName) const
	{
		const EShaderAsset* Asset = FindShaderAsset(InAssetName);
		if (!Asset)
		{
			return nullptr;
		}
		return Asset->FindProgram(InVariantName);
	}

#if _EDITOR_ONLY

	BOOL32 EShaderAssetManager::ImportStageRefs(const EString& InArtifactDirectory, const EShaderManifest& InManifest
		, const EShaderManifestVariant& InVariant, const EString& InSavePath, EShaderAssetStageRef& OutStageRef)
	{
		if ((InVariant.BinaryFile.Length() == 0u) || (InVariant.ReflectionFile.Length() == 0u))
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Variant '") + InVariant.Name.ToString()
				+ EString("' carries no binary / reflection name."));
			return FALSE;
		}

		// The stage asset name is the artifact stem: the binary and its reflection share
		// it, which is what makes one stage asset per (variant, stage) enough.
		const EString StageAssetNameStr = FileNameStemOf(InVariant.BinaryFile);
		const EString BytecodeFullPath = InArtifactDirectory + InVariant.BinaryFile;
		const EString ReflectionFullPath = InArtifactDirectory + InVariant.ReflectionFile;
		if (!EShaderStageAssetManager::GetManagerSingleton()->ImportShaderStageAsset(StageAssetNameStr
			, BytecodeFullPath, ReflectionFullPath, InManifest.Stage, InManifest.ShaderModel, InSavePath))
		{
			return FALSE;
		}

		OutStageRef.Stage = InManifest.Stage;
		OutStageRef.ShaderModel = InManifest.ShaderModel;
		OutStageRef.StageAssetName = StageAssetNameStr;
		return TRUE;
	}

	BOOL32 EShaderAssetManager::ImportShaderAssetFromManifest(const EString& InManifestFullPath
		, const EString& InSavePath, const EString& InSaveName)
	{
		EShaderManifest Manifest;
		if (!ParseShaderManifestJson(InManifestFullPath, Manifest))
		{
			return FALSE;
		}
		if (Manifest.ShaderName.IsNone())
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] The manifest declares an empty shader name: ") + InManifestFullPath);
			return FALSE;
		}

		const EString ArtifactDirectory = ParentDirectoryOf(InManifestFullPath);
		if (ArtifactDirectory.Length() == 0u)
		{
			PE_LOG_ERROR(EString("[ShaderAssetManager] Cannot resolve the artifact directory: ") + InManifestFullPath);
			return FALSE;
		}

		// Folding: read the existing cold backup first so earlier stages survive this
		// import. The compiler writes one manifest per stage.
		const EString PAssetFullPath = InSavePath + InSaveName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		EShaderAssetData AssetData;
		if (EFileHelper::IsFileExists(PAssetFullPath))
		{
			if (!LoadShaderAssetData(PAssetFullPath, AssetData))
			{
				PE_LOG_ERROR(EString("[ShaderAssetManager] The existing shader asset cannot be read, importing from scratch: ") + PAssetFullPath);
				AssetData = EShaderAssetData();
			}
		}
		if (AssetData.ShaderName.IsNone())
		{
			AssetData.ShaderName = Manifest.ShaderName;
		}
		if (AssetData.EntryJsonPath.Length() == 0u)
		{
			AssetData.EntryJsonPath = Manifest.EntryJsonPath;
		}

		UINT32 ImportedVariantNum = 0u;
		for (UINT32 VariantIdx = 0u, VariantNum = Manifest.Variants.Num<UINT32>(); VariantIdx < VariantNum; VariantIdx++)
		{
			const EShaderManifestVariant& Variant = Manifest.Variants[VariantIdx];
			if (!Variant.bSuccess)
			{
				// A failed variant is not imported; the caller sees it in the compiler log.
				PE_LOG_WARN(EString("[ShaderAssetManager] Variant '") + Variant.Name.ToString()
					+ EString("' of '") + Manifest.ShaderName.ToString() + EString("' is not imported: ") + Variant.Errors);
				continue;
			}

			EShaderAssetVariantData* VariantData = AssetData.FindVariant(Variant.Name);
			if (!VariantData)
			{
				EShaderAssetVariantData NewVariantData;
				NewVariantData.Name = Variant.Name;
				NewVariantData.VariantIndex = Variant.Index;
				AssetData.Variants.Add(NewVariantData);
				VariantData = AssetData.FindVariant(Variant.Name);
			}
			if (!VariantData)
			{
				continue;
			}
			VariantData->VariantIndex = Variant.Index;
			VariantData->DefineHash = Variant.DefineHash;
			VariantData->ReplaceHash = Variant.ReplaceHash;
			VariantData->Defines = Variant.Defines;

			EShaderAssetStageRef StageRef;
			if (!ImportStageRefs(ArtifactDirectory, Manifest, Variant, InSavePath, StageRef))
			{
				continue;
			}

			// Replace the stage this manifest carries, keep the other stages.
			BOOL32 bReplaced = FALSE;
			for (UINT32 RefIdx = 0u, RefNum = VariantData->StageRefs.Num<UINT32>(); RefIdx < RefNum; RefIdx++)
			{
				if (VariantData->StageRefs[RefIdx].Stage == StageRef.Stage)
				{
					VariantData->StageRefs[RefIdx] = StageRef;
					bReplaced = TRUE;
					break;
				}
			}
			if (!bReplaced)
			{
				VariantData->StageRefs.Add(StageRef);
			}
			ImportedVariantNum++;
		}

		if (!SaveShaderAssetData(PAssetFullPath, AssetData))
		{
			return FALSE;
		}

		PE_LOG_LOG(EString("[ShaderAssetManager] Imported '") + Manifest.ShaderName.ToString()
			+ EString("' stage ") + Manifest.StageSuffix
			+ EString(" (") + ToString(ImportedVariantNum) + EString(" variants)."));
		return TRUE;
	}

#endif	// _EDITOR_ONLY

} // namespace PigeonEngine
