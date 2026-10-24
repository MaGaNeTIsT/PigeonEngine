#include "LevelActor.h"
#include <Base/Timer/Timer.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#if _EDITOR_ONLY
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#endif
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/EngineShaderInstanceCache.h>
#include "../Component/SceneComponent.h"
#include "../World/World.h"
#include "../Controller/Controller.h"
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/PointLightComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/SpotLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/BezierGrassComponent.h>

//Physics Test
#include <PhysicsTest/PhysicsTest.h>
#include <PigeonBase/Object/Component/Physics/PhysicsComponent.h>
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
#include "PigeonBase/Object/Controller/CharacterController.h"
#include "Editor/EditorLogManager.h"


namespace PigeonEngine
{
	/*
	* The MaterialDemo material. The job builder generates and compiles the stage sources of
	* (material x program x vertex axis), then the instance holds the values; the component keeps
	* a non owning pointer, so the instance has to outlive every component that references it -
	* which is what the process wide cache is for. The editor will call exactly this function later.
	*
	* The axis comes from the mesh's own layout, so the request cannot be named before the mesh
	* exists: the caller asks for the mesh first and builds the request from it.
	*/
	static BOOL32 MakeMaterialDemoRequest(const EStaticMeshAsset* InMeshAsset, EShaderAssemblyRequest& OutRequest)
	{
		EShaderVertexFactoryType VertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
		EShaderVertexProfile Profile;
		if (!EShaderJobBuilder::ResolveVertexAxis((InMeshAsset ? InMeshAsset->GetStoragedResource() : nullptr), VertexFactory, Profile))
		{
			return FALSE;
		}

		OutRequest = EShaderAssemblyRequest();
		OutRequest.ShaderName = EName(PE_TEXT("M_EngineDefaultLit"));
		OutRequest.MaterialName = EString(PE_TEXT("M_EngineDefaultLit"));
		OutRequest.ProgramName = EName(PE_TEXT("BasePass"));
		OutRequest.VertexFactory = VertexFactory;
		OutRequest.Profile = Profile;
		return TRUE;
	}
	/*
	* Any thread: the instance for the demo material when it is already assembled, otherwise null
	* after recording one assembly request for the load thread - the caller asks again on its next
	* tick. Resolved as a material rather than as a plain assembly, so that a material this mesh
	* asked for and did not get comes back as the error material: the mesh stays in the frame
	* wearing the marker, which can be found and fixed, where a null instance would leave it
	* invisible and indistinguishable from a mesh that was never assigned anything.
	*/
	static const EShaderInstance* RequireMaterialDemoShaderInstance(const EStaticMeshAsset* InMeshAsset)
	{
		EShaderAssemblyRequest Request;
		if (!MakeMaterialDemoRequest(InMeshAsset, Request))
		{
			return nullptr;
		}
		return REngineShaderInstanceCache::GetManagerSingleton()->RequireMaterialInstance(Request);
	}

	/*
	* The raw file an asset is imported from when its PAsset is not on disk yet. The load thread runs
	* that import as part of serving the request, so nothing here touches the disk on a frame - which
	* is the whole reason the demo asks for its assets instead of loading them.
	*/
	static FAssetImportSource MakeImportSource(const EString& InImportPath, const EString& InImportName, const EString& InImportFileType)
	{
		FAssetImportSource ImportSource;
		ImportSource.ImportPath = InImportPath;
		ImportSource.ImportName = InImportName;
		ImportSource.ImportFileType = InImportFileType;
		return ImportSource;
	}

