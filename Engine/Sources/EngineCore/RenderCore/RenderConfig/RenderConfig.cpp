#include "RenderConfig.h"
#include <Base/DataStructure/Container/Map.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

	PE_INLINE DirectX::XMFLOAT4X4 TranslateUploadType(const Matrix4x4& InData)
	{
		DirectX::XMFLOAT4X4 Result(InData.GetDirectXTransposeValue4x4());
		return Result;
	}
	PE_INLINE DirectX::XMFLOAT4 TranslateUploadType(const Vector4& InData)
	{
		DirectX::XMFLOAT4 Result(InData.GetDirectXValue4());
		return Result;
	}
	PE_INLINE EString GetEngineDefaultTexturePath(RDefaultTextureType texType)
	{
		static TMap<RDefaultTextureType, EString> engineDefaultTexturePathMap = {
			{ RDefaultTextureType::TEXTURE2D_WHITE, (ENGINE_DEFAULT_TEXTURE2D_WHITE) },
			{ RDefaultTextureType::TEXTURE2D_BLACK, (ENGINE_DEFAULT_TEXTURE2D_BLACK) },
			{ RDefaultTextureType::TEXTURE2D_GRAY, (ENGINE_DEFAULT_TEXTURE2D_GRAY) },
			{ RDefaultTextureType::TEXTURE2D_RED, (ENGINE_DEFAULT_TEXTURE2D_RED) },
			{ RDefaultTextureType::TEXTURE2D_GREEN, (ENGINE_DEFAULT_TEXTURE2D_GREEN) },
			{ RDefaultTextureType::TEXTURE2D_BLUE, (ENGINE_DEFAULT_TEXTURE2D_BLUE) },
			{ RDefaultTextureType::TEXTURE2D_BUMP, (ENGINE_DEFAULT_TEXTURE2D_BUMP) },
			{ RDefaultTextureType::TEXTURE2D_PROPERTY, (ENGINE_DEFAULT_TEXTURE2D_PROPERTY) } };

		return engineDefaultTexturePathMap[texType];
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
	PE_INLINE RShaderSemanticType GetShaderSemanticBaseType(RShaderSemanticType input)
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
	PE_INLINE UINT32 GetShaderSemanticTypeSlot(RShaderSemanticType input)
	{
		if (input == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return 0u; }
		UINT32 SemanticType = input;
		return (SemanticType & 0xffu);
	}
	PE_INLINE void GetEngineDefaultMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT32& OutLayoutNum)
	{
		const static RShaderSemanticType _EngineDefaultMeshInputLayout[7u] = {
			RShaderSemanticType::SHADER_SEMANTIC_POSITION0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0,
			RShaderSemanticType::SHADER_SEMANTIC_NORMAL0,
			RShaderSemanticType::SHADER_SEMANTIC_TANGENT0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1,
			RShaderSemanticType::SHADER_SEMANTIC_COLOR0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2 };

		OutLayouts = _EngineDefaultMeshInputLayout;
		OutLayoutNum = 7u;
	}
	PE_INLINE void GetEngineDefaultSkeletalMeshInputLayouts(const RShaderSemanticType*& OutLayouts, UINT32& OutLayoutNum)
	{
		const static RShaderSemanticType _EngineDefaultSkeletalMeshInputLayout[9u] = {
			RShaderSemanticType::SHADER_SEMANTIC_POSITION0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0,
			RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES0,
			RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT0,
			RShaderSemanticType::SHADER_SEMANTIC_NORMAL0,
			RShaderSemanticType::SHADER_SEMANTIC_TANGENT0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1,
			RShaderSemanticType::SHADER_SEMANTIC_COLOR0,
			RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2 };

		OutLayouts = _EngineDefaultSkeletalMeshInputLayout;
		OutLayoutNum = 9u;
	}

};