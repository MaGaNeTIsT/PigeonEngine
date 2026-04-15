#include "../Headers/PhysicsManager.h"
#include "../Headers/PhysicsManager.h"

PIGEONENGINE_NAMESPACE_BEGIN

static void RegisterClassTypes()
{
    RegisterClassType<FPhysicsManager, EManagerBase>();
}

PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

FPhysicsManager::FPhysicsManager() {}

FPhysicsManager::~FPhysicsManager() {}

void FPhysicsManager::Initialize()
{
    EManagerBase::Initialize();
}

void FPhysicsManager::ShutDown()
{
    EManagerBase::ShutDown();
}

void FPhysicsManager::Init()
{
    InitPhysics();
}

void FPhysicsManager::Uninit()
{
    UninitPhysics();
}

void FPhysicsManager::Update()
{
}

void FPhysicsManager::FixedUpdate(FLOAT DeltaTime)
{
    PhysicsUpdate(DeltaTime);
}

void FPhysicsManager::Draw()
{
}

PIGEONENGINE_NAMESPACE_END
