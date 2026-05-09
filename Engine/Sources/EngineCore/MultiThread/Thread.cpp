#include "Thread.h"
#include <processthreadsapi.h>

namespace PigeonEngine
{

    EThread::EThread() noexcept
        : ThreadName("PigeonThread")
        , bStopRequested(FALSE)
        , bStarted(FALSE)
    {
    }

    EThread::EThread(const EString& InThreadName) noexcept
        : ThreadName(InThreadName)
        , bStopRequested(FALSE)
        , bStarted(FALSE)
    {
    }

    EThread::~EThread()
    {
        if (Worker.joinable())
        {
            RequestStop();
            Worker.join();
        }
    }

    void EThread::Start(const EThreadEntryFunctionType& InEntry)
    {
        Check((!bStarted.Load()));
        Check((!Worker.joinable()));

        bStopRequested.Store(FALSE);
        bStarted.Store(TRUE);

        EThreadEntryFunctionType EntryCopy = InEntry;
        Worker = std::thread([this, EntryCopy]() -> void
        {
            this->EntryTrampoline(EntryCopy);
        });
    }

    void EThread::Join()
    {
        if (Worker.joinable())
        {
            Worker.join();
        }
        bStarted.Store(FALSE);
    }

    void EThread::RequestStop()
    {
        bStopRequested.Store(TRUE);
    }

    BOOL8 EThread::IsStopRequested() const
    {
        return bStopRequested.Load();
    }

    BOOL8 EThread::IsJoinable() const
    {
        return (Worker.joinable());
    }

    BOOL8 EThread::IsStarted() const
    {
        return bStarted.Load();
    }

    void EThread::EntryTrampoline(const EThreadEntryFunctionType& InEntry)
    {
        ApplyDebugName();
        if (InEntry)
        {
            InEntry();
        }
    }

    void EThread::ApplyDebugName()
    {
        const CHAR* AnsiName = (*ThreadName);
        if (!AnsiName)
        {
            return;
        }

        WCHAR WideName[128] = { 0 };
        const INT32 Converted = ::MultiByteToWideChar(CP_UTF8, 0, AnsiName, -1, WideName, static_cast<INT32>(sizeof(WideName) / sizeof(WideName[0])));
        if (Converted <= 0)
        {
            return;
        }

        ::SetThreadDescription(::GetCurrentThread(), WideName);
    }

};
