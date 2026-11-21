#include "NavAgentComponent.h"

#include <Development/EditorLogManager.h>
#include <Navigation/NavigationManager.h>
#include <Navigation/NavigationProviderInterface.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <Reflection/Property.h>

#if ENGINE_EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

namespace PigeonEngine
{

	static void SerializeNavigationAgentSettings(EArchive& InArchive, const CHAR* InName, ENavigationAgentSettings& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeFloat(PE_TEXT("Radius"), InOutValue.Radius);
		InArchive.SerializeFloat(PE_TEXT("Height"), InOutValue.Height);
		InArchive.SerializeFloat(PE_TEXT("MoveSpeed"), InOutValue.MoveSpeed);
		InArchive.SerializeFloat(PE_TEXT("TurnSpeedDegreesPerSecond"), InOutValue.TurnSpeedDegreesPerSecond);
		InArchive.SerializeFloat(PE_TEXT("AcceptanceRadius"), InOutValue.AcceptanceRadius);
		InArchive.SerializeFloat(PE_TEXT("RepathInterval"), InOutValue.RepathInterval);
		InArchive.SerializeFloat(PE_TEXT("MaxAcceleration"), InOutValue.MaxAcceleration);
		InArchive.SerializeFloat(PE_TEXT("CollisionQueryRange"), InOutValue.CollisionQueryRange);
		InArchive.SerializeFloat(PE_TEXT("PathOptimizationRange"), InOutValue.PathOptimizationRange);
		InArchive.SerializeFloat(PE_TEXT("SeparationWeight"), InOutValue.SeparationWeight);
		UINT32 AvoidanceType = static_cast<UINT32>(InOutValue.ObstacleAvoidanceType);
		InArchive.SerializeUInt32(PE_TEXT("ObstacleAvoidanceType"), AvoidanceType);
		InOutValue.ObstacleAvoidanceType = static_cast<UINT8>(AvoidanceType);
		InArchive.SerializeBool(PE_TEXT("TraverseWater"), InOutValue.bTraverseWater);
		InArchive.SerializeBool(PE_TEXT("TraverseDoor"), InOutValue.bTraverseDoor);
		InArchive.SerializeBool(PE_TEXT("TraverseJump"), InOutValue.bTraverseJump);
		InArchive.SerializeFloat(PE_TEXT("GroundCost"), InOutValue.GroundCost);
		InArchive.SerializeFloat(PE_TEXT("WaterCost"), InOutValue.WaterCost);
		InArchive.SerializeFloat(PE_TEXT("DoorCost"), InOutValue.DoorCost);
		InArchive.SerializeFloat(PE_TEXT("JumpCost"), InOutValue.JumpCost);
		InArchive.EndObject();
	}

