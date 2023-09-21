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

