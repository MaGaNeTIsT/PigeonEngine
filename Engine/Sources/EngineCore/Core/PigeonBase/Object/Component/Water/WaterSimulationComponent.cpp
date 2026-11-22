#include "WaterSimulationComponent.h"

#include <Development/EditorLogManager.h>
#include <PigeonBase/Object/World/World.h>
#include <Reflection/Property.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void AddWaterSimulationFloatProperty(const size_t& InClassHash, const CHAR* InName,
		FLOAT FWaterSimulationParameters::* InMember)
	{
		AddClassPropertyAccessors<PWaterSimulationComponent, FLOAT>(InClassHash, InName,
			[InMember](const PWaterSimulationComponent* Component) -> FLOAT
			{
				return (Component->GetSimulationParameters().*InMember);
			},
			[InMember](PWaterSimulationComponent* Component, const FLOAT& Value) -> void
			{
				FWaterSimulationParameters Parameters = Component->GetSimulationParameters();
				Parameters.*InMember = Value;
				Component->SetSimulationParameters(Parameters);
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void SerializeWaterImpulseDescription(EArchive& InArchive, const CHAR* InName, FWaterImpulseDescription& InOutValue)
	{
		InArchive.BeginObject(InName);
		SerializeVector3(InArchive, PE_TEXT("WorldLocation"), InOutValue.WorldLocation);
		InArchive.SerializeFloat(PE_TEXT("Radius"), InOutValue.Radius);
		InArchive.SerializeFloat(PE_TEXT("Volume"), InOutValue.Volume);
		InArchive.SerializeFloat(PE_TEXT("YOffset"), InOutValue.YOffset);
		SerializeVector2(InArchive, PE_TEXT("Velocity"), InOutValue.Velocity);
		InArchive.SerializeFloat(PE_TEXT("Mode"), InOutValue.Mode);
		InArchive.SerializeFloat(PE_TEXT("Intensity"), InOutValue.Intensity);
		InArchive.SerializeFloat(PE_TEXT("Hardness"), InOutValue.Hardness);
		InArchive.SerializeFloat(PE_TEXT("ForwardAngle"), InOutValue.ForwardAngle);
		InArchive.EndObject();
	}

	static void SerializeWaterWhirlpoolDescription(EArchive& InArchive, const CHAR* InName, FWaterWhirlpoolDescription& InOutValue)
	{
		InArchive.BeginObject(InName);
		SerializeVector3(InArchive, PE_TEXT("WorldLocation"), InOutValue.WorldLocation);
		InArchive.SerializeFloat(PE_TEXT("Radius"), InOutValue.Radius);
		InArchive.SerializeFloat(PE_TEXT("Radial"), InOutValue.Radial);
		InArchive.SerializeFloat(PE_TEXT("Suction"), InOutValue.Suction);
		InArchive.SerializeFloat(PE_TEXT("Translucency"), InOutValue.Translucency);
		InArchive.SerializeFloat(PE_TEXT("Duration"), InOutValue.Duration);
		InArchive.EndObject();
	}

	static void SerializeWaterImpulseArray(EArchive& InArchive, const CHAR* InName, TArray<FWaterImpulseDescription>& InOutValues)
	{
		UINT32 ValueNum = InOutValues.Num<UINT32>();
		InArchive.BeginArray(InName, ValueNum);
		if (!InArchive.IsSaving())
		{
			InOutValues.Empty();
		}
		for (UINT32 ValueIndex = 0u; ValueIndex < ValueNum; ValueIndex++)
		{
			const EString ValueKey = EString::FromInt(ValueIndex);
			FWaterImpulseDescription Value = InArchive.IsSaving() ? InOutValues[ValueIndex] : FWaterImpulseDescription();
			SerializeWaterImpulseDescription(InArchive, *ValueKey, Value);
			if (!InArchive.IsSaving())
			{
				InOutValues.Add(Value);
			}
		}
		InArchive.EndArray();
	}

	static void SerializeWaterWhirlpoolArray(EArchive& InArchive, const CHAR* InName, TArray<FWaterWhirlpoolDescription>& InOutValues)
	{
		UINT32 ValueNum = InOutValues.Num<UINT32>();
		InArchive.BeginArray(InName, ValueNum);
		if (!InArchive.IsSaving())
		{
			InOutValues.Empty();
		}
		for (UINT32 ValueIndex = 0u; ValueIndex < ValueNum; ValueIndex++)
		{
			const EString ValueKey = EString::FromInt(ValueIndex);
			FWaterWhirlpoolDescription Value = InArchive.IsSaving() ? InOutValues[ValueIndex] : FWaterWhirlpoolDescription();
			SerializeWaterWhirlpoolDescription(InArchive, *ValueKey, Value);
			if (!InArchive.IsSaving())
			{
				InOutValues.Add(Value);
			}
		}
		InArchive.EndArray();
	}

	static void RegisterWaterInitialConditionStructs()
	{
		const size_t ImpulseHash = GetClassHashCode<FWaterImpulseDescription>();
		const EString ImpulseName(PE_TEXT("FWaterImpulseDescription"));
		AddStructProperty<FWaterImpulseDescription, Vector3>(ImpulseHash, ImpulseName, PE_TEXT("WorldLocation"), &FWaterImpulseDescription::WorldLocation);
		AddStructProperty<FWaterImpulseDescription, FLOAT>(ImpulseHash, ImpulseName, PE_TEXT("Radius"), &FWaterImpulseDescription::Radius);
		AddStructProperty<FWaterImpulseDescription, FLOAT>(ImpulseHash, ImpulseName, PE_TEXT("Volume"), &FWaterImpulseDescription::Volume);
		AddStructProperty<FWaterImpulseDescription, FLOAT>(ImpulseHash, ImpulseName, PE_TEXT("YOffset"), &FWaterImpulseDescription::YOffset);
		AddStructProperty<FWaterImpulseDescription, Vector2>(ImpulseHash, ImpulseName, PE_TEXT("Velocity"), &FWaterImpulseDescription::Velocity);
		AddStructProperty<FWaterImpulseDescription, FLOAT>(ImpulseHash, ImpulseName, PE_TEXT("Mode"), &FWaterImpulseDescription::Mode);
		AddStructProperty<FWaterImpulseDescription, FLOAT>(ImpulseHash, ImpulseName, PE_TEXT("Intensity"), &FWaterImpulseDescription::Intensity);
		AddStructProperty<FWaterImpulseDescription, FLOAT>(ImpulseHash, ImpulseName, PE_TEXT("Hardness"), &FWaterImpulseDescription::Hardness);
		AddStructProperty<FWaterImpulseDescription, FLOAT>(ImpulseHash, ImpulseName, PE_TEXT("ForwardAngle"), &FWaterImpulseDescription::ForwardAngle);

		const size_t WhirlpoolHash = GetClassHashCode<FWaterWhirlpoolDescription>();
		const EString WhirlpoolName(PE_TEXT("FWaterWhirlpoolDescription"));
		AddStructProperty<FWaterWhirlpoolDescription, Vector3>(WhirlpoolHash, WhirlpoolName, PE_TEXT("WorldLocation"), &FWaterWhirlpoolDescription::WorldLocation);
		AddStructProperty<FWaterWhirlpoolDescription, FLOAT>(WhirlpoolHash, WhirlpoolName, PE_TEXT("Radius"), &FWaterWhirlpoolDescription::Radius);
		AddStructProperty<FWaterWhirlpoolDescription, FLOAT>(WhirlpoolHash, WhirlpoolName, PE_TEXT("Radial"), &FWaterWhirlpoolDescription::Radial);
		AddStructProperty<FWaterWhirlpoolDescription, FLOAT>(WhirlpoolHash, WhirlpoolName, PE_TEXT("Suction"), &FWaterWhirlpoolDescription::Suction);
		AddStructProperty<FWaterWhirlpoolDescription, FLOAT>(WhirlpoolHash, WhirlpoolName, PE_TEXT("Translucency"), &FWaterWhirlpoolDescription::Translucency);
		AddStructProperty<FWaterWhirlpoolDescription, FLOAT>(WhirlpoolHash, WhirlpoolName, PE_TEXT("Duration"), &FWaterWhirlpoolDescription::Duration);
	}

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PWaterSimulationComponent>();
		AddClassPropertyAccessors<PWaterSimulationComponent, Vector2Int>(ClassHash, PE_TEXT("Resolution"),
			[](const PWaterSimulationComponent* Component) -> Vector2Int
			{
				return Vector2Int(static_cast<INT32>(Component->GetResolutionX()), static_cast<INT32>(Component->GetResolutionZ()));
			},
			[](PWaterSimulationComponent* Component, const Vector2Int& Value) -> void
			{
				Component->SetResolution(static_cast<UINT32>(EMath::Max(Value.x, 1)), static_cast<UINT32>(EMath::Max(Value.y, 1)));
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PWaterSimulationComponent, Vector2>(ClassHash, PE_TEXT("AreaSize"),
			[](const PWaterSimulationComponent* Component) -> Vector2
			{
				return Vector2(Component->GetAreaSizeX(), Component->GetAreaSizeZ());
			},
			[](PWaterSimulationComponent* Component, const Vector2& Value) -> void
			{
				Component->SetAreaSize(Value.x, Value.y);
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PWaterSimulationComponent, FLOAT>(ClassHash, PE_TEXT("InitialDepth"),
			[](const PWaterSimulationComponent* Component) -> FLOAT { return Component->GetInitialDepth(); },
			[](PWaterSimulationComponent* Component, const FLOAT& Value) -> void { Component->SetInitialDepth(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassEnumPropertyAccessors<PWaterSimulationComponent, EWaterGroundType>(ClassHash, PE_TEXT("GroundType"),
			[](const PWaterSimulationComponent* Component) -> EWaterGroundType { return Component->GetGroundType(); },
			[](PWaterSimulationComponent* Component, const EWaterGroundType& Value) -> void { Component->SetGroundType(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PWaterSimulationComponent, TArray<FLOAT>>(ClassHash, PE_TEXT("GroundRelativeDepths"),
			[](const PWaterSimulationComponent* Component) -> TArray<FLOAT> { return Component->GetGroundRelativeDepths(); },
			[](PWaterSimulationComponent* Component, const TArray<FLOAT>& Value) -> void
			{
				if (Value.Num<UINT32>() > 0u)
				{
					Component->SetGroundHeightMap(Value);
				}
				else
				{
					Component->SetGroundFlat();
				}
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassBoolPropertyAccessors<PWaterSimulationComponent>(ClassHash, PE_TEXT("WaveEnabled"),
			[](const PWaterSimulationComponent* Component) -> BOOL32 { return Component->GetWaveEnabled(); },
			[](PWaterSimulationComponent* Component, const BOOL32& Value) -> void { Component->SetWaveEnabled(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PWaterSimulationComponent, FLOAT>(ClassHash, PE_TEXT("FixedTimeStep"),
			[](const PWaterSimulationComponent* Component) -> FLOAT { return Component->GetFixedTimeStep(); },
			[](PWaterSimulationComponent* Component, const FLOAT& Value) -> void
			{
				Component->SetTimeStep(Value, Component->GetMaxSubsteps(), Component->GetTimeScale());
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PWaterSimulationComponent, UINT32>(ClassHash, PE_TEXT("MaxSubsteps"),
			[](const PWaterSimulationComponent* Component) -> UINT32 { return Component->GetMaxSubsteps(); },
			[](PWaterSimulationComponent* Component, const UINT32& Value) -> void
			{
				Component->SetTimeStep(Component->GetFixedTimeStep(), Value, Component->GetTimeScale());
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PWaterSimulationComponent, FLOAT>(ClassHash, PE_TEXT("TimeScale"),
			[](const PWaterSimulationComponent* Component) -> FLOAT { return Component->GetTimeScale(); },
			[](PWaterSimulationComponent* Component, const FLOAT& Value) -> void
			{
				Component->SetTimeStep(Component->GetFixedTimeStep(), Component->GetMaxSubsteps(), Value);
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));

		// The stable tunables of the SWE parameter block. The Source / Modifier / Whirlpool
		// fields are runtime values overwritten from interaction events, so they stay out of
		// the authoring surface and out of level files.
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("FoamFadeSpeed"), &FWaterSimulationParameters::FoamFadeSpeed);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("FoamShallowDepth"), &FWaterSimulationParameters::FoamShallowDepth);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("FoamShallowGenerate"), &FWaterSimulationParameters::FoamShallowGenerate);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("FoamWaveGenerate"), &FWaterSimulationParameters::FoamWaveGenerate);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("FluidTransportMax"), &FWaterSimulationParameters::FluidTransportMax);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("FoamAdvection"), &FWaterSimulationParameters::FoamAdvection);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("EvaporationDepth"), &FWaterSimulationParameters::EvaporationDepth);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("FluxEnergy"), &FWaterSimulationParameters::FluxEnergy);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("Gravity"), &FWaterSimulationParameters::Gravity);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("VelocityClamp"), &FWaterSimulationParameters::VelocityClamp);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("ScaleY"), &FWaterSimulationParameters::ScaleY);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("Friction"), &FWaterSimulationParameters::Friction);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("Damping"), &FWaterSimulationParameters::Damping);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("AccelerationClamp"), &FWaterSimulationParameters::AccelerationClamp);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OvershootingBlend"), &FWaterSimulationParameters::OvershootingBlend);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OvershootingLimit"), &FWaterSimulationParameters::OvershootingLimit);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("MinWater"), &FWaterSimulationParameters::MinWater);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveVelocityScale"), &FWaterSimulationParameters::WaveVelocityScale);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveCalming"), &FWaterSimulationParameters::WaveCalming);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveSurfaceHeight"), &FWaterSimulationParameters::WaveSurfaceHeight);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveBorderXNegative"), &FWaterSimulationParameters::WaveBorderXNegative);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveBorderZNegative"), &FWaterSimulationParameters::WaveBorderZNegative);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveBorderXPositive"), &FWaterSimulationParameters::WaveBorderXPositive);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveBorderZPositive"), &FWaterSimulationParameters::WaveBorderZPositive);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveLength"), &FWaterSimulationParameters::WaveLength);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveSpeed"), &FWaterSimulationParameters::WaveSpeed);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveAmplitude"), &FWaterSimulationParameters::WaveAmplitude);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveSteepness"), &FWaterSimulationParameters::WaveSteepness);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("WaveDirectionAngle"), &FWaterSimulationParameters::WaveDirectionAngle);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineStateWetHeightOffset"), &FWaterSimulationParameters::OutlineStateWetHeightOffset);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineNoWaterBlend"), &FWaterSimulationParameters::OutlineNoWaterBlend);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineThickness"), &FWaterSimulationParameters::OutlineThickness);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineSurfaceSmoothing"), &FWaterSimulationParameters::OutlineSurfaceSmoothing);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineFoamThickness"), &FWaterSimulationParameters::OutlineFoamThickness);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineFluidThickness"), &FWaterSimulationParameters::OutlineFluidThickness);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineDryingOffset"), &FWaterSimulationParameters::OutlineDryingOffset);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineDryingSpeed"), &FWaterSimulationParameters::OutlineDryingSpeed);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineBorderBlend"), &FWaterSimulationParameters::OutlineBorderBlend);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineWetnessLimit"), &FWaterSimulationParameters::OutlineWetnessLimit);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineMinWater"), &FWaterSimulationParameters::OutlineMinWater);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineFallingSpeed"), &FWaterSimulationParameters::OutlineFallingSpeed);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineMinOutline"), &FWaterSimulationParameters::OutlineMinOutline);
		AddWaterSimulationFloatProperty(ClassHash, PE_TEXT("OutlineAverage"), &FWaterSimulationParameters::OutlineAverage);

		RegisterWaterInitialConditionStructs();
		AddClassPropertyStructCustom<PWaterSimulationComponent, TArray<FWaterImpulseDescription>>(ClassHash, PE_TEXT("InitialImpulses"),
			[](const PWaterSimulationComponent* Component) -> TArray<FWaterImpulseDescription> { return Component->GetInitialImpulses(); },
			[](PWaterSimulationComponent* Component, const TArray<FWaterImpulseDescription>& Value) -> void { Component->SetInitialImpulses(Value); },
			SerializeWaterImpulseArray,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyStructCustom<PWaterSimulationComponent, TArray<FWaterWhirlpoolDescription>>(ClassHash, PE_TEXT("InitialWhirlpools"),
			[](const PWaterSimulationComponent* Component) -> TArray<FWaterWhirlpoolDescription> { return Component->GetInitialWhirlpools(); },
			[](PWaterSimulationComponent* Component, const TArray<FWaterWhirlpoolDescription>& Value) -> void { Component->SetInitialWhirlpools(Value); },
			SerializeWaterWhirlpoolArray,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PWaterSimulationComponent, PPrimitiveComponent>();
		RegisterClassProperties();
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

	void PWaterSimulationComponent::SetGroundType(const EWaterGroundType InGroundType)
	{
		if (InGroundType == GroundType)
		{
			return;
		}
		if (InGroundType == EWaterGroundType::WATER_GROUND_FLAT)
		{
			SetGroundFlat();
		}
		else
		{
			const TArray<FLOAT> Depths = GroundRelativeDepths ? (*GroundRelativeDepths) : TArray<FLOAT>();
			SetGroundHeightMap(Depths);
		}
	}

	const TArray<FLOAT>& PWaterSimulationComponent::GetGroundRelativeDepths() const
	{
		static const TArray<FLOAT> EmptyDepths;
		return GroundRelativeDepths ? (*GroundRelativeDepths) : EmptyDepths;
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

	void PWaterSimulationComponent::SetInitialImpulses(const TArray<FWaterImpulseDescription>& InImpulses)
	{
		InitialImpulses = InImpulses;
	}

	void PWaterSimulationComponent::SetInitialWhirlpools(const TArray<FWaterWhirlpoolDescription>& InWhirlpools)
	{
		InitialWhirlpools = InWhirlpools;
	}

	void PWaterSimulationComponent::BeginAddedToScene(PWorld* InWorld)
	{
		PPrimitiveComponent::BeginAddedToScene(InWorld);
		if (bInitialConditionsApplied != FALSE)
		{
			return;
		}
		// Once per scene entry. The interactions go through the same pending queue as a runtime
		// call, so the render proxy consumes them on its first snapshot exactly as if gameplay
		// had injected them.
		for (UINT32 ImpulseIndex = 0u, ImpulseNum = InitialImpulses.Num<UINT32>(); ImpulseIndex < ImpulseNum; ImpulseIndex++)
		{
			AddImpulse(InitialImpulses[ImpulseIndex]);
		}
		for (UINT32 WhirlpoolIndex = 0u, WhirlpoolNum = InitialWhirlpools.Num<UINT32>(); WhirlpoolIndex < WhirlpoolNum; WhirlpoolIndex++)
		{
			AddWhirlpool(InitialWhirlpools[WhirlpoolIndex]);
		}
		bInitialConditionsApplied = TRUE;
		if ((InitialImpulses.Num<UINT32>() > 0u) || (InitialWhirlpools.Num<UINT32>() > 0u))
		{
			PE_LOG_LOG(EString(PE_TEXT("[WaterSimulation] Applied ")) + ToString(InitialImpulses.Num<UINT32>())
				+ EString(PE_TEXT(" initial impulse(s) and ")) + ToString(InitialWhirlpools.Num<UINT32>())
				+ EString(PE_TEXT(" whirlpool(s) from the level.")));
		}
	}

	void PWaterSimulationComponent::RemovedFromScene()
	{
		PPrimitiveComponent::RemovedFromScene();
		// A re-add replays the level's initial conditions. Anything still queued for the old
		// proxy goes with it, so the replay cannot deliver a duplicate of the previous batch.
		PendingInteractions.Empty();
		bResetRequested = FALSE;
		bInitialConditionsApplied = FALSE;
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
			if (PWorld* World = GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					RenderScene->AddPrimitive(this);
				}
			}
		}
	}

	void PWaterSimulationComponent::DestroyRenderState()
	{
		// Both levels are guarded: a component that never joined a world, or whose world
		// already released its render scene, has nothing registered to remove.
		if (PWorld* World = GetWorld())
		{
			if (RSceneInterface* RenderScene = World->GetRenderScene())
			{
				RenderScene->RemovePrimitive(this);
			}
		}
		PPrimitiveComponent::DestroyRenderState();
	}

	void PWaterSimulationComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			if (PWorld* World = GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					RenderScene->UpdatePrimitive(this);
				}
			}
		}
		PPrimitiveComponent::SendUpdateRenderState();
	}

}	// namespace PigeonEngine
