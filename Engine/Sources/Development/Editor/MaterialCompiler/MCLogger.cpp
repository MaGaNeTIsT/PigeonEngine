#include "MCLogger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace MC
{

MCLogger& MCLogger::Get()
{
    static MCLogger instance;
    return instance;
}

void MCLogger::Init(const std::string& LogPath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_File.is_open()) m_File.close();
    m_File.open(LogPath, std::ios::out | std::ios::trunc);
    if (m_File.is_open())
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm timeInfo;
        localtime_s(&timeInfo, &time);
        m_File << "=== MaterialCompiler Log Started at "
               << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S") << " ===\n\n";
    }
}

void MCLogger::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_File.is_open())
    {
        m_File << "\n=== Log End ===\n";
        m_File.close();
    }
}

void MCLogger::Write(const std::string& Level, const std::string& Msg)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::string line = "[" + Level + "] " + Msg + "\n";
    std::cout << line;
    if (m_File.is_open()) m_File << line << std::flush;
}

void MCLogger::Info(const std::string& Msg)    { Write("INFO", Msg); }
void MCLogger::Warning(const std::string& Msg) { Write("WARN", Msg); }
void MCLogger::Error(const std::string& Msg)   { Write("ERROR", Msg); }
void MCLogger::Stage(const std::string& StageName, const std::string& Detail)
{
    Write("STAGE", StageName + (Detail.empty() ? "" : ": " + Detail));
}

} // namespace MC
