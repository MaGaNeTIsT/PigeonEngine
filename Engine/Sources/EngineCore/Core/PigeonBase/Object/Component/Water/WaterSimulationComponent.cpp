#include "WaterSimulationComponent.h"

#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PWaterSimulationComponent, PPrimitiveComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PWaterSimulationComponent::PWaterSimulationComponent()
	{
		POBJ_DEBUGNAME_SET(this, "WaterSimulationComponent");
	}

	PWaterSimulationComponent::~PWaterSimulationComponent()
	{
	}

	void PWaterSimulationComponent::SetResolution(const UINT32 InResolutionX, const UINT32 InResolutionZ)
	{
		ResolutionX = EMath::Max(1u, InResolutionX);
		ResolutionZ = EMath::Max(1u, InResolutionZ);
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::SetAreaSize(const FLOAT InAreaSizeX, const FLOAT InAreaSizeZ)
	{
		AreaSizeX = EMath::Max(InAreaSizeX, 0.0001f);
		AreaSizeZ = EMath::Max(InAreaSizeZ, 0.0001f);
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::SetInitialDepth(const FLOAT InInitialDepth)
	{
		InitialDepth = EMath::Max(InInitialDepth, 0.f);
		bResetRequested = TRUE;
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::SetGroundFlat()
	{
		GroundType = EWaterGroundType::WATER_GROUND_FLAT;
		GroundRevision++;
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::SetGroundHeightMap(const TArray<FLOAT>& InRelativeDepths)
	{
		TSharedPtr<TArray<FLOAT>> NewGround = EMemory::MakeShared<TArray<FLOAT>>();
		if (NewGround)
		{
			*(NewGround.get()) = InRelativeDepths;
		}
		GroundRelativeDepths = NewGround;
		GroundType = EWaterGroundType::WATER_GROUND_HEIGHT_MAP;
		GroundRevision++;
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::SetSimulationParameters(const FWaterSimulationParameters& InParameters)
	{
		SimulationParameters = InParameters;
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::SetWaveEnabled(const BOOL32 InIsWaveEnabled)
	{
		bEnableWave = InIsWaveEnabled;
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::SetTimeStep(const FLOAT InFixedTimeStep, const UINT32 InMaxSubsteps, const FLOAT InTimeScale)
	{
		FixedTimeStep = EMath::Max(InFixedTimeStep, 0.001f);
		MaxSubsteps = EMath::Max(1u, InMaxSubsteps);
		TimeScale = EMath::Max(InTimeScale, 0.f);
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::AddImpulse(const FWaterImpulseDescription& InImpulse)
	{
		FWaterInteractionEvent Event;
		Event.Type			= EWaterInteractionType::WATER_INTERACTION_IMPULSE;
		Event.WorldLocation	= InImpulse.WorldLocation;
		Event.Size			= Vector2(InImpulse.Radius * 2.f, InImpulse.Radius * 2.f);
		Event.Velocity		= InImpulse.Velocity;
		Event.Volume		= InImpulse.Volume;
		Event.Mode			= InImpulse.Mode;
		Event.Intensity		= InImpulse.Intensity;
		Event.Hardness		= InImpulse.Hardness;
		Event.YOffset		= InImpulse.YOffset;
		// The modifier's Radius is the footprint radius as a fraction of its half extent;
		// the world radius already lives in Size, so the footprint covers the whole extent.
		Event.Radius		= 1.f;
		Event.ForwardAngle	= InImpulse.ForwardAngle;
		PendingInteractions.Add(Event);
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::AddWhirlpool(const FWaterWhirlpoolDescription& InWhirlpool)
	{
		FWaterInteractionEvent Event;
		Event.Type			= EWaterInteractionType::WATER_INTERACTION_WHIRLPOOL;
		Event.WorldLocation	= InWhirlpool.WorldLocation;
		Event.Radius		= InWhirlpool.Radius;
		Event.Radial		= InWhirlpool.Radial;
		Event.Suction		= InWhirlpool.Suction;
		Event.Translucency	= InWhirlpool.Translucency;
		Event.Duration		= InWhirlpool.Duration;
		PendingInteractions.Add(Event);
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSimulationComponent::ResetSimulation()
	{
		bResetRequested = TRUE;
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	const EName& PWaterSimulationComponent::GetRenderProxyTypeName() const
	{
		// Has to match the name RWaterSimulationSceneProxy registers itself with.
		static const EName ProxyTypeName(PE_TEXT("WaterSimulationPrimitive"));
		return ProxyTypeName;
	}

	BOOL32 PWaterSimulationComponent::BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const
	{
		if (!PPrimitiveComponent::BuildRenderSnapshot(OutSnapshot))
		{
			return FALSE;
		}

		const Vector3 WorldLocation = GetComponentWorldLocation();
		FWaterSimulationDescription& Description = OutSnapshot.WaterSimulation;
		Description.ResolutionX			= ResolutionX;
		Description.ResolutionZ			= ResolutionZ;
		Description.AreaLocationX		= WorldLocation.x - (AreaSizeX * 0.5f);
		Description.AreaLocationZ		= WorldLocation.z - (AreaSizeZ * 0.5f);
		Description.AreaSizeX			= AreaSizeX;
		Description.AreaSizeZ			= AreaSizeZ;
		Description.ReferenceY			= WorldLocation.y;
		Description.InitialDepth		= InitialDepth;
		Description.GroundType			= GroundType;
		Description.GroundRelativeDepths = GroundRelativeDepths;
		Description.GroundRevision		= GroundRevision;
		Description.bEnableWave			= bEnableWave;
		Description.bResetSimulation	= bResetRequested;
		Description.FixedTimeStep		= FixedTimeStep;
		Description.MaxSubsteps			= MaxSubsteps;
		Description.TimeScale			= TimeScale;
		Description.Parameters			= SimulationParameters;
		Description.Interactions		= PendingInteractions;
		OutSnapshot.bHasWaterSimulation	= TRUE;

		// The snapshot carries this update's interactions and reset request; a later update
		// must not replay them.
		PendingInteractions.Empty();
		bResetRequested = FALSE;
		return TRUE;
	}

	void PWaterSimulationComponent::CreateRenderState()
	{
		PPrimitiveComponent::CreateRenderState();
		if (ShouldRender())
		{
			GetWorld()->GetRenderScene()->AddPrimitive(this);
		}
	}

	void PWaterSimulationComponent::DestroyRenderState()
	{
		if (GetWorld())
		{
			GetWorld()->GetRenderScene()->RemovePrimitive(this);
		}
		PPrimitiveComponent::DestroyRenderState();
	}

	void PWaterSimulationComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			GetWorld()->GetRenderScene()->UpdatePrimitive(this);
		}
		PPrimitiveComponent::SendUpdateRenderState();
	}

}	// namespace PigeonEngine
