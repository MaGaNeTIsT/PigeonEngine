#include "PhysicsComponent.h"
#include <PigeonBase/Object/Actor.h>

PIGEONENGINE_NAMESPACE_BEGIN

static void RegisterClassTypes()
{
	RegisterClassType<PPhysicsComponent, PActorComponent>();
}

PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);


PPhysicsComponent::~PPhysicsComponent()
{
	if (m_Shape)
		RemoveShape();
}

void PPhysicsComponent::Init()
{
	PActorComponent::Init();
	Vector3 pos = GetOwnerActor()->GetRootComponent()->GetComponentWorldLocation();
	Quaternion rot = GetOwnerActor()->GetRootComponent()->GetComponentWorldRotation();
	if (FPhysicsManager::GetSingleton()->TryCreateBody(m_Shape,false, pos, rot,MotionType,Layer,m_BodyId))
	{
		FPhysicsManager::GetSingleton()->AddBody(GetOwnerActor()->GetUniqueID(), m_BodyId);
	}
}

void PPhysicsComponent::Uninit()
{
	PActorComponent::Uninit();
	FPhysicsManager::GetSingleton()->RemoveBody(GetOwnerActor()->GetUniqueID(), false);
}

void PPhysicsComponent::Destroy()
{
	PActorComponent::Destroy();
	if (m_Shape)
		RemoveShape();
}

void PPhysicsComponent::SetShape(FShape* InShape)
{
	m_Shape = InShape;
}

FShape* PPhysicsComponent::GetShape()
{
	return m_Shape;
}

void PPhysicsComponent::RemoveShape()
{
	delete m_Shape;
	m_Shape = nullptr;
}

void PPhysicsComponent::AddForce(Vector3 InForce)
{
	FPhysicsManager::GetSingleton()->AddForce(m_BodyId, InForce);
}


PIGEONENGINE_NAMESPACE_END