	/*
	* The demo's sky box: the six faces of the engine's default sky, in the order the cube importer
	* expects. The data lives here because it is demo data - the component it is handed to knows only
	* that it was given an import source.
	*/
	static FAssetImportCubeSource MakeSkyCubeMapImportSource()
	{
		constexpr UINT32 SkyFaceNum = 6u;
		const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
		const EString ImportPath(AssetBasePath + EString(PE_TEXT("EngineTextures/DefaultSkyBox/Sky_001/")));

		FAssetImportCubeSource ImportSource;
		for (UINT32 FaceIndex = 0u; FaceIndex < SkyFaceNum; FaceIndex++)
		{
			ImportSource.ImportPaths.Add(ImportPath);
			ImportSource.ImportFileTypes.Add(EString(PE_TEXT("png")));
		}
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Right")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Left")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Top")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Bottom")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Forward")));
		ImportSource.ImportNames.Add(EString(PE_TEXT("Sky_001_Back")));
		// A sky box is colour, and these faces are 8 bit PNGs: when a face declares its own
		// colour space that statement decides, and this fallback answers for the ones that
		// declare nothing (the .tga siblings in the same folder carry no colour information).
		ImportSource.bFallbackSRGB = TRUE;
		return ImportSource;
	}

	PLevelActor::PLevelActor()
	{
		this->SetIsTickable(TRUE);
	}

	PLevelActor::~PLevelActor()
	{
		// this->Destroy();
	}

	void PLevelActor::BeginAddedToScene(PWorld* World)
	{
		PActor::BeginAddedToScene(World);
	}

	void PLevelActor::RemovedFromScene()
	{
		PActor::RemovedFromScene();
	}

	void PLevelActor::UserBeginPlay()
	{
#if 0
		{
			PActor* New = new PActor();
			New->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(New, "StaticMeshActor");
			this->GetWorld()->AddActor(New);
			PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
			NewStaticMeshComp->SetIsTickable(TRUE);
			New->AddComponent(NewStaticMeshComp);
			const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			const FAssetImportSource ImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/SceneModels/Sponza/"))
				, EString(PE_TEXT("sponza")), EString(PE_TEXT("obj")));
			NewStaticMeshComp->RequestMeshAsset(EString(PE_TEXT("Sponza")), &ImportSource);

			New->SetActorScale(Vector3(1.0f, 1.0f, 1.0f));
		}
#endif	// 0

		{
			PActor* New = new PActor();
			New->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(New, "StaticMeshActor");
			this->GetWorld()->AddActor(New);
			PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
			NewStaticMeshComp->SetIsTickable(TRUE);
			New->AddComponent(NewStaticMeshComp);

			const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			const FAssetImportSource ImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/SceneModels/Robot/"))
				, EString(PE_TEXT("Robot")), EString(PE_TEXT("obj")));
			NewStaticMeshComp->RequestMeshAsset(EString(PE_TEXT("Robot")), &ImportSource);

			New->SetActorLocation(Vector3(-500.0f, 100.0f, 0.0f));
		}

		{
			PActor* New = new PActor();
			New->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(New, "SkeletalMeshActor");
			this->GetWorld()->AddActor(New);
			PSkeletalMeshComponent* NewSkeletalMeshComp = new PSkeletalMeshComponent();
			NewSkeletalMeshComp->SetIsTickable(TRUE);
			New->AddComponent(NewSkeletalMeshComp);

			const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			const FAssetImportSource ImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/SceneModels/UnrealCharacter/"))
				, EString(PE_TEXT("SK_Mannequin_UE4_WithWeapon")), EString(PE_TEXT("FBX")));
			NewSkeletalMeshComp->RequestSkeletalMeshAssets(EString(PE_TEXT("SK_Mannequin_UE4_WithWeapon_Mesh"))
				, EString(PE_TEXT("SK_Mannequin_UE4_WithWeapon_Skeleton")), &ImportSource);

			// The demo plays the first clip of an animation authored against the same UE4
			// mannequin skeleton. The request waits for both the animation and the skeleton,
			// so it can be issued next to the mesh request.
			const FAssetImportSource AnimationImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/SceneModels/UnrealCharacter/"))
				, EString(PE_TEXT("Boss_Idle")), EString(PE_TEXT("FBX")));
			NewSkeletalMeshComp->RequestAnimationAsset(EString(PE_TEXT("Boss_Idle")), &AnimationImportSource, TRUE);

