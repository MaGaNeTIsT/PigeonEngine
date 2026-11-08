#pragma once

#include <Base/DataStructure/ObjectBase.h>

#if !(ENGINE_SHIPPING_MODE)
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
		ELog(ELogType InType, const EString& InLog, const EngineSystemTime::ESystemTime& InTimeStamp, const BOOL32 InIsShowInImGui);

		void PrintLog() const;
		PE_NODISCARD EString AsString() const;
		ELogType Type;
		EString Log;
		EngineSystemTime::ESystemTime TimeStamp;
		// FALSE keeps a line in the file and out of the window. The shader compiler prints its
		// per stage progress here - hundreds of lines per compile - and the window lays out every
		// retained line on every frame, so a cold cook would tax every frame that follows it.
		BOOL32 bShowInImGui;
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

		void AddALog(ELogType Type, const EString& NewLog, const BOOL32 InIsShowInImGui = TRUE);
		void WriteDownLogs();
	private:
		// Appends every line added since the last call. The caller holds LogMutex.
		void WriteLogsToFile();
	private:
		TArray<TSharedPtr<ELog>> Logs;
		// Indices into Logs of the entries the window draws, in order. A file only line is in
		// Logs and not here, so the window's index space is dense and a clipper can walk it.
		TArray<INT32> VisibleLogIndices;
		// The widest visible line, in pixels, accumulated as lines arrive. A clipped row
		// contributes no width, so the horizontal scrollbar needs this to describe the log.
		FLOAT MaxVisibleLogWidth = 0.f;
		INT32 NumMeasuredVisibleLogs = 0;
		EString LogFilePath;
		// How many entries of Logs are already on disk. Logs stays the record of the session, so
		// a line is written once and kept; VisibleLogIndices is what the window draws.
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


