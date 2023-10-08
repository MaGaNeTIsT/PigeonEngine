#include "WorldManager.h"

#include "World.h"

namespace PigeonEngine
{
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
            this->World->Uninit();
            this->World = nullptr;
        }
    }

    void EWorldManager::EditorInit()
    {
        
    }
#if _EDITOR_ONLY
    void EWorldManager::EditorUpdate()
    {
        if (!this->GetWorld())
        {
            return;
        }
        ImGui::Begin("PigeonWo", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
        if (ImGui::CollapsingHeader("PigeonOutline", ImGuiTreeNodeFlags_DefaultOpen))
        {
            this->GetWorld()->GenerateWorldOutline();
        }
        if(ImGui::CollapsingHeader("Detail", ImGuiTreeNodeFlags_DefaultOpen))
        {
            this->GetWorld()->GenerateDetail();
        }
        ImGui::End();
        bool bOpen = false;
        ImGui::ShowDemoWindow(&bOpen);
    }
#endif
    PWorld* EWorldManager::GetWorld() const
    {
        return this->World;
    }
}

