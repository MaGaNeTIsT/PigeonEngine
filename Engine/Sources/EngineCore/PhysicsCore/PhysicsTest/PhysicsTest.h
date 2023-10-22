#pragma once
#include <CoreMinimal.h>
#include "../../Core/PigeonBase/Object/Actor/LevelCharacter.h"
#include "../../Core/PigeonBase/Object/Component/Primitive/StaticMeshComponent.h"
#include "../../../../EngineThirdParty/JoltPhysics/Headers/Character/Character.h"


namespace PigeonEngine
{
	class PPhysicsTestCharacter : public PCharacter
	{
	public:
		virtual void InitCharacter(const class FCharacterSettings* InCharacterSettings) override;
		virtual void UninitCharacter() override;
		PStaticMeshComponent* StaticMeshComponent = NULL;
	};
}