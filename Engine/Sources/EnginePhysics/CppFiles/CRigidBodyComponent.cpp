#include "../Headers/CRigidBodyComponent.h"

void CRigidBodyComponent::SetGameObjectHook(const class CGameObject* gameObject)
{
	if (gameObject != NULL)
	{
		this->m_GameObject = gameObject;
		JPH::BodyCreationSettings* BodyCreateSetting = GetPhysicsManager()->CreateBodyCreationSettings();
		BodyCreateSetting->mPosition = PhysicsUtility::Convert(gameObject->GetWorldPosition());
		BodyCreateSetting->mRotation = PhysicsUtility::Convert(gameObject->GetWorldRotation());
		m_Body = GetPhysicsManager()->CreateAndAddBody(gameObject->GetUniqueID(), BodyCreateSetting, EActivation::DontActivate);
	}
}

inline void CRigidBodyComponent::AddForce(CustomType::Vector3 inForce)
{
	GetPhysicsManager()->AddForce(m_Body, PhysicsUtility::Convert(inForce));
}
inline void CRigidBodyComponent::AddForce(CustomType::Vector3 inForce, CustomType::Vector3 inPoint)
{
	GetPhysicsManager()->AddForce(m_Body, PhysicsUtility::Convert(inForce), PhysicsUtility::Convert(inPoint));
}
inline void CRigidBodyComponent::AddImpulse(CustomType::Vector3 inImpulse)
{
	GetPhysicsManager()->AddImpulse(m_Body, PhysicsUtility::Convert(inImpulse));
}
inline void CRigidBodyComponent::AddImpulse(CustomType::Vector3 inImpulse, CustomType::Vector3 inPoint)
{
	GetPhysicsManager()->AddImpulse(m_Body, PhysicsUtility::Convert(inImpulse), PhysicsUtility::Convert(inPoint));
}