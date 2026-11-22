#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	/*
	* One value a material slot overrides on top of its material asset.
	*
	* A material slot names an assembly (material source, shader, program). A level that needs
	* a different look - one texture set per scene object, a tint, a mask threshold - stores
	* those differences here instead of cloning the material by hand. The names are the same
	* (block, field) pair EShaderParameterSet uses, so applying an override is one setter call.
	*
	* Texture overrides name a texture asset rather than a runtime resource: the level file has
	* to survive a process restart, and the runtime texture is re-resolved from the asset name
	* when the variant is built.
	*/
	enum class EMaterialParameterOverrideKindType : UINT8
	{
		SCALAR = 0,
		VECTOR,
		TEXTURE,
	};

	struct FMaterialParameterOverride
	{
	public:
		EString							BlockName;
		EString							FieldName;
		EMaterialParameterOverrideKindType	Kind = EMaterialParameterOverrideKindType::SCALAR;
		// Scalar / vector components, row major. Empty for a texture override.
		TArray<FLOAT>					Values;
		// Texture asset name (ENGINE_TEXTURE_PATH lookup), for a texture override.
		EString							TextureAssetName;
	};

	// Field-for-field comparison; the variant cache uses it to reject a hash collision.
	PE_NODISCARD inline BOOL32 IsSameMaterialParameterOverrides(const TArray<FMaterialParameterOverride>& InLeft
		, const TArray<FMaterialParameterOverride>& InRight) noexcept
	{
		if (InLeft.Num<UINT32>() != InRight.Num<UINT32>())
		{
			return FALSE;
		}
		for (UINT32 Index = 0u, Num = InLeft.Num<UINT32>(); Index < Num; Index++)
		{
			const FMaterialParameterOverride& Left = InLeft[Index];
			const FMaterialParameterOverride& Right = InRight[Index];
			if ((Left.BlockName != Right.BlockName)
				|| (Left.FieldName != Right.FieldName)
				|| (Left.Kind != Right.Kind)
				|| (Left.TextureAssetName != Right.TextureAssetName)
				|| (Left.Values.Num<UINT32>() != Right.Values.Num<UINT32>()))
			{
				return FALSE;
			}
			for (UINT32 ValueIndex = 0u, ValueNum = Left.Values.Num<UINT32>(); ValueIndex < ValueNum; ValueIndex++)
			{
				if (Left.Values[ValueIndex] != Right.Values[ValueIndex])
				{
					return FALSE;
				}
			}
		}
		return TRUE;
	}

}	// namespace PigeonEngine
