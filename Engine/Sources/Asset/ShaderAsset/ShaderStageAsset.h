#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
#include <ShaderAsset/ShaderPAssetIO.h>
#include <ShaderAsset/ShaderReflection.h>
#include <Shader/ShaderStageResource.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - one compiled stage: a cold backup plus the device object.
	*
	* A stage asset is the binary identity of a shader. Its name is the artifact stem
	* the compiler produced (<ShaderName>_<VariantName>_<StageSuffix>), which is unique
	* engine wide, so the same stage is shared by every shader asset that references it.
	*
	* Following the other assets, the PAsset is a cold backup holding only what the
	* compiler produced: the bytecode and the reflection text. Loading restores that
	* and composes what the engine actually needs: the parsed reflection and the device
	* object. The cold backup stays alive for the lifetime of the asset, because the
	* device object is built from it on the render thread, after this call returns.
	*
	* Entry points:
	*   Import*  editor only, converts a compiled artifact into a PAsset.
	*   Load*    runtime and editor, reads a PAsset and builds the runtime object.
	*   TryLoad* (ShaderCompileJob.h) editor convenience: compile, import, load.
	*/

	// Cold backup of one stage: bytecode and reflection text, exactly as they came out
	// of the compiler.
	class EShaderStageData : public EResourceInterface
	{
	public:
		EShaderStageType	Stage = EShaderStageType::SHADER_STAGE_VERTEX;
		EShaderModelType	ShaderModel = EShaderModelType::SHADER_MODEL_6_6;
		TArray<BYTE>		Bytecode;
		EString				ReflectionText;
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	};

	class EShaderStageAsset : public TBaseAsset<EShaderStageData>
	{
	public:
		EShaderStageAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EShaderStageAsset();
	public:
		// Parses the reflection and creates the device object. The reflection has to be
		// parsed first: the vertex shader object needs the input layout it describes.
		virtual BOOL32	InitResource()override;
	public:
		// Render thread only: the proxy this asset's cold data was turned into, or null while
		// the creation command is still queued.
		PE_NODISCARD const RShaderStageResource*	GetRenderResource()const
		{
			return static_cast<const RShaderStageResource*>(RenderBinding.Resolve());
		}
		PE_NODISCARD BOOL32	IsRenderResourceValid()const
		{
			const RShaderStageResource* Resource = GetRenderResource();
			return ((!!Resource) && (Resource->IsRenderResourceValid()));
		}
		virtual void	UninitResource()override
		{
			TBaseAsset<EShaderStageData>::UninitResource();
			RenderBinding.Release();
		}
	public:
		PE_NODISCARD EShaderStageType	GetStage()const { return Stage; }
		PE_NODISCARD EShaderModelType	GetShaderModel()const { return ShaderModel; }
		PE_NODISCARD BOOL32				HasReflection()const { return bHasReflection; }
		PE_NODISCARD const EShaderReflection&	GetReflection()const { return Reflection; }
		PE_NODISCARD UINT32				GetRevision()const { return Revision; }
		// Identity of this stage's compiled input. The compiler emits no hash, so the
		// engine derives it: the reflection text hash mixed with the bytecode. Useful as
		// a cache / hot reload key; it is recomputed on every load, never stored.
		PE_NODISCARD UINT64				GetSourceHash()const { return SourceHash; }
		PE_NODISCARD const RShaderStageResource*	GetShaderStageResource()const { return GetRenderResource(); }
	protected:
		// Runs on the render thread. Everything it needs is passed in: the cold backup plus
		// the reflection snapshot taken where the creation command was enqueued, because a
		// queued creation callback must not read asset members.
		static RShaderStageResource*	CreateShaderStageResource(EShaderStageData* InResource
			, const TArray<EShaderReflectionInputLayout>& InInputLayouts, const EString& InAssetName);
	private:
		// The single connection between this asset and its render side proxy.
		RAssetRenderBinding	RenderBinding;
		EShaderStageType	Stage;
		EShaderModelType	ShaderModel;
		EShaderReflection	Reflection;
		BOOL32				bHasReflection;
		UINT32				Revision;
		UINT64				SourceHash;

	public:
		EShaderStageAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EShaderStageAsset)

	};

	/*
	* Global stage asset cache: two shader assets that reference the same variant and
	* stage hit the same instance, which is what makes the binaries shared.
	*/
	class EShaderStageAssetManager : public EManagerBase
	{
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		virtual void	Uninit()override;
	public:
		// Reads <InLoadPath><InLoadName>.PAsset, restores the cold backup and builds the
		// device object. The asset is returned from the cache unless InIsReload asks for
		// a fresh instance. Runtime and editor.
		BOOL32	LoadShaderStageAsset(const EString& InLoadPath, const EString& InLoadName
			, const EShaderStageAsset*& OutAsset, const BOOL32 InIsReload = FALSE);
		/*
		* Any thread: the stage asset when it is already cached, otherwise FALSE and a null output
		* after recording one load request for the load thread (EAssetLoadManager). Never blocks
		* and never reads the disk on the calling thread; ask again next frame.
		*
		* A reload is deliberately not offered in this form: it replaces an asset the caller
		* already holds a pointer to, which only the load thread may do, so it stays on Load*.
		* InOnLoadComplete runs on the load thread once the request is done; see
		* EAssetLoadCompleteType.
		*/
		BOOL32	RequireLoadShaderStageAsset(const EString& InLoadPath, const EString& InLoadName
			, const EShaderStageAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete = nullptr);
#if _EDITOR_ONLY
		// Converts a compiled artifact (bytecode file + reflection json) into a stage
		// PAsset. This is the import step; Load never needs the artifact tree.
		BOOL32	ImportShaderStageAsset(const EString& InAssetName
			, const EString& InImportBytecodeFullPath, const EString& InImportReflectionFullPath
			, const EShaderStageType InStage, const EShaderModelType InShaderModel
			, const EString& InSavePath);
#endif	// _EDITOR_ONLY
		EShaderStageAsset*			FindShaderStageAsset(const EName& InAssetName);
		const EShaderStageAsset*	FindShaderStageAsset(const EName& InAssetName)const;
		PE_NODISCARD UINT32			GetShaderStageAssetNum()const;
		/*
		* The names this manager replaced under their own name since the previous call, drained into
		* OutNames.
		*
		* A stage asset is shared, so a replacement is not the private matter of the load that caused
		* it: every shader asset that references the name is left composed against a released object
		* and has to be moved onto the new one. This list is the only record of which names actually
		* changed - a reload whose compiled content is unchanged replaces nothing (see
		* LoadShaderStageAsset) - so whoever drove the load drains it once that load's compose is done.
		*/
		void	TakeReplacedStageAssetNames(TArray<EString>& OutNames);
	private:
		// Called where an existing object is about to be swapped out. Records the name the readers
		// of the old object are keyed on, which is the one the file and the shader assets spell.
		void	RecordReplacedStageAssetName(const EString& InAssetName);
		BOOL32	SaveShaderStagePAsset(const EString& InSavePath, const EString& InSaveName, const EShaderStageData& InStageData);

		// Releases every asset this manager caches. Shared by ShutDown and Uninit so the two
		// paths cannot drift apart.
		void	ReleaseAllAssets();
	private:
		typedef TAssetManager<EName, EShaderStageAsset>	EShaderStageAssetCache;
	private:
		EShaderStageAssetCache	ShaderStageAssets;
		// Guarded like the cache table, for the same reason: the load thread records inside a load
		// and drains right after that load's compose, but the list must never be read half appended.
		EMutex					ReplacedNamesMutex;
		TArray<EString>			ReplacedStageAssetNames;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EShaderStageAssetManager)

	};

} // namespace PigeonEngine
