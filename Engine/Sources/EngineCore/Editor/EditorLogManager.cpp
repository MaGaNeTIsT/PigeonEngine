#include "EditorLogManager.h"

namespace PigeonEngine
{
    static void RegisterClassTypes()
    {
        RegisterClassType<EEditorLogManager, EManagerBase>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    ELog::ELog(ELogType InType, const EString& InLog)
        :
    Type(InType),
    Log(InLog)
    {
    }

    void ELog::PrintLog() const
    {
        
        if(Type == ELogType::ELT_LOG)
        {
            EString Output = EString("Logs:") + Log;
            ImGui::Text(*Output);
        }
        else
        {
            EString Output = Type == ELogType::ELT_ERROR ? EString("Error:") : EString("Warning:") + Log;
            
            ImGui::TextColored(Type == ELogType::ELT_ERROR ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.0f, 1.0f), *Output);
        }
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
        ImGui::Begin("PigeonLogs", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("PigeonLogs");
        ImGui::BeginChild("PigeonLogs", ImVec2(400, 0), true, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        for(const auto& elem : Logs)
        {
            elem.PrintLog();
        }
		
        ImGui::EndChild();
        ImGui::End();
    }

    void EEditorLogManager::AddALog(ELogType Type, const EString& NewLog)
    {
        this->Logs.Add(ELog(Type, NewLog));
    }

   

}
