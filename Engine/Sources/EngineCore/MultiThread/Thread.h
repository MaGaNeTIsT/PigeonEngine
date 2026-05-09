#pragma once

#include <CoreMinimal.h>
#include "Atomic.h"
#include <thread>

namespace PigeonEngine
{

    using EThreadEntryFunctionType = TFunction<void()>;

    /*
    * std::thread wrapper following engine naming. Owns the worker; the
    * destructor joins automatically if the thread is still running, so
    * dangling threads cannot survive an EThread going out of scope.
    *
    * Usage:
    *   EThread Worker(EString("RenderThread"));
    *   Worker.Start([]() -> void { Loop(); });
    *   ...
    *   Worker.RequestStop();      // sets the stop flag for cooperative exit
    *   Worker.Join();             // blocks until the entry function returns
    *
    * The stop flag is opt-in: the entry function chooses how (and whether)
    * to observe IsStopRequested().
    */
    class EThread final
    {
    public:
        EThread() noexcept;
        explicit EThread(const EString& InThreadName) noexcept;
        ~EThread();

        // Spawn the worker. Must not be called twice without a Join in between.
        void Start(const EThreadEntryFunctionType& InEntry);

        // Block until the entry function returns. Safe to call multiple times.
        void Join();

        // Cooperative stop request; the entry function should poll IsStopRequested().
        void RequestStop();

        BOOL8 IsStopRequested() const;
        BOOL8 IsJoinable() const;
        BOOL8 IsStarted() const;

        const EString& GetName() const
        {
            return ThreadName;
        }

        std::thread::id GetThreadId() const
        {
            return Worker.get_id();
        }

    public:
        EThread(const EThread&) = delete;
        EThread(EThread&&) = delete;
        EThread& operator=(const EThread&) = delete;
        EThread& operator=(EThread&&) = delete;

    private:
        void EntryTrampoline(const EThreadEntryFunctionType& InEntry);
        void ApplyDebugName();

    private:
        std::thread     Worker;
        EString         ThreadName;
        TAtomic<BOOL8>  bStopRequested;
        TAtomic<BOOL8>  bStarted;
    };

};
