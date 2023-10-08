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
        ImGui::Begin("PigeonWo", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("PigeonWo");
        if(this->GetWorld())
        {
            this->GetWorld()->GenerateWorldOutline();
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