#if _EDITOR_ONLY
			// The state machine is authored as JSON under the source assets. Import it only
			// when its PAsset is missing, so an editor-edited PAsset is not overwritten by
			// the sample on every level load.
			const EString StateMachineAssetName(PE_TEXT("Boss_StateMachine"));
			const EAnimStateMachineAsset* StateMachineAsset = nullptr;
			if (!(EAnimStateMachineManager::GetManagerSingleton()->LoadStateMachineAsset(
				EBaseSettings::ENGINE_STATE_MACHINE_PATH, StateMachineAssetName, StateMachineAsset)))
			{
				const EString StateMachineJsonPath = AssetBasePath
					+ EString(PE_TEXT("EngineModels/SceneModels/UnrealCharacter/Boss_StateMachine.json"));
				if (EAnimStateMachineManager::GetManagerSingleton()->ImportStateMachine(
					StateMachineAssetName, StateMachineJsonPath, EBaseSettings::ENGINE_STATE_MACHINE_PATH))
				{
					StateMachineAsset = nullptr;
					EAnimStateMachineManager::GetManagerSingleton()->LoadStateMachineAsset(
						EBaseSettings::ENGINE_STATE_MACHINE_PATH, StateMachineAssetName, StateMachineAsset);
				}
			}
			if (StateMachineAsset)
			{
				NewSkeletalMeshComp->SetStateMachine(StateMachineAsset->GetStoragedResource());
			}

			// Standalone notify definitions referenced by the montage. They must be on disk
			// before the montage is imported so its event references can be resolved.
			const EString NotifyAssetNames[2] = {
				EString(PE_TEXT("Boss_AttackWindow")),
				EString(PE_TEXT("Boss_AttackStarted"))
			};
			for (INT32 NotifyIndex = 0; NotifyIndex < 2; NotifyIndex++)
			{
				const EAnimNotifyAsset* NotifyAsset = nullptr;
				if (!(EAnimNotifyManager::GetManagerSingleton()->LoadNotifyAsset(
					EBaseSettings::ENGINE_NOTIFY_PATH, NotifyAssetNames[NotifyIndex], NotifyAsset)))
				{
					const EString NotifyJsonPath = AssetBasePath
						+ EString(PE_TEXT("EngineModels/SceneModels/UnrealCharacter/"))
						+ NotifyAssetNames[NotifyIndex] + EString(PE_TEXT(".json"));
					if (EAnimNotifyManager::GetManagerSingleton()->ImportNotify(
						NotifyAssetNames[NotifyIndex], NotifyJsonPath, EBaseSettings::ENGINE_NOTIFY_PATH))
					{
						NotifyAsset = nullptr;
						EAnimNotifyManager::GetManagerSingleton()->LoadNotifyAsset(
							EBaseSettings::ENGINE_NOTIFY_PATH, NotifyAssetNames[NotifyIndex], NotifyAsset);
					}
				}
			}

			// Montage: the sample upper-body section is played once the animation asset is
			// bound, so it layers on top of the state machine instead of racing it.
			const EString MontageAssetName(PE_TEXT("Boss_UpperBody_Attack"));
			const EAnimMontageAsset* MontageAsset = nullptr;
			if (!(EAnimMontageManager::GetManagerSingleton()->LoadMontageAsset(
				EBaseSettings::ENGINE_MONTAGE_PATH, MontageAssetName, MontageAsset)))
			{
				const EString MontageJsonPath = AssetBasePath
					+ EString(PE_TEXT("EngineModels/SceneModels/UnrealCharacter/Boss_Montage.json"));
				if (EAnimMontageManager::GetManagerSingleton()->ImportMontage(
					MontageAssetName, MontageJsonPath, EBaseSettings::ENGINE_MONTAGE_PATH))
				{
					MontageAsset = nullptr;
					EAnimMontageManager::GetManagerSingleton()->LoadMontageAsset(
						EBaseSettings::ENGINE_MONTAGE_PATH, MontageAssetName, MontageAsset);
				}
			}
			if (MontageAsset)
			{
				PSkeletalMeshComponent* Component = NewSkeletalMeshComp;
				TFunction<BOOL32()> MontageStep = [Component, MontageAsset]() -> BOOL32
				{
					if (!Component->GetAnimationAsset())
					{
						return FALSE;
					}
					Component->PlayMontage(MontageAsset->GetStoragedResource(), EString(PE_TEXT("Attack")));
					return TRUE;
				};
				if (MontageStep() == FALSE)
				{
					NewSkeletalMeshComp->AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(MontageStep));
				}
			}
