#pragma once

#include <string>
#include <fstream>
#include <mutex>


namespace MC
{

	class MCLogger final
	{
	public:
		static MCLogger& Get() noexcept;

		void Init(const std::string& InLogPath) noexcept;
		void Shutdown() noexcept;

		void Verbose(const std::string& InMsg) noexcept;
		void Info(const std::string& InMsg) noexcept;
		void Warning(const std::string& InMsg) noexcept;
		void Error(const std::string& InMsg) noexcept;
		void Stage(const std::string& InStageName, const std::string& InDetail = "") noexcept;

	private:
		MCLogger() noexcept;
		~MCLogger() noexcept;

		void Write(const std::string& InLevel, const std::string& InMsg) noexcept;

		std::ofstream		LogFile;
		std::mutex			MutexLock;
	};

}	// namespace MC
