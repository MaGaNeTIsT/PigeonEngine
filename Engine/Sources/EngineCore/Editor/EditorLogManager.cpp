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


	ELog::ELog(ELogType InType, const EString& InLog, const EngineSystemTime::ESystemTime& InTimeStamp, const BOOL32 InIsShowInImGui)
		:
	Type(InType),
	Log(InLog),
	TimeStamp(InTimeStamp),
	bShowInImGui(InIsShowInImGui)
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
		// Keeps the pending tail short without paying an append per line; a burst inside one frame
		// is bounded by the batch size in AddALog.
		{
			EScopedLock LogScope(LogMutex);
			this->WriteLogsToFile();
		}

		ImGui::Begin("PigeonLogs");
		ImGui::BeginChild("PigeonLogsContent", ImVec2(0, 0), TRUE, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysHorizontalScrollbar);
		{
			// The render thread may be appending while this draws, so the list is walked
			// under the same lock AddALog takes.
			EScopedLock LogScope(LogMutex);

			// Only the rows on screen are laid out. The window keeps the whole session, and laying
			// all of it out on every frame is what made a verbose session cost tens of
			// milliseconds per frame for the rest of its life.
			const INT32 VisibleLogNum = this->VisibleLogIndices.Num<INT32>();

			// A clipped row contributes no width, so the content width is accumulated as lines
			// arrive: the horizontal scrollbar has to describe the log, not the visible slab.
			for (INT32 Index = this->NumMeasuredVisibleLogs; Index < VisibleLogNum; Index++)
			{
				const EString Text = this->Logs[this->VisibleLogIndices[Index]]->AsString();
				const FLOAT LineWidth = ImGui::CalcTextSize(*Text).x;
				if (LineWidth > this->MaxVisibleLogWidth)
				{
					this->MaxVisibleLogWidth = LineWidth;
				}
			}
			this->NumMeasuredVisibleLogs = VisibleLogNum;

			ImGuiListClipper Clipper;
			Clipper.Begin(VisibleLogNum, ImGui::GetTextLineHeightWithSpacing());
			while (Clipper.Step())
			{
				for (INT32 Index = Clipper.DisplayStart; Index < Clipper.DisplayEnd; Index++)
				{
					this->Logs[this->VisibleLogIndices[Index]]->PrintLog();
				}
			}
			Clipper.End();
		}

		// The rows above were clipped away before they could widen the content area.
		ImGui::Dummy(ImVec2(this->MaxVisibleLogWidth, 0.0f));

		ImGui::EndChild();
		ImGui::End();
	}

	// A block of lines costs one append instead of one per line. The shader compiler prints about
	// 160 lines per stage, and an append opens, writes and closes the file, so a cold cook spent
	// seconds on the log alone. The batch bounds what a killed process can lose.
	PE_CONSTEXPR_STATIC INT32 EDITOR_LOG_FLUSH_BATCH_LINE_NUM = 64;

	void EEditorLogManager::AddALog(ELogType Type, const EString& NewLog, const BOOL32 InIsShowInImGui)
	{
		// TSharedPtr<ELogType> NewLog = EMemory::MakeShared<ELogType>(Type, NewLog);
		EScopedLock LogScope(LogMutex);
		this->Logs.Add(EMemory::MakeShared<ELog>(Type, NewLog, EngineSystemTime::Now(), InIsShowInImGui));
		if (InIsShowInImGui)
		{
			// The window's own list, by index: a file only line never enters it, so the rows the
			// clipper walks are exactly the rows it draws.
			this->VisibleLogIndices.Add(this->Logs.Num<INT32>() - 1);
		}

		// Write through, in batches: the log is the editor's only diagnostic channel, so an error
		// is on disk at once, while ordinary lines cost one append per batch rather than per line.
		const INT32 PendingLogNum = this->Logs.Num<INT32>() - this->NumFlushedLogs;
		if ((Type == ELogType::ELT_ERROR) || (PendingLogNum >= EDITOR_LOG_FLUSH_BATCH_LINE_NUM))
		{
			this->WriteLogsToFile();
		}
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