#endif

			Quaternion DefaultRot(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.0f)));
			NewSkeletalMeshComp->SetComponentRotation(DefaultRot);

			New->SetActorLocation(Vector3(500.0f, 0.0f, 0.0f));
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "LightActor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);
			PDirectionalLightComponent* LightComp = new PDirectionalLightComponent();
			LightComp->SetLightColor(Color3(1.0f, 1.0f, 1.0f));
			// Perpendicular illuminance in lux: a clear noon sun. It pairs with the default
			// exposure, which is Sunny-16 outdoors.
			LightComp->SetIlluminanceLux(EEngineSettings::ENGINE_DEFAULT_ILLUMINANCE_LUX);
			LightComp->SetSourceAngleDegrees(EEngineSettings::ENGINE_DEFAULT_SOURCE_ANGLE_DEGREES);
			New->AddComponent(LightComp);
			Quaternion Quat = Quaternion(0.5f, 0.5f, 0.0f, 1.0f);
			Quat.Normalize();
			New->SetActorRotation(Quat);
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "PointLightActor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);
			PPointLightComponent* PointLightComp = new PPointLightComponent();
			PointLightComp->SetLightColor(Color3(1.0f, 0.85f, 0.6f));
			// Authored the way a lamp is sold, in lumen. Against the default exposure this is a
			// small contribution next to a sun, which is the physical answer rather than a bug:
			// indoor and outdoor light are about five stops apart. Lower the camera's EV100 to
			// see it on its own.
			PointLightComp->SetLuminousFluxLumen(12566.0f);
			// The cutoff that bounds the shading cost. It is not the light's size: the falloff
			// is inverse square out to here and zero past it.
			PointLightComp->SetAttenuationRadiusMeters(600.f);
			// A real emitter has a size, which is what gives its specular highlight a size
			// rather than a point.
			PointLightComp->SetSourceRadiusMeters(0.05f);
			New->AddComponent(PointLightComp);
			New->SetActorLocation(Vector3(400.0f, 200.0f, 200.0f));
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "SpotLightActor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);
			PSpotLightComponent* SpotLightComp = new PSpotLightComponent();
			SpotLightComp->SetLightColor(Color3(0.7f, 0.85f, 1.0f));
			SpotLightComp->SetLuminousFluxLumen(12566.0f);
			// Full intensity inside 20 degrees, falling to nothing at 30. The pair is what makes
			// the cone's edge a penumbra rather than a step.
			SpotLightComp->SetSpotConeAnglesDegrees(20.f, 30.f);
			SpotLightComp->SetAttenuationRadiusMeters(600.f);
			New->AddComponent(SpotLightComp);
			// The aim is the component's forward axis, so the rotation is the aim. Left at the
			// identity here, which points the cone along whatever the engine's forward is - the
			// point of the actor is to have a spot in the scene to look at, not to aim it at
			// something in particular.
			New->SetActorLocation(Vector3(-400.0f, -200.0f, 300.0f));
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "SkyLightActor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);
			PSkyLightComponent* SkyLightComp = new PSkyLightComponent();

			// The cube map arrives through a binding like every other demo asset: the sky is drawn
			// without it until it does, and nothing on this thread reads or imports a file.
			const FAssetImportCubeSource CubeMapImportSource = MakeSkyCubeMapImportSource();
			SkyLightComp->RequestCubeMapAsset(EString(PE_TEXT("SkyBox001")), &CubeMapImportSource);

			SkyLightComp->SetLightAdjust(Color3(1.0f, 1.0f, 1.0f));
			// The sky's luminance in cd/m2: the cube carries the shape of the sky, this is what
			// puts it on the same physical scale as the lights.
			SkyLightComp->SetIntensity(EEngineSettings::ENGINE_SKY_LUMINANCE);
			New->AddComponent(SkyLightComp);
			New->SetActorScale(Vector3(5000.0f, 5000.0f, 5000.0f));
		}

		// Physics CharacterTest Actor
		{
			PPhysicsTestCharacter* New = new PPhysicsTestCharacter();
			New->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(New, "Physics Test Character");
			FCharacterSettings Settings;
			Settings.Layer = Layers::MOVING;
			New->InitCharacter(Settings);
			this->GetWorld()->AddActor(New);

			PSkeletalMeshComponent* NewSkeletalMeshComp = new PSkeletalMeshComponent();
			NewSkeletalMeshComp->SetIsTickable(TRUE);

			const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			const FAssetImportSource ImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/SceneModels/UnrealCharacter/"))
				, EString(PE_TEXT("SK_Mannequin_UE4_WithWeapon")), EString(PE_TEXT("FBX")));
			NewSkeletalMeshComp->RequestSkeletalMeshAssets(EString(PE_TEXT("SK_Mannequin_UE4_WithWeapon_Mesh"))
				, EString(PE_TEXT("SK_Mannequin_UE4_WithWeapon_Skeleton")), &ImportSource);

			New->SkeletalMeshComponent = NewSkeletalMeshComp;

			New->AddComponent(New->SkeletalMeshComponent, ETransform());

			Quaternion DefaultRot(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.0f)));
			New->SkeletalMeshComponent->SetComponentRotation(DefaultRot);

			New->SetActorLocation(Vector3(-20.0f, 0.0f, 50.0f));

			PCharacterController* CharacterController = new PCharacterController();
			CharacterController->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(CharacterController, "Physics Test Character Controller");
			CharacterController->SetCharacter(New);
			this->GetWorld()->AddActor(CharacterController);
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "Plane Actor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);

			const Vector3 PlaneHalfExtent(1000.f, 10.f, 1000.f);

			PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
			NewStaticMeshComp->SetIsTickable(TRUE);
			const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			const FAssetImportSource ImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/BaseShapes/"))
				, EString(PE_TEXT("Cube")), EString(PE_TEXT("obj")));
			NewStaticMeshComp->RequestMeshAsset(EString(PE_TEXT("Cube")), &ImportSource);

			New->AddComponent(NewStaticMeshComp);

			FBoxShape* Shape = new FBoxShape(PlaneHalfExtent, 0.f);
			PPhysicsComponent* PhysicsComponent = new PPhysicsComponent(Shape);
			PhysicsComponent->SetOwnerActor(New);

			New->AddComponent(PhysicsComponent);

			New->SetActorScale(PlaneHalfExtent * 2.f);
			New->SetActorLocation(Vector3(0.0f, -10.0f, 0.0f));
		}

