#include "EditorManager.h"

#include "PigeonBase/Object/World/WorldManager.h"
#include "AssetManager.h"

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

    void EEditorManager::EditorInit()
    {
        m_WorldManager = EWorldManager::GetManagerSingleton();
        m_AssetManager = EAssetManager::GetManagerSingleton();
        m_AssetManager->EditorInit();
        m_WorldManager->EditorInit();
    }

    void EEditorManager::EditorUpdate()
    {
        BOOL8 TempEditorOpen = m_EditorOpen;
        ImGui::Begin("EditorRoot", &TempEditorOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Frame Rate Average %.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        
        m_EditorOpen = TempEditorOpen;
        
        m_WorldManager->EditorUpdate();
        m_AssetManager->EditorUpdate();
        
        BOOL8 bOpen = FALSE;
        ImGui::ShowDemoWindow(&bOpen);
    }
}