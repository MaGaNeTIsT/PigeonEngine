#include "PhysicsComponent.h"
#include <PigeonBase/Object/Actor.h>
#if _EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

namespace PigeonEngine 
{

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
		InitPhysicsComponent();
	}

	void PPhysicsComponent::Uninit()
	{
		PActorComponent::Uninit();
		UninitPhysicsComponent();
	}

	void PPhysicsComponent::Destroy()
	{
		PActorComponent::Destroy();
		if (m_Shape)
			RemoveShape();
	}
#if _EDITOR_ONLY
	void PPhysicsComponent::EditorTick(FLOAT deltaTime)
	{
		RDebugWireframePrimitiveManager* Manager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		const Vector3 Pos = GetOwnerActor()->GetActorLocation();
	}
#endif
	void PPhysicsComponent::SetShape(FShape* InShape)
	{
		if (m_Shape)
		{
			UninitPhysicsComponent();
			m_Shape = InShape;
			InitPhysicsComponent();
		}
		else
			m_Shape = InShape;
	}

	void PPhysicsComponent::InitPhysicsComponent()
	{
		Vector3 pos = GetOwnerActor()->GetRootComponent()->GetComponentWorldLocation();
		Quaternion rot = GetOwnerActor()->GetRootComponent()->GetComponentWorldRotation();
		if (FPhysicsManager::GetSingleton()->TryCreateBody(m_Shape, FALSE, pos, rot, MotionType, Layer, m_BodyId))
		{
			FPhysicsManager::GetSingleton()->AddBody(GetOwnerActor()->GetUniqueID(), m_BodyId);
		}
	}

	void PPhysicsComponent::UninitPhysicsComponent()
	{
		RemoveShape();
	}

	FShape* PPhysicsComponent::GetShape()
	{
		return m_Shape;
	}

	void PPhysicsComponent::RemoveShape()
	{
		FPhysicsManager::GetSingleton()->RemoveBody(GetOwnerActor()->GetUniqueID(), TRUE);
		m_Shape = nullptr;
	}

	void PPhysicsComponent::AddForce(Vector3 InForce)
	{
		FPhysicsManager::GetSingleton()->AddForce(m_BodyId, InForce);
	}

	void PPhysicsComponent::AddImpluse(Vector3 InImpluse)
	{
		FPhysicsManager::GetSingleton()->AddImpulse(m_BodyId, InImpluse);
	}


}