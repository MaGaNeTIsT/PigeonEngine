#pragma once
#include <string>
#include <functional>
#include <atomic>
#include <thread>

namespace MC
{

// Watches a directory for file changes (Windows only).
// Calls Callback(changedFilePath) on the watcher thread.
class MCFileWatcher
{
public:
    using ChangeCallback = std::function<void(const std::string&)>;

    MCFileWatcher() = default;
    ~MCFileWatcher() { Stop(); }

    // Start watching InDir recursively. Non-blocking — runs on a background thread.
    bool Start(const std::string& InDir, ChangeCallback Callback, std::string& OutError);
    void Stop();

private:
    void WatchLoop(std::string Dir, ChangeCallback Callback);

    std::thread       m_Thread;
    std::atomic<bool> m_Running{ false };
};

} // namespace MC
