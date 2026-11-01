#include "ShaderTarget.h"

#include <Config/EngineConfig.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// Target tables
	//
	// Data, not branches. A new back end is a row here plus its enum member, and a new
	// shader model is a row here plus its enum member and the string the compiler side
	// already spells (TranslateShaderModelToString). Nothing below decides anything by
	// naming a back end or a version, which is what keeps the legality check, the macro
	// values, the variant tokens and the artifact directory in step by construction.
	// ---------------------------------------------------------------------------

	// The mask bit of one shader model, derived from its enum member so two models cannot be
	// given the same bit. A value the enum does not define gets no bit at all, which makes it
	// illegal for every back end through the same mask test that refuses an illegal pair,
	// instead of shifting it out of range. RHI_SM_NONE is a real member and does get a bit; no
	// platform grants it, which is exactly why a target naming no shader model is illegal.
	PE_NODISCARD static constexpr UINT32 MakeShaderModelMaskBit(const ERHIShaderModelType InShaderModel) noexcept
	{
		if (static_cast<UINT32>(InShaderModel) >= static_cast<UINT32>(ERHIShaderModelType::RHI_SM_COUNT))
		{
			return 0u;
		}
		return (1u << static_cast<UINT32>(InShaderModel));
	}

	// One row per back end. Token names it in a variant name and in its flag macro;
	// ShaderModelMask lists the shader models the back end consumes, and that mask is the
	// entire rule about which pairs exist.
	struct EShaderTargetPlatformInfo
	{
	public:
		ERHIBackendType		Backend;
		const CHAR*			Token;
		UINT32				ShaderModelMask;
	};

	// One row per shader model. CompileModel is the compile layer's vocabulary for it; the
	// token and the numeric macro value are derived from TranslateShaderModelToString, so a
	// shader model's name is spelled once in the engine and everything else follows.
	//
	// bSupportsBindless is a capability of the model rather than of a back end: the shader side
	// of the bindless path indexes the descriptor heap directly (ResourceDescriptorHeap), and
	// direct heap indexing is a 6.6 feature, so an earlier model cannot express that path at
	// all. This column is the engine's single bindless switch (see DoesShaderTargetSupportBindless).
	struct EShaderTargetModelInfo
	{
	public:
		ERHIShaderModelType	ShaderModel;
		EShaderModelType	CompileModel;
		BOOL32				bSupportsBindless;
	};

	PE_CONSTEXPR_STATIC EShaderTargetPlatformInfo SHADER_TARGET_PLATFORMS[] =
	{
		{ ERHIBackendType::RHI_BACKEND_D3D11,	PE_TEXT("D3D11"),	MakeShaderModelMaskBit(ERHIShaderModelType::RHI_SM_5_0) },
		{ ERHIBackendType::RHI_BACKEND_D3D12,	PE_TEXT("D3D12"),	(MakeShaderModelMaskBit(ERHIShaderModelType::RHI_SM_6_0)
			| MakeShaderModelMaskBit(ERHIShaderModelType::RHI_SM_6_6)) },
	};

	PE_CONSTEXPR_STATIC EShaderTargetModelInfo SHADER_TARGET_MODELS[] =
	{
		{ ERHIShaderModelType::RHI_SM_5_0,	EShaderModelType::SHADER_MODEL_5_0,	FALSE },
		{ ERHIShaderModelType::RHI_SM_6_0,	EShaderModelType::SHADER_MODEL_6_0,	FALSE },
		{ ERHIShaderModelType::RHI_SM_6_6,	EShaderModelType::SHADER_MODEL_6_6,	TRUE },
	};

	constexpr BOOL32 IsShaderTargetLegal(const EShaderTarget& InTarget) noexcept
	{
		for (const EShaderTargetPlatformInfo& PlatformInfo : SHADER_TARGET_PLATFORMS)
		{
			if (PlatformInfo.Backend == InTarget.Backend)
			{
				return (((PlatformInfo.ShaderModelMask & MakeShaderModelMaskBit(InTarget.ShaderModel)) != 0u) ? TRUE : FALSE);
			}
		}
		// No row for this back end: it is not one the engine builds for.
		return FALSE;
	}

	// The pair this build is configured for. Both members are compile time constants, so the
	// target is a constant too and the assert below is evaluated while the engine is built.
	PE_CONSTEXPR_STATIC EShaderTarget SHADER_TARGET_CONFIGURED =
	{
		static_cast<ERHIBackendType>(EEngineSettings::ENGINE_GRAPHICS_API),
		static_cast<ERHIShaderModelType>(EEngineSettings::ENGINE_SHADERMODEL_VERSION)
	};

	/*
	* A build configured with a pair no device can consume is refused here rather than
	* discovered later as an artifact nothing loads. One example the tables encode: D3D11 has
	* no DXIL profile, so SM6 on it is illegal. Changing a table row changes this check with
	* it, and the message below is what a configuration mistake reads as.
	*/
	static_assert(IsShaderTargetLegal(SHADER_TARGET_CONFIGURED)
		, "ENGINE_GRAPHICS_API and ENGINE_SHADERMODEL_VERSION are not a target this engine can build.");

	// ---------------------------------------------------------------------------
	// Target resolution
	// ---------------------------------------------------------------------------

	PE_NODISCARD static const EShaderTargetPlatformInfo* FindShaderTargetPlatform(const ERHIBackendType InBackend) noexcept
	{
		for (const EShaderTargetPlatformInfo& PlatformInfo : SHADER_TARGET_PLATFORMS)
		{
			if (PlatformInfo.Backend == InBackend)
			{
				return (&PlatformInfo);
			}
		}
		return nullptr;
	}

	// Both configured numbers, for the messages that have to name what was configured.
	PE_NODISCARD static EString MakeConfiguredTargetText() noexcept
	{
		return (EString(PE_TEXT("[ShaderTarget] ENGINE_GRAPHICS_API is ")) + ToString(static_cast<UINT32>(EEngineSettings::ENGINE_GRAPHICS_API))
			+ EString(PE_TEXT(" and ENGINE_SHADERMODEL_VERSION is ")) + ToString(static_cast<UINT32>(EEngineSettings::ENGINE_SHADERMODEL_VERSION)));
	}

	BOOL32 ResolveActiveShaderTarget(EShaderTarget& OutTarget) noexcept
	{
		// The configuration stores the enum member itself, so the two casts are the whole
		// translation, and this is the only place the configured values are judged. There is no
		// separate range check on purpose: a value no member defines has no platform row and no
		// mask bit, so the table refuses it through the same check, and with the same message,
		// as a pair that is simply not legal. The message names both configured numbers, so a
		// value that is out of range is still spelled out where it is reported.
		EShaderTarget ResolvedTarget;
		ResolvedTarget.Backend = static_cast<ERHIBackendType>(EEngineSettings::ENGINE_GRAPHICS_API);
		ResolvedTarget.ShaderModel = static_cast<ERHIShaderModelType>(EEngineSettings::ENGINE_SHADERMODEL_VERSION);
		if (!IsShaderTargetLegal(ResolvedTarget))
		{
			PE_LOG_ERROR(MakeConfiguredTargetText() + EString(PE_TEXT(", which is not a pair this engine builds for; see the platform table.")));
			return FALSE;
		}

		OutTarget = ResolvedTarget;
		return TRUE;
	}

	BOOL32 TranslateShaderTargetToCompileModel(const EShaderTarget& InTarget, EShaderModelType& OutShaderModel) noexcept
	{
		for (const EShaderTargetModelInfo& ModelInfo : SHADER_TARGET_MODELS)
		{
			if (ModelInfo.ShaderModel == InTarget.ShaderModel)
			{
				OutShaderModel = ModelInfo.CompileModel;
				return TRUE;
			}
		}
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderTarget] The target names a shader model the compile layer has no vocabulary for.")));
		return FALSE;
	}

	BOOL32 DoesShaderTargetSupportBindless(const EShaderTarget& InTarget) noexcept
	{
		// A target nobody filled in, or one naming a model the table does not have, does not
		// support it: the answer is a property of the model, and an unknown model has none.
		// No log here on purpose - "this target does not use the bindless path" is the normal
		// answer for every target below 6.6, not a failure.
		for (const EShaderTargetModelInfo& ModelInfo : SHADER_TARGET_MODELS)
		{
			if (ModelInfo.ShaderModel == InTarget.ShaderModel)
			{
				return ModelInfo.bSupportsBindless;
			}
		}
		return FALSE;
	}

	BOOL32 GetShaderTargetTokens(const EShaderTarget& InTarget
		, EString& OutPlatformToken, EString& OutShaderModelToken) noexcept
	{
		const EShaderTargetPlatformInfo* PlatformInfo = FindShaderTargetPlatform(InTarget.Backend);
		if (!PlatformInfo)
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderTarget] The target names a back end this engine does not build for.")));
			return FALSE;
		}

		EShaderModelType CompileModel = EShaderModelType::SHADER_MODEL_5_0;
		if (!TranslateShaderTargetToCompileModel(InTarget, CompileModel))
		{
			return FALSE;
		}
		// The model token is the compiler's own shader model string ("6_6"), which is the
		// name its profile, its artifact names and the directory level already use.
		if (!TranslateShaderModelToString(CompileModel, OutShaderModelToken))
		{
			return FALSE;
		}

		OutPlatformToken = EString(PlatformInfo->Token);
		return TRUE;
	}

	BOOL32 GetShaderTargetDirectoryName(const EShaderTarget& InTarget, EString& OutDirectoryName) noexcept
	{
		const EShaderTargetPlatformInfo* PlatformInfo = FindShaderTargetPlatform(InTarget.Backend);
		EShaderModelType CompileModel = EShaderModelType::SHADER_MODEL_5_0;
		if ((!PlatformInfo) || (!TranslateShaderTargetToCompileModel(InTarget, CompileModel)))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderTarget] The target has no artifact directory: it names no back end or no shader model.")));
			return FALSE;
		}

		// The shader model level is the one ShaderReflection already names, so an artifact
		// directory stays "ShaderModel_6_6" and only gains the back end in front of it.
		EString ShaderModelDirectoryName;
		if (!GetShaderModelDirectoryName(CompileModel, ShaderModelDirectoryName))
		{
			return FALSE;
		}
		OutDirectoryName = EString(PlatformInfo->Token) + EString(PE_TEXT("_")) + ShaderModelDirectoryName;
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Target macros
	// ---------------------------------------------------------------------------

	// "6_6" -> 66: major * 10 + minor, so the value is ordered and a shader can gate on a
	// capability with "#if PE_SHADER_MODEL >= 60" instead of listing every model that has it.
	// One decimal digit has to hold the minor, which is what the single digit rule enforces:
	// a model whose name does not fit fails here rather than producing a value that compares
	// wrong against the models already shipped.
	PE_NODISCARD static BOOL32 MakeShaderModelMacroValue(const EString& InShaderModelToken, INT32& OutValue) noexcept
	{
		const INT32 SeparatorIndex = InShaderModelToken.Find(EString(PE_TEXT("_")));
		if (SeparatorIndex <= 0)
		{
			return FALSE;
		}

		const EString MajorText = InShaderModelToken.Left(static_cast<UINT32>(SeparatorIndex));
		const EString MinorText = InShaderModelToken.Right(InShaderModelToken.Length() - static_cast<UINT32>(SeparatorIndex) - 1u);
		if ((!MajorText.IsNumeric()) || (MinorText.Length() != 1u) || (!MinorText.IsNumeric()))
		{
			return FALSE;
		}

		OutValue = (MajorText.AtoI() * 10) + MinorText.AtoI();
		return TRUE;
	}

	static void AddShaderTargetDefine(TArray<EShaderVariantDefine>& InOutDefines, const EString& InName, const INT32 InValue) noexcept
	{
		EShaderVariantDefine Define;
		Define.Name = EName(InName);
		Define.Value = InValue;
		InOutDefines.Add(Define);
	}

	BOOL32 MakeShaderTargetDefines(const EShaderTarget& InTarget, TArray<EShaderVariantDefine>& InOutDefines) noexcept
	{
		if (!IsShaderTargetLegal(InTarget))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderTarget] Cannot define a target the engine does not build for.")));
			return FALSE;
		}

		// Resolved into a local list first: the array belongs to the caller, and a lookup that
		// fails must not leave it half written.
		TArray<EShaderVariantDefine> TargetDefines;

		// The numeric macros come first, one per axis. The platform value is the back end's
		// own enum member, which is what the configuration stores, so the two cannot drift
		// apart; the model value is derived from the model's name (see above).
		AddShaderTargetDefine(TargetDefines, EString(SHADER_TARGET_DEFINE_PLATFORM), static_cast<INT32>(InTarget.Backend));

		// Each step of the derivation fails where it knows the reason, and the last one names
		// the token it could not read: a model whose token is malformed has to be recognizable
		// from the log alone, and an empty token in the message says the step before it is the
		// one that failed.
		EShaderModelType CompileModel = EShaderModelType::SHADER_MODEL_5_0;
		if (!TranslateShaderTargetToCompileModel(InTarget, CompileModel))
		{
			return FALSE;
		}
		EString ShaderModelToken;
		if (!TranslateShaderModelToString(CompileModel, ShaderModelToken))
		{
			return FALSE;
		}
		INT32 ShaderModelValue = 0;
		if (!MakeShaderModelMacroValue(ShaderModelToken, ShaderModelValue))
		{
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderTarget] The shader model token '")) + ShaderModelToken
				+ EString(PE_TEXT("' has no macro value; it must read <major>_<minor> with a single digit minor.")));
			return FALSE;
		}
		AddShaderTargetDefine(TargetDefines, EString(SHADER_TARGET_DEFINE_MODEL), ShaderModelValue);

		// The complete flag set of both axes, with exactly one flag set per axis: a shader
		// tests a flag, and a json input condition gets something to compare against without
		// spelling a number of its own (see the header for why the set is complete).
		for (const EShaderTargetPlatformInfo& PlatformInfo : SHADER_TARGET_PLATFORMS)
		{
			AddShaderTargetDefine(TargetDefines
				, EString(SHADER_TARGET_DEFINE_PLATFORM) + EString(PE_TEXT("_")) + EString(PlatformInfo.Token)
				, ((PlatformInfo.Backend == InTarget.Backend) ? 1 : 0));
		}
		for (const EShaderTargetModelInfo& ModelInfo : SHADER_TARGET_MODELS)
		{
			EString ModelToken;
			if (!TranslateShaderModelToString(ModelInfo.CompileModel, ModelToken))
			{
				return FALSE;
			}
			AddShaderTargetDefine(TargetDefines
				, EString(SHADER_TARGET_DEFINE_MODEL) + EString(PE_TEXT("_")) + ModelToken
				, ((ModelInfo.ShaderModel == InTarget.ShaderModel) ? 1 : 0));
		}

		for (UINT32 Index = 0u, Num = TargetDefines.Num<UINT32>(); Index < Num; Index++)
		{
			InOutDefines.Add(TargetDefines[Index]);
		}
		return TRUE;
	}

} // namespace PigeonEngine
