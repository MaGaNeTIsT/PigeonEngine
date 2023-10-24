#include "PhysicsTest.h"
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include "../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

void PigeonEngine::PPhysicsTestCharacter::InitCharacter(FCharacterSettings* InCharacterSettings)
{
	PCharacter::InitCharacter(InCharacterSettings);
	FShape* Shape = new FBoxShape(Vector3(1000.f,10.f,1000.f),0.f);
	FPhysicsBodyId BodyId;
	FPhysicsManager::GetSingleton()->TryCreateBody(Shape,false,Vector3(),Quaternion::Identity(),PhysicsUtility::EMotionType::Static,Layers::MOVING, BodyId);
	FPhysicsManager::GetSingleton()->AddBody(this->GetUniqueID(), BodyId, PhysicsUtility::EActivate::Activate);
}

void PigeonEngine::PPhysicsTestCharacter::UninitCharacter()
{
	PCharacter::UninitCharacter();
	DestoyComponent(StaticMeshComponent);
}
