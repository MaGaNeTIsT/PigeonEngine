#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderResource.h>
#include <ShaderAsset/ShaderReflection.h>

namespace PigeonEngine
{

	class ETexture;
	class EShaderProgram;
	class IRRHITexture;

	/*
	* ShaderAsset - value layer. Cpu only: nothing in this file creates, uploads or
	* binds a GPU object.
	*
	* Two key levels, and the split is what makes "one value, several stages" work:
	*   - a VALUE is keyed by (BlockName, FieldName) and has nothing to do with the
	*     stage: one parameter, one value.
	*   - a BUFFER is keyed by (Stage, BlockName): the same constant buffer may be
	*     culled differently per stage, so the two stages can need two different
	*     buffer sizes and must not share one allocation.
	*
	* Resolution order is fixed: reflection defaults (culled entries skipped), then
	* the parent chain from the farthest ancestor to the nearest, then this set. The
	* result is written into the per (stage, block) cpu bytes; the render thread owns
	* the constant buffers those bytes are uploaded into (RShaderParameterBuffers,
	* RenderCore/Shader/ShaderParameterBuffers.h).
	*
	* Resource typed parameters (texture / sampler / buffer / uav) produce no bytes in
	* the constant buffer: they are stored here and bound through the register slot the
	* reflection reported. A texture value holds the runtime texture object, never a
	* slot, handle or heap index - those are binding time concerns.
	*/

	enum EShaderParameterValueKindType : UINT8
	{
		SHADER_PARAMETER_VALUE_KIND_SCALAR		= 0,
		SHADER_PARAMETER_VALUE_KIND_VECTOR		= 1,
		SHADER_PARAMETER_VALUE_KIND_MATRIX		= 2,
		SHADER_PARAMETER_VALUE_KIND_TEXTURE		= 3,
		SHADER_PARAMETER_VALUE_KIND_SAMPLER		= 4,
		SHADER_PARAMETER_VALUE_KIND_BUFFER		= 5,
		SHADER_PARAMETER_VALUE_KIND_UAV			= 6,
		SHADER_PARAMETER_VALUE_KIND_COUNT,
	};

	// A 4x4 matrix is the widest value a single field can carry (16 scalars).
	static constexpr UINT32 SHADER_PARAMETER_VALUE_SCALAR_MAX = 16u;

	struct EShaderParameterValue
	{
	public:
		EShaderParameterValueKindType	Kind		= EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_SCALAR;
		// Valid components inside RawValue, row major for matrices.
		UINT32							ScalarNum	= 0u;
		// Raw 32 bit words: float / int / uint formats all use 4 byte scalars, so
		// storing the bits keeps this struct format independent.
		UINT32							RawValue[SHADER_PARAMETER_VALUE_SCALAR_MAX] = {};
		// Runtime texture object, not an asset: the value layer never reaches into
		// the cold data tier, and the render layer binds exactly what it finds here.
		const IRRHITexture*				Texture		= nullptr;
		const RStructuredBuffer*		StructuredBuffer = nullptr;
		EShaderSamplerDesc				Sampler;
	public:
		PE_NODISCARD BOOL32	IsResource() const noexcept
		{
			return (((Kind == EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_TEXTURE)
				|| (Kind == EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_SAMPLER)
				|| (Kind == EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_BUFFER)
				|| (Kind == EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_UAV)) ? TRUE : FALSE);
		}
	public:
		BOOL32	SetFloatComponents(const FLOAT* InValues, const UINT32 InNum);
		BOOL32	SetFloatValue(const FLOAT InValue);
		BOOL32	SetIntComponents(const INT32* InValues, const UINT32 InNum);
		BOOL32	SetUIntComponents(const UINT32* InValues, const UINT32 InNum);
	};

	struct EShaderParameterOverride
	{
	public:
		EName					BlockName;
		EName					FieldName;
		EShaderParameterValue	Value;
	};

