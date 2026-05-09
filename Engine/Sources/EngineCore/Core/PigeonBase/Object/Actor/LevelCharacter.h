#pragma once
#include <CoreMinimal.h>
#include "Pawn.h"
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/Character/CharacterVirtual.h"

namespace PigeonEngine
{
	class PCharacter : public PPawn, public FCharacterContactEventListenerInterface
	{
		friend class PWorld;
		CLASS_VIRTUAL_NOCOPY_BODY(PCharacter)
	public:
		virtual void InitCharacter(class FCharacterVirtualSettings& InCharacterSettings);
		virtual void UninitCharacter();

		virtual void SetUp(Vector3 UpVector);

		class FShape* GetStandingShape();
		class FShape* GetCrouchingShape();
		class FCharacterVirtual* GetPhysicsCharacter();
		class PMovementComponent* GetMovementComponent();

#if _EDITOR_ONLY
		void EditorTick(FLOAT deltaTime) override;
#endif

	protected:
		virtual void BeginAddedToScene(PWorld* World) override;
		virtual void RemovedFromScene() override;

		virtual void UserBeginPlay();
		virtual void UserTick(FLOAT deltaTime) override;
		virtual void UserEndPlay();

		virtual void OnPhysicsAdjustBodyVelocity(const FPhysicsBodyId& inBodyID2, Vector3& ioLinearVelocity, Vector3& ioAngularVelocity) {}
		virtual BOOL8 OnPhysicsContactValidate(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2) { return TRUE; }
		virtual void OnPhysicsContactAdded(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, FPhysicsCharacterContactSettings& ioSettings) {}
		virtual void OnPhysicsContactSolve(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, const Vector3& inContactVelocity, const Vector3& inCharacterVelocity, Vector3& ioNewCharacterVelocity) {}

	private:
        void HandleWorldUpVectorChanged(const Vector3& InUpVector);
		virtual void OnAdjustBodyVelocity(const FPhysicsBodyId& inBodyID2, Vector3& ioLinearVelocity, Vector3& ioAngularVelocity) override;
		virtual BOOL8 OnContactValidate(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2) override;
		virtual void OnContactAdded(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, FPhysicsCharacterContactSettings& ioSettings) override;
		virtual void OnContactSolve(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, const Vector3& inContactVelocity, const Vector3& inCharacterVelocity, Vector3& ioNewCharacterVelocity) override;

	public:
		FLOAT CharacterHeightStanding = 135.f;
		FLOAT CharacterRadiusStanding = 30.f;
		FLOAT CharacterHeightCrouching = 80.f;
		FLOAT CharacterRadiusCrouching = 30.f;
		BOOL32 ControlMovementDuringJump = TRUE;
		FLOAT CharacterSpeed = 6.0f;
		FLOAT CharacterRunSpeed = 12.0f;
		FLOAT JumpSpeed = 4.0f;

	protected:
		class PMovementComponent* MovementComponent = nullptr;
		class FCharacterVirtual* Character = nullptr;
		class FShape* StandingShape = nullptr;
		class FShape* CrouchingShape = nullptr;
        TFunction<void(const Vector3&)> OnWorldUpVectorChangedHandler;
	};
}
