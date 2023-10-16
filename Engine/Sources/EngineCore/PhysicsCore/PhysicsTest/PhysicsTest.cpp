#include "PhysicsTest.h"
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>

void PigeonEngine::PPhysicsTestCharacter::InitCharacter(const FCharacterSettings* InCharacterSettings)
{
	PCharacter::InitCharacter(InCharacterSettings);
	StaticMeshComponent = new PStaticMeshComponent();
	StaticMeshComponent->SetIsTickable(TRUE);
	const EStaticMeshAsset* Asset = nullptr;
	EString ImportPath("./Engine/Assets/EngineModels/SceneModels/Robot/");
	EString ImportName("Robot");
	EString ImportFileType("obj");
	TryLoadStaticMesh(ESettings::ENGINE_MESH_PATH, "Robot", Asset, &ImportPath, &ImportName, &ImportFileType);
	StaticMeshComponent->SetMeshAsset(Asset);
	AddComponent(StaticMeshComponent,ETransform());
}

void PigeonEngine::PPhysicsTestCharacter::UninitCharacter()
{
	PCharacter::UninitCharacter();
	DestoyComponent(StaticMeshComponent);
}
