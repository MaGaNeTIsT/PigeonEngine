#include "WaterScene.h"

#include <Asset/AssetLoadManager.h>
#include <MeshAsset/MeshAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <Config/EngineConfig.h>
#include <Editor/EditorLogManager.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Water/WaterSimulationComponent.h>
#include <PigeonBase/Object/Component/Water/WaterSurfaceComponent.h>

namespace PigeonEngine
{

	static const CHAR* const GWaterMeshFolder		= PE_TEXT("EngineModels/SceneModels/Water/");
	static const CHAR* const GWaterMeshSourceName	= PE_TEXT("WaterPlane");
	static const CHAR* const GWaterMeshSourceType	= PE_TEXT("obj");
	static const CHAR* const GWaterMeshAssetName	= PE_TEXT("WaterPlane");
	static const CHAR* const GWaterMaterialName		= PE_TEXT("M_WaterDebug");
	static const CHAR* const GWaterProgramName		= PE_TEXT("WaterSurface");

	// The sample pond sits beside the Sponza building rather than inside it: the building is
	// authored around the origin and reaches X = 1800, so a 2000 unit domain centred at
	// X = 3000 starts at X = 2000. The demo level puts its visible ground plane under this
	// location too. The simulation body and the surface actor share it - the body's world XZ
	// is the domain centre, and the surface mesh must cover it.
	const Vector3 GWaterSceneLocation = Vector3(3000.f, 0.f, 0.f);

	struct FWaterSceneBuild
	{
	public:
		BOOL32						bStarted		= FALSE;
		BOOL32						bDone			= FALSE;
		BOOL32						bFailed			= FALSE;
		const EStaticMeshAsset*		MeshAsset		= nullptr;
		const EShaderInstance*		MaterialInstance = nullptr;
	};

	static FWaterSceneBuild GWaterSceneState;

