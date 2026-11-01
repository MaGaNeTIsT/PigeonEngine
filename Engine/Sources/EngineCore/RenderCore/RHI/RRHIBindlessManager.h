#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RHI/RHITypes.h>
#include <Core/Base/DataStructure/ObjectBase.h>
#include <MultiThread/Atomic.h>

namespace PigeonEngine
{

	class IRRHIResource;
	class IRRHIDevice;

	// Which descriptor heap region an index names. The kind is part of the identity rather than
	// implied by the resource, because one GPU object can be read through the heap and written
	// through it at the same time, and those are two different descriptors in two different
	// regions.
	enum ERHIBindlessIndexKind : UINT8
	{
		RHI_BINDLESS_INDEX_SRV	= 0u,
		RHI_BINDLESS_INDEX_UAV	= 1u,
		RHI_BINDLESS_INDEX_KIND_COUNT
	};

	/*
	* Bindless identity and index registry.
	*
	* Two things live here and nothing else:
	*
	*   identity   every GPU heap object is born with a UniqueID (IRRHIResource::GetUniqueID).
	*              The counter is atomic because the first objects of the engine's life are made
	*              on the game thread and the rest on the render thread, and both draw from it.
	*
	*   index      the stable descriptor heap index of a resource, allocated the first time
	*              somebody asks for it and kept until the resource is destroyed. The allocation
	*              itself belongs to the device (IRRHIDevice::RegisterBindlessSRV/UAV, which owns
	*              the heap regions and their free lists); this manager is what makes the answer
	*              stable and what maps an index back to the object it names.
	*
	* What the manager deliberately does not do: it does not decide which resources are index
	* driven. That answer is the compiler's - it emits the index field and reports Bindless in the
	* reflection - and the binding side is what turns it into a write. A resource that is index
	* driven can still be bound at a fixed slot by another pass at the same time, so nothing here
	* reserves a resource for one mode.
	*
	* Threading: identity allocation is atomic. The registry table itself is touched only where the
	* engine is single threaded with respect to it - resource creation and the binding side run on
	* the render thread, and release runs from the device's destroy path, which the shutdown
	* sequence also reaches after the render thread has stopped. Nothing here takes a lock, and the
	* table is expected to stay in that regime.
	*/
	class RRHIBindlessManager final : public EManagerBase
	{
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
	public:
		// Identity allocation, also exposed as the free function the resource constructor calls
		// (AllocRHIResourceUniqueID in IRRHIResource.h). Never returns 0.
		PE_NODISCARD static UINT64	AllocResourceUniqueID() noexcept;

		// The stable heap index of a resource in the given region. The first call registers the
		// resource with the device and caches the answer; every later call for the same resource
		// and kind returns the same index. RHI_INVALID_BINDLESS_INDEX when the object cannot be
		// index driven at all (no identity, a sampler, no live device).
		PE_NODISCARD UINT32	AcquireIndex(IRRHIResource* InResource, const ERHIBindlessIndexKind InKind) noexcept;
		// The same answer without allocating anything. RHI_INVALID_BINDLESS_INDEX when the
		// resource has never been index driven in that region.
		PE_NODISCARD UINT32	FindIndex(const UINT64 InUniqueID, const ERHIBindlessIndexKind InKind) const noexcept;
		// Which object an identity names. Null when nothing registered under it.
		PE_NODISCARD IRRHIResource*	FindResource(const UINT64 InUniqueID) const noexcept;
		// Hands every index the resource holds back to the device and drops its entry, so a stale
		// identity stops resolving instead of pointing at a freed object. Called by the device's
		// destroy path.
		void	ReleaseResource(IRRHIResource* InResource) noexcept;

		PE_NODISCARD UINT32	GetIndexedResourceNum() const noexcept { return Entries.Num<UINT32>(); }
	private:
		static constexpr UINT32 INVALID_ENTRY_INDEX = 0xffffffffu;
	private:
		struct FEntry
		{
		public:
			UINT64			UniqueID	= 0ull;
			IRRHIResource*	Resource	= nullptr;
			// One per region: a resource can be index driven as a read view and as a write view at
			// once, and the two are independent allocations.
			UINT32			Indices[RHI_BINDLESS_INDEX_KIND_COUNT] = { RHI_INVALID_BINDLESS_INDEX, RHI_INVALID_BINDLESS_INDEX };
		};
		// The entry an identity currently has, or INVALID_ENTRY_INDEX. Index based so a caller can
		// also remove what it found; the table is small and walked linearly, the way the rest of the
		// render tier's process wide tables are.
		PE_NODISCARD UINT32	FindEntryIndex(const UINT64 InUniqueID) const noexcept;
		// The device whose heaps the indices point into, asked for when it is needed rather than
		// held: during shutdown it may already be gone, and a null answer is then the correct one.
		PE_NODISCARD static IRRHIDevice*	GetActiveDevice() noexcept;
	private:
		TArray<FEntry>	Entries;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(RRHIBindlessManager)

	};

} // namespace PigeonEngine