	/*
	* The cpu side mirror of one (stage, block): the bytes the compiled shader reads,
	* laid out exactly as the reflection describes them. It owns no GPU object; the
	* constant buffer the bytes are uploaded into is a render tier object built from
	* this mirror (see RShaderParameterBuffers).
	*/
	class EShaderParameterBuffer
	{
	public:
		EShaderParameterBuffer(const EName& InBlockName, const EShaderStageType InStage, const UINT32 InByteSize, const BOOL32 InIsRootConstant);
		~EShaderParameterBuffer();
	public:
		PE_NODISCARD const EName&		GetBlockName() const { return BlockName; }
		PE_NODISCARD EShaderStageType	GetStage() const { return Stage; }
		PE_NODISCARD UINT32				GetByteSize() const { return ByteSize; }
		// TRUE when this block is fed through root 32 bit constants: the render tier must
		// not create a constant buffer view for it, it pushes the cpu bytes with the root
		// constant binding call instead. The mirrored bytes are built the same way
		// regardless of the flag.
		PE_NODISCARD BOOL32				IsRootConstant() const { return bIsRootConstant; }
		PE_NODISCARD BYTE*				GetCpuData();
		// Read only view the render side uploads from; null when the block is empty.
		PE_NODISCARD const BYTE*		GetCpuData() const;
		PE_NODISCARD BOOL32				IsDirty() const { return bDirty; }
	public:
		void	ZeroCpuData();
		BOOL32	WriteBytes(const UINT32 InByteOffset, const void* InData, const UINT32 InByteSize);
		// Writes one row of a scalar / vector / matrix field. Rows of a matrix are
		// 16 bytes apart, which is what the compiler's layout assumes.
		BOOL32	WriteRow(const UINT32 InRowStartByteOffset, const void* InRowData, const UINT32 InRowByteSize);
		void	MarkDirty() { bDirty = TRUE; }
		void	ClearDirty() { bDirty = FALSE; }
		// Value revision of the whole parent chain the cpu bytes were last built from. The
		// set compares it against the current chain revision, which is what keeps an
		// unchanged buffer from being re-evaluated. The render side does not read it: it
		// uploads the buffers IsDirty() reports and clears the flag once the upload went
		// through.
		PE_NODISCARD UINT32	GetUploadedChainRevision() const { return UploadedChainRevision; }
		void	SetUploadedChainRevision(const UINT32 InRevision) { UploadedChainRevision = InRevision; }
	private:
		EName			BlockName;
		EShaderStageType	Stage;
		UINT32			ByteSize;
		TArray<BYTE>	CpuBytes;
		BOOL32			bDirty;
		UINT32			UploadedChainRevision;
		// Mirrors EShaderReflectionConstantBuffer::RootConstant; kept on the mirror so the
		// render tier reads the mode from the buffer it was handed.
		BOOL32			bIsRootConstant;

	public:
		EShaderParameterBuffer() = delete;

		CLASS_REMOVE_COPY_BODY(EShaderParameterBuffer)

	};

