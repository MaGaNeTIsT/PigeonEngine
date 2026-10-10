#include "MCLogger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>


namespace MC
{

	MCLogger::MCLogger() noexcept
	{

	}

	MCLogger::~MCLogger() noexcept
	{

	}

	MCLogger& MCLogger::Get() noexcept
	{
		static MCLogger MCLoggerInstance;
		return MCLoggerInstance;
	}

	void MCLogger::Init(const std::string& InLogPath) noexcept
	{
		std::lock_guard<std::mutex> MutexLockGuard(MutexLock);
		if (LogFile.is_open())
		{
			LogFile.close();
		}
		LogFile.open(InLogPath, std::ios::out | std::ios::trunc);
		if (LogFile.is_open())
		{
			auto SysNow = std::chrono::system_clock::now();
			auto SysTime = std::chrono::system_clock::to_time_t(SysNow);
			std::tm SysTimeInfo;
			localtime_s(&SysTimeInfo, &SysTime);
			LogFile << "=== MaterialCompiler Log Started at " << std::put_time(&SysTimeInfo, "%Y-%m-%d %H:%M:%S") << " ===\n\n";
		}
	}

	void MCLogger::Shutdown() noexcept
	{
		std::lock_guard<std::mutex> MutexLockGuard(MutexLock);
		if (LogFile.is_open())
		{
			LogFile << "\n=== Log End ===\n";
			LogFile.close();
		}
	}

	void MCLogger::Write(const std::string& InLevel, const std::string& InMsg) noexcept
	{
		std::lock_guard<std::mutex> MutexLockGuard(MutexLock);
		std::string StrLine = "[" + InLevel + "] " + InMsg + "\n";
		std::cout << StrLine;
		if (LogFile.is_open())
		{
			LogFile << StrLine << std::flush;
		}
	}

	void MCLogger::Verbose(const std::string& InMsg) noexcept
	{
		Write("VERBOSE", InMsg);
	}

	void MCLogger::Info(const std::string& InMsg) noexcept
	{
		Write("INFO", InMsg);
	}

	void MCLogger::Warning(const std::string& InMsg) noexcept
	{
		Write("WARN", InMsg);
	}

	void MCLogger::Error(const std::string& InMsg) noexcept
	{
		Write("ERROR", InMsg);
	}

	void MCLogger::Stage(const std::string& InStageName, const std::string& InDetail) noexcept
	{
		Write("STAGE", InStageName + (InDetail.empty() ? "" : ": " + InDetail));
	}

};  // namespace MC