	static void RegisterNavigationAgentSettingsStruct()
	{
		const size_t StructHash = GetClassHashCode<ENavigationAgentSettings>();
		const EString StructName(PE_TEXT("ENavigationAgentSettings"));
		const UINT32 EditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE);
		AddStructProperty(StructHash, StructName, PE_TEXT("Radius"), &ENavigationAgentSettings::Radius, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("Height"), &ENavigationAgentSettings::Height, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("MoveSpeed"), &ENavigationAgentSettings::MoveSpeed, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("TurnSpeedDegreesPerSecond"), &ENavigationAgentSettings::TurnSpeedDegreesPerSecond, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("AcceptanceRadius"), &ENavigationAgentSettings::AcceptanceRadius, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("RepathInterval"), &ENavigationAgentSettings::RepathInterval, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("MaxAcceleration"), &ENavigationAgentSettings::MaxAcceleration, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("CollisionQueryRange"), &ENavigationAgentSettings::CollisionQueryRange, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("PathOptimizationRange"), &ENavigationAgentSettings::PathOptimizationRange, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("SeparationWeight"), &ENavigationAgentSettings::SeparationWeight, EditFlags);
		// The serialized avoidance preset is a UINT8 index, not a C++ enum. Expose it as an
		// unsigned field so the generic editor can still present and change it.
		AddStructProperty<ENavigationAgentSettings, UINT32>(StructHash, StructName, PE_TEXT("ObstacleAvoidanceType"),
			[](const ENavigationAgentSettings* Value) -> UINT32 { return static_cast<UINT32>(Value->ObstacleAvoidanceType); },
			[](ENavigationAgentSettings* Value, const UINT32& InValue) -> void { Value->ObstacleAvoidanceType = static_cast<UINT8>(InValue); },
			EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("TraverseWater"), &ENavigationAgentSettings::bTraverseWater, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("TraverseDoor"), &ENavigationAgentSettings::bTraverseDoor, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("TraverseJump"), &ENavigationAgentSettings::bTraverseJump, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("GroundCost"), &ENavigationAgentSettings::GroundCost, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("WaterCost"), &ENavigationAgentSettings::WaterCost, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("DoorCost"), &ENavigationAgentSettings::DoorCost, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("JumpCost"), &ENavigationAgentSettings::JumpCost, EditFlags);
	}

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PNavAgentComponent>();
		RegisterNavigationAgentSettingsStruct();
		AddClassPropertyStructCustom<PNavAgentComponent, ENavigationAgentSettings>(ClassHash, PE_TEXT("Settings"),
			[](const PNavAgentComponent* Component) -> ENavigationAgentSettings { return Component->GetSettings(); },
			[](PNavAgentComponent* Component, const ENavigationAgentSettings& Value) -> void { Component->SetSettings(Value); },
			SerializeNavigationAgentSettings,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassBoolPropertyAccessors<PNavAgentComponent>(ClassHash, PE_TEXT("UseCrowd"),
			[](const PNavAgentComponent* Component) -> BOOL32 { return Component->GetUseCrowd(); },
			[](PNavAgentComponent* Component, const BOOL32& Value) -> void { Component->SetUseCrowd(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PNavAgentComponent, Vector3>(ClassHash, PE_TEXT("TargetLocation"),
			[](const PNavAgentComponent* Component) -> Vector3 { return Component->GetTargetLocation(); },
			[](PNavAgentComponent* Component, const Vector3& Value) -> void { Component->SetTargetLocation(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassBoolPropertyAccessors<PNavAgentComponent>(ClassHash, PE_TEXT("MoveOnBeginPlay"),
			[](const PNavAgentComponent* Component) -> BOOL32 { return Component->GetMoveOnBeginPlay(); },
			[](PNavAgentComponent* Component, const BOOL32& Value) -> void { Component->SetMoveOnBeginPlay(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PNavAgentComponent, PActorComponent>();
		RegisterClassProperties();
	}

	// Horizontal speed below which the actor keeps its facing. The velocity of a crowd agent that is
	// settling on its target, or being nudged by a neighbour, is small and its direction flips from
	// frame to frame; turning towards it would spin the actor in place instead of letting it stand.
	PE_CONSTEXPR_STATIC FLOAT NavigationAgentMinTurnSpeedSquared = 25.0f;

	// The query filter this agent's settings describe: the area classes it may walk on, priced as
	// configured. The cost array follows the ENavigationAreaFlagType bit order, which is the order
	// the backend numbers its area classes in.
	static ENavigationQueryFilter MakeAgentQueryFilter(const ENavigationAgentSettings& InSettings)
	{
		ENavigationQueryFilter Filter;
		UINT16 IncludeFlags = static_cast<UINT16>(ENavigationAreaFlagType::NAVIGATION_AREA_GROUND);
		if (InSettings.bTraverseWater)
		{
			IncludeFlags = static_cast<UINT16>(IncludeFlags | ENavigationAreaFlagType::NAVIGATION_AREA_WATER);
		}
		if (InSettings.bTraverseDoor)
		{
			IncludeFlags = static_cast<UINT16>(IncludeFlags | ENavigationAreaFlagType::NAVIGATION_AREA_DOOR);
		}
		if (InSettings.bTraverseJump)
		{
			IncludeFlags = static_cast<UINT16>(IncludeFlags | ENavigationAreaFlagType::NAVIGATION_AREA_JUMP);
		}
		Filter.IncludeAreaFlags = IncludeFlags;

		Filter.AreaCosts.SetNum(4u);
		Filter.AreaCosts[0u] = InSettings.GroundCost;
		Filter.AreaCosts[1u] = InSettings.WaterCost;
		Filter.AreaCosts[2u] = InSettings.DoorCost;
		Filter.AreaCosts[3u] = InSettings.JumpCost;
		return Filter;
	}

	// Turns the actor towards InDirection at the configured rate. Yaw only: a navigation agent that
	// inherits pitch from a step looks broken, and the movement itself already follows the surface.
	static void FaceDirection(PActor& InActor, const Vector3& InDirection, const FLOAT InTurnSpeedDegreesPerSecond, const FLOAT InDeltaTime)
	{
		const FLOAT HorizontalSpeedSquared = (InDirection.x * InDirection.x) + (InDirection.z * InDirection.z);
		if (HorizontalSpeedSquared <= NavigationAgentMinTurnSpeedSquared)
		{
			return;
		}
		const FLOAT TargetYaw = EMath::RadiansToDegrees(EMath::Atan2(InDirection.x, InDirection.z));
		const Euler CurrentEuler = MakeEuler(InActor.GetActorRotation());
		const FLOAT MaxTurn = InTurnSpeedDegreesPerSecond * InDeltaTime;
		const FLOAT YawDifference = Euler::NormalizeAxis(TargetYaw - CurrentEuler.Yaw);
		Euler NewEuler = CurrentEuler;
		NewEuler.Yaw = CurrentEuler.Yaw + EMath::Clamp(YawDifference, -MaxTurn, MaxTurn);
		InActor.SetActorRotation(MakeQuaternion(NewEuler));
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavAgentComponent::PNavAgentComponent()
	{
		POBJ_DEBUGNAME_SET(this, "NavAgentComponent");
		SetIsTickable(TRUE);
	}

	PNavAgentComponent::~PNavAgentComponent()
	{
	}

	const ENavigationAgentSettings& PNavAgentComponent::GetSettings() const
	{
		return Settings;
	}

	BOOL32 PNavAgentComponent::GetUseCrowd() const
	{
		return bUseCrowd;
	}

	void PNavAgentComponent::SetUseCrowd(const BOOL32& InUseCrowd)
	{
		if (bUseCrowd == InUseCrowd)
		{
			return;
		}
		// Switching modes drops whatever the previous mode had in flight.
		if (bUseCrowd)
		{
			ENavigationManager::GetManagerSingleton()->UnregisterCrowdAgent(GetUniqueID());
		}
		bUseCrowd = InUseCrowd;
		bCrowdMoveRequested = FALSE;
		bHasPath = FALSE;
		bHasMoveRequest = FALSE;
		CurrentPath.Reset();
	}

	void PNavAgentComponent::SetSettings(const ENavigationAgentSettings& InSettings)
	{
		Settings = InSettings;
	}

	BOOL32 PNavAgentComponent::MoveToLocation(const Vector3& InTarget)
	{
		MoveTarget = InTarget;
		bHasMoveRequest = TRUE;
		return TRUE;
	}

	void PNavAgentComponent::StopMovement()
	{
		if (bUseCrowd)
		{
			ENavigationManager::GetManagerSingleton()->StopCrowdAgent(GetUniqueID());
			bCrowdMoveRequested = FALSE;
		}
		bHasMoveRequest = FALSE;
		bHasPath = FALSE;
		NextCornerIndex = 0u;
		CurrentPath.Reset();
	}

	BOOL32 PNavAgentComponent::IsMoving() const
	{
		return (bHasPath || bHasMoveRequest);
	}

	const ENavigationPath& PNavAgentComponent::GetCurrentPath() const
	{
		return CurrentPath;
	}

	ENavigationPathStatusType PNavAgentComponent::GetLastMoveStatus() const
	{
		return LastMoveStatus;
	}

	Vector3 PNavAgentComponent::GetAgentLocation() const
	{
		const PActor* OwnerActor = GetOwnerActor();
		return OwnerActor ? OwnerActor->GetActorWorldLocation() : Vector3::Zero();
	}

	const Vector3& PNavAgentComponent::GetTargetLocation() const
	{
		return TargetLocation;
	}

	void PNavAgentComponent::SetTargetLocation(const Vector3& InTarget)
	{
		TargetLocation = InTarget;
	}

	BOOL32 PNavAgentComponent::GetMoveOnBeginPlay() const
	{
		return bMoveOnBeginPlay;
	}

	void PNavAgentComponent::SetMoveOnBeginPlay(const BOOL32& bInMoveOnBeginPlay)
	{
		bMoveOnBeginPlay = bInMoveOnBeginPlay;
	}

	void PNavAgentComponent::Tick(FLOAT InDeltaTime)
	{
		PActorComponent::Tick(InDeltaTime);

		if (bMoveOnBeginPlay && (!bAutoMoveIssued))
		{
			MoveToLocation(TargetLocation);
			bAutoMoveIssued = TRUE;
		}

		if (bUseCrowd)
		{
			TickCrowd(InDeltaTime);
			return;
		}

		SecondsSincePathRequest += InDeltaTime;

		if (bHasMoveRequest)
		{
			// A target that has not moved does not need a new query, and one that moved less
			// than the acceptance radius cannot change the answer in a useful way.
			const BOOL32 bTargetMoved = ((!bHasPath) || (Vector3::Distance(MoveTarget, RequestedTarget) > Settings.AcceptanceRadius)) ? TRUE : FALSE;
			if ((bTargetMoved) && (SecondsSincePathRequest >= Settings.RepathInterval))
			{
				SecondsSincePathRequest = 0.0f;
				RequestPath(MoveTarget);
			}
		}

		if (bHasPath)
		{
			FollowPath(InDeltaTime);
		}
	}

	void PNavAgentComponent::TickCrowd(FLOAT InDeltaTime)
	{
		PActor* OwnerActor = GetOwnerActor();
		if (!OwnerActor)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the agent component has no owner actor, so it cannot move.")));
			FinishMove(ENavigationPathStatusType::NAVIGATION_PATH_INVALID);
			return;
		}

		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		// Idempotent: it recreates the agent after its volume was rebuilt, which is what keeps a
		// crowd agent walking when the navmesh is rebuilt underneath it.
		if (!Manager->RegisterCrowdAgent(GetUniqueID(), OwnerActor->GetActorWorldLocation(), Settings))
		{
			return;
		}

		if (bHasMoveRequest)
		{
			bHasMoveRequest = FALSE;
			if (!Manager->RequestCrowdAgentMove(GetUniqueID(), MoveTarget))
			{
				FinishMove(ENavigationPathStatusType::NAVIGATION_PATH_INVALID);
				return;
			}
			bCrowdMoveRequested = TRUE;
		}

		ENavigationCrowdAgentState AgentState;
		if (!Manager->GetCrowdAgentState(GetUniqueID(), AgentState))
		{
			return;
		}

		OwnerActor->SetActorLocation(AgentState.Position);
		FaceDirection(*OwnerActor, AgentState.Velocity, Settings.TurnSpeedDegreesPerSecond, InDeltaTime);

		if ((bCrowdMoveRequested) && (AgentState.bTargetReached))
		{
			// Detour does not clear a reached target by itself: it only slows the agent down as the
			// remaining distance shrinks, so an agent left with its target keeps circling the goal
			// polygon. Resetting it collapses the desired velocity to zero and the agent stands.
			Manager->StopCrowdAgent(GetUniqueID());
			bCrowdMoveRequested = FALSE;
			FinishMove((AgentState.bPartial) ? ENavigationPathStatusType::NAVIGATION_PATH_PARTIAL : ENavigationPathStatusType::NAVIGATION_PATH_COMPLETE);
		}
	}

	void PNavAgentComponent::RequestPath(const Vector3& InTarget)
	{
		PActor* OwnerActor = GetOwnerActor();
		if (!OwnerActor)
		{
			// Only scene components are attached to their actor by PActor::AddComponent, so a
			// plain actor component reports a missing owner instead of silently never moving.
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the agent component has no owner actor, so it cannot move.")));
			FinishMove(ENavigationPathStatusType::NAVIGATION_PATH_INVALID);
			return;
		}

		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		if (!Manager->GetDefaultProvider())
		{
			// The level has no navmesh yet: the bake may still be waiting for its assets.
			// Keeping the request pending is what lets an agent spawned in the same frame as
			// the bake start moving on its own once the mesh appears.
			return;
		}

		ENavigationPath Path;
		const ENavigationQueryFilter Filter = MakeAgentQueryFilter(Settings);
		if ((!Manager->FindPath(OwnerActor->GetActorWorldLocation(), InTarget, Filter, Path)) || (!Path.IsValid()))
		{
			bHasMoveRequest = FALSE;
			FinishMove(ENavigationPathStatusType::NAVIGATION_PATH_INVALID);
			return;
		}

		bHasMoveRequest = FALSE;
		RequestedTarget = InTarget;

		if (Path.Num() <= 1u)
		{
			// Both ends project onto the same point, so there is nothing to walk.
			FinishMove(Path.Status);
			return;
		}

		// Starting from rest, the agent is placed on the projection of its own position. That is
		// what puts it on the surface the first time, without a visible snap when the path is
		// refreshed in the middle of a move.
		const BOOL32 bWasMoving = bHasPath;
		if (!bWasMoving)
		{
			OwnerActor->SetActorLocation(Path.Points[0]);
		}

		CurrentPath = Path;
		// The first point is the projection of the agent's own position; walking to it would
		// make the agent step backwards before setting off.
		NextCornerIndex = 1u;
		bHasPath = TRUE;
	}

	void PNavAgentComponent::FollowPath(FLOAT InDeltaTime)
	{
		PActor* OwnerActor = GetOwnerActor();
		if (!OwnerActor)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the agent component lost its owner actor.")));
			FinishMove(ENavigationPathStatusType::NAVIGATION_PATH_INVALID);
			return;
		}

		FLOAT RemainingStep = Settings.MoveSpeed * InDeltaTime;
		Vector3 CurrentLocation = OwnerActor->GetActorWorldLocation();
		Vector3 MoveDirection = Vector3::Zero();

		while ((RemainingStep > 0.0f) && (NextCornerIndex < CurrentPath.Num()))
		{
			const Vector3 Corner = CurrentPath.Points[NextCornerIndex];
			const Vector3 ToCorner = Corner - CurrentLocation;
			const FLOAT CornerDistance = ToCorner.Length();
			if (CornerDistance <= Settings.AcceptanceRadius)
			{
				// Close enough: the corner counts as reached without a visible snap.
				NextCornerIndex++;
				continue;
			}

			// The straight path already lies on the mesh, so walking in three dimensions keeps
			// the agent on a ramp and on a step instead of sliding through it.
			const Vector3 StepDirection = ToCorner * (1.0f / CornerDistance);
			const FLOAT StepLength = EMath::Min(RemainingStep, CornerDistance);
			CurrentLocation = CurrentLocation + (StepDirection * StepLength);
			MoveDirection = StepDirection;
			RemainingStep -= StepLength;
			if (StepLength >= CornerDistance)
			{
				NextCornerIndex++;
			}
		}

		// World-space write on the owner's root component. The component-chain world
		// transform makes this correct for an actor at any depth, not only one parented
		// directly to the world root: the old getter and setter disagreed about the parent
		// frame for a nested actor, which is why the actor-level local write was used.
		OwnerActor->GetRootComponent()->SetComponentWorldLocation(CurrentLocation);

		FaceDirection(*OwnerActor, MoveDirection, Settings.TurnSpeedDegreesPerSecond, InDeltaTime);

		if (NextCornerIndex >= CurrentPath.Num())
		{
			FinishMove(CurrentPath.Status);
		}
	}

	void PNavAgentComponent::RemovedFromScene()
	{
		ENavigationManager::GetManagerSingleton()->UnregisterCrowdAgent(GetUniqueID());
		PActorComponent::RemovedFromScene();
	}

	void PNavAgentComponent::FinishMove(ENavigationPathStatusType InStatus)
	{
		bHasPath = FALSE;
		bHasMoveRequest = FALSE;
		bCrowdMoveRequested = FALSE;
		NextCornerIndex = 0u;
		CurrentPath.Reset();
		LastMoveStatus = InStatus;

		// The outcome goes to whoever asked for the move. The callback may start a new move,
		// which is safe: everything it would overwrite has already been cleared above.
		if (OnMoveFinished.IsValid())
		{
			OnMoveFinished(InStatus);
		}
	}

#if ENGINE_EDITOR_ONLY
	void PNavAgentComponent::EditorTick(FLOAT InDeltaTime)
	{
		PActorComponent::EditorTick(InDeltaTime);

		if ((!bHasPath) || (!ENavigationManager::GetManagerSingleton()->IsDebugDrawEnabled()))
		{
			return;
		}

		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		const Vector3 AgentLocation = GetAgentLocation();
		for (UINT32 Index = NextCornerIndex; Index < CurrentPath.Num(); Index++)
		{
			const Vector3 From = (Index == NextCornerIndex) ? AgentLocation : CurrentPath.Points[Index - 1u];
			WireframeManager->DrawSingleLine(From, CurrentPath.Points[Index], Color4::Green());
		}
	}
#endif

}  // namespace PigeonEngine
