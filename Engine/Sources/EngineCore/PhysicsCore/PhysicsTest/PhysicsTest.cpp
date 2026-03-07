#include "PhysicsTest.h"
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include "../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#if _EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

void PigeonEngine::PPhysicsTestCharacter::InitCharacter(FCharacterSettings* InCharacterSettings)
{
	PCharacter::InitCharacter(InCharacterSettings);
	//Shape = new FBoxShape(Vector3(1000.f,10.f,1000.f),0.f);
	//FPhysicsBodyId BodyId;
	//FPhysicsManager::GetSingleton()->TryCreateBody(Shape,FALSE,Vector3(0.f,-10.f,0.f),Quaternion::Identity(),PhysicsUtility::EMotionType::Static,Layers::MOVING, BodyId);
	//FPhysicsManager::GetSingleton()->AddBody(this->GetUniqueID(), BodyId, PhysicsUtility::EActivate::Activate);
}

void PigeonEngine::PPhysicsTestCharacter::UserTick(FLOAT deltaTime)
{
	PCharacter::UserTick(deltaTime);
	//RDebugWireframePrimitiveManager* Manager = RDebugWireframePrimitiveManager::GetManagerSingleton();
	//const Vector3 Pos = GetActorLocation();
	//const Quaternion Rot = GetActorRotation();
	//Shape->DrawPrimitive(Manager, Pos + Vector3(0.f, -10.f, 0.f), Rot);
}

void PigeonEngine::PPhysicsTestCharacter::UninitCharacter()
{
	PCharacter::UninitCharacter();
	DestoyComponent(StaticMeshComponent);
}
