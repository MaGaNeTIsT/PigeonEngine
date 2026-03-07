#include "MCFileWatcher.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

namespace MC
{

bool MCFileWatcher::Start(const std::string& InDir, ChangeCallback Callback, std::string& OutError)
{
    if (m_Running) { OutError = "Watcher already running"; return false; }
    m_Running = true;
    m_Thread  = std::thread(&MCFileWatcher::WatchLoop, this, InDir, std::move(Callback));
    return true;
}

void MCFileWatcher::Stop()
{
    m_Running = false;
    if (m_Thread.joinable()) m_Thread.join();
}

void MCFileWatcher::WatchLoop(std::string Dir, ChangeCallback Callback)
{
    HANDLE hDir = CreateFileA(
        Dir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (hDir == INVALID_HANDLE_VALUE) return;

    OVERLAPPED ov{};
    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    std::vector<BYTE> buf(65536);

    while (m_Running)
    {
        DWORD bytesReturned = 0;
        ResetEvent(ov.hEvent);

        ReadDirectoryChangesW(
            hDir,
            buf.data(), static_cast<DWORD>(buf.size()),
            TRUE, // watch subtree
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned, &ov, nullptr);

        // Wait up to 500ms so we can check m_Running
        DWORD wait = WaitForSingleObject(ov.hEvent, 500);
        if (wait == WAIT_OBJECT_0)
        {
            GetOverlappedResult(hDir, &ov, &bytesReturned, FALSE);
            DWORD offset = 0;
            while (offset < bytesReturned)
            {
                auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buf.data() + offset);
                // Convert wide filename to narrow
                int len = WideCharToMultiByte(CP_UTF8, 0, info->FileName,
                    info->FileNameLength / sizeof(WCHAR), nullptr, 0, nullptr, nullptr);
                std::string name(len, '\0');
                WideCharToMultiByte(CP_UTF8, 0, info->FileName,
                    info->FileNameLength / sizeof(WCHAR), name.data(), len, nullptr, nullptr);

                Callback(Dir + "/" + name);

                if (info->NextEntryOffset == 0) break;
                offset += info->NextEntryOffset;
            }
        }
    }

    CloseHandle(ov.hEvent);
    CloseHandle(hDir);
}

} // namespace MC
