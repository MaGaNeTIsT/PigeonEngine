#include "Event.h"

namespace PigeonEngine
{

    EEvent::EEvent(BOOL8 InIsInitiallySignaled) noexcept
        : bSignaled(InIsInitiallySignaled)
    {
    }

    void EEvent::Wait()
    {
        std::unique_lock<std::mutex> Lock(InternalMutex);
        InternalCondition.wait(Lock, [this]() -> BOOL8
        {
            return (bSignaled);
        });
    }

    BOOL8 EEvent::TryWait()
    {
        std::lock_guard<std::mutex> Lock(InternalMutex);
        return bSignaled;
    }

    void EEvent::Signal()
    {
        {
            std::lock_guard<std::mutex> Lock(InternalMutex);
            bSignaled = TRUE;
        }
        InternalCondition.notify_one();
    }

    void EEvent::SignalAll()
    {
        {
            std::lock_guard<std::mutex> Lock(InternalMutex);
            bSignaled = TRUE;
        }
        InternalCondition.notify_all();
    }

    void EEvent::Reset()
    {
        std::lock_guard<std::mutex> Lock(InternalMutex);
        bSignaled = FALSE;
    }

    BOOL8 EEvent::IsSignaled() const
    {
        std::lock_guard<std::mutex> Lock(InternalMutex);
        return bSignaled;
    }

};
