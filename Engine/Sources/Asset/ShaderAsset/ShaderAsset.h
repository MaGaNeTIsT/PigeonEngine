#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
#include <ShaderAsset/ShaderPAssetIO.h>
#include <ShaderAsset/ShaderReflection.h>
#include <ShaderAsset/ShaderProgram.h>
#include <ShaderAsset/ShaderParameterSet.h>
#include <ShaderAsset/ShaderStageAsset.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - the aggregate: manifest contract, cold backup and runtime object.
	*
	* Data flow, the same three steps the other assets use:
	*   Import (editor)  reads the compiler artifacts (manifest + reflection + binary),
	*                    writes one stage PAsset per (variant, stage) and folds the
	*                    references into the shader PAsset.
	*   Load   (runtime) reads the shader PAsset, restores the cold backup and composes
	*                    the runtime object: stage entries with parsed reflections and
	*                    binding plans.
	*   TryLoad (ShaderCompileJob.h) editor only: compile when needed, import, load.
	*
	* The cold backup holds no bytecode: a stage binary is its own asset, shared by name
	* across shader assets, and the reflection travels with it (the reflection file and
	* the binary share the artifact stem). The shader PAsset therefore only stores the
	* shader's identity plus, per variant and stage, the name of the stage asset to load.
	*
	* One manifest is written per stage by the compiler, so importing a two stage shader
	* means two Import calls onto the same PAsset; each folds its stage into the variants.
	*/

	struct EShaderManifestVariant
	{
	public:
		UINT32							Index			= 0u;
		EName							Name;
		// Compiler identities of the variant, kept verbatim for logging and caches.
		EString							DefineHash;
		EString							ReplaceHash;
		TArray<EShaderVariantDefine>	Defines;
		EString							BinaryFile;
		EString							ReflectionFile;
		EString							PdbFile;
		EString							Errors;
		BOOL32							bSuccess		= FALSE;
	};

	struct EShaderManifest
	{
	public:
		EName							ShaderName;
		EName							EntryPoint;
		EShaderStageType				Stage		= EShaderStageType::SHADER_STAGE_VERTEX;
		EString							StageSuffix;
		EShaderModelType				ShaderModel	= EShaderModelType::SHADER_MODEL_6_6;
		EString							EntryJsonPath;
		TArray<EShaderManifestVariant>	Variants;
	public:
		PE_NODISCARD const EShaderManifestVariant*	FindVariant(const EName& InVariantName) const;
	};

	PE_NODISCARD extern BOOL32	ParseShaderManifestJson(const EString& InManifestJsonPath, EShaderManifest& OutManifest);
	PE_NODISCARD extern BOOL32	ParseShaderManifestText(const EString& InManifestJsonText, EShaderManifest& OutManifest);

	// One stage of one variant, as the shader PAsset stores it.
	struct EShaderAssetStageRef
	{
	public:
		EShaderStageType	Stage			= EShaderStageType::SHADER_STAGE_VERTEX;
		EShaderModelType	ShaderModel		= EShaderModelType::SHADER_MODEL_6_6;
		EString				StageAssetName;
	};

	struct EShaderAssetVariantData
	{
	public:
		EName							Name;
		UINT32							VariantIndex	= 0u;
		EString							DefineHash;
		EString							ReplaceHash;
		TArray<EShaderVariantDefine>	Defines;
		TArray<EShaderAssetStageRef>	StageRefs;
	public:
		PE_NODISCARD const EShaderAssetStageRef*	FindStageRef(const EShaderStageType InStage) const;
	};

	// Cold backup of a shader asset: identity plus references, no binary payload.
	class EShaderAssetData : public EResourceInterface
	{
	public:
		EName								ShaderName;
		EString								EntryJsonPath;
		TArray<EShaderAssetVariantData>		Variants;
	public:
		PE_NODISCARD EShaderAssetVariantData*		FindVariant(const EName& InVariantName);
		PE_NODISCARD const EShaderAssetVariantData*	FindVariant(const EName& InVariantName) const;
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	};

	/*
	* A shader asset owns no GPU object of its own, so it derives from the game side asset base
	* directly rather than from TRenderBaseAsset: every stage's device object belongs to the
	* EShaderStageAsset a stage reference points at, and the parameter buffers belong to the
	* EShaderInstance that created them. What it does own is the cold backup, the composed
	* programs, and the asset level default parameter sets.
	*/
	class EShaderAsset : public TBaseAsset<EShaderAssetData>
	{
	public:
		EShaderAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EShaderAsset() override;
	public:
		// The asset cache keys on the hashed name; the base carries the string form. Both are
		// built from the same string, so a cache and a display path cannot disagree.
		PE_NODISCARD const EName&	GetName() const { return Name; }
		PE_NODISCARD const EString&	GetEntryJsonPath() const { return EntryJsonPath; }
		PE_NODISCARD UINT32			GetRevision() const { return Revision; }
		PE_NODISCARD UINT32			GetProgramNum() const { return Programs.Num<UINT32>(); }
		PE_NODISCARD UINT32			GetStageMask() const;
		PE_NODISCARD BOOL32			HasStage(const EShaderStageType InStage) const;
		PE_NODISCARD UINT32			GetCulledFieldNum() const;
		PE_NODISCARD UINT32			GetExposedFieldNum() const;
	public:
		void	BumpRevision() { Revision++; }
	public:
		// A shader asset is usable once its cold backup is parsed and at least one stage
		// composed. It never waits for a render object, because it owns none.
		virtual BOOL32	IsValid() const override { return IsResourceValid(); }
		// Restores the runtime objects from the cold backup the load stored: this is the
		// "restore and compose" half of a load, and the only place that reads ResourceData.
		virtual BOOL32	InitResource() override;
	public:
		EShaderProgram*			FindProgram(const EName& InVariantName);
		PE_NODISCARD const EShaderProgram*	FindProgram(const EName& InVariantName) const;
		PE_NODISCARD const EShaderProgram*	GetProgramByIndex(const UINT32 InIndex) const;
		// Creates the program when the variant is new, else returns the existing one.
		// The returned pointer stays valid for the lifetime of the asset.
		EShaderProgram*			EnsureProgram(const EName& InVariantName, const UINT32 InVariantIndex);
	public:
		// Composes the runtime object from a cold backup: loads the referenced stage
		// assets (binary + reflection + device object) and builds one stage entry per
		// reference. InitResource drives it from the stored backup; the import path calls it
		// directly with the data it just folded in, before a backup exists.
		BOOL32	ComposeFromData(const EShaderAssetData& InAssetData, const EString& InAssetDirectory, const BOOL32 InIsReload);
	public:
		// Asset level default parameter set of one variant. Every user that references
		// the asset directly shares this set; instances keep their own set and point
		// their parent at it. It is created lazily, per variant, because two variants
		// can legitimately have different reflections.
		EShaderParameterSet*		GetOrCreateDefaultParameterSet(const EName& InVariantName);
		PE_NODISCARD const EShaderParameterSet*	FindDefaultParameterSet(const EName& InVariantName) const;
		PE_NODISCARD UINT32			GetDefaultParameterSetNum() const { return DefaultParameterSets.Num<UINT32>(); }
	private:
		// Hashed form of the base's AssetName, kept because the asset cache and every stage
		// reference lookup are keyed on it.
		EName									Name;
		EString									EntryJsonPath;
		UINT32									Revision;
		TArray<EShaderProgram*>					Programs;
		THashMap<EName, EShaderParameterSet*>	DefaultParameterSets;

	public:
		EShaderAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EShaderAsset)

	};

	class EShaderAssetManager : public EManagerBase
	{
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		virtual void	Uninit()override;
	public:
		// Reads <InLoadPath><InLoadName>.PAsset, restores the cold backup and composes
		// the runtime object. Runtime and editor.
		BOOL32	LoadShaderAsset(const EString& InLoadPath, const EString& InLoadName
			, const EShaderAsset*& OutAsset, const BOOL32 InIsReload = FALSE);
		/*
		* Any thread: the shader asset when it is already cached, otherwise FALSE and a null
		* output after recording one load request for the load thread (EAssetLoadManager). Never
		* blocks and never reads the disk on the calling thread; ask again next frame.
		*
		* A reload is deliberately not offered in this form: it replaces an asset the caller
		* already holds a pointer to, which only the load thread may do, so it stays on Load*.
		* InOnLoadComplete runs on the load thread once the request is done; see
		* EAssetLoadCompleteType.
		*/
		BOOL32	RequireLoadShaderAsset(const EString& InLoadPath, const EString& InLoadName
			, const EShaderAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete = nullptr);
		EShaderAsset*			FindShaderAsset(const EName& InAssetName);
		const EShaderAsset*		FindShaderAsset(const EName& InAssetName)const;
		PE_NODISCARD UINT32		GetShaderAssetNum()const;
		PE_NODISCARD const EShaderProgram*	FindProgram(const EName& InAssetName, const EName& InVariantName)const;
#if _EDITOR_ONLY
		// Folds one compiled stage manifest into the shader PAsset, importing the stage
		// binaries it references. A full shader is imported by calling this once per
		// stage; earlier stages stay in the PAsset because the existing cold backup is
		// read back before folding.
		BOOL32	ImportShaderAssetFromManifest(const EString& InManifestFullPath
			, const EString& InSavePath, const EString& InSaveName);
#endif	// _EDITOR_ONLY
	private:
		BOOL32	LoadShaderAssetData(const EString& InFullPathName, EShaderAssetData& OutAssetData);
		BOOL32	SaveShaderAssetData(const EString& InFullPathName, const EShaderAssetData& InAssetData);
		/*
		* Rebuilds and republishes every published shader asset that references one of
		* InReplacedStageAssets, skipping InSkipAssetName (the asset whose load replaced them, which is
		* already composed against the new objects).
		*
		* A stage asset is shared across shader assets, so replacing one under its own name leaves every
		* other shader that references it composed against a released object: its programs would draw
		* through a stage asset that is gone, and the reflection and binding plan copied into its stage
		* entries would no longer describe the stage they claim to. The rebuild is the same "new object,
		* publish, retire the previous one" shape LoadShaderAsset uses, and it composes from the cache
		* rather than as a reload, so it cannot replace anything itself; each republished asset's
		* instances are re-pointed and the render thread is told to drop what it derived, which is the
		* step a reload of that name runs.
		*
		* Returns FALSE when a referencing asset could not be rebuilt. That is a hard error rather than
		* a log line: the asset keeps pointers into an object the retirement is about to free.
		*/
		PE_NODISCARD BOOL32	RepublishShadersReferencingStages(const TArray<EString>& InReplacedStageAssets, const EName& InSkipAssetName);
		PE_NODISCARD BOOL32	RebuildAndRepublishShaderAsset(EShaderAsset& InAsset);
		// Releases every asset this manager caches. Shared by ShutDown and Uninit so the two
		// paths cannot drift apart.
		void	ReleaseAllAssets();
#if _EDITOR_ONLY
		BOOL32	ImportStageRefs(const EString& InArtifactDirectory, const EShaderManifest& InManifest
			, const EShaderManifestVariant& InVariant, const EString& InSavePath, EShaderAssetStageRef& OutStageRef);
#endif	// _EDITOR_ONLY
	private:
		typedef TAssetManager<EName, EShaderAsset>	EShaderAssetCache;
	private:
		EShaderAssetCache	ShaderAssets;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EShaderAssetManager)

	};

} // namespace PigeonEngine
