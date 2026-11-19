#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Component/ActorComponent.h>

namespace PigeonEngine
{
	/*
	* Demo-only motion: swings the owner actor around BaseLocation along Axis. It exists so
	* the demo level's obstacle animation is data (a component with reflected properties)
	* instead of a hardcoded PLevelActor tick. Game content should use a gameplay component,
	* not this one.
	*/
	class PDemoObstacleMotionComponent : public PActorComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PDemoObstacleMotionComponent)
	public:
		virtual void Tick(FLOAT InDeltaTime) override;

		PE_NODISCARD const Vector3& GetBaseLocation() const { return BaseLocation; }
		void SetBaseLocation(const Vector3& InLocation) { BaseLocation = InLocation; }
		PE_NODISCARD FLOAT GetAmplitude() const { return Amplitude; }
		void SetAmplitude(const FLOAT InAmplitude) { Amplitude = InAmplitude; }
		PE_NODISCARD FLOAT GetSpeed() const { return Speed; }
		void SetSpeed(const FLOAT InSpeed) { Speed = InSpeed; }
		PE_NODISCARD const Vector3& GetAxis() const { return Axis; }
		void SetAxis(const Vector3& InAxis) { Axis = InAxis; }

	private:
		Vector3 BaseLocation = Vector3::Zero();
		FLOAT Amplitude = 90.0f;
		FLOAT Speed = 0.7f;
		Vector3 Axis = Vector3(0.0f, 0.0f, 1.0f);
		FLOAT ElapsedSeconds = 0.0f;
	};
}
