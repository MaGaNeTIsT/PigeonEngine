#include "LevelActor.h"
#include <Base/Timer/Timer.h>
#include <Demo/SponzaScene.h>
#include <Demo/WaterScene.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#if ENGINE_EDITOR_ONLY
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
	// The demo's material, named by the path of its own folder. A material that lives in a folder is
	// asked for that way, and the builder resolves the json named after that folder
	// (Projects/Demo/M_DemoUnlit/M_DemoUnlit.json): the json, the hlsl and the textures of one
	// material stay in one directory.
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_FOLDER			= "Projects/Demo/M_DemoUnlit";
	// The emission map the material samples, imported from its own folder on first use. The file
	// name is the asset's identity, which is why it carries the material's name, and the file has
	// to be a format the importer's WIC codecs understand (see BindMaterialDemoTexture).
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_EMISSIVE_ASSET	= "M_DemoUnlit_Emissive";
	// The material's own names as the shader side declares them: the block its parameters live in
	// and the texture it reads. They are the compiler's vocabulary, so the demo spells exactly what
	// the material json declared.
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_BLOCK				= "MaterialParams";
	PE_CONSTEXPR_STATIC const CHAR* MATERIAL_DEMO_EMISSIVE_TEXTURE	= "EmissiveMap";

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
		// The asset is named after the material, the request is named by the material's folder: two
		// namespaces, one material. The shader name is what the compiled asset is stored and looked
		// up under, the material name is what the builder resolves the source files from.
		OutRequest.ShaderName = EName(PE_TEXT("M_DemoUnlit"));
		OutRequest.MaterialName = EString(PE_TEXT(MATERIAL_DEMO_FOLDER));
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
	static FAssetImportSource MakeImportSource(const EString& InImportPath, const EString& InImportName, const EString& InImportFileType
		, const BOOL32 InIsFallbackSRGB = FALSE)
	{
		FAssetImportSource ImportSource;
		ImportSource.ImportPath = InImportPath;
		ImportSource.ImportName = InImportName;
		ImportSource.ImportFileType = InImportFileType;
		// Answers only for an image that declares no colour space of its own: a colour map wants
		// sRGB, a data map (roughness, a height) wants linear, and the importer cannot tell a
		// colour map from a data map by looking at the file.
		ImportSource.bFallbackSRGB = InIsFallbackSRGB;
		return ImportSource;
	}

	/*
	* The demo material's base colour texture, published into the instance's parameter set before
	* anything draws with that instance.
	*
	* Asked for rather than loaded, like every other asset here: the first call records a request
	* that imports the tga in the material's own folder on the load thread, and the caller tries
	* again on its next tick. The value goes into the parameter set because that is where a
	* material's runtime resources live - the render side reads them when it binds the material, and
	* a bindless build turns this texture into the descriptor heap index the material's constant
	* buffer carries.
	*
	* Ordering is what makes this safe: it runs before the instance is handed to a component, so the
	* set is never written while a draw reads it.
	*/
	static BOOL32 BindMaterialDemoTexture(const EShaderInstance* InInstance)
	{
		if (!InInstance)
		{
			return FALSE;
		}
		// The importer reads the image through WIC, whose built-in codecs cover jpg and png but not
		// tga: the file in the material's folder has to be one of those, and the same name is what
		// the asset ends up stored under.
		//
		// The fallback colour space stays linear, and that is a decision rather than an omission:
		// this map is the shape of an emission, not a colour to be displayed. The material states
		// the emission in physical units and the map only scales it, so a texel is a 0..1 level -
		// and a WIC export carries no colour space of its own for the importer to read anyway.
		const FAssetImportSource ImportSource = MakeImportSource(
			EString(EBaseSettings::ENGINE_SHADER_SOURCE_DIRECTORY) + EString(PE_TEXT(MATERIAL_DEMO_FOLDER)) + EString(PE_TEXT("/"))
			, EString(PE_TEXT(MATERIAL_DEMO_EMISSIVE_ASSET)), EString(PE_TEXT("jpg")), FALSE);
		const ETexture2DAsset* TextureAsset = nullptr;
		if (!ETextureAssetManager::GetManagerSingleton()->RequireLoadTexture2DAsset(
			EBaseSettings::ENGINE_TEXTURE_PATH, EString(PE_TEXT(MATERIAL_DEMO_EMISSIVE_ASSET)), TextureAsset, &ImportSource))
		{
			return FALSE;	// not imported yet: the component asks again on its next tick
		}
		const RTexture2DResource* TextureResource = (TextureAsset ? TextureAsset->GetRenderResource() : nullptr);
		if (!TextureResource)
		{
			// Imported, but the render thread has not built its gpu object yet: the creation runs as
			// a queued command, so this is the same "ask again" state as a missing asset.
			return FALSE;
		}
		// The instance arrives const because the process wide cache hands it out that way; what this
		// call owns is the value layer, and the render side const_casts the same object for the same
		// reason.
		EShaderParameterSet& Parameters = const_cast<EShaderParameterSet&>(InInstance->GetParameters());
		return Parameters.SetTexture(EName(PE_TEXT(MATERIAL_DEMO_BLOCK)), EName(PE_TEXT(MATERIAL_DEMO_EMISSIVE_TEXTURE))
			, TextureResource->GetResolvedRHITexture());
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

#if ENGINE_EDITOR_ONLY
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
					// Two different skeleton slots: Default is the base full-body layer,
					// UpperBody is the attack overlay played on top of it.
					Component->PlayMontage(MontageAsset->GetStoragedResource(), EString(PE_TEXT("BaseIdle")));
					Component->PlayMontage(MontageAsset->GetStoragedResource(), EString(PE_TEXT("Attack")));
					PE_LOG_LOG(EString("Active montage layers: ") + ToString(Component->GetMontageInstanceCount()));
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
			ECascadeShadowData* CascadeShadowData = LightComp->GetCascadeShadowData();
			{
				TArray<FLOAT> CSMLayers = { 3000.0f, 7000.0f, 10000.0f };
				TArray<FLOAT> CSMBorders = { 500.0f, 2000.0f };
				CascadeShadowData->Layers = CSMLayers;
				CascadeShadowData->Borders = CSMBorders;
			}
			LightComp->SetLightColor(Color3(1.0f, 1.0f, 1.0f));
			// Perpendicular illuminance in lux: a clear noon sun. It pairs with the default
			// exposure, which is Sunny-16 outdoors.
			LightComp->SetIlluminanceLux(EEngineSettings::ENGINE_DEFAULT_ILLUMINANCE_LUX);
			LightComp->SetSourceAngleDegrees(EEngineSettings::ENGINE_DEFAULT_SOURCE_ANGLE_DEGREES);
			// The sun casts shadows. The flag is off on a fresh light because a light that has not
			// opted in should not cost an atlas, and this is where the demo opts in: the shadow
			// map size is the placeholder the component still carries and the renderer clamps the
			// real resolution against RShadowSettings, so any positive pair works here.
			LightComp->SetShadowMapSize(TRUE, Vector2Int(2048, 2048));
			New->AddComponent(LightComp);
			// The sun: up, behind the default camera and leaning to its left, so the face of every
			// object that points at the viewer is the lit one.
			//
			// A directional light's forward axis points back at its emitter rather than along the
			// light it casts - the render side stores its negation as the direction of travel and the
			// shading does dot(normal, forward) - so pointing this axis down instead makes the sun
			// shine upwards and leaves every up-facing surface unlit. Only the direction of this
			// position matters; the distance is read by nothing.
			const Vector3 SunLocation(-4500.0f, 7500.0f, 500.0f);
			New->SetActorRotation(LookAtTargetQuaternionRotation(SunLocation, Vector3::Zero(), Vector3::YVector()));
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "PointLightActor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);
			PPointLightComponent* PointLightComp = new PPointLightComponent();
			PointLightComp->SetLightColor(Color3(1.0f, 0.85f, 0.6f));
			// Authored the way a lamp is sold, in lumen: 12566 lm over a whole sphere is about a
			// thousand candela, which is five stops below a sun. That is the physical answer rather
			// than a bug, and it means the sun and the sky swamp this light at every exposure that
			// does not blow them out - no placement fixes that. To see it on its own, take the sun's
			// IlluminanceLux and the sky's luminance down with their own controls, then lower the
			// camera's EV100 until the pool reads.
			PointLightComp->SetLuminousFluxLumen(12566.0f);
			// The cutoff that bounds the shading cost. It is not the light's size: the falloff
			// is inverse square out to here and zero past it.
			PointLightComp->SetAttenuationRadiusMeters(600.f);
			// A real emitter has a size, which is what gives its specular highlight a size
			// rather than a point.
			PointLightComp->SetSourceRadiusMeters(0.05f);
			// Enables the local shadow path for verification: the light owns six consecutive atlas
			// slots, one per cube face. Its shadow is invisible at the default exposure for the
			// reason above, so dimming the sun and the sky is what makes it show.
			PointLightComp->SetShadowMapSize(TRUE, Vector2Int(512, 512));
			New->AddComponent(PointLightComp);
			// Hung over open floor in the right half of the default view, clear of the material cube
			// (which ends at z = 50) and of the mannequin (x = 500), so the pool it casts is
			// unmistakably its own. A point light stores no cone, so its rotation is read by nothing.
			New->SetActorLocation(Vector3(300.0f, 220.0f, 220.0f));
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
			// The local shadow path's spot case: one atlas slot for the single perspective view.
			SpotLightComp->SetShadowMapSize(TRUE, Vector2Int(1024, 1024));
			New->AddComponent(SpotLightComp);
			// Hung over open floor in the left half of the default view and aimed at the ground, so
			// the inner cone lands as one disc, a little nearer to the viewer than the light is. The
			// lean off vertical is deliberate: the aim helper rejects a forward axis inside eighteen
			// degrees of straight up or down.
			//
			// A spot points its forward axis at what it lights, which is the opposite of the
			// directional light above, whose forward axis points back at its own emitter. It used to
			// sit at y = -200, below the ground plane's top face at y = 0, aimed horizontally: it was
			// underground and could not light anything.
			const Vector3 SpotLocation(-300.0f, 250.0f, 100.0f);
			const Vector3 SpotTarget(-300.0f, 0.0f, -45.0f);
			New->SetActorLocation(SpotLocation);
			New->SetActorRotation(LookAtTargetQuaternionRotation(SpotTarget, SpotLocation, Vector3::YVector()));
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

			// Inside the Sponza hall, at the centre of its footprint: the scene is appended to this level,
			// so the controllable character is what the building is walked through with.
			New->SetActorLocation(Vector3(-60.0f, 0.0f, -39.0f));

			PCharacterController* CharacterController = new PCharacterController();
			CharacterController->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(CharacterController, "Physics Test Character Controller");
			CharacterController->SetCharacter(New);
			this->GetWorld()->AddActor(CharacterController);
		}

		const Vector3 GroundHalfExtent(1000.f, 10.f, 1000.f);

		// The visible ground plane sits under the water sample: its 2000 x 2000 footprint is
		// exactly the simulation domain, and Sponza already covers the origin.
		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "Plane Actor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);

			PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
			NewStaticMeshComp->SetIsTickable(TRUE);
			const EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			const FAssetImportSource ImportSource = MakeImportSource(
				AssetBasePath + EString(PE_TEXT("EngineModels/BaseShapes/"))
				, EString(PE_TEXT("Cube")), EString(PE_TEXT("obj")));
			NewStaticMeshComp->RequestMeshAsset(EString(PE_TEXT("Cube")), &ImportSource);

			New->AddComponent(NewStaticMeshComp);

			// The static body captures the owner's world transform when the component is
			// added, so the transform has to be in place first.
			New->SetActorScale(GroundHalfExtent * 2.f);
			New->SetActorLocation(GWaterSceneLocation + Vector3(0.0f, -10.0f, 0.0f));

			FBoxShape* Shape = new FBoxShape(GroundHalfExtent, 0.f);
			PPhysicsComponent* PhysicsComponent = new PPhysicsComponent(Shape);
			PhysicsComponent->SetOwnerActor(New);

			New->AddComponent(PhysicsComponent);
		}

		// The physics character still stands where the visible plane used to be, so its old
		// collision box stays behind at the origin: no mesh, and therefore no visible change
		// under Sponza.
		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "Physics Floor Actor");
			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);

			FBoxShape* Shape = new FBoxShape(GroundHalfExtent, 0.f);
			PPhysicsComponent* PhysicsComponent = new PPhysicsComponent(Shape);
			PhysicsComponent->SetOwnerActor(New);
			New->AddComponent(PhysicsComponent);
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
				// The material's texture before the material: the instance is shared through the
				// process wide cache, so its values are published while nothing draws with it yet.
				if (!BindMaterialDemoTexture(MatInstance))
				{
					return FALSE;	// the texture is still being imported: ask again next tick
				}
				NewStaticMeshComp->SetMaterialInstance(MatInstance);
				return TRUE;
			});

			New->AddComponent(NewStaticMeshComp);		// The proxy snapshots the material slots when it is added and re-snapshots them on the next update tick, so this order is a convenience rather than a requirement.

			New->SetActorLocation(Vector3(250.0f, 50.0f, 0.0f));
			New->SetActorScale(Vector3(100.0f, 100.0f, 100.0f));
		}

		// The Sponza scene, added to the level rather than replacing it: its own objects are placed at
		// the source's world space, so the content above stays where it is and the building grows
		// around it. The build waits on assets, so it continues across frames on its own.
		BuildDemoSponzaScene(*GetWorld());
		// The water sample is independent of the Sponza build and waits on its own assets.
		BuildDemoWaterScene(*GetWorld());
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
