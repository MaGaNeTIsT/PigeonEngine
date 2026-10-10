#include <Shader/ShaderParameterBuffers.h>

#include <RHI/IRRHIDevice.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	RShaderParameterBuffers::RShaderParameterBuffers()
		: Entries{}
	{
	}

	RShaderParameterBuffers::~RShaderParameterBuffers()
	{
		ReleaseRenderResource();
	}

	RShaderParameterBuffers::RShaderParameterBuffers(const RShaderParameterBuffers& Other)
		: Entries{}
	{
	}

	RShaderParameterBuffers& RShaderParameterBuffers::operator=(const RShaderParameterBuffers& Other)
	{
		if ((&Other) != this)
		{
			ReleaseRenderResource();
		}
		return (*this);
	}

	BOOL8 RShaderParameterBuffers::IsRenderResourceValid() const noexcept
	{
		if (Entries.Num<UINT32>() == 0u)
		{
			return FALSE;
		}
		for (UINT32 EntryIdx = 0u, EntryNum = Entries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			const FEntry* Entry = Entries[EntryIdx];
			if ((!Entry) || (!(Entry->Buffer.IsRenderResourceValid())))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	BOOL8 RShaderParameterBuffers::InitRenderResource() noexcept
	{
		// The buffers are created by CreateFromParameterSet, which needs the mirrors to
		// size them; there is nothing a raw init could do here.
		return IsRenderResourceValid();
	}

	void RShaderParameterBuffers::ReleaseRenderResource() noexcept
	{
		// Walked from the end: dropping an entry at the visited index only shifts the ones
		// after it, so every entry is still destroyed exactly once.
		for (UINT32 EntryIdx = Entries.Num<UINT32>(); EntryIdx > 0u; EntryIdx--)
		{
			DestroyEntry(Entries[EntryIdx - 1u]);
		}
		if (Entries.Num<UINT32>() > 0u)
		{
			Entries.Empty();
		}
	}

	void RShaderParameterBuffers::DestroyEntry(FEntry* InEntry)
	{
		if (!InEntry)
		{
			return;
		}
		InEntry->Buffer.ReleaseRenderResource();
		for (UINT32 EntryIdx = 0u, EntryNum = Entries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (Entries[EntryIdx] != InEntry)
			{
				continue;
			}
			Entries.RemoveAt(EntryIdx);
			break;
		}
		delete InEntry;
	}

	RShaderParameterBuffers::FEntry* RShaderParameterBuffers::FindEntry(const EShaderStageType InStage, const EName& InBlockName) const
	{
		for (UINT32 EntryIdx = 0u, EntryNum = Entries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			FEntry* Entry = Entries[EntryIdx];
			if (!Entry)
			{
				continue;
			}
			if ((Entry->Stage == InStage) && (Entry->BlockName == InBlockName))
			{
				return Entry;
			}
		}
		return nullptr;
	}

	RShaderParameterBuffers::FEntry* RShaderParameterBuffers::FindEntryOfBlock(const EName& InBlockName) const
	{
		// The stage of the first entry that carries the block: a block publisher writes values,
		// not stages, so every mirror of one block holds the same bytes and any of them is the
		// right buffer to bind for a stage this table has no mirror for.
		for (UINT32 EntryIdx = 0u, EntryNum = Entries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			FEntry* Entry = Entries[EntryIdx];
			if (!Entry)
			{
				continue;
			}
			if (Entry->BlockName == InBlockName)
			{
				return Entry;
			}
		}
		return nullptr;
	}

	BOOL32 RShaderParameterBuffers::GetRootConstantData(const EShaderStageType InStage, const EName& InBlockName
		, const BYTE*& OutData, UINT32& OutNum32BitValues) const
	{
		OutData = nullptr;
		OutNum32BitValues = 0u;
		const FEntry* Entry = FindEntry(InStage, InBlockName);
		if (!Entry)
		{
			// The stage the program reads the block in is not the stage its publisher was laid
			// out from; the bytes are the same either way.
			Entry = FindEntryOfBlock(InBlockName);
		}
		if ((!Entry) || (!Entry->bIsRootConstant))
		{
			return FALSE;
		}
		const UINT32 ByteSize = Entry->RootConstantData.Num<UINT32>();
		// Root constants are counted in 32 bit values, so a block whose size is not a whole
		// number of them cannot be expressed and is reported as absent instead of truncated.
		if ((ByteSize == 0u) || ((ByteSize % 4u) != 0u))
		{
			return FALSE;
		}
		OutData = Entry->RootConstantData.GetData();
		OutNum32BitValues = ByteSize / 4u;
		return TRUE;
	}

	RShaderParameterBuffers::FEntry* RShaderParameterBuffers::CreateEntry(IRRHIDevice* InRenderDevice, const EShaderParameterBuffer& InMirror) const
	{
		const UINT32 ByteSize = InMirror.GetByteSize();
		if ((!InRenderDevice) || (ByteSize == 0u))
		{
			return nullptr;
		}
		FEntry* Entry = new FEntry();
		Entry->Stage = InMirror.GetStage();
		Entry->BlockName = InMirror.GetBlockName();
		Entry->bIsRootConstant = InMirror.IsRootConstant();
		if (!(Entry->Buffer.InitFromRHI(InRenderDevice
			, RBufferDesc(static_cast<UINT64>(ByteSize), RBindFlagType::BIND_CONSTANT_BUFFER, 0u))))
		{
			PE_LOG_ERROR(EString("[ShaderParameterBuffers] Cannot create the constant buffer of '")
				+ InMirror.GetBlockName().ToString() + EString("' (") + ToString(ByteSize) + EString(" bytes)."));
			delete Entry;
			return nullptr;
		}
		return Entry;
	}

	BOOL32 RShaderParameterBuffers::CreateFromParameterSet(IRRHIDevice* InRenderDevice, const EShaderParameterSet& InParameters)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		// Rebuilding: a set can be re-sized by a rebuilt program, so the old table must not
		// survive next to the new one.
		ReleaseRenderResource();

		BOOL32 bResult = TRUE;
		for (UINT32 BufferIdx = 0u, BufferNum = InParameters.GetParameterBufferNum(); BufferIdx < BufferNum; BufferIdx++)
		{
			const EShaderParameterBuffer* Mirror = InParameters.GetParameterBufferByIndex(BufferIdx);
			if (!Mirror)
			{
				continue;
			}
			FEntry* Entry = CreateEntry(InRenderDevice, *Mirror);
			if (!Entry)
			{
				bResult = FALSE;
				continue;
			}
			Entries.Add(Entry);
		}
		return bResult;
	}

	BOOL32 RShaderParameterBuffers::UploadEntry(IRRHIDevice* InRenderDevice, const EShaderParameterBuffer& InMirror)
	{
		const BYTE* CpuData = InMirror.GetCpuData();
		if (!CpuData)
		{
			return FALSE;
		}
		const UINT32 ByteSize = InMirror.GetByteSize();
		FEntry* Entry = FindEntry(InMirror.GetStage(), InMirror.GetBlockName());
		if ((Entry) && (Entry->Buffer.GetByteSize() != ByteSize))
		{
			// The block changed size (the program was rebuilt), so the existing buffer cannot
			// take the new bytes and is replaced instead of being written past its end.
			DestroyEntry(Entry);
			Entry = nullptr;
		}
		if (!Entry)
		{
			// Either the set grew after the table was built or the entry was just dropped:
			// the buffer is created here rather than leaving the block unbound.
			Entry = CreateEntry(InRenderDevice, InMirror);
			if (!Entry)
			{
				return FALSE;
			}
			Entries.Add(Entry);
		}
		Entry->Buffer.UploadDataViaRHI(InRenderDevice, CpuData, static_cast<SIZE_T>(ByteSize));
		// A root constant block is written into the command list rather than read from the
		// buffer, so its current bytes are mirrored here as well.
		if (Entry->bIsRootConstant)
		{
			Entry->RootConstantData.SetNum(ByteSize);
			if (ByteSize > 0u)
			{
				EMemory::Memcpy(Entry->RootConstantData.GetData(), CpuData, static_cast<SIZE_T>(ByteSize));
			}
		}
		// The render side is the only owner of "the gpu copy is current", so it is also the
		// side that retires the asset side's dirty flag once the bytes reached the device.
		const_cast<EShaderParameterBuffer&>(InMirror).ClearDirty();
		return TRUE;
	}

	BOOL32 RShaderParameterBuffers::UploadFromParameterSet(IRRHIDevice* InRenderDevice, const EShaderParameterSet& InParameters)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		BOOL32 bResult = TRUE;
		// Walked from the end: an upload retires one dirty flag, which shifts the entries
		// that follow it. The ones still to be visited keep their index, so no mirror is
		// skipped and none is uploaded twice.
		for (UINT32 DirtyIdx = InParameters.GetDirtyBufferNum(); DirtyIdx > 0u; DirtyIdx--)
		{
			const EShaderParameterBuffer* Mirror = InParameters.GetDirtyBufferByIndex(DirtyIdx - 1u);
			if (!Mirror)
			{
				continue;
			}
			if (!UploadEntry(InRenderDevice, *Mirror))
			{
				bResult = FALSE;
			}
		}
		return bResult;
	}

	const RBufferResource* RShaderParameterBuffers::FindBuffer(const EShaderStageType InStage, const EName& InBlockName) const
	{
		const FEntry* Entry = FindEntry(InStage, InBlockName);
		if (!Entry)
		{
			// Same fallback as GetRootConstantData: the block's buffer belongs to the stage its
			// publisher was laid out from, and a program reading it in another stage binds it.
			Entry = FindEntryOfBlock(InBlockName);
		}
		return (Entry ? (&(Entry->Buffer)) : nullptr);
	}

	IRRHIBuffer* RShaderParameterBuffers::GetResolvedBuffer(const EShaderStageType InStage, const EName& InBlockName) const
	{
		const RBufferResource* Buffer = FindBuffer(InStage, InBlockName);
		if ((!Buffer) || (!(Buffer->IsRenderResourceValid())))
		{
			return nullptr;
		}
		return const_cast<RBufferResource*>(Buffer)->GetResolvedRHIBuffer();
	}

} // namespace PigeonEngine
