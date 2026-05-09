#pragma once

#include <CoreMinimal.h>
#include <mutex>
#include <condition_variable>

namespace PigeonEngine
{

    /*
    * Manual-reset waitable event built on std::condition_variable.
    *
    * Wait() blocks until Signal() is called; the signaled state persists
    * until Reset() clears it. Designed for the GameThread / RenderThread
    * lockstep handshake where one side signals frame-done and the other
    * waits for it; both sides Reset before issuing their next handshake.
    *
    * Multiple waiters are supported via SignalAll(). Single-waiter callers
    * should prefer Signal() to avoid needless wakeups.
    */
    class EEvent final
    {
    public:
        explicit EEvent(BOOL8 InIsInitiallySignaled = FALSE) noexcept;
        ~EEvent() = default;

        // Block until signaled. Does not auto-reset.
        void Wait();

        // Non-blocking probe; returns TRUE if currently signaled.
        BOOL8 TryWait();

        // Wake one waiter (does not consume the signaled state).
        void Signal();

        // Wake all waiters (does not consume the signaled state).
        void SignalAll();

        // Clear the signaled state. Subsequent Wait() will block again.
        void Reset();

        BOOL8 IsSignaled() const;

    public:
        EEvent(const EEvent&) = delete;
        EEvent(EEvent&&) = delete;
        EEvent& operator=(const EEvent&) = delete;
        EEvent& operator=(EEvent&&) = delete;

    private:
        mutable std::mutex      InternalMutex;
        std::condition_variable InternalCondition;
        BOOL8                   bSignaled;
    };

};
