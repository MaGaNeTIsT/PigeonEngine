#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	/*
	* How a water simulation obtains its ground height.
	*
	* Flat: the ground is the reference plane and GroundRelativeDepths is empty.
	* HeightMap: GroundRelativeDepths holds ResolutionX * ResolutionZ values in row major
	* plane order (world X fastest, world Z next). Each value is
	* ReferenceY - groundWorldY, so a ground below the reference plane is positive.
	*/
	enum EWaterGroundType : UINT8
	{
		WATER_GROUND_FLAT		= 0,
		WATER_GROUND_HEIGHT_MAP	= 1
	};

	enum EWaterInteractionType : UINT8
	{
		WATER_INTERACTION_IMPULSE	= 0,
		WATER_INTERACTION_WHIRLPOOL	= 1
	};

	/*
	* One interaction injected into the simulation.
	*
	* WorldLocation is a world position. Only its X and Z enter the simulation plane;
	* Y is the height the modifier operates at. Velocity is a plane velocity in world
	* X and Z: the simulation state stores it in .xy and the volume/depth in .z, with
	* the height axis being world Y.
	*/
	struct FWaterInteractionEvent
	{
	public:
		EWaterInteractionType	Type			= EWaterInteractionType::WATER_INTERACTION_IMPULSE;
		Vector3					WorldLocation	= Vector3(0.f, 0.f, 0.f);
		Vector2					Size			= Vector2(1.f, 1.f);
		Vector2					Velocity		= Vector2(0.f, 0.f);
		FLOAT					Volume			= 0.f;
		FLOAT					Mode			= 0.f;
		FLOAT					Intensity		= 1.f;
		FLOAT					Hardness		= 1.f;
		FLOAT					YOffset			= 0.f;
		FLOAT					Radius			= 0.5f;
		FLOAT					Radial			= 0.f;
		FLOAT					Suction			= 0.f;
		FLOAT					Translucency	= 1.f;
		FLOAT					Duration		= 0.f;
		FLOAT					ForwardAngle	= 0.f;
	};

	/*
	* The component facing impulse: a world position and a world radius. The component
	* converts it to the modifier's plane footprint, which is why a caller does not have to
	* know the modifier's normalized shape parameters.
	*/
	struct FWaterImpulseDescription
	{
	public:
		Vector3	WorldLocation	= Vector3(0.f, 0.f, 0.f);
		FLOAT	Radius			= 100.f;
		FLOAT	Volume			= 0.f;
		FLOAT	YOffset			= 0.f;
		Vector2	Velocity		= Vector2(0.f, 0.f);
		FLOAT	Mode			= 0.f;
		FLOAT	Intensity		= 1.f;
		FLOAT	Hardness		= 1.f;
		FLOAT	ForwardAngle	= 0.f;
	};

	/*
	* The component facing whirlpool: a world position, a world radius and a lifetime.
	* The proxy owns the remaining lifetime once it receives the creation event.
	*/
	struct FWaterWhirlpoolDescription
	{
	public:
		Vector3	WorldLocation	= Vector3(0.f, 0.f, 0.f);
		FLOAT	Radius			= 100.f;
		FLOAT	Radial			= 0.f;
		FLOAT	Suction			= 0.f;
		FLOAT	Translucency	= 1.f;
		FLOAT	Duration		= 1.f;
	};

	/*
	* The complete SWE parameter block, after the coordinate rename to the engine's
	* left handed X right / Z forward / Y up world.
	*
	* The runtime fields at the top are overwritten by RWaterSimulation from the
	* FWaterSimulationDescription and the current step. A component authors only the
	* tunables below them.
	*
	* Height values (CaptureY, GroundHeight, SurfaceHeight and the outline fields) are
	* world Y. The simulation plane is world XZ: the state .xy is (velocityX, velocityZ)
	* and .z is the water volume/depth along Y.
	*/
	struct FWaterSimulationParameters
	{
	public:
		// Runtime fields. RWaterSimulation overwrites these every frame.
		FLOAT		ResolutionX				= 0.f;
		FLOAT		ResolutionZ				= 0.f;
		FLOAT		InverseResolutionX		= 0.f;
		FLOAT		InverseResolutionZ		= 0.f;
		FLOAT		AreaLocationX			= 0.f;
		FLOAT		AreaLocationZ			= 0.f;
		FLOAT		AreaSizeX				= 1.f;
		FLOAT		AreaSizeZ				= 1.f;
		FLOAT		SimulationDeltaTime		= 0.f;
		FLOAT		CaptureY				= 0.f;
		FLOAT		InitialDepth			= 0.f;
		Vector2Int	PixelAnchor				= Vector2Int(0, 0);
		Vector2Int	PixelSize				= Vector2Int(0, 0);
	public:
		// Foam and transport, shared by the height pass.
		FLOAT		FoamFadeSpeed			= 0.f;
		FLOAT		FoamShallowDepth		= 0.f;
		FLOAT		FoamShallowGenerate		= 0.f;
		FLOAT		FoamWaveGenerate		= 0.f;
		FLOAT		FluidTransportMax		= 1.f;
		FLOAT		FoamAdvection			= 1.f;
		FLOAT		EvaporationDepth		= 0.f;
		FLOAT		FluxEnergy				= 1.f;
	public:
		// Velocity integration.
		FLOAT		Gravity					= 981.f;
		FLOAT		VelocityClamp			= 1000.f;
		FLOAT		ScaleY					= 1.f;
		FLOAT		Friction				= 0.5f;
		FLOAT		Damping					= 0.1f;
		FLOAT		AccelerationClamp		= 1000.f;
		FLOAT		OvershootingBlend		= 0.5f;
		FLOAT		OvershootingLimit		= 0.1f;
		FLOAT		MinWater				= 0.0001f;
	public:
		// Modifier.
		FLOAT		SourceVelocityX			= 0.f;
		FLOAT		SourceVelocityZ			= 0.f;
		FLOAT		SourceVolume			= 0.f;
		FLOAT		SourceMode				= 0.f;
		Vector2		ModifierLocation		= Vector2(0.f, 0.f);
		Vector2		ModifierSize			= Vector2(1.f, 1.f);
		Vector3		ModifierForward			= Vector3(1.f, 0.f, 0.f);
		FLOAT		YOffset					= 0.f;
		FLOAT		Hardness				= 1.f;
		FLOAT		Intensity				= 1.f;
		FLOAT		Radius					= 0.5f;
	public:
		// Whirlpool.
		FLOAT		ModifierRadius			= 1.f;
		FLOAT		ModifierRadial			= 0.f;
		FLOAT		ModifierSuction			= 0.f;
		FLOAT		WhirlpoolTranslucency	= 1.f;
		FLOAT		WhirlpoolTime			= 0.f;
	public:
		// Gerstner wave.
		FLOAT		WaveVelocityScale		= 1.f;
		FLOAT		WaveCalming				= 0.1f;
		FLOAT		WaveSurfaceHeight		= 1.f;
		FLOAT		WaveBorderXNegative		= 1.f;
		FLOAT		WaveBorderZNegative		= 1.f;
		FLOAT		WaveBorderXPositive		= 1.f;
		FLOAT		WaveBorderZPositive		= 1.f;
		FLOAT		WaveLength				= 10.f;
		FLOAT		WaveSpeed				= 1.f;
		FLOAT		WaveAmplitude			= 0.1f;
		FLOAT		WaveSteepness			= 0.5f;
		FLOAT		WaveActiveTime			= 0.f;
		FLOAT		WaveDirectionAngle		= 0.f;
	public:
		// Height and wetness outline.
		FLOAT		OutlineStateWetHeightOffset	= 0.f;
		FLOAT		OutlineNoWaterBlend			= 0.f;
		FLOAT		OutlineThickness			= 0.1f;
		FLOAT		OutlineSurfaceSmoothing		= 0.5f;
		FLOAT		OutlineFoamThickness		= 0.f;
		FLOAT		OutlineFluidThickness		= 0.f;
		FLOAT		OutlineDryingOffset			= 0.f;
		FLOAT		OutlineDryingSpeed			= 0.f;
		FLOAT		OutlineBorderBlend			= 0.01f;
		FLOAT		OutlineWetnessLimit			= 1.f;
		FLOAT		OutlineMinWater				= 0.007f;
		FLOAT		OutlineFallingSpeed			= 0.7f;
		FLOAT		OutlineMinOutline			= -1300.f;
		FLOAT		OutlineAverage				= 0.f;
	};

	/*
	* What a water simulation component reports to its render proxy.
	*
	* AreaLocationX/Z and AreaSizeX/Z describe the simulation plane in world XZ and
	* ReferenceY is the reference plane's world Y. GroundRelativeDepths is only filled
	* for WATER_GROUND_HEIGHT_MAP; each value is ReferenceY - groundWorldY.
	*
	* Parameters carries the tunables. The proxy overwrites the runtime copies of the
	* resolution, area, reference Y and timestep before it writes the shader block, so
	* the component does not have to keep them in sync.
	*
	* Interactions carries this update's new one shot impulses and whirlpool creations.
	* Continuous whirlpool lifetimes are owned by the proxy once it received them.
	*/
	struct FWaterSimulationDescription
	{
	public:
		UINT32						ResolutionX			= 256u;
		UINT32						ResolutionZ			= 256u;
		FLOAT						AreaLocationX		= -50.f;
		FLOAT						AreaLocationZ		= -50.f;
		FLOAT						AreaSizeX			= 100.f;
		FLOAT						AreaSizeZ			= 100.f;
		FLOAT						ReferenceY			= 0.f;
		FLOAT						InitialDepth		= 1.f;
		EWaterGroundType			GroundType			= EWaterGroundType::WATER_GROUND_FLAT;
		// Shared and treated as immutable once published: the snapshot copies the pointer, so
		// a static height map crosses to the render thread without copying its texels every
		// frame. The component replaces the pointer when the ground changes.
		TSharedPtr<TArray<FLOAT>>	GroundRelativeDepths;
		// The component increments this when the ground array changes. The proxy compares it
		// instead of comparing the whole array, so a static height map is not re-uploaded.
		UINT32						GroundRevision		= 0u;
		BOOL32						bEnableWave			= FALSE;
		BOOL32						bResetSimulation	= FALSE;
		FLOAT						FixedTimeStep		= 1.f / 60.f;
		UINT32						MaxSubsteps			= 3u;
		FLOAT						TimeScale			= 1.f;
		FWaterSimulationParameters	Parameters;
		TArray<FWaterInteractionEvent>	Interactions;
	};

}	// namespace PigeonEngine
