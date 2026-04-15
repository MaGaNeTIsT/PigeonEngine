#include "WorldTickManager.h"

namespace PigeonEngine
{
    static void RegisterClassTypes()
    {
        RegisterClassType<EWorldTickManager, EManagerBase>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    EWorldTickManager::EWorldTickManager()
    {
    }

    EWorldTickManager::~EWorldTickManager()
    {
    }

    void EWorldTickManager::Initialize()
    {
    }

    void EWorldTickManager::ShutDown()
    {
        OnPrePhysicsTick.RemoveAll();
        OnPhysicsTick.RemoveAll();
        OnPostPhysicsTick.RemoveAll();
    }

    void EWorldTickManager::Init()
    {
    }

    void EWorldTickManager::Uninit()
    {
    }

    void EWorldTickManager::Tick(FLOAT deltaTime)
    {
        OnPrePhysicsTick.Broadcast(deltaTime);
        OnPhysicsTick.Broadcast(deltaTime);
        OnPostPhysicsTick.Broadcast(deltaTime);
    }

    void EWorldTickManager::RegisterPrePhysicsTick(const TFunction<void(FLOAT)>& InHandler)
    {
        OnPrePhysicsTick.Add(InHandler);
    }

    void EWorldTickManager::UnregisterPrePhysicsTick(const TFunction<void(FLOAT)>& InHandler)
    {
        OnPrePhysicsTick.Remove(InHandler);
    }

    void EWorldTickManager::RegisterPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
    {
        OnPhysicsTick.Add(InHandler);
    }

    void EWorldTickManager::UnregisterPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
    {
        OnPhysicsTick.Remove(InHandler);
    }

    void EWorldTickManager::RegisterPostPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
    {
        OnPostPhysicsTick.Add(InHandler);
    }

    void EWorldTickManager::UnregisterPostPhysicsTick(const TFunction<void(FLOAT)>& InHandler)
    {
        OnPostPhysicsTick.Remove(InHandler);
    }
}
