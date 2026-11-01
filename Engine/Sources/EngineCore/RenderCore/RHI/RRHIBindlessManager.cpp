#include <RHI/RRHIBindlessManager.h>

#include <RHI/IRRHIResource.h>
#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>
#include <Editor/EditorLogManager.h>
#include <MultiThread/Atomic.h>

namespace PigeonEngine
{

	// The identity counter. File scope and atomic on purpose: the first resources of the engine's
	// life are created on the game thread, everything after them on the render thread, and the
	// counter has to answer both without either waiting. It starts at 1 because 0 is the value
	// every reader uses for "no identity" - a counter that could hand out 0 would make a real
	// object indistinguishable from an unidentified one.
	PE_CONSTEXPR_STATIC UINT64 RHI_RESOURCE_UNIQUE_ID_FIRST = 1ull;
	static TAtomic<UINT64> NextRHIResourceUniqueID(RHI_RESOURCE_UNIQUE_ID_FIRST);

	UINT64 AllocRHIResourceUniqueID() noexcept
	{
		return (NextRHIResourceUniqueID.FetchAdd(1ull));
	}

	// The singleton pair the manager macro declares: the class owns the definitions, because the
	// macro only announces that they exist. Both stay private, which is what keeps
	// GetManagerSingleton the only way to reach an instance.
	RRHIBindlessManager::RRHIBindlessManager()
	{
	}
	RRHIBindlessManager::~RRHIBindlessManager()
	{
	}

	void RRHIBindlessManager::Initialize()
	{
		// A device that comes up after a shutdown gets a table with nothing left over from the
		// previous one: the indices it handed out named descriptors that no longer exist.
		Entries.Empty();
	}

	void RRHIBindlessManager::ShutDown()
	{
		// Nothing is handed back to the device here - it is on its way out and owns the heaps the
		// indices name - so this only drops the table, which is what makes a stale identity stop
		// resolving. The identities themselves stay allocated: an ID is never reused.
		Entries.Empty();
	}

	UINT64 RRHIBindlessManager::AllocResourceUniqueID() noexcept
	{
		return AllocRHIResourceUniqueID();
	}

	IRRHIDevice* RRHIBindlessManager::GetActiveDevice() noexcept
	{
		EMainManager* MainManager = EMainManager::GetManagerSingleton();
		return ((MainManager != nullptr) ? MainManager->GetRenderDevice() : nullptr);
	}

	UINT32 RRHIBindlessManager::FindEntryIndex(const UINT64 InUniqueID) const noexcept
	{
		if (InUniqueID == 0ull)
		{
			return INVALID_ENTRY_INDEX;
		}
		for (UINT32 EntryIdx = 0u, EntryNum = Entries.Num<UINT32>(); EntryIdx < EntryNum; EntryIdx++)
		{
			if (Entries[EntryIdx].UniqueID == InUniqueID)
			{
				return EntryIdx;
			}
		}
		return INVALID_ENTRY_INDEX;
	}

