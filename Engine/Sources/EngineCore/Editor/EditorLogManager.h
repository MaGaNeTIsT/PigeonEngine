#pragma once

#include <Base/DataStructure/ObjectBase.h>

#if !(_SHIPPING_MODE)
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"
#include "Base/Timer/Timer.h"
#include "MultiThread/Mutex.h"

namespace PigeonEngine
{

	enum ELogType : UINT8
	{
		ELT_LOG,
		ELT_WARNING,
		ELT_ERROR,
	};
	struct ELog
	{
		ELog() = delete;
		ELog(ELogType InType, const EString& InLog, const EngineSystemTime::ESystemTime& InTimeStamp);

		void PrintLog() const;
		PE_NODISCARD EString AsString() const;
		ELogType Type;
		EString Log;
		EngineSystemTime::ESystemTime TimeStamp;
	};

	class EEditorLogManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorLogManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();

	public:
		void EditorInit();
		void EditorUpdate();

		void AddALog(ELogType Type, const EString& NewLog);
		void WriteDownLogs();
	private:
		// Appends every line added since the last call. The caller holds LogMutex.
		void WriteLogsToFile();
	private:
		TArray<TSharedPtr<ELog>> Logs;
		EString LogFilePath;
		// How many entries of Logs are already on disk. The array stays the display list for
		// the log window, so a line is written once and shown forever.
		INT32 NumFlushedLogs = 0;
		// Both the game thread and the render thread log - a shader compile reports its
		// diagnostics from inside the draw - so the array, the flush cursor and the file
		// they are written to are guarded together. Without it the two appends race on the
		// array's storage, which is heap corruption rather than a lost line.
		EMutex LogMutex;


	};
#define PE_LOG_LOG(__Log)       { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_LOG, (__Log)); }
#define PE_LOG_WARN(__Log)      { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_WARNING, (__Log)); }
#define PE_LOG_ERROR(__Log)     { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_ERROR, (__Log)); }

}

#else

namespace PigeonEngine
{

#define PE_LOG_LOG(__Log)       {;}
#define PE_LOG_WARN(__Log)      {;}
#define PE_LOG_ERROR(__Log)     {;}

};

#endif


