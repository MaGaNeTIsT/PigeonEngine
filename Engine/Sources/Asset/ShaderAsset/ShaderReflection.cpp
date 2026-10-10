#include "ShaderReflection.h"

#include <RapidJson/Headers/JsonObject.h>
#include <IO/FileHelper.h>
#include <Editor/EditorLogManager.h>
#include <Config/EngineShaderConfig.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// Stage / shader model
	// ---------------------------------------------------------------------------

	BOOL32 TranslateShaderStageToString(const EShaderStageType InStage, EString& OutStr)
	{
		switch (InStage)
		{
		case EShaderStageType::SHADER_STAGE_VERTEX:
		{
			OutStr = EString("VS");
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_PIXEL:
		{
			OutStr = EString("PS");
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_COMPUTE:
		{
			OutStr = EString("CS");
			return TRUE;
		}
		default:
		{
			break;
		}
		}
		return FALSE;
	}

	BOOL32 ParseShaderStageFromString(const EString& InStr, EShaderStageType& OutStage)
	{
		if (InStr == EString("VS"))
		{
			OutStage = EShaderStageType::SHADER_STAGE_VERTEX;
			return TRUE;
		}
		if (InStr == EString("PS"))
		{
			OutStage = EShaderStageType::SHADER_STAGE_PIXEL;
			return TRUE;
		}
		if (InStr == EString("CS"))
		{
			OutStage = EShaderStageType::SHADER_STAGE_COMPUTE;
			return TRUE;
		}
		return FALSE;
	}

	BOOL32 GetShaderStageSuffix(const EShaderStageType InStage, EString& OutStr)
	{
		switch (InStage)
		{
		case EShaderStageType::SHADER_STAGE_VERTEX:
		{
			OutStr = EString(MC_SHADER_VS_SUFFIX);
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_PIXEL:
		{
			OutStr = EString(MC_SHADER_PS_SUFFIX);
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_COMPUTE:
		{
			OutStr = EString(MC_SHADER_CS_SUFFIX);
			return TRUE;
		}
		default:
		{
			break;
		}
		}
		return FALSE;
	}

	BOOL32 GetShaderStageMaskBit(const EShaderStageType InStage, UINT32& OutMaskBit)
	{
		switch (InStage)
		{
		case EShaderStageType::SHADER_STAGE_VERTEX:
		{
			OutMaskBit = SHADER_STAGE_MASK_VERTEX;
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_PIXEL:
		{
			OutMaskBit = SHADER_STAGE_MASK_PIXEL;
			return TRUE;
		}
		case EShaderStageType::SHADER_STAGE_COMPUTE:
		{
			OutMaskBit = SHADER_STAGE_MASK_COMPUTE;
			return TRUE;
		}
		default:
		{
			break;
		}
		}
		return FALSE;
	}

	BOOL32 TranslateShaderModelToString(const EShaderModelType InShaderModel, EString& OutStr)
	{
		switch (InShaderModel)
		{
		case EShaderModelType::SHADER_MODEL_5_0:
		{
			OutStr = EString("5_0");
			return TRUE;
		}
		case EShaderModelType::SHADER_MODEL_6_0:
		{
			OutStr = EString("6_0");
			return TRUE;
		}
		case EShaderModelType::SHADER_MODEL_6_6:
		{
			OutStr = EString("6_6");
			return TRUE;
		}
		default:
		{
			break;
		}
		}
		return FALSE;
	}

	BOOL32 ParseShaderModelFromString(const EString& InStr, EShaderModelType& OutShaderModel)
	{
		if (InStr == EString("5_0"))
		{
			OutShaderModel = EShaderModelType::SHADER_MODEL_5_0;
			return TRUE;
		}
		if (InStr == EString("6_0"))
		{
			OutShaderModel = EShaderModelType::SHADER_MODEL_6_0;
			return TRUE;
		}
		if (InStr == EString("6_6"))
		{
			OutShaderModel = EShaderModelType::SHADER_MODEL_6_6;
			return TRUE;
		}
		return FALSE;
	}

	BOOL32 GetShaderModelDirectoryName(const EShaderModelType InShaderModel, EString& OutStr)
	{
		EString ModelStr;
		if (!TranslateShaderModelToString(InShaderModel, ModelStr))
		{
			return FALSE;
		}
		OutStr = EString("ShaderModel_") + ModelStr;
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Value format
	// ---------------------------------------------------------------------------

	BOOL32 EShaderFieldFormat::IsMatrix() const noexcept
	{
		return (Suffix >= EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X2);
	}

	UINT32 EShaderFieldFormat::GetRowNum() const noexcept
	{
		switch (Suffix)
		{
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_1:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4:
		{
			return 1u;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X2:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X2:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X2:
		{
			return 2u;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X3:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X3:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X3:
		{
			return 3u;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X4:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X4:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X4:
		{
			return 4u;
		}
		default:
		{
			break;
		}
		}
		return 1u;
	}

	UINT32 EShaderFieldFormat::GetColumnNum() const noexcept
	{
		switch (Suffix)
		{
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_1:
		{
			return 1u;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X2:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X2:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X2:
		{
			return 2u;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X3:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X3:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X3:
		{
			return 3u;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X4:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X4:
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X4:
		{
			return 4u;
		}
		default:
		{
			break;
		}
		}
		return 1u;
	}

	UINT32 EShaderFieldFormat::GetScalarNum() const noexcept
	{
		return (GetRowNum() * GetColumnNum());
	}

	UINT32 EShaderFieldFormat::GetScalarByteSize() const noexcept
	{
		return static_cast<UINT32>(sizeof(UINT32));
	}

	UINT32 EShaderFieldFormat::GetElementSlotBytes() const noexcept
	{
		return (GetRowNum() * SHADER_FIELD_ROW_STRIDE_BYTES);
	}

	static BOOL32 ParseFieldFormatPrefix(const EString& InStr, EShaderFieldFormatPrefixType& OutPrefix)
	{
		if (InStr == EString("float"))
		{
			OutPrefix = EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_FLOAT;
			return TRUE;
		}
		if (InStr == EString("int"))
		{
			OutPrefix = EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_INT;
			return TRUE;
		}
		if (InStr == EString("uint"))
		{
			OutPrefix = EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_UINT;
			return TRUE;
		}
		return FALSE;
	}

	static BOOL32 TranslateFieldFormatPrefix(const EShaderFieldFormatPrefixType InPrefix, EString& OutStr)
	{
		switch (InPrefix)
		{
		case EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_FLOAT:
		{
			OutStr = EString("float");
			return TRUE;
		}
		case EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_INT:
		{
			OutStr = EString("int");
			return TRUE;
		}
		case EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_UINT:
		{
			OutStr = EString("uint");
			return TRUE;
		}
		default:
		{
			break;
		}
		}
		return FALSE;
	}

	// The suffix is the empty string for a scalar, so the prefix must be matched
	// first and the remainder compared as a whole.
	static BOOL32 ParseFieldFormatSuffix(const EString& InStr, EShaderFieldFormatSuffixType& OutSuffix)
	{
		if (InStr.Length() == 0u)
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_1;
			return TRUE;
		}
		if (InStr == EString("2"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2;
			return TRUE;
		}
		if (InStr == EString("3"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3;
			return TRUE;
		}
		if (InStr == EString("4"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4;
			return TRUE;
		}
		if (InStr == EString("2x2"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X2;
			return TRUE;
		}
		if (InStr == EString("3x2"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X2;
			return TRUE;
		}
		if (InStr == EString("2x3"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X3;
			return TRUE;
		}
		if (InStr == EString("3x3"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X3;
			return TRUE;
		}
		if (InStr == EString("4x2"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X2;
			return TRUE;
		}
		if (InStr == EString("2x4"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X4;
			return TRUE;
		}
		if (InStr == EString("4x3"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X3;
			return TRUE;
		}
		if (InStr == EString("3x4"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X4;
			return TRUE;
		}
		if (InStr == EString("4x4"))
		{
			OutSuffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X4;
			return TRUE;
		}
		return FALSE;
	}

	static BOOL32 TranslateFieldFormatSuffix(const EShaderFieldFormatSuffixType InSuffix, EString& OutStr)
	{
		switch (InSuffix)
		{
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_1:
		{
			OutStr = EString();
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2:
		{
			OutStr = EString("2");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3:
		{
			OutStr = EString("3");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4:
		{
			OutStr = EString("4");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X2:
		{
			OutStr = EString("2x2");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X2:
		{
			OutStr = EString("3x2");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X3:
		{
			OutStr = EString("2x3");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X3:
		{
			OutStr = EString("3x3");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X2:
		{
			OutStr = EString("4x2");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_2X4:
		{
			OutStr = EString("2x4");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X3:
		{
			OutStr = EString("4x3");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_3X4:
		{
			OutStr = EString("3x4");
			return TRUE;
		}
		case EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4X4:
		{
			OutStr = EString("4x4");
			return TRUE;
		}
		default:
		{
			break;
		}
		}
		return FALSE;
	}

	BOOL32 ParseShaderFieldFormatString(const EString& InStr, EShaderFieldFormat& OutFormat)
	{
		// Longest prefix first: "float" would otherwise never be reached because
		// "int" / "uint" are not prefixes of it, but keeping the order explicit
		// documents the two step split (prefix, then the whole remainder as suffix).
		static const CHAR* PrefixStrings[] = { "float", "uint", "int" };
		static const UINT32 PrefixNum = 3u;

		for (UINT32 PrefixIdx = 0u; PrefixIdx < PrefixNum; PrefixIdx++)
		{
			const EString PrefixStr(PrefixStrings[PrefixIdx]);
			if (!InStr.StartWith(PrefixStr))
			{
				continue;
			}
			const UINT32 PrefixLength = PrefixStr.Length();
			if (InStr.Length() < PrefixLength)
			{
				continue;
			}
			EShaderFieldFormatPrefixType Prefix = EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_FLOAT;
			EShaderFieldFormatSuffixType Suffix = EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_1;
			if (!ParseFieldFormatPrefix(InStr.Left(PrefixLength), Prefix))
			{
				continue;
			}
			if (!ParseFieldFormatSuffix(InStr.Right(InStr.Length() - PrefixLength), Suffix))
			{
				continue;
			}
			OutFormat.Prefix = Prefix;
			OutFormat.Suffix = Suffix;
			return TRUE;
		}
		return FALSE;
	}

	BOOL32 MakeShaderFieldFormatString(const EShaderFieldFormat& InFormat, EString& OutStr)
	{
		EString PrefixStr;
		EString SuffixStr;
		if (!TranslateFieldFormatPrefix(InFormat.Prefix, PrefixStr))
		{
			return FALSE;
		}
		if (!TranslateFieldFormatSuffix(InFormat.Suffix, SuffixStr))
		{
			return FALSE;
		}
		OutStr = PrefixStr + SuffixStr;
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Field helpers
	// ---------------------------------------------------------------------------

	UINT32 EShaderReflectionField::GetElementNum() const noexcept
	{
		if (!IsArray)
		{
			return 1u;
		}
		return ((NumElements > 0u) ? NumElements : 0u);
	}

	UINT32 EShaderReflectionField::GetDefaultScalarNum() const noexcept
	{
		const UINT32 ElementNum = GetElementNum();
		if ((ElementNum == 0u) || (Format.GetScalarNum() == 0u))
		{
			return 0u;
		}
		return (ElementNum * Format.GetScalarNum());
	}

	BOOL32 EShaderReflectionField::BuildWriteDesc(EShaderFieldWriteDesc& OutDesc) const noexcept
	{
		const UINT32 ElementNum = GetElementNum();
		if ((ElementNum == 0u) || (Format.GetScalarNum() == 0u))
		{
			return FALSE;
		}
		// An array without a reported stride cannot be addressed correctly. A single
		// element array does not need one.
		if ((ElementNum > 1u) && (ArrayStride == 0u))
		{
			return FALSE;
		}
		OutDesc.StartByteOffset		= ByteOffset;
		OutDesc.ElementNum			= ElementNum;
		OutDesc.ElementStrideBytes	= ArrayStride;
		OutDesc.RowNum				= Format.GetRowNum();
		OutDesc.ColumnNum			= Format.GetColumnNum();
		OutDesc.RowStrideBytes		= SHADER_FIELD_ROW_STRIDE_BYTES;
		OutDesc.ScalarNum			= Format.GetScalarNum();
		return TRUE;
	}

	UINT32 EShaderReflectionField::GetSpanEndByteOffset() const noexcept
	{
		EShaderFieldWriteDesc WriteDesc;
		if (!BuildWriteDesc(WriteDesc))
		{
			return 0u;
		}
		if (WriteDesc.ElementNum == 0u)
		{
			return 0u;
		}
		const UINT32 ElementIdx = WriteDesc.ElementNum - 1u;
		const UINT32 LastElementBase = WriteDesc.StartByteOffset + (WriteDesc.ElementStrideBytes * ElementIdx);
		if (WriteDesc.RowNum <= 1u)
		{
			// Scalar and vector: one row, rows do not need a stride of their own.
			return (LastElementBase + (WriteDesc.ColumnNum * Format.GetScalarByteSize()));
		}
		const UINT32 LastRowBase = LastElementBase + (WriteDesc.RowStrideBytes * (WriteDesc.RowNum - 1u));
		return (LastRowBase + (WriteDesc.ColumnNum * Format.GetScalarByteSize()));
	}

	const EShaderReflectionField* EShaderReflectionConstantBuffer::FindField(const EName& InFieldName) const
	{
		for (UINT32 FieldIdx = 0u, FieldNum = Fields.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
		{
			if (Fields[FieldIdx].Name == InFieldName)
			{
				return (&(Fields[FieldIdx]));
			}
		}
		return nullptr;
	}

	// ---------------------------------------------------------------------------
	// Reflection queries
	// ---------------------------------------------------------------------------

	const EShaderReflectionConstantBuffer* EShaderReflection::FindConstantBuffer(const EName& InName) const
	{
		for (UINT32 CbIdx = 0u, CbNum = ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			if (ConstantBuffers[CbIdx].Name == InName)
			{
				return (&(ConstantBuffers[CbIdx]));
			}
		}
		return nullptr;
	}

	const EShaderReflectionField* EShaderReflection::FindField(const EName& InBlockName, const EName& InFieldName) const
	{
		const EShaderReflectionConstantBuffer* ConstantBuffer = FindConstantBuffer(InBlockName);
		if (!ConstantBuffer)
		{
			return nullptr;
		}
		return (ConstantBuffer->FindField(InFieldName));
	}

	const EShaderReflectionTexture* EShaderReflection::FindTexture(const EName& InName, const BOOL32 InIsRW) const
	{
		const TArray<EShaderReflectionTexture>& TextureArray = (InIsRW ? RWTextures : Textures);
		for (UINT32 TexIdx = 0u, TexNum = TextureArray.Num<UINT32>(); TexIdx < TexNum; TexIdx++)
		{
			if (TextureArray[TexIdx].Name == InName)
			{
				return (&(TextureArray[TexIdx]));
			}
		}
		return nullptr;
	}

	const EShaderReflectionResource* EShaderReflection::FindResource(const EName& InName, const BOOL32 InIsRW) const
	{
		const TArray<EShaderReflectionResource>& ResourceArray = (InIsRW ? RWResources : Resources);
		for (UINT32 ResIdx = 0u, ResNum = ResourceArray.Num<UINT32>(); ResIdx < ResNum; ResIdx++)
		{
			if (ResourceArray[ResIdx].Name == InName)
			{
				return (&(ResourceArray[ResIdx]));
			}
		}
		return nullptr;
	}

	const EShaderReflectionSampler* EShaderReflection::FindSampler(const EName& InName) const
	{
		for (UINT32 SamplerIdx = 0u, SamplerNum = Samplers.Num<UINT32>(); SamplerIdx < SamplerNum; SamplerIdx++)
		{
			if (Samplers[SamplerIdx].Name == InName)
			{
				return (&(Samplers[SamplerIdx]));
			}
		}
		return nullptr;
	}

	BOOL32 EShaderReflection::HasAnyLayout() const noexcept
	{
		for (UINT32 CbIdx = 0u, CbNum = ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			if (!ConstantBuffers[CbIdx].bCompilerCulled)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	UINT32 EShaderReflection::GetCulledFieldNum() const noexcept
	{
		UINT32 CulledNum = 0u;
		for (UINT32 CbIdx = 0u, CbNum = ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			const EShaderReflectionConstantBuffer& ConstantBuffer = ConstantBuffers[CbIdx];
			if (ConstantBuffer.bCompilerCulled)
			{
				CulledNum += ConstantBuffer.Fields.Num<UINT32>();
				continue;
			}
			for (UINT32 FieldIdx = 0u, FieldNum = ConstantBuffer.Fields.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
			{
				if (ConstantBuffer.Fields[FieldIdx].bCompilerCulled)
				{
					CulledNum++;
				}
			}
		}
		return CulledNum;
	}

	UINT32 EShaderReflection::GetCulledResourceNum() const noexcept
	{
		UINT32 CulledNum = 0u;
		for (UINT32 TexIdx = 0u, TexNum = Textures.Num<UINT32>(); TexIdx < TexNum; TexIdx++)
		{
			if (Textures[TexIdx].bCompilerCulled)
			{
				CulledNum++;
			}
		}
		for (UINT32 TexIdx = 0u, TexNum = RWTextures.Num<UINT32>(); TexIdx < TexNum; TexIdx++)
		{
			if (RWTextures[TexIdx].bCompilerCulled)
			{
				CulledNum++;
			}
		}
		for (UINT32 ResIdx = 0u, ResNum = Resources.Num<UINT32>(); ResIdx < ResNum; ResIdx++)
		{
			if (Resources[ResIdx].bCompilerCulled)
			{
				CulledNum++;
			}
		}
		for (UINT32 ResIdx = 0u, ResNum = RWResources.Num<UINT32>(); ResIdx < ResNum; ResIdx++)
		{
			if (RWResources[ResIdx].bCompilerCulled)
			{
				CulledNum++;
			}
		}
		for (UINT32 SamplerIdx = 0u, SamplerNum = Samplers.Num<UINT32>(); SamplerIdx < SamplerNum; SamplerIdx++)
		{
			if (Samplers[SamplerIdx].bCompilerCulled)
			{
				CulledNum++;
			}
		}
		return CulledNum;
	}

	EShaderParameterTargetType EShaderReflection::FindParameterTarget(const EName& InBlockName, const EName& InParameterName) const
	{
		// Constant buffer field first: it is the only target that carries a block name.
		const EShaderReflectionConstantBuffer* ConstantBuffer = FindConstantBuffer(InBlockName);
		if ((ConstantBuffer != nullptr) && (!ConstantBuffer->bCompilerCulled))
		{
			const EShaderReflectionField* Field = ConstantBuffer->FindField(InParameterName);
			if (Field != nullptr)
			{
				return (Field->bCompilerCulled
					? EShaderParameterTargetType::SHADER_PARAMETER_TARGET_CULLED
					: EShaderParameterTargetType::SHADER_PARAMETER_TARGET_ACTIVE);
			}
		}
		else if ((ConstantBuffer != nullptr) && (ConstantBuffer->bCompilerCulled) && (ConstantBuffer->FindField(InParameterName) != nullptr))
		{
			// The whole buffer was dropped, so every field in it is gone as well.
			return EShaderParameterTargetType::SHADER_PARAMETER_TARGET_CULLED;
		}

		// Resource parameters carry the resource name as their field name and ignore the
		// block, because the resource tables are flat: there is no constant buffer that
		// could key them.
		const EShaderReflectionTexture* Texture = FindTexture(InParameterName, FALSE);
		if (Texture == nullptr)
		{
			Texture = FindTexture(InParameterName, TRUE);
		}
		if (Texture != nullptr)
		{
			return (Texture->bCompilerCulled
				? EShaderParameterTargetType::SHADER_PARAMETER_TARGET_CULLED
				: EShaderParameterTargetType::SHADER_PARAMETER_TARGET_ACTIVE);
		}

		const EShaderReflectionResource* Resource = FindResource(InParameterName, FALSE);
		if (Resource == nullptr)
		{
			Resource = FindResource(InParameterName, TRUE);
		}
		if (Resource != nullptr)
		{
			return (Resource->bCompilerCulled
				? EShaderParameterTargetType::SHADER_PARAMETER_TARGET_CULLED
				: EShaderParameterTargetType::SHADER_PARAMETER_TARGET_ACTIVE);
		}

		const EShaderReflectionSampler* Sampler = FindSampler(InParameterName);
		if (Sampler != nullptr)
		{
			return (Sampler->bCompilerCulled
				? EShaderParameterTargetType::SHADER_PARAMETER_TARGET_CULLED
				: EShaderParameterTargetType::SHADER_PARAMETER_TARGET_ACTIVE);
		}

		return EShaderParameterTargetType::SHADER_PARAMETER_TARGET_MISSING;
	}

	UINT32 EShaderReflection::GetExposedFieldNum() const noexcept
	{
		UINT32 ExposedNum = 0u;
		for (UINT32 CbIdx = 0u, CbNum = ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			const EShaderReflectionConstantBuffer& ConstantBuffer = ConstantBuffers[CbIdx];
			for (UINT32 FieldIdx = 0u, FieldNum = ConstantBuffer.Fields.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
			{
				if (ConstantBuffer.Fields[FieldIdx].Exposure)
				{
					ExposedNum++;
				}
			}
		}
		return ExposedNum;
	}

	// ---------------------------------------------------------------------------
	// Parsing
	// ---------------------------------------------------------------------------

	static UINT64 HashShaderReflectionText(const EString& InText) noexcept
	{
		UINT64 HashValue = 14695981039346656037ull;
		const CHAR* Data = *InText;
		while (*Data)
		{
			HashValue ^= static_cast<UINT64>(static_cast<UINT8>(*Data++));
			HashValue *= 1099511628211ull;
		}
		return HashValue;
	}

	static BOOL32 ParseReflectionTextureType(const EString& InStr, EShaderTextureType& OutType)
	{
		if (InStr == EString("1D"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_1D;
			return TRUE;
		}
		if (InStr == EString("2D"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_2D;
			return TRUE;
		}
		if (InStr == EString("3D"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_3D;
			return TRUE;
		}
		if (InStr == EString("1DArray"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_1D_ARRAY;
			return TRUE;
		}
		if (InStr == EString("2DArray"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_2D_ARRAY;
			return TRUE;
		}
		if (InStr == EString("3DArray"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_3D_ARRAY;
			return TRUE;
		}
		if (InStr == EString("Cube"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_CUBE;
			return TRUE;
		}
		if (InStr == EString("CubeArray"))
		{
			OutType = EShaderTextureType::SHADER_TEXTURE_TYPE_CUBE_ARRAY;
			return TRUE;
		}
		return FALSE;
	}

	static BOOL32 ParseReflectionResourceKind(const EString& InStr, EShaderResourceKindType& OutKind)
	{
		if (InStr == EString("Buffer"))
		{
			OutKind = EShaderResourceKindType::SHADER_RESOURCE_KIND_BUFFER;
			return TRUE;
		}
		if (InStr == EString("StructuredBuffer"))
		{
			OutKind = EShaderResourceKindType::SHADER_RESOURCE_KIND_STRUCTURED_BUFFER;
			return TRUE;
		}
		if (InStr == EString("ByteAddressBuffer"))
		{
			OutKind = EShaderResourceKindType::SHADER_RESOURCE_KIND_BYTE_ADDRESS_BUFFER;
			return TRUE;
		}
		return FALSE;
	}

	static BOOL32 ParseReflectionSamplerFilter(const EString& InStr, EShaderSamplerFilterType& OutFilter)
	{
		if (InStr == EString("Point"))
		{
			OutFilter = EShaderSamplerFilterType::SHADER_SAMPLER_FILTER_POINT;
			return TRUE;
		}
		if (InStr == EString("MinMagLinear"))
		{
			OutFilter = EShaderSamplerFilterType::SHADER_SAMPLER_FILTER_MIN_MAG_LINEAR;
			return TRUE;
		}
		if (InStr == EString("MinMagMipLinear"))
		{
			OutFilter = EShaderSamplerFilterType::SHADER_SAMPLER_FILTER_MIN_MAG_MIP_LINEAR;
			return TRUE;
		}
		if (InStr == EString("Anisotropic"))
		{
			OutFilter = EShaderSamplerFilterType::SHADER_SAMPLER_FILTER_ANISOTROPIC;
			return TRUE;
		}
		return FALSE;
	}

	static BOOL32 ParseReflectionSamplerAddress(const EString& InStr, EShaderSamplerAddressModeType& OutAddress)
	{
		if (InStr == EString("Wrap"))
		{
			OutAddress = EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_WRAP;
			return TRUE;
		}
		if (InStr == EString("Mirror"))
		{
			OutAddress = EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_MIRROR;
			return TRUE;
		}
		if (InStr == EString("Clamp"))
		{
			OutAddress = EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_CLAMP;
			return TRUE;
		}
		if (InStr == EString("Border"))
		{
			OutAddress = EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_BORDER;
			return TRUE;
		}
		if (InStr == EString("MirrorOnce"))
		{
			OutAddress = EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_MIRROR_ONCE;
			return TRUE;
		}
		return FALSE;
	}

	static BOOL32 ParseReflectionSamplerComparison(const EString& InStr, EShaderSamplerComparisonFuncType& OutFunc)
	{
		if (InStr == EString("Never"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_NEVER;
			return TRUE;
		}
		if (InStr == EString("Less"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_LESS;
			return TRUE;
		}
		if (InStr == EString("Equal"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_EQUAL;
			return TRUE;
		}
		if (InStr == EString("LessEqual"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_LESS_EQUAL;
			return TRUE;
		}
		if (InStr == EString("Greater"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_GREATER;
			return TRUE;
		}
		if (InStr == EString("NotEqual"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_NOT_EQUAL;
			return TRUE;
		}
		if (InStr == EString("GreaterEqual"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_GREATER_EQUAL;
			return TRUE;
		}
		if (InStr == EString("Always"))
		{
			OutFunc = EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_ALWAYS;
			return TRUE;
		}
		return FALSE;
	}

	static BOOL32 ParseReflectionField(const EString& InDebugStr, CJsonObject& InJson, const EName& InBlockName, EShaderReflectionField& OutField)
	{
		EString FieldName;
		if (!InJson.GetStringField(EString("Name"), FieldName))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), (PE_TEXT("[ShaderReflection] Field without a name.")));
			return FALSE;
		}
		// Names are converted to EName exactly once, while parsing on the loading
		// thread; every later lookup compares indices.
		OutField.Name = EName(FieldName);
		OutField.BlockName = InBlockName;

		EString FormatStr;
		if (InJson.GetStringField(EString("Format"), FormatStr))
		{
			if (!ParseShaderFieldFormatString(FormatStr, OutField.Format))
			{
				PE_LOG_ERROR(EString("[ShaderReflection] ") + InDebugStr + EString(" field '") + FieldName + EString("' has an unknown format '") + FormatStr + EString("'."));
				return FALSE;
			}
		}

		InJson.GetUIntField(EString("ByteOffset"), OutField.ByteOffset);
		InJson.GetUIntField(EString("ContentBytes"), OutField.ContentBytes);
		InJson.GetUIntField(EString("ArrayStride"), OutField.ArrayStride);

		BOOL8 bValue = FALSE;
		if (InJson.GetBoolField(EString("CompilerCulled"), bValue))
		{
			OutField.bCompilerCulled = (bValue ? TRUE : FALSE);
		}
		if (InJson.GetBoolField(EString("Exposure"), bValue))
		{
			OutField.Exposure = (bValue ? TRUE : FALSE);
		}
		if (InJson.GetBoolField(EString("IsColor"), bValue))
		{
			OutField.IsColor = (bValue ? TRUE : FALSE);
		}
		if (InJson.GetBoolField(EString("IsArray"), bValue))
		{
			OutField.IsArray = (bValue ? TRUE : FALSE);
		}
		InJson.GetUIntField(EString("NumElements"), OutField.NumElements);
		InJson.GetUIntArrayField(EString("Default"), OutField.Default);

		const UINT32 ExpectedScalarNum = OutField.GetDefaultScalarNum();
		const UINT32 ParsedScalarNum = OutField.Default.Num<UINT32>();
		if ((ParsedScalarNum > 0u) && (ExpectedScalarNum > 0u) && (ParsedScalarNum != ExpectedScalarNum))
		{
			PE_LOG_WARN(EString("[ShaderReflection] ") + InDebugStr + EString(" field '") + FieldName + EString("' carries ") + ToString(ParsedScalarNum) + EString(" default values but the format and array size expect ") + ToString(ExpectedScalarNum) + EString("."));
		}
		return TRUE;
	}

	static BOOL32 ParseReflectionConstantBuffers(CJsonObject& InJson, EShaderReflection& OutReflection)
	{
		TArray<CJsonObject*> CbArray;
		if (!InJson.GetObjectArrayField(EString("CBuffers"), CbArray))
		{
			return TRUE;
		}
		for (UINT32 CbIdx = 0u, CbNum = CbArray.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
		{
			CJsonObject* CbJson = CbArray[CbIdx];
			if (!CbJson)
			{
				continue;
			}
			EShaderReflectionConstantBuffer ConstantBuffer;

			EString CbName;
			if (!CbJson->GetStringField(EString("Name"), CbName))
			{
				PE_LOG_WARN(EString("[ShaderReflection] Constant buffer without a name, skipped."));
				delete CbJson;
				continue;
			}
			ConstantBuffer.Name = EName(CbName);
			CbJson->GetUIntField(EString("Slot"), ConstantBuffer.Slot);
			CbJson->GetUIntField(EString("ByteSize"), ConstantBuffer.ByteSize);
			BOOL8 bValue = FALSE;
			if (CbJson->GetBoolField(EString("CompilerCulled"), bValue))
			{
				ConstantBuffer.bCompilerCulled = (bValue ? TRUE : FALSE);
			}
			// Root constant blocks are declared by the source and validated by the compiler
			// (at most one per assembled shader, within the root constant size limit); the
			// engine only forwards the flag.
			if (CbJson->GetBoolField(EString("RootConstant"), bValue))
			{
				ConstantBuffer.RootConstant = (bValue ? TRUE : FALSE);
			}

			TArray<CJsonObject*> FieldArray;
			if (CbJson->GetObjectArrayField(EString("Fields"), FieldArray))
			{
				for (UINT32 FieldIdx = 0u, FieldNum = FieldArray.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
				{
					CJsonObject* FieldJson = FieldArray[FieldIdx];
					if (!FieldJson)
					{
						continue;
					}
					EShaderReflectionField Field;
					const EString FieldDebugStr = EString("ConstantBuffer '") + CbName + EString("'");
					if (ParseReflectionField(FieldDebugStr, *FieldJson, ConstantBuffer.Name, Field))
					{
						ConstantBuffer.Fields.Add(Field);
					}
					delete FieldJson;
				}
			}
			OutReflection.ConstantBuffers.Add(ConstantBuffer);
			delete CbJson;
		}
		return TRUE;
	}

	static BOOL32 ParseReflectionTextures(const EString& InFieldName, CJsonObject& InJson, const BOOL32 InIsRW, EShaderReflection& OutReflection)
	{
		TArray<CJsonObject*> TextureArray;
		if (!InJson.GetObjectArrayField(InFieldName, TextureArray))
		{
			return TRUE;
		}
		for (UINT32 TexIdx = 0u, TexNum = TextureArray.Num<UINT32>(); TexIdx < TexNum; TexIdx++)
		{
			CJsonObject* TexJson = TextureArray[TexIdx];
			if (!TexJson)
			{
				continue;
			}
			EShaderReflectionTexture Texture;
			Texture.bIsRW = InIsRW;

			EString TexName;
			if (!TexJson->GetStringField(EString("Name"), TexName))
			{
				PE_LOG_WARN(EString("[ShaderReflection] Texture without a name, skipped."));
				delete TexJson;
				continue;
			}
			Texture.Name = EName(TexName);

			EString FormatStr;
			if (TexJson->GetStringField(EString("Format"), FormatStr))
			{
				if (!ParseShaderFieldFormatString(FormatStr, Texture.Format))
				{
					return FALSE;
				}
			}

			EString TypeStr;
			if (TexJson->GetStringField(EString("Type"), TypeStr))
			{
				if (!ParseReflectionTextureType(TypeStr, Texture.Type))
				{
					PE_LOG_ERROR(EString("[ShaderReflection] Texture '") + TexName + EString("' has an unknown type '") + TypeStr + EString("'."));
				}
			}

			TexJson->GetUIntField(EString("Slot"), Texture.Slot);

			EString SamplerName;
			if (TexJson->GetStringField(EString("Sampler"), SamplerName))
			{
				// An empty name is an invalid EName, which is exactly the "no sampler"
				// marker the rest of the module tests with IsValid().
				Texture.SamplerName = EName(SamplerName);
			}

			BOOL8 bValue = FALSE;
			if (TexJson->GetBoolField(EString("Exposure"), bValue))
			{
				Texture.Exposure = (bValue ? TRUE : FALSE);
			}
			// Declared but stripped from the compiled shader: the slot does not exist
			// there anymore, so it must not reach the binding plan.
			if (TexJson->GetBoolField(EString("CompilerCulled"), bValue))
			{
				Texture.bCompilerCulled = (bValue ? TRUE : FALSE);
			}

			if (InIsRW)
			{
				OutReflection.RWTextures.Add(Texture);
			}
			else
			{
				OutReflection.Textures.Add(Texture);
			}
			delete TexJson;
		}
		return TRUE;
	}

	static BOOL32 ParseReflectionResources(const EString& InFieldName, CJsonObject& InJson, const BOOL32 InIsRW, EShaderReflection& OutReflection)
	{
		TArray<CJsonObject*> ResourceArray;
		if (!InJson.GetObjectArrayField(InFieldName, ResourceArray))
		{
			return TRUE;
		}
		for (UINT32 ResIdx = 0u, ResNum = ResourceArray.Num<UINT32>(); ResIdx < ResNum; ResIdx++)
		{
			CJsonObject* ResJson = ResourceArray[ResIdx];
			if (!ResJson)
			{
				continue;
			}
			EShaderReflectionResource Resource;
			Resource.bIsRW = InIsRW;

			EString ResName;
			if (!ResJson->GetStringField(EString("Name"), ResName))
			{
				PE_LOG_WARN(EString("[ShaderReflection] Resource without a name, skipped."));
				delete ResJson;
				continue;
			}
			Resource.Name = EName(ResName);

			ResJson->GetUIntField(EString("Slot"), Resource.Slot);

			BOOL8 bValue = FALSE;
			if (ResJson->GetBoolField(EString("IsStruct"), bValue))
			{
				Resource.bIsStruct = (bValue ? TRUE : FALSE);
			}
			if (ResJson->GetBoolField(EString("Exposure"), bValue))
			{
				Resource.Exposure = (bValue ? TRUE : FALSE);
			}
			// Same as textures: a culled buffer is not bound by the compiled shader.
			if (ResJson->GetBoolField(EString("CompilerCulled"), bValue))
			{
				Resource.bCompilerCulled = (bValue ? TRUE : FALSE);
			}

			// A structured buffer declares its struct fields and no element format;
			// every other buffer kind declares an element format instead.
			TArray<CJsonObject*> StructFieldArray;
			if (Resource.bIsStruct && ResJson->GetObjectArrayField(EString("StructFields"), StructFieldArray))
			{
				for (UINT32 FieldIdx = 0u, FieldNum = StructFieldArray.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
				{
					CJsonObject* FieldJson = StructFieldArray[FieldIdx];
					if (!FieldJson)
					{
						continue;
					}
					EShaderReflectionField Field;
					const EString FieldDebugStr = EString("Resource '") + ResName + EString("'");
					if (ParseReflectionField(FieldDebugStr, *FieldJson, Resource.Name, Field))
					{
						Resource.StructFields.Add(Field);
					}
					delete FieldJson;
				}
			}
			else
			{
				EString FormatStr;
				if (ResJson->GetStringField(EString("Format"), FormatStr))
				{
					if (!ParseShaderFieldFormatString(FormatStr, Resource.Format))
					{
						return FALSE;
					}
				}
			}

			if (InIsRW)
			{
				OutReflection.RWResources.Add(Resource);
			}
			else
			{
				OutReflection.Resources.Add(Resource);
			}
			delete ResJson;
		}
		return TRUE;
	}

	static BOOL32 ParseReflectionSamplers(CJsonObject& InJson, EShaderReflection& OutReflection)
	{
		TArray<CJsonObject*> SamplerArray;
		if (!InJson.GetObjectArrayField(EString("Samplers"), SamplerArray))
		{
			return TRUE;
		}
		for (UINT32 SamplerIdx = 0u, SamplerNum = SamplerArray.Num<UINT32>(); SamplerIdx < SamplerNum; SamplerIdx++)
		{
			CJsonObject* SamplerJson = SamplerArray[SamplerIdx];
			if (!SamplerJson)
			{
				continue;
			}
			EShaderReflectionSampler Sampler;

			EString SamplerName;
			if (!SamplerJson->GetStringField(EString("Name"), SamplerName))
			{
				PE_LOG_WARN(EString("[ShaderReflection] Sampler without a name, skipped."));
				delete SamplerJson;
				continue;
			}
			Sampler.Name = EName(SamplerName);
			SamplerJson->GetUIntField(EString("Slot"), Sampler.Slot);

			EString ValueStr;
			if (SamplerJson->GetStringField(EString("Filter"), ValueStr))
			{
				if (!ParseReflectionSamplerFilter(ValueStr, Sampler.Desc.Filter))
				{
					PE_LOG_ERROR(EString("[ShaderReflection] Sampler '") + SamplerName + EString("' has an unknown filter '") + ValueStr + EString("'."));
				}
			}
			if (SamplerJson->GetStringField(EString("AddressU"), ValueStr))
			{
				ParseReflectionSamplerAddress(ValueStr, Sampler.Desc.AddressU);
			}
			if (SamplerJson->GetStringField(EString("AddressV"), ValueStr))
			{
				ParseReflectionSamplerAddress(ValueStr, Sampler.Desc.AddressV);
			}
			if (SamplerJson->GetStringField(EString("AddressW"), ValueStr))
			{
				ParseReflectionSamplerAddress(ValueStr, Sampler.Desc.AddressW);
			}
			if (SamplerJson->GetStringField(EString("ComparisonFunc"), ValueStr))
			{
				ParseReflectionSamplerComparison(ValueStr, Sampler.Desc.ComparisonFunc);
			}

			SamplerJson->GetFloatField(EString("MinLOD"), Sampler.Desc.MinLOD);
			SamplerJson->GetFloatField(EString("MaxLOD"), Sampler.Desc.MaxLOD);
			SamplerJson->GetFloatField(EString("LODBias"), Sampler.Desc.LODBias);
			SamplerJson->GetUIntField(EString("Anisotropy"), Sampler.Desc.Anisotropy);

			TArray<FLOAT> BorderColorArray;
			if (SamplerJson->GetFloatArrayField(EString("BorderColor"), BorderColorArray))
			{
				for (UINT32 ColorIdx = 0u; ColorIdx < 4u; ColorIdx++)
				{
					if (ColorIdx < BorderColorArray.Num<UINT32>())
					{
						Sampler.Desc.BorderColor[ColorIdx] = BorderColorArray[ColorIdx];
					}
				}
			}

			BOOL8 bValue = FALSE;
			if (SamplerJson->GetBoolField(EString("Exposure"), bValue))
			{
				Sampler.Exposure = (bValue ? TRUE : FALSE);
			}
			// A sampler that survived only in the declaration is not part of the
			// compiled shader, so nothing is bound at its slot.
			if (SamplerJson->GetBoolField(EString("CompilerCulled"), bValue))
			{
				Sampler.bCompilerCulled = (bValue ? TRUE : FALSE);
			}

			OutReflection.Samplers.Add(Sampler);
			delete SamplerJson;
		}
		return TRUE;
	}

	static BOOL32 ParseReflectionInputLayouts(CJsonObject& InJson, EShaderReflection& OutReflection)
	{
		TArray<CJsonObject*> InputArray;
		if (!InJson.GetObjectArrayField(EString("InputLayout"), InputArray))
		{
			return TRUE;
		}
		for (UINT32 InputIdx = 0u, InputNum = InputArray.Num<UINT32>(); InputIdx < InputNum; InputIdx++)
		{
			CJsonObject* InputJson = InputArray[InputIdx];
			if (!InputJson)
			{
				continue;
			}
			EShaderReflectionInputLayout InputLayout;

			EString ValueStr;
			if (InputJson->GetStringField(EString("Name"), ValueStr))
			{
				InputLayout.Name = EName(ValueStr);
			}
			if (InputJson->GetStringField(EString("Format"), ValueStr))
			{
				if (!ParseShaderFieldFormatString(ValueStr, InputLayout.Format))
				{
					return FALSE;
				}
			}
			if (InputJson->GetStringField(EString("Semantic"), ValueStr))
			{
				InputLayout.Semantic = EName(ValueStr);
			}
			InputJson->GetUIntField(EString("SemanticIndex"), InputLayout.SemanticIndex);
			InputJson->GetUIntField(EString("Slot"), InputLayout.Slot);

			OutReflection.InputLayouts.Add(InputLayout);
			delete InputJson;
		}
		return TRUE;
	}

	BOOL32 ParseShaderReflectionText(const EString& InReflectionJsonText, EShaderReflection& OutReflection)
	{
		if (InReflectionJsonText.Length() == 0u)
		{
			return FALSE;
		}

		OutReflection = EShaderReflection();
		OutReflection.SourceHash = HashShaderReflectionText(InReflectionJsonText);

		CJsonObject ReflectionJson(InReflectionJsonText);

		EString ValueStr;
		if (ReflectionJson.GetStringField(EString("ShaderName"), ValueStr))
		{
			OutReflection.ShaderName = EName(ValueStr);
		}
		if (ReflectionJson.GetStringField(EString("EntryPoint"), ValueStr))
		{
			OutReflection.EntryPoint = EName(ValueStr);
		}
		if (ReflectionJson.GetStringField(EString("Stage"), ValueStr))
		{
			if (!ParseShaderStageFromString(ValueStr, OutReflection.Stage))
			{
				PE_LOG_ERROR(EString("[ShaderReflection] Unknown stage '") + ValueStr + EString("' in the reflection."));
				return FALSE;
			}
		}
		else
		{
			PE_LOG_ERROR(EString("[ShaderReflection] The reflection does not declare a stage."));
			return FALSE;
		}

		if (!ParseReflectionConstantBuffers(ReflectionJson, OutReflection))
		{
			return FALSE;
		}
		if (!ParseReflectionTextures(EString("Textures"), ReflectionJson, FALSE, OutReflection))
		{
			return FALSE;
		}
		if (!ParseReflectionTextures(EString("RWTextures"), ReflectionJson, TRUE, OutReflection))
		{
			return FALSE;
		}
		if (!ParseReflectionResources(EString("Resources"), ReflectionJson, FALSE, OutReflection))
		{
			return FALSE;
		}
		if (!ParseReflectionResources(EString("RWResources"), ReflectionJson, TRUE, OutReflection))
		{
			return FALSE;
		}
		if (!ParseReflectionSamplers(ReflectionJson, OutReflection))
		{
			return FALSE;
		}
		if (!ParseReflectionInputLayouts(ReflectionJson, OutReflection))
		{
			return FALSE;
		}

		// ShaderModel is not part of the reflection json; the asset loader fills it
		// in from the manifest so the reflection stays a faithful mirror of the file.
		return TRUE;
	}

	BOOL32 ParseShaderReflectionJson(const EString& InReflectionJsonPath, EShaderReflection& OutReflection)
	{
		EString ReflectionText;
		if (!EFileHelper::ReadFileAsString(InReflectionJsonPath, ReflectionText))
		{
			PE_LOG_ERROR(EString("[ShaderReflection] Cannot read reflection: ") + InReflectionJsonPath);
			return FALSE;
		}
		if (!ParseShaderReflectionText(ReflectionText, OutReflection))
		{
			PE_LOG_ERROR(EString("[ShaderReflection] Cannot parse reflection: ") + InReflectionJsonPath);
			return FALSE;
		}
		return TRUE;
	}

} // namespace PigeonEngine