	UINT32 RRHIBindlessManager::AcquireIndex(IRRHIResource* InResource, const ERHIBindlessIndexKind InKind) noexcept
	{
		if ((!InResource) || (InKind >= ERHIBindlessIndexKind::RHI_BINDLESS_INDEX_KIND_COUNT))
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}
		// Only the two kinds a descriptor heap can hold are index driven. A sampler has a heap of
		// its own and a shader has none, so neither has an index here; the caller sees the invalid
		// answer and reports the resource it could not bind.
		const ERHIResourceTypeTag ResourceType = InResource->GetResourceType();
		if ((ResourceType != ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE)
			&& (ResourceType != ERHIResourceTypeTag::RHI_RESOURCE_TYPE_BUFFER))
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}

		const UINT64 ResUniqueID = InResource->GetUniqueID();
		if (ResUniqueID == 0ull)
		{
			// Every RHI object is born with an identity, so this is an object that never went
			// through the interface. Refusing beats handing out an index nobody can look up again.
			PE_LOG_ERROR(EString(PE_TEXT("[RRHIBindlessManager] A resource without an identity cannot be index driven.")));
			return RHI_INVALID_BINDLESS_INDEX;
		}

		UINT32 EntryIdx = FindEntryIndex(ResUniqueID);
		if (EntryIdx == INVALID_ENTRY_INDEX)
		{
			FEntry NewEntry;
			NewEntry.UniqueID = ResUniqueID;
			NewEntry.Resource = InResource;
			Entries.Add(NewEntry);
			EntryIdx = (Entries.Num<UINT32>() - 1u);
		}

		// The cached answer is the whole point: a resource keeps one index for its whole life, so
		// a constant buffer written once keeps meaning the same descriptor.
		if (Entries[EntryIdx].Indices[InKind] != RHI_INVALID_BINDLESS_INDEX)
		{
			return Entries[EntryIdx].Indices[InKind];
		}

		IRRHIDevice* Device = GetActiveDevice();
		if (!Device)
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}

		UINT32 Index = RHI_INVALID_BINDLESS_INDEX;
		const BOOL32 bIsTexture = ((ResourceType == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE) ? TRUE : FALSE);
		if (InKind == ERHIBindlessIndexKind::RHI_BINDLESS_INDEX_SRV)
		{
			Index = (bIsTexture
				? Device->RegisterBindlessSRV(static_cast<IRRHITexture*>(InResource))
				: Device->RegisterBindlessSRV(static_cast<IRRHIBuffer*>(InResource)));
		}
		else
		{
			Index = (bIsTexture
				? Device->RegisterBindlessUAV(static_cast<IRRHITexture*>(InResource))
				: Device->RegisterBindlessUAV(static_cast<IRRHIBuffer*>(InResource)));
		}
		if (Index == RHI_INVALID_BINDLESS_INDEX)
		{
			// The device refused it. Nothing is cached, so the next attempt asks again - which is
			// what a pool that was momentarily exhausted needs. The one case that does not recover
			// is a resource the device cannot make a descriptor for at all (an SRV-less texture that
			// declared the heap path), and that one is a source error: the compiler only marks a
			// resource index driven when it emitted the descriptor the declaration asked for.
			PE_LOG_ERROR(EString(PE_TEXT("[RRHIBindlessManager] The device refused a bindless index; see the device log for the pool state.")));
			return RHI_INVALID_BINDLESS_INDEX;
		}

		Entries[EntryIdx].Indices[InKind] = Index;
		return Index;
	}

	UINT32 RRHIBindlessManager::FindIndex(const UINT64 InUniqueID, const ERHIBindlessIndexKind InKind) const noexcept
	{
		if (InKind >= ERHIBindlessIndexKind::RHI_BINDLESS_INDEX_KIND_COUNT)
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}
		const UINT32 EntryIdx = FindEntryIndex(InUniqueID);
		if (EntryIdx == INVALID_ENTRY_INDEX)
		{
			return RHI_INVALID_BINDLESS_INDEX;
		}
		return Entries[EntryIdx].Indices[InKind];
	}

	IRRHIResource* RRHIBindlessManager::FindResource(const UINT64 InUniqueID) const noexcept
	{
		const UINT32 EntryIdx = FindEntryIndex(InUniqueID);
		if (EntryIdx == INVALID_ENTRY_INDEX)
		{
			return nullptr;
		}
		return Entries[EntryIdx].Resource;
	}

	void RRHIBindlessManager::ReleaseResource(IRRHIResource* InResource) noexcept
	{
		if (!InResource)
		{
			return;
		}
		const UINT32 EntryIdx = FindEntryIndex(InResource->GetUniqueID());
		if (EntryIdx == INVALID_ENTRY_INDEX)
		{
			// Never index driven, so there is nothing to hand back.
			return;
		}

		// The device owns the free lists, so every index goes back through it. A device that is
		// already gone needs none of this: its heaps went with it.
		IRRHIDevice* Device = GetActiveDevice();
		if (Device)
		{
			for (UINT32 KindIdx = 0u; KindIdx < static_cast<UINT32>(ERHIBindlessIndexKind::RHI_BINDLESS_INDEX_KIND_COUNT); KindIdx++)
			{
				const UINT32 Index = Entries[EntryIdx].Indices[KindIdx];
				if (Index != RHI_INVALID_BINDLESS_INDEX)
				{
					Device->ReleaseBindlessIndex(Index);
					Entries[EntryIdx].Indices[KindIdx] = RHI_INVALID_BINDLESS_INDEX;
				}
			}
		}

		Entries.RemoveAt(EntryIdx);
	}

} // namespace PigeonEngine
