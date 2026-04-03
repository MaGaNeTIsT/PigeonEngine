#include "RenderConfig.h"
#include <Base/DataStructure/Container/Map.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

	PE_INLINE EString GetEngineDefaultTexturePath(RDefaultTextureType InTexType)
	{
		return (EString(EBaseSettings::ENGINE_ASSET_DIRECTORY) + (TEXT("EngineTextures/Default/")));
	}
	PE_INLINE EString GetEngineDefaultTextureName(RDefaultTextureType InTexType)
	{
		static TMap<RDefaultTextureType, EString> EngineDefaultTextureNameMap = {
			{ RDefaultTextureType::TEXTURE2D_WHITE, TEXT("EngineDefaultWhite")},
			{ RDefaultTextureType::TEXTURE2D_BLACK, TEXT("EngineDefaultBlack") },
			{ RDefaultTextureType::TEXTURE2D_GRAY, TEXT("EngineDefaultGray") },
			{ RDefaultTextureType::TEXTURE2D_RED, TEXT("EngineDefaultRed") },
			{ RDefaultTextureType::TEXTURE2D_GREEN, TEXT("EngineDefaultGreen") },
			{ RDefaultTextureType::TEXTURE2D_BLUE, TEXT("EngineDefaultBlue") },
			{ RDefaultTextureType::TEXTURE2D_BUMP, TEXT("EngineDefaultBump") },
			{ RDefaultTextureType::TEXTURE2D_PROPERTY, TEXT("EngineDefaultProperty") } };

		return EngineDefaultTextureNameMap[InTexType];
	}
	PE_INLINE UINT32 GetShaderSemanticSizeByByte(const RInputLayoutDesc& input)
	{
		if (input.SemanticName == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return 0u; }
		UINT32 SemanticName = input.SemanticName;
		if ((SemanticName >> 15) & 0x1u) { /*SHADER_SEMANTIC_TEXCOORD[n]*/return 8u; }
		else if ((SemanticName >> 14) & 0x1u) { /*SHADER_SEMANTIC_POSITION[n]*/return 16u; }
		else if ((SemanticName >> 13) & 0x1u) { /*SHADER_SEMANTIC_NORMAL[n]*/return 16u; }
		else if ((SemanticName >> 12) & 0x1u) { /*SHADER_SEMANTIC_TANGENT[n]*/return 16u; }
		else if ((SemanticName >> 11) & 0x1u) { /*SHADER_SEMANTIC_COLOR[n]*/return 16u; }
		else if ((SemanticName >> 10) & 0x1u) { /*SHADER_SEMANTIC_BLENDINDICES[n]*/return 8u; }
		else if ((SemanticName >> 9) & 0x1u) { /*SHADER_SEMANTIC_BLENDWEIGHT[n]*/return 16u; }
		else if ((SemanticName >> 8) & 0x1u) { /*SHADER_SEMANTIC_BINORMAL[n]*/return 16u; }
		else if ((SemanticName >> 7) & 0x1u) { /*SHADER_SEMANTIC_POSITIONT[n]*/return 16u; }
		else if ((SemanticName >> 6) & 0x1u) { /*SHADER_SEMANTIC_PSIZE[n]*/return 8u; }
		return 0u;
	}
	PE_INLINE UINT32 GetShaderSemanticSizeBy32Bits(const RInputLayoutDesc& input)
	{
		if (input.SemanticName == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return 0u; }
		UINT32 SemanticName = input.SemanticName;
		if ((SemanticName >> 15) & 0x1u) { /*SHADER_SEMANTIC_TEXCOORD[n]*/return 2u; }
		else if ((SemanticName >> 14) & 0x1u) { /*SHADER_SEMANTIC_POSITION[n]*/return 4u; }
		else if ((SemanticName >> 13) & 0x1u) { /*SHADER_SEMANTIC_NORMAL[n]*/return 4u; }
		else if ((SemanticName >> 12) & 0x1u) { /*SHADER_SEMANTIC_TANGENT[n]*/return 4u; }
		else if ((SemanticName >> 11) & 0x1u) { /*SHADER_SEMANTIC_COLOR[n]*/return 4u; }
		else if ((SemanticName >> 10) & 0x1u) { /*SHADER_SEMANTIC_BLENDINDICES[n]*/return 2u; }
		else if ((SemanticName >> 9) & 0x1u) { /*SHADER_SEMANTIC_BLENDWEIGHT[n]*/return 4u; }
		else if ((SemanticName >> 8) & 0x1u) { /*SHADER_SEMANTIC_BINORMAL[n]*/return 4u; }
		else if ((SemanticName >> 7) & 0x1u) { /*SHADER_SEMANTIC_POSITIONT[n]*/return 4u; }
		else if ((SemanticName >> 6) & 0x1u) { /*SHADER_SEMANTIC_PSIZE[n]*/return 2u; }
		return 0u;
	}
	PE_INLINE RShaderSemanticType GetShaderSemanticBaseType(const UINT16 input)
	{
		if (input == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return RShaderSemanticType::SHADER_SEMANTIC_NONE; }
		UINT32 SemanticType = input;
		if ((SemanticType >> 15) & 0x1u) { /*SHADER_SEMANTIC_TEXCOORD[n]*/return RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD; }
		else if ((SemanticType >> 14) & 0x1u) { /*SHADER_SEMANTIC_POSITION[n]*/return RShaderSemanticType::SHADER_SEMANTIC_POSITION; }
		else if ((SemanticType >> 13) & 0x1u) { /*SHADER_SEMANTIC_NORMAL[n]*/return RShaderSemanticType::SHADER_SEMANTIC_NORMAL; }
		else if ((SemanticType >> 12) & 0x1u) { /*SHADER_SEMANTIC_TANGENT[n]*/return RShaderSemanticType::SHADER_SEMANTIC_TANGENT; }
		else if ((SemanticType >> 11) & 0x1u) { /*SHADER_SEMANTIC_COLOR[n]*/return RShaderSemanticType::SHADER_SEMANTIC_COLOR; }
		else if ((SemanticType >> 10) & 0x1u) { /*SHADER_SEMANTIC_BLENDINDICES[n]*/return RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES; }
		else if ((SemanticType >> 9) & 0x1u) { /*SHADER_SEMANTIC_BLENDWEIGHT[n]*/return RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT; }
		else if ((SemanticType >> 8) & 0x1u) { /*SHADER_SEMANTIC_BINORMAL[n]*/return RShaderSemanticType::SHADER_SEMANTIC_BINORMAL; }
		else if ((SemanticType >> 7) & 0x1u) { /*SHADER_SEMANTIC_POSITIONT[n]*/return RShaderSemanticType::SHADER_SEMANTIC_POSITIONT; }
		else if ((SemanticType >> 6) & 0x1u) { /*SHADER_SEMANTIC_PSIZE[n]*/return RShaderSemanticType::SHADER_SEMANTIC_PSIZE; }
		return RShaderSemanticType::SHADER_SEMANTIC_NONE;
	}
	PE_INLINE UINT32 GetShaderSemanticTypeSlot(const UINT16 input)
	{
		if (input == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return 0u; }
		UINT32 SemanticType = input;
		return (SemanticType & 0xffu);
	}
	PE_INLINE void GetEngineDefaultMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT32& OutLayoutNum)
	{
		PE_CONSTEXPR_STATIC RShaderSemanticType _EngineDefaultMeshInputLayout[7u] =
		{
			RShaderSemanticType::SHADER_SEMANTIC_POSITION0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0,
			RShaderSemanticType::SHADER_SEMANTIC_NORMAL0,
			RShaderSemanticType::SHADER_SEMANTIC_TANGENT0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1,
			RShaderSemanticType::SHADER_SEMANTIC_COLOR0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2
		};

		OutLayouts = _EngineDefaultMeshInputLayout;
		OutLayoutNum = 7u;
	}
	PE_INLINE void GetEngineDefaultSkeletalMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT32& OutLayoutNum)
	{
		PE_CONSTEXPR_STATIC RShaderSemanticType _EngineDefaultSkeletalMeshInputLayout[9u] =
		{
			RShaderSemanticType::SHADER_SEMANTIC_POSITION0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0,
			RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES0,
			RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT0,
			RShaderSemanticType::SHADER_SEMANTIC_NORMAL0,
			RShaderSemanticType::SHADER_SEMANTIC_TANGENT0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1,
			RShaderSemanticType::SHADER_SEMANTIC_COLOR0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2
		};

		OutLayouts = _EngineDefaultSkeletalMeshInputLayout;
		OutLayoutNum = 9u;
	}

};