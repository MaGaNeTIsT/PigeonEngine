#include "EditorLogManager.h"

namespace PigeonEngine
{
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
            EString Output = EString("Logs:") + elem;
            ImGui::Text(*Output);
        }
		
        ImGui::EndChild();
        ImGui::End();
    }

    void EEditorLogManager::AddALog(const EString& NewLog)
    {
        this->Logs.Add(NewLog);
    }


}
