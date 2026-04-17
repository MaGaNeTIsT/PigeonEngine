#include "WorldManager.h"
#include "WorldManager.h"

#include "World.h"
#include "../../Development/Message/Message.h"
#include <Editor/EditorLogManager.h>
#if _EDITOR_ONLY
#include <imgui.h>
#endif

namespace PigeonEngine
{
    static void RegisterClassTypes()
    {
        RegisterClassType<EWorldManager, EManagerBase>();
    }

    static void LogTestMessage(const EString& InMessage)
    {
        PE_LOG_LOG(EString("[TestMessage] ") + InMessage);
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);
    
    EWorldManager::EWorldManager()
	{
        this->World = nullptr;
	}

    EWorldManager::~EWorldManager()
	{
		
	}
    void EWorldManager::Initialize()
    {
        EManagerBase::Initialize();

        this->World = new PWorld();
    }

    void EWorldManager::ShutDown()
    {
        if(this->World)
        {
            this->World->Destroy();
            this->World = nullptr;
        }
        EManagerBase::ShutDown();
    }

    void EWorldManager::Init()
    {
        if(!this->World)
        {
            this->World = new PWorld();
        }
        this->World->Init();
    }

    void EWorldManager::Uninit()
    {
        if(this->World)
        {
            this->World->Destroy();
            this->World = nullptr;
        }
    }

#if _EDITOR_ONLY
    void EWorldManager::EditorInit()
    {

    }

    void EWorldManager::EditorUpdate()
    {
        if (!this->GetWorld())
        {
            return;
        }

        // ---- World Outline panel ----
        ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::Begin("World Outline");
        this->GetWorld()->GenerateWorldOutline();
        ImGui::End();

        // ---- Details panel ----
        ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 440), ImGuiCond_FirstUseEver);
        ImGui::Begin("Details");
        this->GetWorld()->GenerateDetail();
        ImGui::End();
    }
#endif
    PWorld* EWorldManager::GetWorld() const
    {
        return this->World;
    }
}

