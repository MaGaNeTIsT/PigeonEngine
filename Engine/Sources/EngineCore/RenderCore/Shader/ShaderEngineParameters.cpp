#include "ShaderEngineParameters.h"

#include <ShaderAsset/ShaderProgram.h>
#include <ShaderAsset/ShaderStageAsset.h>
#include <Shader/ShaderAssetBinding.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/IRCommandList.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	void FlattenShaderMatrix(const Matrix4x4& InMatrix, FLOAT* OutScalars)
	{
		// Row major, no transpose: the shader side matrix is the transpose of this one,
		// which is what mul(Matrix, Vector) expects for a row vector cpu matrix. See the
		// convention note in the header before changing the loop order.
		for (UINT32 Row = 0u; Row < 4u; Row++)
		{
			for (UINT32 Column = 0u; Column < 4u; Column++)
			{
				OutScalars[(Row * 4u) + Column] = InMatrix(Row, Column);
			}
		}
	}

	BOOL32 WriteShaderMatrix(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const Matrix4x4& InMatrix)
	{
		FLOAT Scalars[16] = {};
		FlattenShaderMatrix(InMatrix, Scalars);
		return (InOutParameters.SetFloatArray(InBlockName, InFieldName, Scalars, 16u));
	}

	BOOL32 WriteShaderVector4(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const Vector4& InValue)
	{
		const FLOAT Scalars[4] = { InValue.x, InValue.y, InValue.z, InValue.w };
		return (InOutParameters.SetFloatArray(InBlockName, InFieldName, Scalars, 4u));
	}

	BOOL32 WriteShaderFloatArray(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const FLOAT* InValues, const UINT32 InScalarNum)
	{
		if ((!InValues) || (InScalarNum == 0u))
		{
			return FALSE;
		}
		return (InOutParameters.SetFloatArray(InBlockName, InFieldName, InValues, InScalarNum));
	}

	BOOL32 WriteShaderScalar(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const FLOAT InValue)
	{
		return (InOutParameters.SetScalar(InBlockName, InFieldName, InValue));
	}

	BOOL32 WriteShaderUIntArray(EShaderParameterSet& InOutParameters
		, const EName& InBlockName, const EName& InFieldName, const UINT32* InValues, const UINT32 InScalarNum)
	{
		if ((!InValues) || (InScalarNum == 0u))
		{
			return FALSE;
		}
		return (InOutParameters.SetUIntArray(InBlockName, InFieldName, InValues, InScalarNum));
	}

	namespace
	{
		// Names the render tier's own block objects publish. Written where a block object is built
		// - engine init for the renderer's block, the render thread for the proxies' and the
		// class prototypes - and read by the material bind, which runs on the render thread. The
		// table only ever grows and only holds a handful of interned names.
		TArray<EName>& GetRenderTierShaderBlocks()
		{
			static TArray<EName> Blocks;
			return Blocks;
		}
	}

	BOOL32 IsRenderTierShaderBlock(const EName& InBlockName)
	{
		if (!InBlockName.IsValid())
		{
			return FALSE;
		}
		const TArray<EName>& Blocks = GetRenderTierShaderBlocks();
		for (UINT32 BlockIdx = 0u, BlockNum = Blocks.Num<UINT32>(); BlockIdx < BlockNum; BlockIdx++)
		{
			if (Blocks[BlockIdx] == InBlockName)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	void RegisterRenderTierShaderBlock(const EName& InBlockName)
	{
		if ((!InBlockName.IsValid()) || (IsRenderTierShaderBlock(InBlockName) != FALSE))
		{
			return;
		}
		GetRenderTierShaderBlocks().Add(InBlockName);
	}

	RShaderParameterBlock::RShaderParameterBlock(const EName& InBlockName)
		: BlockName(InBlockName)
		, bInitialized(FALSE)
		, Parameters{}
		, Buffers{}
	{
		// One name, one kind of publisher: the render tier owns the blocks it publishes, so the
		// name becomes visible to the material bind here and nowhere else.
		RegisterRenderTierShaderBlock(BlockName);
	}
	RShaderParameterBlock::RShaderParameterBlock(const RShaderParameterBlock& Other)
		: BlockName(Other.BlockName)
		, bInitialized(FALSE)
		, Parameters{}
		, Buffers{}
	{
		RegisterRenderTierShaderBlock(BlockName);
	}
	RShaderParameterBlock::~RShaderParameterBlock()
	{
	}

	BOOL32 RShaderParameterBlock::HasBlock(const EShaderProgram& InProgram, const EName& InBlockName)
	{
		for (UINT32 EntryIdx = 0u, EntryNum = InProgram.GetStageEntryNum(); EntryIdx < EntryNum; EntryIdx++)
		{
			const EShaderStageEntry* StageEntry = InProgram.GetStageEntryByIndex(EntryIdx);
			if ((!StageEntry) || (!StageEntry->bValid))
			{
				continue;
			}
			const EShaderReflectionConstantBuffer* ConstantBuffer = StageEntry->Reflection.FindConstantBuffer(InBlockName);
			// A zero byte block is a declared but empty one - a full screen program reports the
			// engine blocks that way - and it cannot describe a layout. Taking it as "present"
			// would latch the block with nothing behind it, and the latch is forever: the
			// program that does carry the block would never get to supply the layout.
			if ((ConstantBuffer) && (!ConstantBuffer->bCompilerCulled) && (ConstantBuffer->ByteSize > 0u))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	BOOL32 RShaderParameterBlock::EnsureInitialized(IRRHIDevice* InRenderDevice, const EShaderProgram& InProgram)
	{
		if (bInitialized)
		{
			return TRUE;
		}
		if ((!InRenderDevice) || (!BlockName.IsValid()))
		{
			return FALSE;
		}
		if (!HasBlock(InProgram, BlockName))
		{
			// This program cannot describe the layout of the block, so it stays
			// uninitialized instead of latching an empty one: a later program that does
			// declare it still gets to supply the layout.
			return FALSE;
		}
		// Scoped to this block: the mirrors, and therefore the gpu buffers built from them, are
		// this block's only. Reaching this point with no mirror means the name the object was
		// built with and the name the reflection reports disagree, which is worth a log rather
		// than a silently inert block.
		if (!Parameters.InitializeFromProgram(InProgram, BlockName))
		{
			PE_LOG_ERROR(EString("[ShaderParameterBlock] Cannot build the mirrors of '") + BlockName.ToString()
				+ EString("' from the program's reflection."));
			return FALSE;
		}
		// Creating the buffers up front keeps the first bind from racing the first upload;
		// the upload path creates any entry the set grows later on its own.
		Buffers.CreateFromParameterSet(InRenderDevice, Parameters);
		bInitialized = TRUE;
		return TRUE;
	}

	void RShaderParameterBlock::ReleaseRenderResources()
	{
		// Both halves are dropped and the initialized flag with them: the mirrors describe a
		// layout that belongs to the program they were built from, and the buffers belong to the
		// device. Leaving either behind would let a restarted owner bind a buffer of the previous
		// device, or keep a layout no program of this session declared.
		Buffers.ReleaseRenderResource();
		Parameters.ReleaseBuffers();
		bInitialized = FALSE;
	}

	BOOL32 RShaderParameterBlock::Upload(IRRHIDevice* InRenderDevice)
	{
		if ((!bInitialized) || (!InRenderDevice))
		{
			return FALSE;
		}
		// Re-evaluates what moved and leaves it dirty; the upload walks the dirty mirrors
		// and retires their flags, so an unchanged block costs one revision compare.
		Parameters.PrepareBuffers();
		return (Buffers.UploadFromParameterSet(InRenderDevice, Parameters));
	}

	void RShaderParameterBlock::Bind(IRCommandList* InCommandList, const EShaderProgram& InProgram) const
	{
		if ((!bInitialized) || (!InCommandList))
		{
			return;
		}
		// Every stage the program can have is offered the block. Which stage actually
		// carries it is the reflection's call and a stage without it is skipped, so a
		// binding can never put a buffer where the shader would read something else.
		// The name filter is what keeps this object on its own block: the set holds only this
		// block's mirrors, and the bind refuses anything else even if the set ever grew.
		BindShaderParameterBlock(InCommandList, const_cast<RShaderParameterBuffers&>(Buffers)
			, InProgram, EShaderStageType::SHADER_STAGE_VERTEX, FALSE, BlockName);
		BindShaderParameterBlock(InCommandList, const_cast<RShaderParameterBuffers&>(Buffers)
			, InProgram, EShaderStageType::SHADER_STAGE_PIXEL, FALSE, BlockName);
		BindShaderParameterBlock(InCommandList, const_cast<RShaderParameterBuffers&>(Buffers)
			, InProgram, EShaderStageType::SHADER_STAGE_COMPUTE, TRUE, BlockName);
	}

} // namespace PigeonEngine
