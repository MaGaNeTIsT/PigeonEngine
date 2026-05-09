#pragma once

#include <CoreMinimal.h>
#include <mutex>

namespace PigeonEngine
{

    /*
    * Non-recursive mutual exclusion primitive. Wraps std::mutex with engine
    * naming conventions. Always acquire through EScopedLock unless interacting
    * with a condition variable; manual Lock/Unlock pairs are error-prone.
    */
    class EMutex final
    {
    public:
        EMutex() noexcept = default;
        ~EMutex() = default;

        PE_FORCEINLINE void Lock()
        {
            InternalMutex.lock();
        }

        PE_FORCEINLINE BOOL8 TryLock()
        {
            return (InternalMutex.try_lock() ? TRUE : FALSE);
        }

        PE_FORCEINLINE void Unlock()
        {
            InternalMutex.unlock();
        }

        PE_FORCEINLINE std::mutex& GetNativeHandle()
        {
            return InternalMutex;
        }

    public:
        EMutex(const EMutex&) = delete;
        EMutex(EMutex&&) = delete;
        EMutex& operator=(const EMutex&) = delete;
        EMutex& operator=(EMutex&&) = delete;

    private:
        std::mutex InternalMutex;
    };

    /*
    * RAII helper that locks an EMutex on construction and unlocks on
    * destruction. Holds the mutex by reference; the referenced mutex must
    * outlive the EScopedLock.
    */
    class EScopedLock final
    {
    public:
        explicit EScopedLock(EMutex& InMutex) noexcept
            : MutexRef(InMutex)
        {
            MutexRef.Lock();
        }

        ~EScopedLock()
        {
            MutexRef.Unlock();
        }

    public:
        EScopedLock(const EScopedLock&) = delete;
        EScopedLock(EScopedLock&&) = delete;
        EScopedLock& operator=(const EScopedLock&) = delete;
        EScopedLock& operator=(EScopedLock&&) = delete;

    private:
        EMutex& MutexRef;
    };

};
