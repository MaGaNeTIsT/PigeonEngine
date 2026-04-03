#pragma once
#include <string>
#include <fstream>
#include <mutex>

namespace MC
{

class MCLogger
{
public:
    static MCLogger& Get();

    void Init(const std::string& LogPath);
    void Shutdown();

    void Info(const std::string& Msg);
    void Warning(const std::string& Msg);
    void Error(const std::string& Msg);
    void Stage(const std::string& StageName, const std::string& Detail = "");

private:
    MCLogger() = default;
    void Write(const std::string& Level, const std::string& Msg);

    std::ofstream m_File;
    std::mutex m_Mutex;
};

} // namespace MC
