#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <document.h>


namespace MC
{

#define MC_STRINGIFY_IMPL(__Txt)			#__Txt
#define MC_STRINGIFY(__Txt)					MC_STRINGIFY_IMPL(__Txt)
#define MC_CONCAT_IMPL(__TxtL, __TxtR)		__TxtL##__TxtR
#define MC_CONCAT(__TxtL, __TxtR)			MC_CONCAT_IMPL(__TxtL, __TxtR)

	struct MCDefine
	{
		int				Value		= 1;
		std::string		Name		= "";
	};

	struct MCFinalDefine
	{
		std::map<std::string, int>		DefineMappings		= {};
		std::vector<MCDefine>			Defines				= {};
	};

	enum class MCConditionOpType : uint8_t
	{
		Equal,				// ==
		NotEqual,			// !=
		Greater,			// >
		GreaterEqual,		// >=
		Less,				// <
		LessEqual,			// <=
	};

	struct MCConditionField
	{
		int						Ref				= 0;
		MCConditionOpType		Op				= MCConditionOpType::GreaterEqual;
		std::string				DefineName		= "";
	};

	enum class MCVarFormatPrefixType : uint8_t
	{
		Var_Int,
		Var_Uint,
		Var_Float,
	};

	enum class MCVarFormatSuffixType : uint8_t
	{
		Var_1,
		Var_2,
		Var_3,
		Var_4,
		Var_2x2,
		Var_3x2,
		Var_2x3,
		Var_3x3,
		Var_4x2,
		Var_2x4,
		Var_4x3,
		Var_3x4,
		Var_4x4,
	};

	struct MCVarFormat
	{
		MCVarFormatPrefixType		Prefix		= MCVarFormatPrefixType::Var_Float;
		MCVarFormatSuffixType		Suffix		= MCVarFormatSuffixType::Var_4;
	};

	struct MCVFInput
	{
		int				SemanticIdx		= 0;
		int				Slot			= 0;
		MCVarFormat		Format			= {};
		std::string		Name			= "";
		std::string		Semantic		= "";
	};

	struct MCVFInputTemplate
	{
		MCVFInput				Input			= {};
		MCConditionField		Condition		= {};
	};

	struct MCVFInputStream
	{
		std::string					Name		= "";
		std::vector<MCVFInput>		Inputs		= {};
	};

	struct MCVariableInput
	{
		int							ArrayNumElems		= 0;			// Used for arrays; 0 = not an array, >0 = number of elements
		MCVarFormat					Format				= {};
		std::string					Name				= "";
		std::vector<uint32_t>		Default				= {};
		bool						bArray				= false;
		bool						bColor				= false;
		bool						bExposure			= false;
	};

	struct MCVariableInputTemplate
	{
		MCVariableInput			Input			= {};
		MCConditionField		Condition		= {};
	};

	struct MCConstantBuffer
	{
		int				Slot		= 0;
		std::string			Name		= "";
		std::vector<MCVariableInput>	Fields		= {};
		bool				bRootConstant	= false;	// Marked by the shader json: this buffer is fed through root 32 bit constants.
	};

	enum class MCTextureSamplerFilterType : uint8_t
	{
		Tex_Filter_Point,
		Tex_Filter_MinMagLinear,
		Tex_Filter_MinMagMipLinear,
		Tex_Filter_Anisotropic,
	};

	enum class MCTexSamplerAddressModeType : uint8_t
	{
		Tex_Sampler_Wrap,
		Tex_Sampler_Mirror,
		Tex_Sampler_Clamp,
		Tex_Sampler_Border,
		Tex_Sampler_MirrorOnce,
	};

	enum class MCTextureSamplerComparisonFuncType : uint8_t
	{
		Tex_ComparisonFunc_Never,
		Tex_ComparisonFunc_Less,
		Tex_ComparisonFunc_Equal,
		Tex_ComparisonFunc_LessEqual,
		Tex_ComparisonFunc_Greater,
		Tex_ComparisonFunc_NotEqual,
		Tex_ComparisonFunc_GreaterEqual,
		Tex_ComparisonFunc_Always,
	};

	struct MCTextureSampler
	{
		float									BorderColor[4]		= { 0.0f, 0.0f, 0.0f, 0.0f };
		int										Slot				= 0;
		float									MinLOD				= 0.0f;
		float									MaxLOD				= 1024.0f;
		float									LODBias				= 0.0f;
		int										Anisotropy			= 1;
		MCTextureSamplerFilterType				Filter				= MCTextureSamplerFilterType::Tex_Filter_Point;
		MCTexSamplerAddressModeType				AddressU			= MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
		MCTexSamplerAddressModeType				AddressV			= MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
		MCTexSamplerAddressModeType				AddressW			= MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
		MCTextureSamplerComparisonFuncType		ComparisonFunc		= MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Never;
		std::string								Name				= "";
		bool									bExposure			= false;
	};

