#include "WorldTickScheduler.h"

#include <PigeonBase/Object/Object.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/ActorComponent.h>

namespace PigeonEngine
{
	FWorldTickScheduler::~FWorldTickScheduler()
	{
		Shutdown();
	}

	void FWorldTickScheduler::Initialize(PWorld& InWorld)
	{
		Shutdown();
		World = &InWorld;
		bInitialized = TRUE;
		bOrderDirty = TRUE;
	}

	void FWorldTickScheduler::Shutdown()
	{
		Entries.Empty();
		SlotById.Empty();
		for (UINT32 DomainIndex = 0u; DomainIndex < static_cast<UINT32>(ETickDomain::COUNT); DomainIndex++)
		{
			Ordered[DomainIndex].Empty();
		}
		OwnerSequences.Empty();
		NextOwnerOrders.Empty();
		OwnerEntryCounts.Empty();
		World = nullptr;
		bInitialized = FALSE;
		bOrderDirty = TRUE;
		bTicking = FALSE;
		NextOwnerSequence = 1u;
		NextRegistrationSequence = 1u;
		NextHandleId = 1u;
		InactiveEntryCount = 0u;
	}

	BOOL32 FWorldTickScheduler::IsInitialized() const
	{
		return bInitialized;
	}

	FWorldTickScheduler::FTickEntry* FWorldTickScheduler::FindEntry(const FTickHandle& InHandle)
	{
		if (!InHandle.IsValid())
		{
			return nullptr;
		}
		UINT32* Slot = SlotById.FindValueAsPtr(InHandle.Id);
		if ((!Slot) || (*Slot >= Entries.Num<UINT32>()))
		{
			return nullptr;
		}
		FTickEntry& Entry = Entries[*Slot];
		return (Entry.bActive && (Entry.Generation == InHandle.Generation)) ? &Entry : nullptr;
	}

	const FWorldTickScheduler::FTickEntry* FWorldTickScheduler::FindEntry(const FTickHandle& InHandle) const
	{
		if (!InHandle.IsValid())
		{
			return nullptr;
		}
		const UINT32* Slot = SlotById.FindValueAsPtr(InHandle.Id);
		if ((!Slot) || (*Slot >= Entries.Num<UINT32>()))
		{
			return nullptr;
		}
		const FTickEntry& Entry = Entries[*Slot];
		return (Entry.bActive && (Entry.Generation == InHandle.Generation)) ? &Entry : nullptr;
	}

	void FWorldTickScheduler::EnsureOwner(const PActor& InOwnerActor, UINT64& OutSequence, UINT64& OutNextOrder)
	{
		UINT64* ExistingSequence = OwnerSequences.FindValueAsPtr(&InOwnerActor);
		if (ExistingSequence)
		{
			OutSequence = *ExistingSequence;
		}
		else
		{
			OutSequence = NextOwnerSequence++;
			OwnerSequences.Add(&InOwnerActor, OutSequence);
		}

		UINT64* ExistingNextOrder = NextOwnerOrders.FindValueAsPtr(&InOwnerActor);
		if (ExistingNextOrder)
		{
			OutNextOrder = *ExistingNextOrder;
		}
		else
		{
			OutNextOrder = 0u;
			NextOwnerOrders.Add(&InOwnerActor, OutNextOrder);
		}
	}

	void FWorldTickScheduler::ReleaseOwner(const PActor& InOwnerActor)
	{
		UINT32* Count = OwnerEntryCounts.FindValueAsPtr(&InOwnerActor);
		if ((!Count) || (*Count == 0u))
		{
			return;
		}
		*Count -= 1u;
		if (*Count == 0u)
		{
			OwnerEntryCounts.Remove(&InOwnerActor);
			NextOwnerOrders.Remove(&InOwnerActor);
			OwnerSequences.Remove(&InOwnerActor);
		}
	}

