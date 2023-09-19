#include "WorldManager.h"

#include "World.h"

namespace PigeonEngine
{
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
        }
        EManagerBase::ShutDown();
    }

    void EWorldManager::Init()
    {
        if(this->World)
        {
            this->World->Init();
        }
    }

    void EWorldManager::Uninit()
    {
        if(this->World)
        {
            this->World->Uninit();
        }
    }

    PWorld* EWorldManager::GetWorld() const
    {
        return this->World;
    }
}