	static BOOL32 StepWaterSceneBuild(PWorld& InWorld)
	{
		if ((GWaterSceneState.bDone != FALSE) || (GWaterSceneState.bFailed != FALSE))
		{
			return TRUE;
		}

		if (!GWaterSceneState.MeshAsset)
		{
			FAssetImportSource ImportSource;
			ImportSource.ImportPath = EString(EBaseSettings::ENGINE_ASSET_DIRECTORY) + EString(GWaterMeshFolder);
			ImportSource.ImportName = EString(GWaterMeshSourceName);
			ImportSource.ImportFileType = EString(GWaterMeshSourceType);
			ImportSource.bIsCombineSubmeshes = TRUE;
			ImportSource.bIsOptimizeMeshes = TRUE;

			const EStaticMeshAsset* MeshAsset = nullptr;
			if (EMeshAssetManager::GetManagerSingleton()->RequireLoadStaticMeshAsset(
				EBaseSettings::ENGINE_MESH_PATH, EString(GWaterMeshAssetName), MeshAsset, &ImportSource) == FALSE)
			{
				return FALSE;
			}
			if (!MeshAsset)
			{
				return FALSE;
			}
			GWaterSceneState.MeshAsset = MeshAsset;
		}

		if (!GWaterSceneState.MaterialInstance)
		{
			EShaderVertexFactoryType VertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
			EShaderVertexProfile Profile;
			if (!EShaderJobBuilder::ResolveVertexAxis(GWaterSceneState.MeshAsset->GetStoragedResource(), VertexFactory, Profile))
			{
				GWaterSceneState.bFailed = TRUE;
				PE_LOG_ERROR(EString(PE_TEXT("[WaterScene] The water plane has no usable vertex layout.")));
				return TRUE;
			}

			EShaderAssemblyRequest Request;
			Request.ShaderName	= EName(GWaterMaterialName);
			Request.MaterialName = EString(GWaterMaterialName);
			Request.ProgramName	= EName(GWaterProgramName);
			Request.VertexFactory = VertexFactory;
			Request.Profile		= Profile;
			const EShaderInstance* Instance = REngineShaderInstanceCache::GetManagerSingleton()->RequireMaterialInstance(Request);
			if (!Instance)
			{
				return FALSE;
			}
			GWaterSceneState.MaterialInstance = Instance;
		}

		// ---- The simulation body. Its world position is the area centre and its world Y is
		// the reference plane; the ground stays flat at that plane. It is placed beside the
		// Sponza building so the pond is not hidden by the scene.
		PActor* SimulationActor = new PActor();
		SimulationActor->SetIsTickable(TRUE);
		POBJ_DEBUGNAME_SET(SimulationActor, "WaterSimulationActor");
		InWorld.AddActor(SimulationActor);

		PWaterSimulationComponent* SimulationComponent = new PWaterSimulationComponent();
		SimulationComponent->SetIsTickable(TRUE);
		SimulationActor->SetActorLocation(GWaterSceneLocation);
		SimulationComponent->SetResolution(256u, 256u);
		SimulationComponent->SetAreaSize(2000.f, 2000.f);
		SimulationComponent->SetInitialDepth(50.f);

		FWaterSimulationParameters Parameters = SimulationComponent->GetSimulationParameters();
		Parameters.Gravity			= 981.f;
		Parameters.VelocityClamp	= 1000.f;
		Parameters.AccelerationClamp = 1000.f;
		Parameters.ScaleY			= 1.f;
		// Low enough that the initial drop keeps sloshing for a while instead of settling in
		// a couple of seconds, high enough that the height and velocity passes stay stable.
		Parameters.Friction			= 0.2f;
		Parameters.Damping			= 0.05f;
		Parameters.OvershootingBlend = 0.5f;
		Parameters.OvershootingLimit = 0.1f;
		Parameters.MinWater			= 0.0001f;
		Parameters.FluxEnergy		= 1.f;
		Parameters.FluidTransportMax = 1.f;
		Parameters.FoamAdvection	= 1.f;
		Parameters.FoamFadeSpeed	= 0.1f;
		Parameters.FoamShallowDepth	= 0.1f;
		// Foam is the debug material's near white colour, and both generators saturate at
		// this wave scale: the shallow term sees 50 units of water everywhere, and the wave
		// term's velocity divergence is large enough to drive the channel to one over most of
		// the pond. Keep both off so the depth colour is what the sample shows; a final water
		// material tunes foam against its own wave scale.
		Parameters.FoamShallowGenerate = 0.f;
		Parameters.FoamWaveGenerate	= 0.f;
		Parameters.WaveLength		= 500.f;
		Parameters.WaveSpeed		= 200.f;
		Parameters.WaveAmplitude	= 20.f;
		Parameters.WaveSteepness	= 0.5f;
		Parameters.WaveSurfaceHeight = 250.f;
		SimulationComponent->SetSimulationParameters(Parameters);
		SimulationComponent->SetWaveEnabled(TRUE);
		SimulationComponent->SetTimeStep(1.f / 60.f, 3u, 1.f);

		FWaterImpulseDescription Impulse;
		// World space: the modifier footprint is relative to the pond, not to the origin.
		// A tall, wide drop at the centre: it reads as a real splash and the low friction above
		// lets the resulting slosh run for several seconds.
		Impulse.WorldLocation	= GWaterSceneLocation + Vector3(0.f, 50.f, 0.f);
		Impulse.Radius			= 400.f;
		Impulse.Volume			= 200.f;
		// Mode 2 is the modifier's height mode: the footprint is driven to Volume above the
		// ground. Mode 0 has zero blend weight by the shader's own definition.
		Impulse.Mode			= 2.f;
		Impulse.Intensity		= 1.f;
		Impulse.Hardness		= 1.f;
		SimulationComponent->AddImpulse(Impulse);

		// Configure first, then attach: the component's create snapshot carries the configured
		// domain instead of rebuilding once the defaults reached the render thread.
		SimulationActor->AddComponent(SimulationComponent);

		// ---- The surface. The plane is authored in [-0.5, 0.5] world XZ at Y = 0, so scaling
		// X and Z by the area size makes it cover the simulation domain exactly. It shares the
		// simulation body's location, which is what keeps the material's world XZ lookup on
		// the domain.
		PActor* SurfaceActor = new PActor();
		SurfaceActor->SetIsTickable(TRUE);
		POBJ_DEBUGNAME_SET(SurfaceActor, "WaterSurfaceActor");
		InWorld.AddActor(SurfaceActor);

		PWaterSurfaceComponent* SurfaceComponent = new PWaterSurfaceComponent();
		SurfaceComponent->SetIsTickable(TRUE);
		SurfaceActor->SetActorLocation(GWaterSceneLocation);
		SurfaceActor->SetActorScale(Vector3(2000.f, 1.f, 2000.f));
		SurfaceComponent->SetMeshAsset(GWaterSceneState.MeshAsset);
		SurfaceComponent->SetMaterialInstance(GWaterSceneState.MaterialInstance);
		SurfaceComponent->SetSimulation(SimulationComponent);
		SurfaceComponent->SetWaterHeightScale(1.f);
		SurfaceComponent->SetWaterMaxHeight(300.f);
		SurfaceActor->AddComponent(SurfaceComponent);

		GWaterSceneState.bDone = TRUE;
		PE_LOG_LOG(EString(PE_TEXT("[WaterScene] Built one simulation body and one surface.")));
		return TRUE;
	}

	void BuildDemoWaterScene(PWorld& InWorld)
	{
		if ((GWaterSceneState.bStarted != FALSE) || (GWaterSceneState.bDone != FALSE) || (GWaterSceneState.bFailed != FALSE))
		{
			return;
		}
		GWaterSceneState.bStarted = TRUE;

		PActor* BuilderActor = new PActor();
		BuilderActor->SetIsTickable(TRUE);
		POBJ_DEBUGNAME_SET(BuilderActor, "WaterSceneBuilder");
		InWorld.AddActor(BuilderActor);

		PSceneComponent* BuilderComponent = new PSceneComponent();
		BuilderComponent->SetIsTickable(TRUE);
		BuilderActor->AddComponent(BuilderComponent);

		PWorld* WorldPointer = &InWorld;
		BuilderComponent->AddDeferredApply([WorldPointer]() -> BOOL32
		{
			return StepWaterSceneBuild(*WorldPointer);
		});
	}

}	// namespace PigeonEngine
