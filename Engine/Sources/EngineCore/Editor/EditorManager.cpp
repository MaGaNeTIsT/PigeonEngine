#include "EditorManager.h"

#include "PigeonBase/Object/World/WorldManager.h"

namespace PigeonEngine
{
    static void RegisterClassTypes()
    {
        RegisterClassType<EEditorManager, EManagerBase>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);
    
    EEditorManager::EEditorManager()
    {
    }

    EEditorManager::~EEditorManager()
    {
		
    }

    void EEditorManager::Initialize()
    {

    }

    void EEditorManager::ShutDown()
    {

    }

    void EEditorManager::Init()
    {

    }

    void EEditorManager::Uninit()
    {

    }

    void EEditorManager::EditorUpdate()
    {
        BOOL8 TempEditorOpen = m_EditorOpen;
        ImGui::Begin("EditorRoot", &TempEditorOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Frame Rate Average %.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        m_EditorOpen = TempEditorOpen;
        EWorldManager* m_WorldManager = EWorldManager::GetManagerSingleton();
        m_WorldManager->EditorUpdate();

        bool bOpen = false;
        ImGui::ShowDemoWindow(&bOpen);
    }
}