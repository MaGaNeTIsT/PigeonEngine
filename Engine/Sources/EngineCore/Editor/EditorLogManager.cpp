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
        for(const auto& elem : Logs)
        {
            elem->PrintLog();
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void EEditorLogManager::AddALog(ELogType Type, const EString& NewLog)
    {
       // TSharedPtr<ELogType> NewLog = EMemory::MakeShared<ELogType>(Type, NewLog);
        this->Logs.Add(EMemory::MakeShared<ELog>(Type, NewLog, EngineSystemTime::Now()));
    }

    void EEditorLogManager::WriteDownLogs()
    {
        if (this->Logs.Num() <= 0)
        {
            return;
        }
        EString Str;
        for(const auto& elem : this->Logs)
        {
            Str += elem->AsString() + "\r\n";
        }

        EString FileName = EString(EBaseSettings::EDITOR_LOGS_PATH);
        EString FileName1 = EString("[") + EngineSystemTime::Now().AsString() + EString("]");
        FileName1 = FileName1.Replace(":", "-");
        FileName = FileName + FileName1 + EString("Log.txt");
        EFileHelper::SaveStringToFile(FileName, Str);
        this->Logs.Empty();

        // Prune old log files — keep only the 5 most recent
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

            // Insertion sort ascending by filename (timestamp names → oldest first)
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