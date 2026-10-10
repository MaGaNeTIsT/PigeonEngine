#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Base/DataStructure/Text/Name.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - reflection layer.
	*
	* This file mirrors ShaderCompiler's reflection json one to one:
	*   - every member name equals the json key it is parsed from (ByteOffset stays
	*     ByteOffset, Exposure stays Exposure). That is an intentional exception to
	*     the engine b-prefix rule: a literal field mapping removes mapping bugs.
	*   - the reflection json is produced per stage, so one EShaderReflection always
	*     describes exactly one stage. There is no cross stage merge in this layer.
	*
	* Layout semantics reported by the compiler (must not be re-derived here):
	*   ByteOffset   : offset of the variable inside its constant buffer.
	*   ContentBytes : span from the variable start to the end of the last element
	*                  content. Interior padding is included, the trailing slot
	*                  padding of the last element is not. Example: float aaa[7]
	*                  occupies 112 slots but reports 100 = 6 * 16 + 4.
	*                  It is therefore NOT a stride and must never be used as one.
	*   ArrayStride  : byte distance between two array elements, 0 when not an array.
	*   Zero ByteOffset / ContentBytes / ArrayStride means "unknown", not "empty".
	*   CompilerCulled is emitted per entry for six groups: CBuffers (entry), CBuffers[].Fields[],
	*   Textures, Resources, Samplers, RWTextures and RWResources. true means "this declaration
	*   is not in the compiled result of this stage" -> the runtime must skip binding and
	*   writing it, and the editor labels it unused in this variant. false is a reliable
	*   "present" signal, including for constant buffer members that the shader never reads
	*   (the compiler does not cull CB members by reference).
	*   The compiler decides the flags itself: constant buffers by name, resource sections by
	*   (kind, slot) because optimized builds strip resource names. The engine therefore only
	*   reads the flags and never re-derives presence by name. The input layout is never
	*   culled: an input signature is what it is.
	*   Sections that were never declared are absent from the json entirely, so every section
	*   is optional and a missing key means "no such section", not "empty / invalid".
	*/

	// ---------------------------------------------------------------------------
	// Stage / shader model
	// ---------------------------------------------------------------------------

	enum EShaderStageType : UINT8
	{
		SHADER_STAGE_VERTEX			= 0,
		SHADER_STAGE_PIXEL			= 1,
		SHADER_STAGE_COMPUTE		= 2,
		// Reserved for the stages the pipeline will grow into; keep them after the
		// three stages the compiler can currently emit so the serialized order stays.
		SHADER_STAGE_GEOMETRY		= 3,
		SHADER_STAGE_HULL			= 4,
		SHADER_STAGE_DOMAIN			= 5,
		SHADER_STAGE_COUNT,
	};

	enum EShaderStageMask : UINT32
	{
		SHADER_STAGE_MASK_NONE		= 0u,
		SHADER_STAGE_MASK_VERTEX	= (1u << 0),
		SHADER_STAGE_MASK_PIXEL		= (1u << 1),
		SHADER_STAGE_MASK_COMPUTE	= (1u << 2),
		SHADER_STAGE_MASK_GEOMETRY	= (1u << 3),
		SHADER_STAGE_MASK_HULL		= (1u << 4),
		SHADER_STAGE_MASK_DOMAIN	= (1u << 5),
	};

	enum EShaderModelType : UINT8
	{
		SHADER_MODEL_5_0			= 0,
		SHADER_MODEL_6_0			= 1,
		SHADER_MODEL_6_6			= 2,
		SHADER_MODEL_COUNT,
	};

	// "VS" / "PS" / "CS" - the exact strings the compiler writes into the compile
	// description and echoes back in the manifest / reflection.
	PE_NODISCARD extern BOOL32	TranslateShaderStageToString(const EShaderStageType InStage, EString& OutStr);
	PE_NODISCARD extern BOOL32	ParseShaderStageFromString(const EString& InStr, EShaderStageType& OutStage);
	// "PEVS" / "PEPS" / "PECS" - the token the compiler appends to artifact names.
	PE_NODISCARD extern BOOL32	GetShaderStageSuffix(const EShaderStageType InStage, EString& OutStr);
	PE_NODISCARD extern BOOL32	GetShaderStageMaskBit(const EShaderStageType InStage, UINT32& OutMaskBit);

	// "5_0" / "6_0" / "6_6" - the compiler's ShaderModel string.
	PE_NODISCARD extern BOOL32	TranslateShaderModelToString(const EShaderModelType InShaderModel, EString& OutStr);
	PE_NODISCARD extern BOOL32	ParseShaderModelFromString(const EString& InStr, EShaderModelType& OutShaderModel);
	// "ShaderModel_5_0" / "ShaderModel_6_0" / "ShaderModel_6_6" - the output directory level.
	PE_NODISCARD extern BOOL32	GetShaderModelDirectoryName(const EShaderModelType InShaderModel, EString& OutStr);

	// ---------------------------------------------------------------------------
	// Value format - prefix and suffix are kept apart on purpose: "float" is the
	// empty suffix, so "float" / "float4" / "float4x4" cannot be split by digits.
	// ---------------------------------------------------------------------------

	enum EShaderFieldFormatPrefixType : UINT8
	{
		SHADER_FIELD_FORMAT_PREFIX_INT		= 0,
		SHADER_FIELD_FORMAT_PREFIX_UINT		= 1,
		SHADER_FIELD_FORMAT_PREFIX_FLOAT	= 2,
		SHADER_FIELD_FORMAT_PREFIX_COUNT,
	};

	enum EShaderFieldFormatSuffixType : UINT8
	{
		SHADER_FIELD_FORMAT_SUFFIX_1		= 0,
		SHADER_FIELD_FORMAT_SUFFIX_2		= 1,
		SHADER_FIELD_FORMAT_SUFFIX_3		= 2,
		SHADER_FIELD_FORMAT_SUFFIX_4		= 3,
		SHADER_FIELD_FORMAT_SUFFIX_2X2		= 4,
		SHADER_FIELD_FORMAT_SUFFIX_3X2		= 5,
		SHADER_FIELD_FORMAT_SUFFIX_2X3		= 6,
		SHADER_FIELD_FORMAT_SUFFIX_3X3		= 7,
		SHADER_FIELD_FORMAT_SUFFIX_4X2		= 8,
		SHADER_FIELD_FORMAT_SUFFIX_2X4		= 9,
		SHADER_FIELD_FORMAT_SUFFIX_4X3		= 10,
		SHADER_FIELD_FORMAT_SUFFIX_3X4		= 11,
		SHADER_FIELD_FORMAT_SUFFIX_4X4		= 12,
		SHADER_FIELD_FORMAT_SUFFIX_COUNT,
	};

	// Constant buffer packing rule this engine relies on: a matrix row and an array
	// element always start on a 16 byte boundary. It is the HLSL default (the
	// compiler is invoked without -Zpr), and it is what ByteOffset + ArrayStride
	// based addressing assumes.
	static constexpr UINT32 SHADER_FIELD_ROW_STRIDE_BYTES = 16u;

	struct EShaderFieldFormat
	{
	public:
		EShaderFieldFormatPrefixType	Prefix	= EShaderFieldFormatPrefixType::SHADER_FIELD_FORMAT_PREFIX_FLOAT;
		EShaderFieldFormatSuffixType	Suffix	= EShaderFieldFormatSuffixType::SHADER_FIELD_FORMAT_SUFFIX_4;
	public:
		PE_NODISCARD BOOL32	IsMatrix() const noexcept;
		PE_NODISCARD UINT32	GetRowNum() const noexcept;
		PE_NODISCARD UINT32	GetColumnNum() const noexcept;
		// Row * Column: the number of scalar 32 bit values of one element.
		PE_NODISCARD UINT32	GetScalarNum() const noexcept;
		PE_NODISCARD UINT32	GetScalarByteSize() const noexcept;
		// Bytes one element occupies in the buffer (rows padded to 16 bytes).
		PE_NODISCARD UINT32	GetElementSlotBytes() const noexcept;
	};

	PE_NODISCARD extern BOOL32	ParseShaderFieldFormatString(const EString& InStr, EShaderFieldFormat& OutFormat);
	PE_NODISCARD extern BOOL32	MakeShaderFieldFormatString(const EShaderFieldFormat& InFormat, EString& OutStr);

	// ---------------------------------------------------------------------------
	// Sampler description (mirrors the reflection Samplers[] entry)
	// ---------------------------------------------------------------------------

	enum EShaderSamplerFilterType : UINT8
	{
		SHADER_SAMPLER_FILTER_POINT				= 0,
		SHADER_SAMPLER_FILTER_MIN_MAG_LINEAR	= 1,
		SHADER_SAMPLER_FILTER_MIN_MAG_MIP_LINEAR = 2,
		SHADER_SAMPLER_FILTER_ANISOTROPIC		= 3,
		SHADER_SAMPLER_FILTER_COUNT,
	};

	enum EShaderSamplerAddressModeType : UINT8
	{
		SHADER_SAMPLER_ADDRESS_WRAP			= 0,
		SHADER_SAMPLER_ADDRESS_MIRROR		= 1,
		SHADER_SAMPLER_ADDRESS_CLAMP		= 2,
		SHADER_SAMPLER_ADDRESS_BORDER		= 3,
		SHADER_SAMPLER_ADDRESS_MIRROR_ONCE	= 4,
		SHADER_SAMPLER_ADDRESS_COUNT,
	};

	enum EShaderSamplerComparisonFuncType : UINT8
	{
		SHADER_SAMPLER_COMPARISON_NEVER			= 0,
		SHADER_SAMPLER_COMPARISON_LESS			= 1,
		SHADER_SAMPLER_COMPARISON_EQUAL			= 2,
		SHADER_SAMPLER_COMPARISON_LESS_EQUAL	= 3,
		SHADER_SAMPLER_COMPARISON_GREATER		= 4,
		SHADER_SAMPLER_COMPARISON_NOT_EQUAL		= 5,
		SHADER_SAMPLER_COMPARISON_GREATER_EQUAL	= 6,
		SHADER_SAMPLER_COMPARISON_ALWAYS		= 7,
		SHADER_SAMPLER_COMPARISON_COUNT,
	};

	struct EShaderSamplerDesc
	{
	public:
		EShaderSamplerFilterType			Filter			= EShaderSamplerFilterType::SHADER_SAMPLER_FILTER_POINT;
		EShaderSamplerAddressModeType		AddressU		= EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_CLAMP;
		EShaderSamplerAddressModeType		AddressV		= EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_CLAMP;
		EShaderSamplerAddressModeType		AddressW		= EShaderSamplerAddressModeType::SHADER_SAMPLER_ADDRESS_CLAMP;
		EShaderSamplerComparisonFuncType	ComparisonFunc	= EShaderSamplerComparisonFuncType::SHADER_SAMPLER_COMPARISON_NEVER;
		FLOAT								MinLOD			= 0.0f;
		FLOAT								MaxLOD			= 1024.0f;
		FLOAT								LODBias			= 0.0f;
		UINT32								Anisotropy		= 1u;
		FLOAT								BorderColor[4]	= { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	// ---------------------------------------------------------------------------
	// Texture / resource classification (values match the reflection strings)
	// ---------------------------------------------------------------------------

	enum EShaderTextureType : UINT8
	{
		SHADER_TEXTURE_TYPE_1D			= 0,
		SHADER_TEXTURE_TYPE_2D			= 1,
		SHADER_TEXTURE_TYPE_3D			= 2,
		SHADER_TEXTURE_TYPE_1D_ARRAY	= 3,
		SHADER_TEXTURE_TYPE_2D_ARRAY	= 4,
		SHADER_TEXTURE_TYPE_3D_ARRAY	= 5,
		SHADER_TEXTURE_TYPE_CUBE		= 6,
		SHADER_TEXTURE_TYPE_CUBE_ARRAY	= 7,
		SHADER_TEXTURE_TYPE_COUNT,
	};

	enum EShaderResourceKindType : UINT8
	{
		SHADER_RESOURCE_KIND_BUFFER				= 0,
		SHADER_RESOURCE_KIND_STRUCTURED_BUFFER	= 1,
		SHADER_RESOURCE_KIND_BYTE_ADDRESS_BUFFER = 2,
		SHADER_RESOURCE_KIND_COUNT,
	};

	// ---------------------------------------------------------------------------
	// Reflection blocks
	// ---------------------------------------------------------------------------

	// Forward declarations have to match the keyword the definition uses: the compiler
	// treats struct / class as different spellings of the same type and warns (C4099) when
	// they disagree, which the build promotes to an error.
	struct EShaderReflectionConstantBuffer;
	class EShaderReflection;

	// How one field value has to be written into the cpu side constant buffer.
	// Only this description may be used to locate a write, never ContentBytes.
	struct EShaderFieldWriteDesc
	{
	public:
		UINT32	StartByteOffset		= 0u;	// first element, first row
		UINT32	ElementNum			= 1u;	// 1 when the field is not an array
		UINT32	ElementStrideBytes	= 0u;	// ArrayStride reported by the compiler
		UINT32	RowNum				= 1u;	// 1 for scalar and vector, N for matrix
		UINT32	ColumnNum			= 1u;	// scalars per row
		UINT32	RowStrideBytes		= SHADER_FIELD_ROW_STRIDE_BYTES;
		UINT32	ScalarNum			= 1u;	// RowNum * ColumnNum
	};

	struct EShaderReflectionField
	{
	public:
		EName			Name;
		EName			BlockName;			// owning constant buffer, keys the value lookup
		EShaderFieldFormat	Format;
		UINT32			ByteOffset			= 0u;
		UINT32			ContentBytes		= 0u;
		UINT32			ArrayStride			= 0u;
		BOOL32			bCompilerCulled		= FALSE;
		// Exposure means "visible and editable in the editor". Parameters that are
		// not exposed are still stored and uploaded, they just stay out of the UI.
		BOOL32			Exposure			= FALSE;
		BOOL32			IsColor				= FALSE;
		BOOL32			IsArray				= FALSE;
		UINT32			NumElements			= 0u;
		// Default values as raw 32 bit words in row major order, exactly the way the
		// compiler emitted them. Format independent so they can be written straight
		// into the constant buffer through the write description below.
		TArray<UINT32>	Default;
	public:
		PE_NODISCARD UINT32		GetElementNum() const noexcept;
		PE_NODISCARD UINT32		GetDefaultScalarNum() const noexcept;
		PE_NODISCARD BOOL32		BuildWriteDesc(EShaderFieldWriteDesc& OutDesc) const noexcept;
		// Byte offset just past the last scalar this field can write. Used by the
		// editor only checks; returns 0 when the layout is unknown.
		PE_NODISCARD UINT32		GetSpanEndByteOffset() const noexcept;
	};

	struct EShaderReflectionConstantBuffer
	{
	public:
		EName							Name;
		UINT32							Slot			= 0u;
		// Shader visible size of the buffer, reported by the compiler reflection.
		// The cpu side constant buffer is allocated with exactly this size.
		UINT32							ByteSize		= 0u;
		BOOL32							bCompilerCulled	= FALSE;
		// Marked by the shader source ("RootConstant"): this buffer is fed through root
		// 32 bit constants, so it has no constant buffer view. The compiler enforces the
		// two rules that come with the marking (at most one such buffer per assembled
		// shader, and ByteSize within MC_ROOT_CONSTANT_CB_MAX_BYTES), which is why this
		// stays a flag and not a size. The engine keeps the cpu bytes of such a block but
		// never creates a buffer for it: the render layer pushes them with the root
		// constant binding call instead.
		BOOL32							RootConstant	= FALSE;
		TArray<EShaderReflectionField>	Fields;
	public:
		PE_NODISCARD const EShaderReflectionField*	FindField(const EName& InFieldName) const;
	};

	struct EShaderReflectionTexture
	{
	public:
		EName				Name;
		EShaderFieldFormat	Format;
		EShaderTextureType	Type			= EShaderTextureType::SHADER_TEXTURE_TYPE_2D;
		UINT32				Slot			= 0u;
		// Empty when the texture has no sampler assigned. An empty EName is invalid,
		// so test SamplerName.IsValid() instead of looking it up.
		EName				SamplerName;
		BOOL32				Exposure		= FALSE;
		BOOL32				bIsRW			= FALSE;
		// Declared but not present in the compiled shader: not bound, not bindable.
		BOOL32				bCompilerCulled	= FALSE;
	};

	struct EShaderReflectionResource
	{
	public:
		EName							Name;
		UINT32							Slot			= 0u;
		EShaderResourceKindType			Kind			= EShaderResourceKindType::SHADER_RESOURCE_KIND_BUFFER;
		BOOL32							bIsStruct		= FALSE;
		TArray<EShaderReflectionField>	StructFields;
		EShaderFieldFormat				Format;
		BOOL32							Exposure		= FALSE;
		BOOL32							bIsRW			= FALSE;
		// Declared but not present in the compiled shader: not bound, not bindable.
		BOOL32							bCompilerCulled	= FALSE;
	};

	struct EShaderReflectionSampler
	{
	public:
		EName				Name;
		UINT32				Slot		= 0u;
		EShaderSamplerDesc	Desc;
		BOOL32				Exposure	= FALSE;
		// Declared but not present in the compiled shader: not bound, not bindable.
		BOOL32				bCompilerCulled	= FALSE;
	};

	struct EShaderReflectionInputLayout
	{
	public:
		EName				Name;
		EShaderFieldFormat	Format;
		// Kept as the compiler wrote it (e.g. "POSITION", "TEXCOORD"): translating to
		// the render semantic enum belongs to the render layer, not to this module.
		EName				Semantic;
		UINT32				SemanticIndex	= 0u;
		UINT32				Slot			= 0u;
	};

	// ---------------------------------------------------------------------------
	// EShaderReflection - one instance per stage, one to one with the reflection json
	// ---------------------------------------------------------------------------

	// What a parameter name refers to in one stage's reflection.
	enum EShaderParameterTargetType : UINT8
	{
		// The reflection never declared it. Kept as an override so that commenting a
		// field out of the shader and putting it back does not lose the authored value.
		SHADER_PARAMETER_TARGET_MISSING		= 0,
		// Declared, but the compiler removed it from the compiled shader.
		SHADER_PARAMETER_TARGET_CULLED		= 1,
		// Declared and present: readable / writable / bindable.
		SHADER_PARAMETER_TARGET_ACTIVE		= 2,
		SHADER_PARAMETER_TARGET_COUNT,
	};

	class EShaderReflection
	{
	public:
		EName								ShaderName;
		EName								EntryPoint;
		EShaderStageType					Stage		= EShaderStageType::SHADER_STAGE_VERTEX;
		EShaderModelType					ShaderModel	= EShaderModelType::SHADER_MODEL_6_6;
		TArray<EShaderReflectionConstantBuffer>	ConstantBuffers;
		TArray<EShaderReflectionTexture>	Textures;
		TArray<EShaderReflectionTexture>	RWTextures;
		TArray<EShaderReflectionResource>	Resources;
		TArray<EShaderReflectionResource>	RWResources;
		TArray<EShaderReflectionSampler>	Samplers;
		TArray<EShaderReflectionInputLayout>	InputLayouts;
		// Identity of the parsed source. Used to dedupe reflections inside one asset.
		UINT64								SourceHash	= 0u;
	public:
		PE_NODISCARD const EShaderReflectionConstantBuffer*	FindConstantBuffer(const EName& InName) const;
		PE_NODISCARD const EShaderReflectionField*			FindField(const EName& InBlockName, const EName& InFieldName) const;
		PE_NODISCARD const EShaderReflectionTexture*		FindTexture(const EName& InName, const BOOL32 InIsRW = FALSE) const;
		PE_NODISCARD const EShaderReflectionResource*		FindResource(const EName& InName, const BOOL32 InIsRW = FALSE) const;
		PE_NODISCARD const EShaderReflectionSampler*		FindSampler(const EName& InName) const;
		PE_NODISCARD BOOL32	HasAnyLayout() const noexcept;
		PE_NODISCARD UINT32	GetCulledFieldNum() const noexcept;
		// Culled textures, buffers and samplers (both the SRV and the UAV tables).
		PE_NODISCARD UINT32	GetCulledResourceNum() const noexcept;
		PE_NODISCARD UINT32	GetExposedFieldNum() const noexcept;
		// Where a parameter name lands in this reflection, whether it is a constant
		// buffer field or one of the resource tables. Resource parameters are keyed by
		// the resource name, so their block name is ignored.
		PE_NODISCARD EShaderParameterTargetType	FindParameterTarget(const EName& InBlockName, const EName& InParameterName) const;
	};

	// ---------------------------------------------------------------------------
	// Parsing
	// ---------------------------------------------------------------------------

	PE_NODISCARD extern BOOL32	ParseShaderReflectionJson(const EString& InReflectionJsonPath, EShaderReflection& OutReflection);
	PE_NODISCARD extern BOOL32	ParseShaderReflectionText(const EString& InReflectionJsonText, EShaderReflection& OutReflection);

} // namespace PigeonEngine
