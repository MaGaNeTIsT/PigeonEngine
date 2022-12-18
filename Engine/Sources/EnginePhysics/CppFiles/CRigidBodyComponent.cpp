#include "../Headers/CRigidBodyComponent.h"

void CRigidBodyComponent::SetGameObjectHook(const CGameObject* gameObject)
{
	if (gameObject != NULL)
	{
		this->m_GameObject = gameObject;
		CustomType::Vector3 BoxMin, BoxMax;
		gameObject->GetRenderWorldAABBBoundingBox(BoxMin, BoxMax);
		FLOAT X, Y, Z;
		X = (BoxMax.X() - BoxMin.X()) / 2.f;
		Y = (BoxMax.Y() - BoxMin.Y()) / 2.f;
		Z = (BoxMax.Z() - BoxMin.Z()) / 2.f;

		X = X > 1.f ? X : 1.f;
		Y = Y > 1.f ? Y : 1.f;
		Z = Z > 1.f ? Z : 1.f;
		CustomType::Vector3 HalfExtent(X, Y, Z);
		JPH::BoxShapeSettings* BoxShapeSetting = CPhysicsManager::GetPhysicsManager()->CreateBoxShapeSettings(PhysicsUtility::Convert(HalfExtent), 0.05f);
		JPH::BodyCreationSettings* BodyCreateSetting = CPhysicsManager::GetPhysicsManager()->CreateBodyCreationSettings(BoxShapeSetting,
			PhysicsUtility::Convert(gameObject->GetWorldPosition()),
			PhysicsUtility::Convert(gameObject->GetWorldRotation()),
			m_EMotionType,
			m_ObjectLayer);
		BodyCreateSetting->ConvertShapeSettings();
		if (CPhysicsManager::GetPhysicsManager()->TryCreateBody(BodyCreateSetting, m_Body))
		{
			CPhysicsManager::GetPhysicsManager()->AddBody(gameObject->GetUniqueID(), m_Body, m_EActivation);
		}
		//m_Body = CPhysicsManager::GetPhysicsManager()->CreateAndAddBody(gameObject->GetUniqueID(), BodyCreateSetting, EActivation::DontActivate);
	}
}

inline void CRigidBodyComponent::AddForce(CustomType::Vector3 inForce)
{
	CPhysicsManager::GetPhysicsManager()->AddForce(m_Body, PhysicsUtility::Convert(inForce));
}
inline void CRigidBodyComponent::AddForce(CustomType::Vector3 inForce, CustomType::Vector3 inPoint)
{
	CPhysicsManager::GetPhysicsManager()->AddForce(m_Body, PhysicsUtility::Convert(inForce), PhysicsUtility::Convert(inPoint));
}
inline void CRigidBodyComponent::AddImpulse(CustomType::Vector3 inImpulse)
{
	CPhysicsManager::GetPhysicsManager()->AddImpulse(m_Body, PhysicsUtility::Convert(inImpulse));
}
inline void CRigidBodyComponent::AddImpulse(CustomType::Vector3 inImpulse, CustomType::Vector3 inPoint)
{
	CPhysicsManager::GetPhysicsManager()->AddImpulse(m_Body, PhysicsUtility::Convert(inImpulse), PhysicsUtility::Convert(inPoint));
}