#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RHI/RHITypes.h>
#include <ShaderAsset/ShaderProgram.h>
#include <ShaderAsset/ShaderReflection.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - the compile target.
	*
	* One target is a pair: the back end that consumes the artifacts, and the shader model
	* they are compiled at. The two axes are independent - the back end says which device
	* loads the artifact, the shader model says which language level it was built at - and
	* which pairs exist is a table in the implementation rather than a branch, so a new back
	* end or a new shader model is a table row.
	*
	* The configuration (EEngineSettings::ENGINE_GRAPHICS_API and ENGINE_SHADERMODEL_VERSION)
	* is read in exactly one place, ResolveActiveShaderTarget, and everything downstream
	* carries the resolved value instead of asking again. A request derives nothing from
	* ambient state, which is the rule ShaderCompileJob.h already states for the rest of it.
	*
	* Why the target is an axis: the macros below are injected as variant defines, so a
	* variant compiled for another target is a different variant, the manifest reports the
	* target it was built for, and the artifact directory is keyed on it. Nothing else can
	* tell a D3D12 artifact from a D3D11 one, because the manifest and the PAsset next to it
	* do not carry the target in their file names.
	*/

	// Engine macros the target axis injects into every variant. The complete set, one flag
	// per target, is declared for the shader side in Components/C_EngineSystem.json as well,
	// so the vocabulary a shader may read is visible where a shader author looks. The define
	// the compiler passes for the active variant is applied after that json and therefore
	// wins over the value the json declares.
	static constexpr const CHAR* SHADER_TARGET_DEFINE_PLATFORM	= PE_TEXT("PE_PLATFORM");
	static constexpr const CHAR* SHADER_TARGET_DEFINE_MODEL		= PE_TEXT("PE_SHADER_MODEL");

	// The compile target of one assembly. Both members default to NONE, so a target nobody
	// filled in is refused by IsShaderTargetLegal rather than silently compiling for
	// whichever pair happens to sit first in the table.
	struct EShaderTarget
	{
	public:
		ERHIBackendType			Backend		= ERHIBackendType::RHI_BACKEND_NONE;
		ERHIShaderModelType		ShaderModel	= ERHIShaderModelType::RHI_SM_NONE;
	};

	// TRUE when the pair is one this engine can build. Every back end declares the shader
	// models it consumes, and a pair outside that declaration is a configuration error
	// rather than a fallback. constexpr, so a build configured with an illegal pair fails to
	// compile instead of producing artifacts nothing can load.
	PE_NODISCARD constexpr BOOL32	IsShaderTargetLegal(const EShaderTarget& InTarget) noexcept;

	// Reads EEngineSettings and validates the result: the only place the configuration is
	// read, so the answer cannot change halfway through an assembly.
	PE_NODISCARD extern BOOL32	ResolveActiveShaderTarget(EShaderTarget& OutTarget) noexcept;

	// "D3D11" / "D3D12" and "5_0" / "6_0" / "6_6": the tokens a variant name carries, so one
	// name identifies one target. The model token is the compiler's own shader model string.
	PE_NODISCARD extern BOOL32	GetShaderTargetTokens(const EShaderTarget& InTarget
		, EString& OutPlatformToken, EString& OutShaderModelToken) noexcept;

	// The target macro set of one variant: a numeric macro per axis, plus the complete flag
	// set of both axes the way the vertex axis emits every VF_HAS_* flag. A flag that is
	// missing rather than 0 evaluates to 0 in a json input condition and silently culls what
	// it guards, so the set is always complete.
	//
	// Appends to InOutDefines: the caller owns the array it is building.
	PE_NODISCARD extern BOOL32	MakeShaderTargetDefines(const EShaderTarget& InTarget
		, TArray<EShaderVariantDefine>& InOutDefines) noexcept;

	// The compile layer's own shader model vocabulary for the same target, for the compile
	// request and for every artifact name that is keyed on the model.
	PE_NODISCARD extern BOOL32	TranslateShaderTargetToCompileModel(const EShaderTarget& InTarget
		, EShaderModelType& OutShaderModel) noexcept;

	// TRUE when the target can carry the engine bindless path: the shader side of that path
	// indexes the descriptor heap directly, and direct heap indexing is a shader model 6.6
	// feature, so an earlier model cannot express it. This is the engine's single bindless
	// switch - the configured shader model decides, nothing else - and the assembly layer is
	// what turns the answer into the compile request's bindless flag. Enabling it also means
	// the target's root signatures carry no descriptor tables, so the binding side has to feed
	// heap indices instead of table slots (see ShaderBindless.hlsl for that half).
	PE_NODISCARD extern BOOL32	DoesShaderTargetSupportBindless(const EShaderTarget& InTarget) noexcept;

	// "<Backend>_ShaderModel_<Model>", the output directory level, e.g. "D3D12_ShaderModel_6_6".
	// Both axes are in the name because the files inside the directory are not keyed on the
	// target: two targets sharing one directory would overwrite each other's manifest.
	PE_NODISCARD extern BOOL32	GetShaderTargetDirectoryName(const EShaderTarget& InTarget
		, EString& OutDirectoryName) noexcept;

} // namespace PigeonEngine
