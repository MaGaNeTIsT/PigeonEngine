#pragma once

#include <CoreMinimal.h>
#include <atomic>

namespace PigeonEngine
{

    /*
    * Thin wrapper around std::atomic<T> that exposes engine-styled methods.
    * Default memory order is sequential consistency; pass weaker orders
    * explicitly when correctness has been audited.
    */
    template<typename _Ty>
    class TAtomic final
    {
    public:
        using TValueType = _Ty;
    public:
        TAtomic() noexcept
            : InternalValue()
        {
        }

        explicit TAtomic(TValueType InInitialValue) noexcept
            : InternalValue(InInitialValue)
        {
        }

        PE_FORCEINLINE TValueType Load(std::memory_order InOrder = std::memory_order_seq_cst) const noexcept
        {
            return InternalValue.load(InOrder);
        }

        PE_FORCEINLINE void Store(TValueType InValue, std::memory_order InOrder = std::memory_order_seq_cst) noexcept
        {
            InternalValue.store(InValue, InOrder);
        }

        PE_FORCEINLINE TValueType Exchange(TValueType InValue, std::memory_order InOrder = std::memory_order_seq_cst) noexcept
        {
            return InternalValue.exchange(InValue, InOrder);
        }

        PE_FORCEINLINE BOOL8 CompareExchangeStrong(TValueType& InOutExpected, TValueType InDesired,
            std::memory_order InOrder = std::memory_order_seq_cst) noexcept
        {
            return (InternalValue.compare_exchange_strong(InOutExpected, InDesired, InOrder));
        }

        PE_FORCEINLINE BOOL8 CompareExchangeWeak(TValueType& InOutExpected, TValueType InDesired,
            std::memory_order InOrder = std::memory_order_seq_cst) noexcept
        {
            return (InternalValue.compare_exchange_weak(InOutExpected, InDesired, InOrder));
        }

        PE_FORCEINLINE TValueType FetchAdd(TValueType InDelta, std::memory_order InOrder = std::memory_order_seq_cst) noexcept
        {
            return InternalValue.fetch_add(InDelta, InOrder);
        }

        PE_FORCEINLINE TValueType FetchSub(TValueType InDelta, std::memory_order InOrder = std::memory_order_seq_cst) noexcept
        {
            return InternalValue.fetch_sub(InDelta, InOrder);
        }

        PE_FORCEINLINE std::atomic<TValueType>& GetNativeHandle() noexcept
        {
            return InternalValue;
        }

    public:
        TAtomic(const TAtomic&) = delete;
        TAtomic(TAtomic&&) = delete;
        TAtomic& operator=(const TAtomic&) = delete;
        TAtomic& operator=(TAtomic&&) = delete;

    private:
        std::atomic<TValueType> InternalValue;
    };

};
