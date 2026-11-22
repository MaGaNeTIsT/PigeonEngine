#pragma once

#include <Base/DataStructure/ObjectBase.h>

#if !(ENGINE_SHIPPING_MODE)

#include <Development/LogStore.h>
#include <MultiThread/Mutex.h>

namespace PigeonEngine
{
	/*
	* Shared non-shipping log service: session storage, thread-safe append, file append
	* and old-file pruning. The editor window is a separate editor-only module
	* (FOutputLogPanel); runtime Debug/Develop builds use this service without any ImGui.
	*/
	class EEditorLogManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorLogManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		// Flushes the pending tail once per editor frame. Cheap when nothing is pending.
		void Tick();

		void AddALog(ELogType Type, const EString& NewLog, const BOOL32 InIsShowInImGui = TRUE);
		void WriteDownLogs();

	public:
		// Read interface for FOutputLogPanel. Every call takes the data mutex.
		void SnapshotVisibleLogs(TArray<TSharedPtr<ELog>>& OutLogs);
		PE_NODISCARD UINT32 GetLogRevision();
		PE_NODISCARD UINT32 GetLogCount(ELogType InType);
		void ClearVisibleLogs();
		PE_NODISCARD const EString& GetLogFilePath() const;

	private:
		void FlushPendingToFile();
		void PruneOldLogFiles();
		PE_NODISCARD EString MakeLogFilePath() const;

	private:
		FLogStore	Store;
		// Guards Store and the write-failure latch. Never held across a disk write: a
		// failing append can log, and logging re-enters AddALog.
		EMutex		LogMutex;
		// Serializes file appends; taken only after the data lock is released.
		EMutex		FileMutex;
		EString		LogFilePath;
		BOOL32		bFileWriteFailureReported = FALSE;
	};

#define PE_LOG_LOG(__Log)       { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_LOG, (__Log)); }
#define PE_LOG_WARN(__Log)      { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_WARNING, (__Log)); }
#define PE_LOG_ERROR(__Log)     { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_ERROR, (__Log)); }
// A line that belongs in the log file but not in the editor window. The shader compiler prints
// its per stage progress through this, and the window keeps every line of the session in memory.
#define PE_LOG_FILE_ONLY(__Log) { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_LOG, (__Log), FALSE); }

}

#else

namespace PigeonEngine
{

#define PE_LOG_LOG(__Log)       {;}
#define PE_LOG_WARN(__Log)      {;}
#define PE_LOG_ERROR(__Log)     {;}
#define PE_LOG_FILE_ONLY(__Log) {;}

};

#endif
