#pragma once
#include <CoreMinimal.h>
#include "../../Core/PigeonBase/Object/Actor/LevelCharacter.h"
#include "../../Core/PigeonBase/Object/Component/Primitive/StaticMeshComponent.h"
#include "../../../../EngineThirdParty/JoltPhysics/Headers/Character/Character.h"
#include "PigeonBase/Object/Controller/CharacterController.h"


namespace PigeonEngine
{
	class PPhysicsTestCharacter : public PCharacter
	{
	public:
		virtual void InitCharacter(class FCharacterSettings* InCharacterSettings) override;
		virtual void UserTick(FLOAT deltaTime) override;
		virtual void UninitCharacter() override;
		PStaticMeshComponent* StaticMeshComponent = NULL;
		FShape* Shape = NULL;
		PCharacterController* CharacterController = NULL;
	};
}