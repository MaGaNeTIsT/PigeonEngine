#include <Development/EditorLogManager.h>

#if !(ENGINE_SHIPPING_MODE)

#include <Config/EngineConfig.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace PigeonEngine
{
	namespace
	{
		// A block of lines costs one append instead of one per line. The shader compiler prints
		// about 160 lines per stage, and an append opens, writes and closes the file, so a cold
		// cook spent seconds on the log alone. The batch bounds what a killed process can lose.
		PE_CONSTEXPR_STATIC INT32 EDITOR_LOG_FLUSH_BATCH_LINE_NUM = 64;
		PE_CONSTEXPR_STATIC UINT32 EDITOR_LOG_MAX_VISIBLE_LINE_NUM = 10000u;
		PE_CONSTEXPR_STATIC UINT32 EDITOR_LOG_KEEP_FILE_NUM = 5u;

		EString FormatPadded(const UINT32 InValue, const UINT32 InWidth)
		{
			CHAR Buffer[16] = { 0 };
			::snprintf(Buffer, sizeof(Buffer), "%0*u", InWidth, InValue);
			return EString(Buffer);
		}

		EString FormatFileTimestamp(const EngineSystemTime::ESystemTime& InTime)
		{
			EString Result;
			Result += FormatPadded(InTime.Year, 4u);
			Result += "-";
			Result += FormatPadded(InTime.Month, 2u);
			Result += "-";
			Result += FormatPadded(InTime.Day, 2u);
			Result += "_";
			Result += FormatPadded(InTime.Hour, 2u);
			Result += "-";
			Result += FormatPadded(InTime.Minute, 2u);
			Result += "-";
			Result += FormatPadded(InTime.Second, 2u);
			Result += "-";
			Result += FormatPadded(InTime.Milliseconds, 3u);
			return Result;
		}

		BOOL32 AppendTextToFile(const EString& InPath, const EString& InText)
		{
			if (InPath.Length() == 0u)
			{
				return FALSE;
			}
			const std::filesystem::path FilePath(*InPath);
			std::error_code Error;
			if (FilePath.has_parent_path())
			{
				std::filesystem::create_directories(FilePath.parent_path(), Error);
			}
			std::ofstream Out(FilePath, std::ios::out | std::ios::app);
			if (!Out.is_open())
			{
				return FALSE;
			}
			Out.write(*InText, static_cast<std::streamsize>(InText.Length()));
			Out.flush();
			return (Out.good() != false) ? TRUE : FALSE;
		}
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<EEditorLogManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EEditorLogManager::EEditorLogManager()
	{
	}

	EEditorLogManager::~EEditorLogManager()
	{
	}

	void EEditorLogManager::Initialize()
	{
		EManagerBase::Initialize();
		if (LogFilePath.Length() == 0u)
		{
			LogFilePath = MakeLogFilePath();
		}
		PruneOldLogFiles();
	}

	void EEditorLogManager::ShutDown()
	{
		// Runtime builds need the tail too: the editor used to be the only caller that
		// flushed on exit, so a Develop run lost every line after the last error.
		WriteDownLogs();
		EManagerBase::ShutDown();
	}

	void EEditorLogManager::Tick()
	{
		{
			EScopedLock LogScope(LogMutex);
			if (Store.GetPendingFileLineNum() == 0u)
			{
				return;
			}
		}
		FlushPendingToFile();
	}

	void EEditorLogManager::AddALog(ELogType Type, const EString& NewLog, const BOOL32 InIsShowInImGui)
	{
		BOOL32 bShouldFlush = FALSE;
		{
			EScopedLock LogScope(LogMutex);
			Store.Add(Type, NewLog, InIsShowInImGui);

			// Write through, in batches: the log is the editor's only diagnostic channel, so an
			// error is on disk at once, while ordinary lines cost one append per batch rather
			// than per line. The append itself runs without the data lock.
			bShouldFlush = ((Type == ELogType::ELT_ERROR)
				|| (Store.GetPendingFileLineNum() >= static_cast<UINT32>(EDITOR_LOG_FLUSH_BATCH_LINE_NUM))) ? TRUE : FALSE;
		}
		if (bShouldFlush != FALSE)
		{
			FlushPendingToFile();
		}
	}

	void EEditorLogManager::WriteDownLogs()
	{
		FlushPendingToFile();
		PruneOldLogFiles();
	}

	void EEditorLogManager::SnapshotVisibleLogs(TArray<TSharedPtr<ELog>>& OutLogs)
	{
		EScopedLock LogScope(LogMutex);
		Store.SnapshotVisible(OutLogs);
	}

	UINT32 EEditorLogManager::GetLogRevision()
	{
		EScopedLock LogScope(LogMutex);
		return Store.GetRevision();
	}

	UINT32 EEditorLogManager::GetLogCount(ELogType InType)
	{
		EScopedLock LogScope(LogMutex);
		return Store.GetCount(InType);
	}

	void EEditorLogManager::ClearVisibleLogs()
	{
		EScopedLock LogScope(LogMutex);
		Store.ClearVisible();
	}

	const EString& EEditorLogManager::GetLogFilePath() const
	{
		return LogFilePath;
	}

	void EEditorLogManager::FlushPendingToFile()
	{
		TArray<EString> Lines;
		{
			EScopedLock LogScope(LogMutex);
			Store.TakePendingFileLines(Lines);
		}
		if (Lines.IsEmpty())
		{
			return;
		}

		EString Batch;
		for (INT32 Index = 0, LineNum = Lines.Num<INT32>(); Index < LineNum; Index++)
		{
			Batch += Lines[Index];
			Batch += "\n";
		}

		BOOL32 bWritten = FALSE;
		{
			EScopedLock FileScope(FileMutex);
			if (LogFilePath.Length() == 0u)
			{
				// Initialize() should have run before any producer exists; this keeps a
				// missing lifecycle call from dropping the session.
				LogFilePath = MakeLogFilePath();
			}
			bWritten = AppendTextToFile(LogFilePath, Batch);
		}

		if (bWritten == FALSE)
		{
			BOOL32 bReportFailure = FALSE;
			{
				EScopedLock LogScope(LogMutex);
				Store.RequeuePendingFileLines(Lines);
				if (bFileWriteFailureReported == FALSE)
				{
					bFileWriteFailureReported = TRUE;
					bReportFailure = TRUE;
				}
			}
			if (bReportFailure != FALSE)
			{
				// Adds one more pending line; the next flush retries the batch. The latch
				// above stops a dead disk from turning every following log into another
				// report, which would recurse through AddALog without bound.
				PE_LOG_ERROR(EString("EditorLogManager: failed to append log file: ") + LogFilePath);
			}
		}
	}

	void EEditorLogManager::PruneOldLogFiles()
	{
		const EString LogDir(EBaseSettings::EDITOR_LOGS_PATH);
		const std::filesystem::path LogDirPath(*LogDir);
		std::error_code Error;
		if (!std::filesystem::is_directory(LogDirPath, Error))
		{
			return;
		}

		struct FLogFileEntry
		{
			EString		FullPath;
			EString		Name;
			std::filesystem::file_time_type Time;
		};

		TArray<FLogFileEntry> Entries;
		std::filesystem::directory_iterator Iterator(LogDirPath, Error);
		const std::filesystem::directory_iterator End;
		while ((Error.value() == 0) && (Iterator != End))
		{
			const std::filesystem::directory_entry DirEntry = *Iterator;
			Iterator.increment(Error);
			std::error_code EntryError;
			if (!DirEntry.is_regular_file(EntryError))
			{
				continue;
			}
			const EString Name(DirEntry.path().filename().string());
			// Only our own session files; the directory is shared with other engine output.
			if (!Name.Contains("_Log.txt"))
			{
				continue;
			}
			FLogFileEntry Entry;
			Entry.Name = Name;
			Entry.FullPath = EString(DirEntry.path().string());
			Entry.Time = std::filesystem::last_write_time(DirEntry.path(), EntryError);
			if (EntryError)
			{
				// A file whose timestamp cannot be read is treated as the oldest.
				Entry.Time = std::filesystem::file_time_type();
			}
			Entries.Add(Entry);
		}

		// Sort by actual write time, not filename. The filename is padded now, but mtime
		// remains the authority if someone renamed or copied a log by hand.
		for (INT32 Index = 1, EntryNum = Entries.Num<INT32>(); Index < EntryNum; Index++)
		{
			const FLogFileEntry Current = Entries[Index];
			INT32 InsertIndex = Index - 1;
			while ((InsertIndex >= 0) && (Entries[InsertIndex].Time > Current.Time))
			{
				Entries[InsertIndex + 1] = Entries[InsertIndex];
				InsertIndex--;
			}
			Entries[InsertIndex + 1] = Current;
		}

		const UINT32 EntryNum = Entries.Num<UINT32>();
		if (EntryNum <= EDITOR_LOG_KEEP_FILE_NUM)
		{
			return;
		}
		const UINT32 DeleteNum = EntryNum - EDITOR_LOG_KEEP_FILE_NUM;
		for (UINT32 Index = 0u; Index < DeleteNum; Index++)
		{
			std::error_code RemoveError;
			// FullPath is already absolute; prepending LogDir produced ...\Logs\...\Logs
			// before, so every remove failed and the directory kept every session.
			std::filesystem::remove(std::filesystem::path(*Entries[Index].FullPath), RemoveError);
		}
	}

	EString EEditorLogManager::MakeLogFilePath() const
	{
		return EString(EBaseSettings::EDITOR_LOGS_PATH) + FormatFileTimestamp(EngineSystemTime::Now()) + "_Log.txt";
	}
}

#endif