#if 0
		{
			PActor* New = new PActor();
			New->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(New, "BezierGrassActor");
			this->GetWorld()->AddActor(New);
			PBezierGrassComponent* NewBezierGrassComp = new PBezierGrassComponent();
			NewBezierGrassComp->SetIsTickable(TRUE);
			New->AddComponent(NewBezierGrassComp);

			New->SetActorLocation(Vector3(0.0f, 0.0f, 0.0f));
			PE_LOG_LOG(NewBezierGrassComp->GetComponentWorldLocation().AsString());
			PE_LOG_LOG(New->GetBounds().AsString());
			New->SetActorLocation(Vector3(0.0f, 0.0f, 0.0f));
			PE_LOG_LOG(New->GetBounds().AsString());
		}
#endif

		// Material demo: create a static mesh actor and assign a material to its primitive component
		{
			PActor* New = new PActor();
			New->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(New, "MaterialDemoActor");
			this->GetWorld()->AddActor(New);

			PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
			NewStaticMeshComp->SetIsTickable(TRUE);

			// The mesh and the material it is demoed with, as two bindings. The material waits for the
			// mesh because its request is named after the mesh's vertex axis: it asks for the mesh
			// itself and reports not-done until that exists.
			const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			const FAssetImportSource ImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/BaseShapes/"))
				, EString(PE_TEXT("Cube")), EString(PE_TEXT("obj")));
			NewStaticMeshComp->RequestMeshAsset(EString(PE_TEXT("Cube")), &ImportSource);
			NewStaticMeshComp->AddDeferredApply([NewStaticMeshComp, ImportSource]() -> BOOL32
			{
				const EStaticMeshAsset* MeshAsset = nullptr;
				if (!EMeshAssetManager::GetManagerSingleton()->RequireLoadStaticMeshAsset(
					EBaseSettings::ENGINE_MESH_PATH, EString(PE_TEXT("Cube")), MeshAsset, &ImportSource))
				{
					return FALSE;
				}
				const EShaderInstance* MatInstance = RequireMaterialDemoShaderInstance(MeshAsset);
				if (!MatInstance)
				{
					return FALSE;	// not assembled yet: the component asks again on its next tick
				}
				NewStaticMeshComp->SetMaterialInstance(MatInstance);
				return TRUE;
			});

			New->AddComponent(NewStaticMeshComp);		// The proxy snapshots the material slots when it is added and re-snapshots them on the next update tick, so this order is a convenience rather than a requirement.

			New->SetActorLocation(Vector3(250.0f, 50.0f, 0.0f));
			New->SetActorScale(Vector3(100.0f, 100.0f, 100.0f));
		}
	}

	void PLevelActor::UserEndPlay()
	{

	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{
		// Nothing to do here: the demo's assets are bound to its components, and each component retries
		// its own pending bindings on its own tick (PSceneComponent::AddDeferredApply). That is also
		// what keeps the scene setup above free of per-actor bookkeeping.
		(void)deltaTime;
	}


}