	struct MCTextureSamplerTemplate
	{
		MCTextureSampler		Sampler			= {};
		MCConditionField		Condition		= {};
	};

	enum class MCTextureType : uint8_t
	{
		Tex_1D,
		Tex_2D,
		Tex_3D,
		Tex_1DArray,
		Tex_2DArray,
		Tex_3DArray,
		Tex_Cube,
		Tex_CubeArray,
	};

	struct MCTexture
	{
		int						Slot			= 0;
		MCVarFormat				Format			= {};
		MCTextureType			Type			= MCTextureType::Tex_2D;
		std::string				Name			= "";
		std::string				SamplerName		= "";
		bool					bExposure		= false;
	};

	struct MCTextureTemplate
	{
		MCTexture				Texture			= {};
		MCConditionField		Condition		= {};
	};

	enum class MCShaderResourceType : uint8_t
	{
		Buff_Buffer,
		Buff_ByteAddressBuffer,
		Buff_StructuredBuffer,
	};

	struct MCShaderResource
	{
		int									Slot				= 0;
		MCShaderResourceType				Type				= MCShaderResourceType::Buff_Buffer;
		MCVarFormat							ElemFmt				= {};
		std::string							Name				= "";
		std::vector<MCVariableInput>		StructFields		= {};
		bool								bHasStruct			= false;
		bool								bExposure			= false;
	};

	struct MCShaderResourceTemplate
	{
		MCShaderResource		Resource		= {};
		MCConditionField		Condition		= {};
	};

	struct MCReplace
	{
		std::string		Name		= "";
		std::string		Element		= "";
	};

	struct MCFinalReplace
	{
		std::map<std::string, std::string>		ReplaceMappings		= {};
		std::vector<MCReplace>					Replaces			= {};
	};

	struct MCShaderBindings
	{
		std::vector<MCConstantBuffer>		CBuffers		= {};
		std::vector<MCTextureSampler>		TexSamplers		= {};
		std::vector<MCTexture>				Textures		= {};
		std::vector<MCShaderResource>		Resources		= {};
		std::vector<MCTexture>				RWTextures		= {};
		std::vector<MCShaderResource>		RWResources		= {};
	};

	struct MCShaderComponent
	{
		std::string						Template		= "";
		std::vector<MCDefine>			Defines			= {};
		std::vector<MCReplace>			Replaces		= {};
	};

	struct MCShaderBase
	{
		MCShaderComponent		ShaderComponent		= {};
		MCShaderBindings		Bindings			= {};
	};

	struct MCVertexFactory
	{
		MCVFInputStream		InputStream		= {};
		MCShaderBase		Shader			= {};
		std::string			Name			= "";
	};

	struct MCGeneralShader
	{
		MCShaderBase		Shader		= {};
		std::string			Name		= "";
	};

	enum class MCShaderBaseReplaceType : uint8_t
	{
		ShaderReplace_VertexFactory,
		ShaderReplace_General,
	};

	struct MCShaderBaseReplace
	{
		int							Index			= 0;
		MCShaderBaseReplaceType		Type			= MCShaderBaseReplaceType::ShaderReplace_General;
		std::string					Name			= "";
		std::string					RplElem			= "";
		std::string					JsonFile		= "";
	};

	struct MCShader
	{
		MCShaderBase							Shader			= {};
		std::vector<MCShaderBaseReplace>		ShaderRpls		= {};
		std::string								Name			= "";
		std::string								Entry			= "";
	};

	struct MCLoadParam
	{
		std::string		RootDir				= "";			// Base directory of every relative path inside the json chain (keeps its trailing '/').
		std::string		JsonFilePath			= "";			// Entry json, relative to RootDir.
		std::string		Stage				= "";			// Compile target label, written into the reflection output for double verification.
		std::string		OutDir				= "";			// Directory the reflection output is written to (keeps its trailing '/').
		std::string		OutFileName			= "";			// File name of the reflection output, decided by the caller.
		std::string		DefHash				= "";			// Reserved: build identity, meant for future ShaderMap / logging.
		std::string		RplHash				= "";			// Reserved: build identity, meant for future ShaderMap / logging.
		bool			bSM66				= false;		// Reserved: SM66 dedicated features are not consumed by the parser yet.
	};

	bool ParseShader(const MCLoadParam& InLoadParam, std::vector<MCDefine>& InOutDefs, const std::vector<MCReplace>& InRpls, std::string& OutHlsl);

}	// namespace MC
