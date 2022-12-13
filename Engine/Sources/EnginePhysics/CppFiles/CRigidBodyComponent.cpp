#include "../Headers/CRigidBodyComponent.h"

void CRigidBodyComponent::SetGameObject(const class CGameObject* gameObject)
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

void CRigidBodyComponent::FixedUpdate()
{
	if (m_GameObject != NULL)
	{
		//m_GameObject->SetWorldPosition(PhysicsUtility::Convert(GetPhysicsManager()->GetPosition(m_Body)));
		//m_GameObject->GetWorldRotation(PhysicsUtility::Convert(GetPhysicsManager()->GetRotation(m_Body)));
	}
}