	FTickHandle FWorldTickScheduler::RegisterEntry(PObject& InTarget, const PActor* InOwnerActor, const ETickTargetKind& InKind,
		const ETickDomain& InDomain, const INT32& InPriority)
	{
		FTickHandle Handle;
		if ((!bInitialized) || (InDomain == ETickDomain::COUNT))
		{
			return Handle;
		}
		if ((World) && (InTarget.GetWorld() != World))
		{
			PE_CHECK(ENGINE_ACTOR_ERROR, "Registering a tick entry in a world it does not belong to.", FALSE);
			return Handle;
		}

		UINT64 OwnerSequence = 0u;
		UINT64 OrderInOwner = 0u;
		if (InOwnerActor)
		{
			UINT64 NextOrder = 0u;
			EnsureOwner(*InOwnerActor, OwnerSequence, NextOrder);
			if (InKind == ETickTargetKind::ACTOR)
			{
				// Actor self ticks run after the actor's components at equal priority.
				OrderInOwner = static_cast<UINT64>(-1);
			}
			else
			{
				OrderInOwner = NextOrder;
				if (UINT64* StoredNext = NextOwnerOrders.FindValueAsPtr(InOwnerActor))
				{
					*StoredNext = NextOrder + 1u;
				}
			}
			UINT32* Count = OwnerEntryCounts.FindValueAsPtr(InOwnerActor);
			if (Count)
			{
				*Count += 1u;
			}
			else
			{
				OwnerEntryCounts.Add(InOwnerActor, 1u);
			}
		}
		else
		{
			OwnerSequence = NextOwnerSequence++;
		}

		FTickEntry Entry;
		Entry.Target = &InTarget;
		Entry.OwnerActor = InOwnerActor;
		Entry.Kind = InKind;
		Entry.Domain = InDomain;
		Entry.Priority = InPriority;
		Entry.Id = NextHandleId++;
		Entry.Generation = 1u;
		Entry.OwnerSequence = OwnerSequence;
		Entry.OrderInOwner = OrderInOwner;
		Entry.RegistrationSequence = NextRegistrationSequence++;
		Entry.bEnabled = TRUE;
		Entry.bActive = TRUE;

		const UINT32 Slot = Entries.Num<UINT32>();
		Entries.Add(Entry);
		Handle.Id = Entry.Id;
		Handle.Generation = Entry.Generation;
		SlotById.Add(Handle.Id, Slot);
		bOrderDirty = TRUE;
		return Handle;
	}

	FTickHandle FWorldTickScheduler::RegisterActor(PActor& InActor, const ETickDomain& InDomain, const INT32& InPriority)
	{
		return RegisterEntry(InActor, &InActor, ETickTargetKind::ACTOR, InDomain, InPriority);
	}

	FTickHandle FWorldTickScheduler::RegisterComponent(PActorComponent& InComponent, const ETickDomain& InDomain, const INT32& InPriority)
	{
		return RegisterEntry(InComponent, InComponent.GetOwnerActor(), ETickTargetKind::COMPONENT, InDomain, InPriority);
	}

	void FWorldTickScheduler::Unregister(FTickHandle& InOutHandle)
	{
		if (!InOutHandle.IsValid())
		{
			return;
		}
		UINT32* Slot = SlotById.FindValueAsPtr(InOutHandle.Id);
		if ((!Slot) || (*Slot >= Entries.Num<UINT32>()))
		{
			InOutHandle.Reset();
			return;
		}
		FTickEntry& Entry = Entries[*Slot];
		if ((!Entry.bActive) || (Entry.Generation != InOutHandle.Generation))
		{
			InOutHandle.Reset();
			return;
		}
		Entry.bActive = FALSE;
		SlotById.Remove(InOutHandle.Id);
		if (Entry.OwnerActor)
		{
			ReleaseOwner(*Entry.OwnerActor);
		}
		InactiveEntryCount += 1u;
		InOutHandle.Reset();
		bOrderDirty = TRUE;
	}

	void FWorldTickScheduler::SetEnabled(const FTickHandle& InHandle, const BOOL32& bInEnabled)
	{
		FTickEntry* Entry = FindEntry(InHandle);
		if (Entry)
		{
			Entry->bEnabled = bInEnabled;
		}
	}

	void FWorldTickScheduler::SetPriority(const FTickHandle& InHandle, const INT32& InPriority)
	{
		FTickEntry* Entry = FindEntry(InHandle);
		if ((Entry) && (Entry->Priority != InPriority))
		{
			Entry->Priority = InPriority;
			bOrderDirty = TRUE;
		}
	}

	void FWorldTickScheduler::MarkDirty()
	{
		bOrderDirty = TRUE;
	}

