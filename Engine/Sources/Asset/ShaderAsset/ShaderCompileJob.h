#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <ShaderAsset/ShaderProgram.h>

namespace PigeonEngine
{

	class EShaderAsset;

	/*
	* ShaderAsset - compile layer (phase A).
	*
	* A compile request carries everything the compiler needs, and nothing is derived
	* from ambient state: the caller (editor / engine code) decides which shader, which
	* stage, which variant and which target is wanted. Stages and variants are compile
	* variables, they are not read out of the shader source.
	*
	* One request compiles exactly one stage, because ShaderCompiler takes a single
	* stage per invocation and emits one variant set per invocation. Loading the
	* resulting manifest folds that stage into the shader asset, so compiling VS and PS
	* means two requests and two folds onto the same asset.
	*
	* Output layout (the compile description is a build time intermediate, it is
	* rewritten from the request on demand and is not part of the source tree):
	*     <OutputDir>/<BuildConfig>/<ShaderModel>/
	*         <ShaderName>_<StageSuffix>.Compile.json
	*         <ShaderName>_<StageSuffix>.Manifest.json
	*         <ShaderName>_<VariantName>_<StageSuffix>.dxil | .dxbc
	*         <ShaderName>_<VariantName>_<StageSuffix>.Reflection.json
	*/

	struct EShaderCompileRequest
	{
	public:
		// Base directory every relative path inside the shader source json chain is
		// resolved against. Empty means <Assets>/ShaderSources/.
		EString				RootDir;
		// Entry json, relative to RootDir, e.g. "M_DefaultLit/M_DefaultLit.json".
		EString				EntryJsonPath;
		// Root of the compiled output. Empty means the engine temp shader directory.
		EString				OutputDir;
		// Include search path. Empty means RootDir.
		EString				ShaderIncludeDir;
		EShaderStageType	Stage			= EShaderStageType::SHADER_STAGE_VERTEX;
		EShaderModelType	ShaderModel		= EShaderModelType::SHADER_MODEL_6_6;
		EShaderVariantDesc	Variant;
		BOOL32				bDebug			= FALSE;
		BOOL32				bDumpHlsl		= FALSE;
		// Compile even when an up to date manifest already exists.
		BOOL32				bForceRecompile	= FALSE;
		// Requests the engine bindless path. When the target can express it, the engine
		// macro declared in ShaderProgram.h is injected into this variant's defines, and
		// the runtime reads the binding mode back from the compiled manifest.
		BOOL32				bBindlessPath	= FALSE;
	};

	/*
	* Where a request's artifacts live. These are needed by both builds: a shipping
	* build reads the manifests a build step produced, without owning the compiler.
	*/
	// "Debug" / "DebugEditor" / "Develop" / "DevelopEditor" / "Shipping", derived from
	// the build macros so two configurations never share one output directory.
	PE_NODISCARD extern BOOL32	GetShaderBuildConfigName(EString& OutName);
	PE_NODISCARD extern BOOL32	GetShaderOutputDirectory(const EShaderCompileRequest& InRequest, EString& OutDirectory);
	// <ShaderName>_<StageSuffix>.Manifest.json inside the output directory. The
	// underscore belongs to the template, the stage suffix is a bare token.
	PE_NODISCARD extern BOOL32	GetShaderManifestPath(const EShaderCompileRequest& InRequest, EString& OutManifestPath);

	/*
	* Editor only: turns a request into artifacts. Nothing in here is needed by a
	* shipping build, which simply loads the manifests a build step produced.
	*/
#if _EDITOR_ONLY

	class EShaderCompileJob
	{
	public:
		PE_NODISCARD static BOOL32	GetCompileDescriptionJsonPath(const EShaderCompileRequest& InRequest, EString& OutCompileJsonPath);
		// v1 only checks that a manifest of this request exists; the dependency graph
		// (entry json chain, includes, compiler binary) is a follow up. Callers that
		// know better can always set bForceRecompile.
		PE_NODISCARD static BOOL32	IsUpToDate(const EShaderCompileRequest& InRequest);
		static BOOL32	WriteCompileDescriptionJson(const EShaderCompileRequest& InRequest, EString& OutCompileJsonPath);
		static BOOL32	RunShaderCompiler(const EString& InCompileJsonPath);
		static BOOL32	CompileShader(const EShaderCompileRequest& InRequest, EString& OutManifestPath);
	private:
		EShaderCompileJob() = delete;
	};

#endif	// _EDITOR_ONLY

	/*
	* Loads the shader asset for a request. In an editor build it compiles first when
	* needed; in a shipping build it only reads the manifest the build step produced.
	* Either way the result is folded into EShaderAssetManager, so repeated calls for
	* the same shader name return the same asset with one more stage entry.
	*/
	PE_NODISCARD extern BOOL32	TryLoadShaderAsset(const EShaderCompileRequest& InRequest, const EShaderAsset*& OutAsset);

} // namespace PigeonEngine
