#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{
	class PWorld;
	class PObject;
	class PActor;
	class PActorComponent;

	// Which entry point a scheduled object uses. The domain is the actual frame phase,
	// while priority orders entries inside that phase.
	enum class ETickDomain : UINT8
	{
		VARIABLE = 0u,
		FIXED,
		POST_PHYSICS,
		EDITOR,
		COUNT,
	};

	// Lower values run first. Game code normally uses Early/Default/Late; engine code that
	// must bracket gameplay uses EngineEarly/EngineLate.
	namespace ETickPriority
	{
		PE_CONSTEXPR_STATIC INT32 EngineEarly = -10000;
		PE_CONSTEXPR_STATIC INT32 Early = -1000;
		PE_CONSTEXPR_STATIC INT32 Default = 0;
		PE_CONSTEXPR_STATIC INT32 Late = 1000;
		PE_CONSTEXPR_STATIC INT32 EngineLate = 10000;
	}

	struct FTickHandle
	{
		UINT64 Id = 0u;
		UINT32 Generation = 0u;

		PE_NODISCARD BOOL32 IsValid() const
		{
			return (Id != 0u) ? TRUE : FALSE;
		}

		void Reset()
		{
			Id = 0u;
			Generation = 0u;
		}
	};

	/*
	* Per-world gameplay tick schedule.
	*
	* The scheduler owns no game objects. Actor and component lifecycle hooks register and
	* unregister their entries; the scheduler caches the ordered view and executes one domain
	* at a time. It is a plain member of PWorld, never a singleton: two worlds must not share
	* tick state, especially the post-physics handlers.
	*/
	class FWorldTickScheduler final
	{
	public:
		FWorldTickScheduler() = default;
		~FWorldTickScheduler();

		void Initialize(PWorld& InWorld);
		void Shutdown();
		PE_NODISCARD BOOL32 IsInitialized() const;

		PE_NODISCARD FTickHandle RegisterActor(PActor& InActor, const ETickDomain& InDomain, const INT32& InPriority);
		PE_NODISCARD FTickHandle RegisterComponent(PActorComponent& InComponent, const ETickDomain& InDomain, const INT32& InPriority);
		void Unregister(FTickHandle& InOutHandle);
		void SetEnabled(const FTickHandle& InHandle, const BOOL32& bInEnabled);
		void SetPriority(const FTickHandle& InHandle, const INT32& InPriority);

		// Marks the cached order stale. Called by registration and by priority changes.
		void MarkDirty();

		void Tick(const ETickDomain& InDomain, FLOAT InDeltaTime);

	private:
		enum class ETickTargetKind : UINT8
		{
			ACTOR = 0u,
			COMPONENT,
		};

		struct FTickEntry
		{
			PObject* Target = nullptr;
			const PActor* OwnerActor = nullptr;
			ETickTargetKind Kind = ETickTargetKind::COMPONENT;
			ETickDomain Domain = ETickDomain::VARIABLE;
			INT32 Priority = ETickPriority::Default;
			UINT64 Id = 0u;
			UINT32 Generation = 1u;
			UINT64 OwnerSequence = 0u;
			UINT64 OrderInOwner = 0u;
			UINT64 RegistrationSequence = 0u;
			BOOL32 bEnabled = TRUE;
			BOOL32 bActive = TRUE;
		};

		FTickEntry* FindEntry(const FTickHandle& InHandle);
		const FTickEntry* FindEntry(const FTickHandle& InHandle) const;
		FTickHandle RegisterEntry(PObject& InTarget, const PActor* InOwnerActor, const ETickTargetKind& InKind,
			const ETickDomain& InDomain, const INT32& InPriority);
		void EnsureOwner(const PActor& InOwnerActor, UINT64& OutSequence, UINT64& OutNextOrder);
		void ReleaseOwner(const PActor& InOwnerActor);
		void RebuildOrder();
		void CompactIfNeeded();
		void ExecuteEntry(const FTickEntry& InEntry, FLOAT InDeltaTime);

	private:
		PWorld* World = nullptr;
		BOOL32 bInitialized = FALSE;
		BOOL32 bOrderDirty = TRUE;
		BOOL32 bTicking = FALSE;

		TArray<FTickEntry> Entries;
		TMap<UINT64, UINT32> SlotById;
		TArray<UINT32> Ordered[static_cast<UINT32>(ETickDomain::COUNT)];
		TMap<const PActor*, UINT64> OwnerSequences;
		TMap<const PActor*, UINT64> NextOwnerOrders;
		TMap<const PActor*, UINT32> OwnerEntryCounts;

		UINT64 NextOwnerSequence = 1u;
		UINT64 NextRegistrationSequence = 1u;
		UINT64 NextHandleId = 1u;
		UINT32 InactiveEntryCount = 0u;
	};
}