	/*
	* A set of parameter values: the shared default set of an asset, or the private
	* set of one instance. Both use the very same class, an instance simply points its
	* parent at the asset level set so that "no override means default value" falls out
	* of the evaluation order instead of being special cased.
	*/
	class EShaderParameterSet
	{
	public:
		EShaderParameterSet();
		~EShaderParameterSet();
	public:
		PE_NODISCARD BOOL32	IsInitialized() const { return bInitialized; }
		PE_NODISCARD UINT64	GetReflectionHash() const { return ReflectionHash; }
		PE_NODISCARD UINT32	GetRevision() const { return Revision; }
		PE_NODISCARD const EShaderParameterSet*	GetParent() const { return Parent; }
	public:
		// The parent chain is applied from the farthest ancestor down to this set.
		void	SetParent(const EShaderParameterSet* InParent) { Parent = InParent; }
	public:
		// Builds one buffer per (stage, block) from the program's stage reflections
		// and seeds every field with its reflection default.
		//
		// InOnlyBlock narrows that to one block, which is what a publisher of a single block
		// asks for: the renderer's global block, a view block and a draw block each own one
		// block and pass their own name, so the mirrors - and the gpu buffers built from them -
		// cannot reach a block they do not publish. An invalid name builds every block, which is
		// the instance and asset level case: a material set holds all blocks and answers for the
		// ones it has values for. FALSE when a name was given and no stage declares it.
		BOOL32	InitializeFromProgram(const EShaderProgram& InProgram, const EName& InOnlyBlock = EName());
		void	ReleaseBuffers();
	public:
		// Setters. The key is (block name, field name) and both are EName. Resource
		// parameters (texture / sampler / buffer / uav) carry the resource name as the
		// field name and ignore the block name, because the resource tables are flat.
		//
		// A parameter the compiler culled is silently ignored: it is declared in the
		// source, but the compiled shader has no such field or slot, so storing a value
		// would only create something nothing reads and nothing binds. The setters return
		// TRUE for that case - the request is not an error, the parameter simply has no
		// effect. A name the reflection never declared is still stored (it becomes an
		// orphan) so that commenting a field out of the shader and putting it back does
		// not lose the authored value.
		BOOL32	SetScalar(const EName& InBlockName, const EName& InFieldName, const FLOAT InValue);
		// Generic entry point for vectors and matrices: raw components in row major
		// order. The field format in the reflection decides how they are laid out.
		BOOL32	SetFloatArray(const EName& InBlockName, const EName& InFieldName, const FLOAT* InValues, const UINT32 InScalarNum);
		BOOL32	SetIntArray(const EName& InBlockName, const EName& InFieldName, const INT32* InValues, const UINT32 InScalarNum);
		BOOL32	SetUIntArray(const EName& InBlockName, const EName& InFieldName, const UINT32* InValues, const UINT32 InScalarNum);
		BOOL32	SetTexture(const EName& InBlockName, const EName& InFieldName, const IRRHITexture* InTexture);
		BOOL32	SetSampler(const EName& InBlockName, const EName& InFieldName, const EShaderSamplerDesc& InSampler);
		BOOL32	SetStructuredBuffer(const EName& InBlockName, const EName& InFieldName, const RStructuredBuffer* InBuffer, const BOOL32 InIsUAV = FALSE);
	public:
		PE_NODISCARD BOOL32	HasOverride(const EName& InBlockName, const EName& InFieldName) const;
		PE_NODISCARD const EShaderParameterValue*	FindOverride(const EName& InBlockName, const EName& InFieldName) const;
		// Resource parameters (texture / sampler / buffer / uav) have no meaningful
		// block: the reflection reports them in a flat table, so binding them looks
		// them up by resource name alone.
		PE_NODISCARD const EShaderParameterValue*	FindResourceOverride(const EName& InFieldName) const;
		BOOL32	RemoveOverride(const EName& InBlockName, const EName& InFieldName);
		PE_NODISCARD UINT32	GetOverrideNum() const { return Overrides.Num<UINT32>(); }
		PE_NODISCARD const EShaderParameterOverride*	GetOverrideByIndex(const UINT32 InIndex) const;
		// Overrides whose field does not exist in the current reflection. They are
		// kept (so that commenting a field out and back in does not lose the value)
		// but reported to the editor as unused.
		PE_NODISCARD UINT32	GetOrphanNum() const { return Orphans.Num<UINT32>(); }
		PE_NODISCARD const EShaderParameterOverride*	GetOrphanByIndex(const UINT32 InIndex) const;
	public:
		PE_NODISCARD UINT32	GetParameterBufferNum() const { return Buffers.Num<UINT32>(); }
		PE_NODISCARD const EShaderParameterBuffer*	GetParameterBufferByIndex(const UINT32 InIndex) const;
		PE_NODISCARD const EShaderParameterBuffer*	FindParameterBuffer(const EShaderStageType InStage, const EName& InBlockName) const;
	public:
		// The buffers whose cpu bytes moved since the render side last took them. Iterating
		// them by index skips the clean ones, so an upload pass never has to look at the
		// whole table; GetDirtyBufferByIndex returns null past the end.
		PE_NODISCARD UINT32	GetDirtyBufferNum() const;
		PE_NODISCARD const EShaderParameterBuffer*	GetDirtyBufferByIndex(const UINT32 InIndex) const;
	public:
		// Re-evaluates every buffer (defaults, parent chain, own overrides) whose cpu bytes
		// are stale, which leaves the dirty flag set on everything it touched: uploading the
		// result into the gpu buffers is the render side's job (RShaderParameterBuffers).
		BOOL32	PrepareBuffers();
		// Cheap change counter for the render thread: it only has to compare it with
		// the value it saw last time to know whether something moved. Every mirror is
		// marked dirty as well, which is what an upload pass walks.
		void	MarkDirty();
	private:
		EShaderParameterValue*		FindOrAddOverride(const EName& InBlockName, const EName& InFieldName);
		// TRUE when no stage of the program keeps this parameter: it was declared but the
		// compiler removed it everywhere. A parameter that is live in at least one stage
		// is writable, and a name no stage ever declared is not culled either - it goes
		// down the orphan path instead.
		PE_NODISCARD BOOL32			IsParameterCulled(const EName& InBlockName, const EName& InFieldName) const;
		// Sum of the revisions along the parent chain plus this set. Any setter on any
		// ancestor moves it, which is exactly what has to invalidate a buffer.
		PE_NODISCARD UINT32			ComputeChainRevision() const;
		void	RefreshOrphans(const EShaderProgram& InProgram);
		BOOL32	EvaluateBuffer(const EShaderParameterBuffer& InBuffer, const EShaderProgram& InProgram);
		BOOL32	ApplySetToBuffer(const EShaderParameterSet& InSet, const EShaderReflection& InReflection, EShaderParameterBuffer& InOutBuffer);
		BOOL32	ApplyValueToBuffer(const EShaderReflectionField& InField, const EShaderParameterValue& InValue, EShaderParameterBuffer& InOutBuffer);
		// Defaults are written straight from the reflection raw words instead of
		// going through EShaderParameterValue: an array default easily carries more
		// than SHADER_PARAMETER_VALUE_SCALAR_MAX scalars.
		BOOL32	ApplyDefaultToBuffer(const EShaderReflectionField& InField, EShaderParameterBuffer& InOutBuffer);
	private:
		const EShaderParameterSet*		Parent;
		BOOL32							bInitialized;
		UINT64							ReflectionHash;
		UINT32							Revision;
		// The program the buffers were built from. Not owned: programs live in the
		// shader asset, and the asset bumps its revision when it is rebuilt.
		const EShaderProgram*			SourceProgram;
		TArray<EShaderParameterOverride>	Overrides;
		TArray<EShaderParameterOverride>	Orphans;
		// Buffers are heap allocated because the addresses have to stay stable while the
		// render side iterates them.
		TArray<EShaderParameterBuffer*>	Buffers;

	public:
		CLASS_REMOVE_COPY_BODY(EShaderParameterSet)

	};

} // namespace PigeonEngine
