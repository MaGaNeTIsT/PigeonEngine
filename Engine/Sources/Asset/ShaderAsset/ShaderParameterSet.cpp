#include "ShaderParameterSet.h"

#include <ShaderAsset/ShaderProgram.h>
#include <Main/MainManager.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// EShaderParameterValue
	// ---------------------------------------------------------------------------

	BOOL32 EShaderParameterValue::SetFloatComponents(const FLOAT* InValues, const UINT32 InNum)
	{
		if ((!InValues) || (InNum == 0u) || (InNum > SHADER_PARAMETER_VALUE_SCALAR_MAX))
		{
			return FALSE;
		}
		for (UINT32 ValueIdx = 0u; ValueIdx < InNum; ValueIdx++)
		{
			::memcpy_s(&(RawValue[ValueIdx]), sizeof(UINT32), &(InValues[ValueIdx]), sizeof(FLOAT));
		}
		ScalarNum = InNum;
		return TRUE;
	}

	BOOL32 EShaderParameterValue::SetFloatValue(const FLOAT InValue)
	{
		return SetFloatComponents(&InValue, 1u);
	}

	BOOL32 EShaderParameterValue::SetIntComponents(const INT32* InValues, const UINT32 InNum)
	{
		if ((!InValues) || (InNum == 0u) || (InNum > SHADER_PARAMETER_VALUE_SCALAR_MAX))
		{
			return FALSE;
		}
		for (UINT32 ValueIdx = 0u; ValueIdx < InNum; ValueIdx++)
		{
			::memcpy_s(&(RawValue[ValueIdx]), sizeof(UINT32), &(InValues[ValueIdx]), sizeof(INT32));
		}
		ScalarNum = InNum;
		return TRUE;
	}

	BOOL32 EShaderParameterValue::SetUIntComponents(const UINT32* InValues, const UINT32 InNum)
	{
		if ((!InValues) || (InNum == 0u) || (InNum > SHADER_PARAMETER_VALUE_SCALAR_MAX))
		{
			return FALSE;
		}
		for (UINT32 ValueIdx = 0u; ValueIdx < InNum; ValueIdx++)
		{
			RawValue[ValueIdx] = InValues[ValueIdx];
		}
		ScalarNum = InNum;
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// EShaderParameterBuffer
	// ---------------------------------------------------------------------------

	EShaderParameterBuffer::EShaderParameterBuffer(const EName& InBlockName, const EShaderStageType InStage, const UINT32 InByteSize, const BOOL32 InIsRootConstant)
		: BlockName(InBlockName)
		, Stage(InStage)
		, ByteSize(InByteSize)
		, CpuBytes{}
		, bDirty(TRUE)
		, UploadedChainRevision(0u)
		, bIsRootConstant(InIsRootConstant)
	{
		if (ByteSize > 0u)
		{
			CpuBytes.SetNum(ByteSize);
			ZeroCpuData();
		}
	}

	EShaderParameterBuffer::~EShaderParameterBuffer()
	{
	}

	BYTE* EShaderParameterBuffer::GetCpuData()
	{
		if (CpuBytes.Num<UINT32>() == 0u)
		{
			return nullptr;
		}
		return CpuBytes.GetData();
	}

	const BYTE* EShaderParameterBuffer::GetCpuData() const
	{
		if (CpuBytes.Num<UINT32>() == 0u)
		{
			return nullptr;
		}
		return CpuBytes.GetData();
	}

	void EShaderParameterBuffer::ZeroCpuData()
	{
		BYTE* CpuData = GetCpuData();
		if (!CpuData)
		{
			return;
		}
		::memset(CpuData, 0, static_cast<rsize_t>(ByteSize));
	}

	BOOL32 EShaderParameterBuffer::WriteBytes(const UINT32 InByteOffset, const void* InData, const UINT32 InByteSize)
	{
		if ((!InData) || (InByteSize == 0u))
		{
			return FALSE;
		}
		if ((InByteOffset + InByteSize) > ByteSize)
		{
			return FALSE;
		}
		BYTE* CpuData = GetCpuData();
		if (!CpuData)
		{
			return FALSE;
		}
		::memcpy_s((CpuData + InByteOffset), static_cast<rsize_t>(ByteSize - InByteOffset), InData, static_cast<rsize_t>(InByteSize));
		MarkDirty();
		return TRUE;
	}

	BOOL32 EShaderParameterBuffer::WriteRow(const UINT32 InRowStartByteOffset, const void* InRowData, const UINT32 InRowByteSize)
	{
		return WriteBytes(InRowStartByteOffset, InRowData, InRowByteSize);
	}

	// ---------------------------------------------------------------------------
	// EShaderParameterSet
	// ---------------------------------------------------------------------------

	EShaderParameterSet::EShaderParameterSet()
		: Parent(nullptr)
		, bInitialized(FALSE)
		, ReflectionHash(0u)
		, Revision(1u)
		, SourceProgram(nullptr)
		, Overrides{}
		, Orphans{}
		, Buffers{}
	{
	}

	EShaderParameterSet::~EShaderParameterSet()
	{
		ReleaseBuffers();
	}

	void EShaderParameterSet::ReleaseBuffers()
	{
		for (UINT32 BufferIdx = 0u, BufferNum = Buffers.Num<UINT32>(); BufferIdx < BufferNum; BufferIdx++)
		{
			if (Buffers[BufferIdx])
			{
				delete Buffers[BufferIdx];
				Buffers[BufferIdx] = nullptr;
			}
		}
		if (Buffers.Num<UINT32>() > 0u)
		{
			Buffers.Empty();
		}
		bInitialized = FALSE;
		SourceProgram = nullptr;
	}

	BOOL32 EShaderParameterSet::InitializeFromProgram(const EShaderProgram& InProgram, const EName& InOnlyBlock)
	{
		ReleaseBuffers();

		// One buffer per (stage, block): stages are compiled separately and the
		// compiler may cull the same block differently in each of them, so the two
		// stages can legitimately need different sizes.
		//
		// A valid block name narrows the walk to that block, which is how the render tier's
		// single block publishers (global, view, draw) get a set that cannot hold anything but
		// their own block. The mirror count is what the answer is measured by, so a name no
		// stage declares is reported as a failure instead of producing an empty set.
		const BOOL32 bFilterByBlock = (InOnlyBlock.IsValid() != FALSE);
		for (UINT32 EntryIdx = 0u, EntryNum = InProgram.GetStageEntryNum(); EntryIdx < EntryNum; EntryIdx++)
		{
			const EShaderStageEntry* StageEntry = InProgram.GetStageEntryByIndex(EntryIdx);
			if ((!StageEntry) || (!StageEntry->bValid))
			{
				continue;
			}
			const EShaderReflection& Reflection = StageEntry->Reflection;
			for (UINT32 CbIdx = 0u, CbNum = Reflection.ConstantBuffers.Num<UINT32>(); CbIdx < CbNum; CbIdx++)
			{
				const EShaderReflectionConstantBuffer& ConstantBuffer = Reflection.ConstantBuffers[CbIdx];
				if (ConstantBuffer.bCompilerCulled)
				{
					continue;
				}
				// A declared but empty block (a full screen program reports the engine blocks that
				// way) has no layout to mirror and no bytes to upload, so it gets no mirror: an
				// empty one would only make a set look like it answers for a block it cannot.
				if (ConstantBuffer.ByteSize == 0u)
				{
					continue;
				}
				if ((bFilterByBlock != FALSE) && (ConstantBuffer.Name != InOnlyBlock))
				{
					continue;
				}
				EShaderParameterBuffer* NewBuffer = new EShaderParameterBuffer(ConstantBuffer.Name, StageEntry->Stage, ConstantBuffer.ByteSize, ConstantBuffer.RootConstant);
				Buffers.Add(NewBuffer);
			}
		}

		if ((bFilterByBlock != FALSE) && (Buffers.Num<UINT32>() == 0u))
		{
			return FALSE;
		}

		SourceProgram = (&InProgram);
		ReflectionHash = 0u;
		bInitialized = TRUE;
		RefreshOrphans(InProgram);
		MarkDirty();
		return TRUE;
	}

	void EShaderParameterSet::RefreshOrphans(const EShaderProgram& InProgram)
	{
		// An override survives a reflection change: keeping it means that commenting a
		// field out of the shader and putting it back does not lose the authored value.
		TArray<EShaderParameterOverride> KeptOverrides;
		for (UINT32 OverrideIdx = 0u, OverrideNum = Overrides.Num<UINT32>(); OverrideIdx < OverrideNum; OverrideIdx++)
		{
			const EShaderParameterOverride& Override = Overrides[OverrideIdx];
			BOOL32 bFound = FALSE;
			for (UINT32 EntryIdx = 0u, EntryNum = InProgram.GetStageEntryNum(); EntryIdx < EntryNum; EntryIdx++)
			{
				const EShaderStageEntry* StageEntry = InProgram.GetStageEntryByIndex(EntryIdx);
				if ((!StageEntry) || (!StageEntry->bValid))
				{
					continue;
				}
				if (StageEntry->Reflection.FindParameterTarget(Override.BlockName, Override.FieldName)
					!= EShaderParameterTargetType::SHADER_PARAMETER_TARGET_MISSING)
				{
					bFound = TRUE;
					break;
				}
			}
			if (bFound)
			{
				KeptOverrides.Add(Override);
			}
			else
			{
				Orphans.Add(Override);
			}
		}
		Overrides = KeptOverrides;
	}

	BOOL32 EShaderParameterSet::IsParameterCulled(const EName& InBlockName, const EName& InFieldName) const
	{
		if (!SourceProgram)
		{
			return FALSE;
		}
		BOOL32 bHasCulledTarget = FALSE;
		for (UINT32 EntryIdx = 0u, EntryNum = SourceProgram->GetStageEntryNum(); EntryIdx < EntryNum; EntryIdx++)
		{
			const EShaderStageEntry* StageEntry = SourceProgram->GetStageEntryByIndex(EntryIdx);
			if ((!StageEntry) || (!StageEntry->bValid))
			{
				continue;
			}
			const EShaderParameterTargetType TargetType = StageEntry->Reflection.FindParameterTarget(InBlockName, InFieldName);
			if (TargetType == EShaderParameterTargetType::SHADER_PARAMETER_TARGET_ACTIVE)
			{
				// Live in one stage is enough: the value has to reach that stage.
				return FALSE;
			}
			if (TargetType == EShaderParameterTargetType::SHADER_PARAMETER_TARGET_CULLED)
			{
				bHasCulledTarget = TRUE;
			}
		}
		// Culled everywhere it was declared. A target that no stage declares at all is not
		// culled: it takes the orphan path so an authored value survives a temporary
		// removal from the source.
		return bHasCulledTarget;
	}

	EShaderParameterValue* EShaderParameterSet::FindOrAddOverride(const EName& InBlockName, const EName& InFieldName)
	{
		for (UINT32 OverrideIdx = 0u, OverrideNum = Overrides.Num<UINT32>(); OverrideIdx < OverrideNum; OverrideIdx++)
		{
			EShaderParameterOverride& Override = Overrides[OverrideIdx];
			if ((Override.BlockName == InBlockName) && (Override.FieldName == InFieldName))
			{
				return (&(Override.Value));
			}
		}
		EShaderParameterOverride NewOverride;
		NewOverride.BlockName = InBlockName;
		NewOverride.FieldName = InFieldName;
		Overrides.Add(NewOverride);
		return (&(Overrides[Overrides.Num<UINT32>() - 1u].Value));
	}

	BOOL32 EShaderParameterSet::HasOverride(const EName& InBlockName, const EName& InFieldName) const
	{
		return ((FindOverride(InBlockName, InFieldName) != nullptr) ? TRUE : FALSE);
	}

	const EShaderParameterValue* EShaderParameterSet::FindOverride(const EName& InBlockName, const EName& InFieldName) const
	{
		for (UINT32 OverrideIdx = 0u, OverrideNum = Overrides.Num<UINT32>(); OverrideIdx < OverrideNum; OverrideIdx++)
		{
			const EShaderParameterOverride& Override = Overrides[OverrideIdx];
			if ((Override.BlockName == InBlockName) && (Override.FieldName == InFieldName))
			{
				return (&(Override.Value));
			}
		}
		return nullptr;
	}

	const EShaderParameterValue* EShaderParameterSet::FindResourceOverride(const EName& InFieldName) const
	{
		for (UINT32 OverrideIdx = 0u, OverrideNum = Overrides.Num<UINT32>(); OverrideIdx < OverrideNum; OverrideIdx++)
		{
			const EShaderParameterOverride& Override = Overrides[OverrideIdx];
			if ((Override.FieldName == InFieldName) && (Override.Value.IsResource()))
			{
				return (&(Override.Value));
			}
		}
		return nullptr;
	}

	BOOL32 EShaderParameterSet::RemoveOverride(const EName& InBlockName, const EName& InFieldName)	{
		for (UINT32 OverrideIdx = 0u, OverrideNum = Overrides.Num<UINT32>(); OverrideIdx < OverrideNum; OverrideIdx++)
		{
			const EShaderParameterOverride& Override = Overrides[OverrideIdx];
			if ((Override.BlockName == InBlockName) && (Override.FieldName == InFieldName))
			{
				Overrides.RemoveAt(OverrideIdx);
				MarkDirty();
				return TRUE;
			}
		}
		return FALSE;
	}

	const EShaderParameterOverride* EShaderParameterSet::GetOverrideByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= Overrides.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(Overrides[InIndex]));
	}

	const EShaderParameterOverride* EShaderParameterSet::GetOrphanByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= Orphans.Num<UINT32>())
		{
			return nullptr;
		}
		return (&(Orphans[InIndex]));
	}

	BOOL32 EShaderParameterSet::SetScalar(const EName& InBlockName, const EName& InFieldName, const FLOAT InValue)
	{
		if (IsParameterCulled(InBlockName, InFieldName))
		{
			// Declared but removed by the compiler: storing the value would create
			// something nothing reads and nothing binds, so it is dropped silently.
			return TRUE;
		}
		EShaderParameterValue* Value = FindOrAddOverride(InBlockName, InFieldName);
		if (!Value)
		{
			return FALSE;
		}
		Value->Kind = EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_SCALAR;
		if (!Value->SetFloatValue(InValue))
		{
			return FALSE;
		}
		MarkDirty();
		return TRUE;
	}

	BOOL32 EShaderParameterSet::SetFloatArray(const EName& InBlockName, const EName& InFieldName, const FLOAT* InValues, const UINT32 InScalarNum)
	{
		if (IsParameterCulled(InBlockName, InFieldName))
		{
			// Declared but removed by the compiler: storing the value would create
			// something nothing reads and nothing binds, so it is dropped silently.
			return TRUE;
		}
		EShaderParameterValue* Value = FindOrAddOverride(InBlockName, InFieldName);
		if (!Value)
		{
			return FALSE;
		}
		Value->Kind = ((InScalarNum > 1u)
			? EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_VECTOR
			: EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_SCALAR);
		if (!Value->SetFloatComponents(InValues, InScalarNum))
		{
			return FALSE;
		}
		MarkDirty();
		return TRUE;
	}

	BOOL32 EShaderParameterSet::SetIntArray(const EName& InBlockName, const EName& InFieldName, const INT32* InValues, const UINT32 InScalarNum)
	{
		if (IsParameterCulled(InBlockName, InFieldName))
		{
			// Declared but removed by the compiler: storing the value would create
			// something nothing reads and nothing binds, so it is dropped silently.
			return TRUE;
		}
		EShaderParameterValue* Value = FindOrAddOverride(InBlockName, InFieldName);
		if (!Value)
		{
			return FALSE;
		}
		Value->Kind = EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_VECTOR;
		if (!Value->SetIntComponents(InValues, InScalarNum))
		{
			return FALSE;
		}
		MarkDirty();
		return TRUE;
	}

	BOOL32 EShaderParameterSet::SetUIntArray(const EName& InBlockName, const EName& InFieldName, const UINT32* InValues, const UINT32 InScalarNum)
	{
		if (IsParameterCulled(InBlockName, InFieldName))
		{
			// Declared but removed by the compiler: storing the value would create
			// something nothing reads and nothing binds, so it is dropped silently.
			return TRUE;
		}
		EShaderParameterValue* Value = FindOrAddOverride(InBlockName, InFieldName);
		if (!Value)
		{
			return FALSE;
		}
		Value->Kind = EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_VECTOR;
		if (!Value->SetUIntComponents(InValues, InScalarNum))
		{
			return FALSE;
		}
		MarkDirty();
		return TRUE;
	}

	BOOL32 EShaderParameterSet::SetTexture(const EName& InBlockName, const EName& InFieldName, const IRRHITexture* InTexture)
	{
		if (IsParameterCulled(InBlockName, InFieldName))
		{
			// Declared but removed by the compiler: storing the value would create
			// something nothing reads and nothing binds, so it is dropped silently.
			return TRUE;
		}
		EShaderParameterValue* Value = FindOrAddOverride(InBlockName, InFieldName);
		if (!Value)
		{
			return FALSE;
		}
		Value->Kind = EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_TEXTURE;
		Value->ScalarNum = 0u;
		Value->Texture = InTexture;
		MarkDirty();
		return TRUE;
	}

	BOOL32 EShaderParameterSet::SetSampler(const EName& InBlockName, const EName& InFieldName, const EShaderSamplerDesc& InSampler)
	{
		if (IsParameterCulled(InBlockName, InFieldName))
		{
			// Declared but removed by the compiler: storing the value would create
			// something nothing reads and nothing binds, so it is dropped silently.
			return TRUE;
		}
		EShaderParameterValue* Value = FindOrAddOverride(InBlockName, InFieldName);
		if (!Value)
		{
			return FALSE;
		}
		Value->Kind = EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_SAMPLER;
		Value->ScalarNum = 0u;
		Value->Sampler = InSampler;
		MarkDirty();
		return TRUE;
	}

	BOOL32 EShaderParameterSet::SetStructuredBuffer(const EName& InBlockName, const EName& InFieldName, const RStructuredBuffer* InBuffer, const BOOL32 InIsUAV)
	{
		if (IsParameterCulled(InBlockName, InFieldName))
		{
			// Declared but removed by the compiler: storing the value would create
			// something nothing reads and nothing binds, so it is dropped silently.
			return TRUE;
		}
		EShaderParameterValue* Value = FindOrAddOverride(InBlockName, InFieldName);
		if (!Value)
		{
			return FALSE;
		}
		Value->Kind = (InIsUAV
			? EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_UAV
			: EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_BUFFER);
		Value->ScalarNum = 0u;
		Value->StructuredBuffer = InBuffer;
		MarkDirty();
		return TRUE;
	}

	const EShaderParameterBuffer* EShaderParameterSet::GetParameterBufferByIndex(const UINT32 InIndex) const
	{
		if (InIndex >= Buffers.Num<UINT32>())
		{
			return nullptr;
		}
		return Buffers[InIndex];
	}

	const EShaderParameterBuffer* EShaderParameterSet::FindParameterBuffer(const EShaderStageType InStage, const EName& InBlockName) const
	{
		for (UINT32 BufferIdx = 0u, BufferNum = Buffers.Num<UINT32>(); BufferIdx < BufferNum; BufferIdx++)
		{
			const EShaderParameterBuffer* ParameterBuffer = Buffers[BufferIdx];
			if (!ParameterBuffer)
			{
				continue;
			}
			if ((ParameterBuffer->GetStage() == InStage) && (ParameterBuffer->GetBlockName() == InBlockName))
			{
				return ParameterBuffer;
			}
		}
		return nullptr;
	}

	UINT32 EShaderParameterSet::GetDirtyBufferNum() const
	{
		UINT32 DirtyNum = 0u;
		for (UINT32 BufferIdx = 0u, BufferNum = Buffers.Num<UINT32>(); BufferIdx < BufferNum; BufferIdx++)
		{
			const EShaderParameterBuffer* ParameterBuffer = Buffers[BufferIdx];
			if ((ParameterBuffer) && (ParameterBuffer->IsDirty()))
			{
				DirtyNum++;
			}
		}
		return DirtyNum;
	}

	const EShaderParameterBuffer* EShaderParameterSet::GetDirtyBufferByIndex(const UINT32 InIndex) const
	{
		UINT32 DirtyIdx = 0u;
		for (UINT32 BufferIdx = 0u, BufferNum = Buffers.Num<UINT32>(); BufferIdx < BufferNum; BufferIdx++)
		{
			const EShaderParameterBuffer* ParameterBuffer = Buffers[BufferIdx];
			if ((!ParameterBuffer) || (!(ParameterBuffer->IsDirty())))
			{
				continue;
			}
			if (DirtyIdx == InIndex)
			{
				return ParameterBuffer;
			}
			DirtyIdx++;
		}
		return nullptr;
	}

	void EShaderParameterSet::MarkDirty()
	{
		Revision++;
		for (UINT32 BufferIdx = 0u, BufferNum = Buffers.Num<UINT32>(); BufferIdx < BufferNum; BufferIdx++)
		{
			if (Buffers[BufferIdx])
			{
				Buffers[BufferIdx]->MarkDirty();
			}
		}
	}

	BOOL32 EShaderParameterSet::ApplyValueToBuffer(const EShaderReflectionField& InField, const EShaderParameterValue& InValue, EShaderParameterBuffer& InOutBuffer)
	{
		// Resource typed values carry no bytes: they are bound through their register
		// slot, which is the binding plan's business.
		if (InValue.IsResource())
		{
			return TRUE;
		}
		if (InField.bCompilerCulled)
		{
			return TRUE;
		}

		EShaderFieldWriteDesc WriteDesc;
		if (!InField.BuildWriteDesc(WriteDesc))
		{
			return FALSE;
		}
		if (InValue.ScalarNum == 0u)
		{
			return FALSE;
		}
		if (InValue.ScalarNum > WriteDesc.ScalarNum)
		{
			PE_LOG_WARN(EString("[ShaderParameterSet] Field '") + InField.Name.ToString() + EString("' received ")
				+ ToString(InValue.ScalarNum) + EString(" components but the format only has ")
				+ ToString(WriteDesc.ScalarNum) + EString("; the extra values are ignored."));
		}

		// An array override writes the same value to every element: per element
		// overrides are a follow up, the value struct would carry the element index.
		for (UINT32 ElementIdx = 0u; ElementIdx < WriteDesc.ElementNum; ElementIdx++)
		{
			const UINT32 ElementBase = WriteDesc.StartByteOffset + (WriteDesc.ElementStrideBytes * ElementIdx);
			for (UINT32 RowIdx = 0u; RowIdx < WriteDesc.RowNum; RowIdx++)
			{
				const UINT32 ScalarStart = (RowIdx * WriteDesc.ColumnNum);
				if (ScalarStart >= InValue.ScalarNum)
				{
					break;
				}
				UINT32 RowScalarNum = WriteDesc.ColumnNum;
				if ((ScalarStart + RowScalarNum) > InValue.ScalarNum)
				{
					RowScalarNum = InValue.ScalarNum - ScalarStart;
				}
				const UINT32 RowByteOffset = ElementBase + (WriteDesc.RowStrideBytes * RowIdx);
				const UINT32 RowWriteBytes = RowScalarNum * InField.Format.GetScalarByteSize();
				if (!InOutBuffer.WriteRow(RowByteOffset, &(InValue.RawValue[ScalarStart]), RowWriteBytes))
				{
					PE_LOG_ERROR(EString("[ShaderParameterSet] Field '") + InField.Name.ToString()
						+ EString("' does not fit into buffer '") + InOutBuffer.GetBlockName().ToString()
						+ EString("' (offset ") + ToString(RowByteOffset)
						+ EString(", bytes ") + ToString(RowWriteBytes) + EString(")."));
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	BOOL32 EShaderParameterSet::ApplyDefaultToBuffer(const EShaderReflectionField& InField, EShaderParameterBuffer& InOutBuffer)
	{
		const UINT32 DefaultScalarNum = InField.Default.Num<UINT32>();
		if (DefaultScalarNum == 0u)
		{
			return TRUE;
		}

		EShaderFieldWriteDesc WriteDesc;
		if (!InField.BuildWriteDesc(WriteDesc))
		{
			return TRUE;
		}

		const UINT32 TotalScalarNum = WriteDesc.ElementNum * WriteDesc.ScalarNum;
		if (DefaultScalarNum > TotalScalarNum)
		{
			PE_LOG_WARN(EString("[ShaderParameterSet] Field '") + InField.Name.ToString() + EString("' carries ")
				+ ToString(DefaultScalarNum) + EString(" default values but only ")
				+ ToString(TotalScalarNum) + EString(" fit into the declared layout."));
		}

		// Default words are authored in declaration order: element major, then row,
		// then column. Padding between rows and elements is skipped, which is exactly
		// what the write description describes.
		for (UINT32 ElementIdx = 0u; ElementIdx < WriteDesc.ElementNum; ElementIdx++)
		{
			for (UINT32 RowIdx = 0u; RowIdx < WriteDesc.RowNum; RowIdx++)
			{
				const UINT32 ScalarStart = (ElementIdx * WriteDesc.ScalarNum) + (RowIdx * WriteDesc.ColumnNum);
				if (ScalarStart >= DefaultScalarNum)
				{
					return TRUE;
				}
				UINT32 RowScalarNum = WriteDesc.ColumnNum;
				if ((ScalarStart + RowScalarNum) > DefaultScalarNum)
				{
					RowScalarNum = DefaultScalarNum - ScalarStart;
				}
				const UINT32 RowByteOffset = WriteDesc.StartByteOffset
					+ (WriteDesc.ElementStrideBytes * ElementIdx)
					+ (WriteDesc.RowStrideBytes * RowIdx);
				if (!InOutBuffer.WriteRow(RowByteOffset, &(InField.Default[ScalarStart]), RowScalarNum * InField.Format.GetScalarByteSize()))
				{
					PE_LOG_ERROR(EString("[ShaderParameterSet] Default value of field '") + InField.Name.ToString()
						+ EString("' does not fit into buffer '") + InOutBuffer.GetBlockName().ToString()
						+ EString("' (offset ") + ToString(RowByteOffset) + EString(")."));
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	BOOL32 EShaderParameterSet::ApplySetToBuffer(const EShaderParameterSet& InSet, const EShaderReflection& InReflection, EShaderParameterBuffer& InOutBuffer)
	{
		for (UINT32 OverrideIdx = 0u, OverrideNum = InSet.GetOverrideNum(); OverrideIdx < OverrideNum; OverrideIdx++)
		{
			const EShaderParameterOverride* Override = InSet.GetOverrideByIndex(OverrideIdx);
			if (!Override)
			{
				continue;
			}
			if (Override->BlockName != InOutBuffer.GetBlockName())
			{
				continue;
			}
			const EShaderReflectionField* Field = InReflection.FindField(Override->BlockName, Override->FieldName);
			if (!Field)
			{
				continue;
			}
			if (!ApplyValueToBuffer(*Field, Override->Value, InOutBuffer))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	BOOL32 EShaderParameterSet::EvaluateBuffer(const EShaderParameterBuffer& InBuffer, const EShaderProgram& InProgram)
	{
		const EShaderStageEntry* StageEntry = InProgram.FindStageEntry(InBuffer.GetStage());
		if ((!StageEntry) || (!StageEntry->bValid))
		{
			return FALSE;
		}
		const EShaderReflection& Reflection = StageEntry->Reflection;
		const EShaderReflectionConstantBuffer* ConstantBuffer = Reflection.FindConstantBuffer(InBuffer.GetBlockName());
		if (!ConstantBuffer)
		{
			return FALSE;
		}

		EShaderParameterBuffer* MutableBuffer = const_cast<EShaderParameterBuffer*>(&InBuffer);
		MutableBuffer->ZeroCpuData();

		// 1. Reflection defaults, culled entries skipped: the compiler dropped them,
		//    so there is nothing to seed.
		if (!ConstantBuffer->bCompilerCulled)
		{
			for (UINT32 FieldIdx = 0u, FieldNum = ConstantBuffer->Fields.Num<UINT32>(); FieldIdx < FieldNum; FieldIdx++)
			{
				const EShaderReflectionField& Field = ConstantBuffer->Fields[FieldIdx];
				if (Field.bCompilerCulled)
				{
					continue;
				}
				if (!ApplyDefaultToBuffer(Field, *MutableBuffer))
				{
					return FALSE;
				}
			}
		}

		// 2. The parent chain, farthest ancestor first, then this set: "no override
		//    means default value" is the result of this order, not a special case.
		static constexpr UINT32 ParentChainMaxDepth = 16u;
		const EShaderParameterSet* ParentChain[ParentChainMaxDepth] = {};
		UINT32 ParentDepth = 0u;
		const EShaderParameterSet* IterSet = Parent;
		while ((IterSet) && (ParentDepth < ParentChainMaxDepth))
		{
			ParentChain[ParentDepth++] = IterSet;
			IterSet = IterSet->GetParent();
		}
		for (UINT32 ChainIdx = ParentDepth; ChainIdx > 0u; ChainIdx--)
		{
			if (!ApplySetToBuffer(*(ParentChain[ChainIdx - 1u]), Reflection, *MutableBuffer))
			{
				return FALSE;
			}
		}
		if (!ApplySetToBuffer(*this, Reflection, *MutableBuffer))
		{
			return FALSE;
		}
		return TRUE;
	}

	UINT32 EShaderParameterSet::ComputeChainRevision() const
	{
		UINT32 ChainRevision = Revision;
		const EShaderParameterSet* IterSet = Parent;
		// Bounded walk: a cycle in the parent chain must not hang the loader.
		UINT32 Depth = 0u;
		while ((IterSet) && (Depth < 16u))
		{
			ChainRevision += IterSet->GetRevision();
			IterSet = IterSet->GetParent();
			Depth++;
		}
		return ChainRevision;
	}

	BOOL32 EShaderParameterSet::PrepareBuffers()
	{
		if (!bInitialized)
		{
			return FALSE;
		}
		if (!SourceProgram)
		{
			return FALSE;
		}

		const UINT32 ChainRevision = ComputeChainRevision();
		BOOL32 bResult = TRUE;
		for (UINT32 BufferIdx = 0u, BufferNum = Buffers.Num<UINT32>(); BufferIdx < BufferNum; BufferIdx++)
		{
			EShaderParameterBuffer* ParameterBuffer = Buffers[BufferIdx];
			if (!ParameterBuffer)
			{
				continue;
			}
			// Nothing on the chain moved since the last build, so the buffer already
			// matches the values and does not have to be touched at all.
			if (ParameterBuffer->GetUploadedChainRevision() == ChainRevision)
			{
				continue;
			}
			if (!EvaluateBuffer(*ParameterBuffer, *SourceProgram))
			{
				PE_LOG_ERROR(EString("[ShaderParameterSet] Cannot evaluate buffer '") + ParameterBuffer->GetBlockName().ToString()
					+ EString("' for stage ") + ToString(static_cast<UINT32>(ParameterBuffer->GetStage())) + EString("."));
				bResult = FALSE;
				continue;
			}
			// The evaluation writes every value, which marks the buffer dirty: the render
			// side uploads exactly the buffers IsDirty() reports and clears the flag itself.
			ParameterBuffer->SetUploadedChainRevision(ChainRevision);
		}
		return bResult;
	}

} // namespace PigeonEngine