	void FWorldTickScheduler::CompactIfNeeded()
	{
		if (bTicking || (InactiveEntryCount == 0u))
		{
			return;
		}
		if ((InactiveEntryCount * 2u) < Entries.Num<UINT32>())
		{
			return;
		}

		TArray<FTickEntry> CompactedEntries;
		for (UINT32 Index = 0u, Num = Entries.Num<UINT32>(); Index < Num; Index++)
		{
			if (Entries[Index].bActive)
			{
				CompactedEntries.Add(Entries[Index]);
			}
		}
		// Handles use IDs, not array slots, so slot movement is invisible to callers. The ID
		// map has to be rebuilt from the same entries.
		SlotById.Empty();
		for (UINT32 Index = 0u, Num = CompactedEntries.Num<UINT32>(); Index < Num; Index++)
		{
			SlotById.Add(CompactedEntries[Index].Id, Index);
		}
		Entries = CompactedEntries;
		InactiveEntryCount = 0u;
		bOrderDirty = TRUE;
	}

	void FWorldTickScheduler::RebuildOrder()
	{
		CompactIfNeeded();
		for (UINT32 DomainIndex = 0u; DomainIndex < static_cast<UINT32>(ETickDomain::COUNT); DomainIndex++)
		{
			Ordered[DomainIndex].Empty();
		}
		for (UINT32 Index = 0u, Num = Entries.Num<UINT32>(); Index < Num; Index++)
		{
			const FTickEntry& Entry = Entries[Index];
			if (!Entry.bActive)
			{
				continue;
			}
			Ordered[static_cast<UINT32>(Entry.Domain)].Add(Index);
		}
		for (UINT32 DomainIndex = 0u; DomainIndex < static_cast<UINT32>(ETickDomain::COUNT); DomainIndex++)
		{
			Ordered[DomainIndex].Sort([this](const UINT32& InA, const UINT32& InB) -> BOOL32
			{
				const FTickEntry& A = Entries[InA];
				const FTickEntry& B = Entries[InB];
				if (A.Priority != B.Priority)
				{
					return (A.Priority < B.Priority) ? TRUE : FALSE;
				}
				if (A.OwnerSequence != B.OwnerSequence)
				{
					return (A.OwnerSequence < B.OwnerSequence) ? TRUE : FALSE;
				}
				if (A.OrderInOwner != B.OrderInOwner)
				{
					return (A.OrderInOwner < B.OrderInOwner) ? TRUE : FALSE;
				}
				return (A.RegistrationSequence < B.RegistrationSequence) ? TRUE : FALSE;
			});
		}
		bOrderDirty = FALSE;
	}

	void FWorldTickScheduler::ExecuteEntry(const FTickEntry& InEntry, FLOAT InDeltaTime)
	{
		if (!InEntry.Target)
		{
			return;
		}
		switch (InEntry.Domain)
		{
		case ETickDomain::VARIABLE:
			InEntry.Target->Tick(InDeltaTime);
			break;
		case ETickDomain::FIXED:
			if (InEntry.Kind == ETickTargetKind::COMPONENT)
			{
				static_cast<PActorComponent*>(InEntry.Target)->FixedTick(InDeltaTime);
			}
			break;
		case ETickDomain::POST_PHYSICS:
			if (InEntry.Kind == ETickTargetKind::COMPONENT)
			{
				static_cast<PActorComponent*>(InEntry.Target)->HandlePostPhysicsTick(InDeltaTime);
			}
			break;
#if ENGINE_EDITOR_ONLY
		case ETickDomain::EDITOR:
			InEntry.Target->EditorTick(InDeltaTime);
			break;
#endif
		default:
			break;
		}
	}

	void FWorldTickScheduler::Tick(const ETickDomain& InDomain, FLOAT InDeltaTime)
	{
		if ((!bInitialized) || (bTicking) || (InDomain == ETickDomain::COUNT))
		{
			return;
		}
		if (bOrderDirty)
		{
			RebuildOrder();
		}

		bTicking = TRUE;
		TArray<UINT32>& DomainOrder = Ordered[static_cast<UINT32>(InDomain)];
		for (UINT32 Index = 0u, Num = DomainOrder.Num<UINT32>(); Index < Num; Index++)
		{
			const UINT32 Slot = DomainOrder[Index];
			if (Slot >= Entries.Num<UINT32>())
			{
				continue;
			}
			const FTickEntry& Entry = Entries[Slot];
			if ((!Entry.bActive) || (!Entry.bEnabled) || (!Entry.Target))
			{
				continue;
			}
			if ((!Entry.Target->IsAddedToScene()) || Entry.Target->IsPendingDestroy())
			{
				continue;
			}
			ExecuteEntry(Entry, InDeltaTime);
		}
		bTicking = FALSE;
	}
}
