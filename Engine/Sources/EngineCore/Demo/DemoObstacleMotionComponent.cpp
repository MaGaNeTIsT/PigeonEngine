#include "DemoObstacleMotionComponent.h"

#include <PigeonBase/Object/Actor.h>
#include <Reflection/Property.h>

namespace PigeonEngine
{
	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PDemoObstacleMotionComponent>();
		AddClassPropertyAccessors<PDemoObstacleMotionComponent, Vector3>(ClassHash, PE_TEXT("BaseLocation"),
			[](const PDemoObstacleMotionComponent* Component) -> Vector3 { return Component->GetBaseLocation(); },
			[](PDemoObstacleMotionComponent* Component, const Vector3& Value) -> void { Component->SetBaseLocation(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PDemoObstacleMotionComponent, FLOAT>(ClassHash, PE_TEXT("Amplitude"),
			[](const PDemoObstacleMotionComponent* Component) -> FLOAT { return Component->GetAmplitude(); },
			[](PDemoObstacleMotionComponent* Component, const FLOAT& Value) -> void { Component->SetAmplitude(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PDemoObstacleMotionComponent, FLOAT>(ClassHash, PE_TEXT("Speed"),
			[](const PDemoObstacleMotionComponent* Component) -> FLOAT { return Component->GetSpeed(); },
			[](PDemoObstacleMotionComponent* Component, const FLOAT& Value) -> void { Component->SetSpeed(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PDemoObstacleMotionComponent, Vector3>(ClassHash, PE_TEXT("Axis"),
			[](const PDemoObstacleMotionComponent* Component) -> Vector3 { return Component->GetAxis(); },
			[](PDemoObstacleMotionComponent* Component, const Vector3& Value) -> void { Component->SetAxis(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PDemoObstacleMotionComponent, PActorComponent>();
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PDemoObstacleMotionComponent::PDemoObstacleMotionComponent()
	{
		POBJ_DEBUGNAME_SET(this, "DemoObstacleMotionComponent");
		SetIsTickable(TRUE);
	}

	PDemoObstacleMotionComponent::~PDemoObstacleMotionComponent()
	{
	}

	void PDemoObstacleMotionComponent::Tick(FLOAT InDeltaTime)
	{
		PActorComponent::Tick(InDeltaTime);
		PActor* OwnerActor = GetOwnerActor();
		if (!OwnerActor)
		{
			return;
		}
		ElapsedSeconds += InDeltaTime;
		OwnerActor->SetActorLocation(BaseLocation + Axis * (Amplitude * EMath::Sin(ElapsedSeconds * Speed)));
	}
}
