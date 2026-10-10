#include "EditorLogManager.h"
#if !(_SHIPPING_MODE)
#include "Base/Timer/Timer.h"
#include <imgui.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<EEditorLogManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);


	ELog::ELog(ELogType InType, const EString& InLog, const EngineSystemTime::ESystemTime& InTimeStamp)
		:
	Type(InType),
	Log(InLog),
	TimeStamp(InTimeStamp)
	{
	}

	void ELog::PrintLog() const
	{
		EString LogStr = this->AsString();
		if(Type == ELogType::ELT_LOG)
		{
			ImGui::Text(*LogStr);
		}
		else
		{
			ImGui::TextColored(Type == ELogType::ELT_ERROR ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.0f, 1.0f), *LogStr);
		}
	}

	EString ELog::AsString() const
	{
		const EString TimeStampStr = EString("[") + this->TimeStamp.AsString() + EString("]");
		EString Output;
		if(Type == ELogType::ELT_LOG)
		{
			Output = TimeStampStr + " " + EString("Logs:") + Log;
		}
		else
		{
			Output = TimeStampStr + " " + (Type == ELogType::ELT_ERROR ? EString("Error:") : EString("Warning:")) + Log;
		}
		return Output;
	}

	EEditorLogManager::EEditorLogManager()
	{
	}

	EEditorLogManager::~EEditorLogManager()
	{
	}
	void EEditorLogManager::Initialize()
	{
		EManagerBase::Initialize();
		EString TimeStamp = EngineSystemTime::Now().AsString();
		TimeStamp = TimeStamp.Replace(":", "-");
		TimeStamp = TimeStamp.Replace(" ", "_");
		LogFilePath = EString(EBaseSettings::EDITOR_LOGS_PATH) + TimeStamp + EString("_Log.txt");
	}

	void EEditorLogManager::ShutDown()
	{
#if _EDITOR_ONLY
		WriteDownLogs();
#endif

		EManagerBase::ShutDown();
	}

	void EEditorLogManager::Init()
	{
	}

	void EEditorLogManager::Uninit()
	{
	}

	void EEditorLogManager::EditorInit()
	{
	}

	void EEditorLogManager::EditorUpdate()
	{
		ImGui::Begin("PigeonLogs");
		ImGui::BeginChild("PigeonLogsContent", ImVec2(0, 0), TRUE, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysHorizontalScrollbar);
		{
			// The render thread may be appending while this draws, so the list is walked
			// under the same lock AddALog takes.
			EScopedLock LogScope(LogMutex);
			for(const auto& elem : Logs)
			{
				elem->PrintLog();
			}
		}

		ImGui::EndChild();
		ImGui::End();
	}

	void EEditorLogManager::AddALog(ELogType Type, const EString& NewLog)
	{
		// TSharedPtr<ELogType> NewLog = EMemory::MakeShared<ELogType>(Type, NewLog);
		EScopedLock LogScope(LogMutex);
		this->Logs.Add(EMemory::MakeShared<ELog>(Type, NewLog, EngineSystemTime::Now()));
		// Write through: the log is the editor's only diagnostic channel, and a process that
		// is killed shows nothing unless the lines are already on disk.
		this->WriteLogsToFile();
	}

	void EEditorLogManager::WriteLogsToFile()
	{
		const INT32 LogNum = this->Logs.Num<INT32>();
		if (LogNum <= this->NumFlushedLogs)
		{
			return;
		}

		// Fallback: if Initialize() was never called, generate the path now
		if (this->LogFilePath.Length() == 0u)
		{
			EString TimeStamp = EngineSystemTime::Now().AsString();
			TimeStamp = TimeStamp.Replace(":", "-");
			TimeStamp = TimeStamp.Replace(" ", "_");
			this->LogFilePath = EString(EBaseSettings::EDITOR_LOGS_PATH) + TimeStamp + EString("_Log.txt");
		}

		EString Str;
		for(INT32 Index = this->NumFlushedLogs; Index < LogNum; Index++)
		{
			Str += this->Logs[Index]->AsString() + "\n";
		}

		EFileHelper::SaveStringToFile(this->LogFilePath, Str, TRUE, TRUE);
		this->NumFlushedLogs = LogNum;
	}

	void EEditorLogManager::WriteDownLogs()
	{
		// Everything but a possible tail is on disk already; the entries stay in memory so
		// the log window keeps its history.
		{
			EScopedLock LogScope(LogMutex);
			this->WriteLogsToFile();
		}

		// Prune old log files - keep only the 5 most recent
		const EString LogDir(EBaseSettings::EDITOR_LOGS_PATH);
		TArray<EString> Folders, LogFiles;
		if (EFileHelper::ScanDirectory(LogDir, Folders, LogFiles, TRUE))
		{
			// Keep only files that match our naming pattern (contain "Log.txt")
			TArray<EString> FilteredLogs;
			for (INT32 i = 0, n = LogFiles.Num<INT32>(); i < n; i++)
			{
				if (LogFiles[i].Contains("Log.txt"))
				{
					FilteredLogs.Add(LogFiles[i]);
				}
			}

			// Insertion sort ascending by filename (timestamp names -> oldest first)
			for (INT32 i = 1, n = FilteredLogs.Num<INT32>(); i < n; i++)
			{
				for (INT32 j = i; j > 0 && FilteredLogs[j] < FilteredLogs[j - 1]; j--)
				{
					EString Tmp = FilteredLogs[j];
					FilteredLogs[j] = FilteredLogs[j - 1];
					FilteredLogs[j - 1] = Tmp;
				}
			}

			// Delete oldest entries beyond the limit
			const INT32 MaxLogCount = 5;
			const INT32 NumToDelete = FilteredLogs.Num<INT32>() - MaxLogCount;
			for (INT32 i = 0; i < NumToDelete; i++)
			{
				::DeleteFileA(*FilteredLogs[i]);
			}
		}
	}
}
#endif
