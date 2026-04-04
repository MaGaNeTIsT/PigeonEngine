#include "LevelActor.h"
#include <Base/Timer/Timer.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <MaterialAsset/MaterialAsset.h>
#include "../Component/SceneComponent.h"
#include "../World/World.h"
#include "../Controller/Controller.h"
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
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
		this->SetWorld(World);
    	UserBeginPlay();
	}

	void PLevelActor::RemovedFromScene()
	{

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
			const EStaticMeshAsset* Asset = nullptr;
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineModels/SceneModels/Robot/");
			EString ImportName("Robot");
			EString ImportFileType("obj");
			TryLoadStaticMesh(EBaseSettings::ENGINE_MESH_PATH, "Robot", Asset, &ImportPath, &ImportName, &ImportFileType, TRUE);

			NewStaticMeshComp->SetMeshAsset(Asset);

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

			const ESkinnedMeshAsset* MeshAsset = nullptr;
			const ESkeletonAsset* SkeletonAsset = nullptr;
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineModels/SceneModels/UnrealCharacter/");
			EString ImportName("SK_Mannequin_UE4_WithWeapon");
			EString ImportFileType("FBX");
			EString MeshAssetName("SK_Mannequin_UE4_WithWeapon_Mesh");
			EString SkeletonAssetName("SK_Mannequin_UE4_WithWeapon_Skeleton");
			TryLoadSkinnedMesh(EBaseSettings::ENGINE_MESH_PATH, MeshAssetName, MeshAsset, &ImportPath, &ImportName, &ImportFileType, TRUE);
			TryLoadSkeleton(EBaseSettings::ENGINE_SKELETON_PATH, SkeletonAssetName, SkeletonAsset, &ImportPath, &ImportName, &ImportFileType);

			NewSkeletalMeshComp->SetMeshAsset(MeshAsset);
			NewSkeletalMeshComp->SetSkeletonAsset(SkeletonAsset);
			{
				NewSkeletalMeshComp->GetBoneMemoryPool().SetRootTransform(Vector3::Zero(), Quaternion::Identity(), Vector3::One());
				NewSkeletalMeshComp->GetBoneMemoryPool().GenerateFromSkeleton(SkeletonAsset->GetStoragedResource());
				Quaternion DefaultRot(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.0f)));
				NewSkeletalMeshComp->SetComponentRotation(DefaultRot);
			}

			New->SetActorLocation(Vector3(500.0f, 0.0f, 0.0f));
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "LightActor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);
			PDirectionalLightComponent* LightComp = new PDirectionalLightComponent();
			LightComp->SetLightColor(Color3(1.0f, 1.0f, 1.0f));
			LightComp->SetLightIntensity(1.5f);
			New->AddComponent(LightComp);
			Quaternion Quat = Quaternion(0.5f, 0.5f, 0.0f, 1.0f);
			Quat.Normalize();
			New->SetActorRotation(Quat);
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "SkyLightActor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);
			PSkyLightComponent* SkyLightComp = new PSkyLightComponent();

			const ETextureCubeAsset* CubeMap = nullptr;
			TArray<EString> ImportPaths;
			TArray<EString> ImportNames;
			TArray<EString> ImportFileTypes;
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			for (UINT32 i = 0u; i < 6u; i++)
			{
				ImportPaths.Add(AssetBasePath + "EngineTextures/DefaultSkyBox/Sky_001/");
				ImportFileTypes.Add("png");
			}
			ImportNames.Add("Sky_001_Right");
			ImportNames.Add("Sky_001_Left");
			ImportNames.Add("Sky_001_Top");
			ImportNames.Add("Sky_001_Bottom");
			ImportNames.Add("Sky_001_Forward");
			ImportNames.Add("Sky_001_Back");
			TryLoadTextureCube(EBaseSettings::ENGINE_TEXTURE_PATH, "SkyBox001", CubeMap, &ImportPaths, &ImportNames, &ImportFileTypes);

			SkyLightComp->SetCubeMapAsset(CubeMap);
			SkyLightComp->SetLightAdjust(Color3(1.0f, 1.0f, 1.0f));
			SkyLightComp->SetIntensity(1.0f);
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

			const ESkinnedMeshAsset* MeshAsset = nullptr;
			const ESkeletonAsset* SkeletonAsset = nullptr;
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineModels/SceneModels/UnrealCharacter/");
			EString ImportName("SK_Mannequin_UE4_WithWeapon");
			EString ImportFileType("FBX");
			EString MeshAssetName("SK_Mannequin_UE4_WithWeapon_Mesh");
			EString SkeletonAssetName("SK_Mannequin_UE4_WithWeapon_Skeleton");
			TryLoadSkinnedMesh(EBaseSettings::ENGINE_MESH_PATH, MeshAssetName, MeshAsset, &ImportPath, &ImportName, &ImportFileType, TRUE);
			TryLoadSkeleton(EBaseSettings::ENGINE_SKELETON_PATH, SkeletonAssetName, SkeletonAsset, &ImportPath, &ImportName, &ImportFileType);

			NewSkeletalMeshComp->SetMeshAsset(MeshAsset);
			NewSkeletalMeshComp->SetSkeletonAsset(SkeletonAsset);
			{
				NewSkeletalMeshComp->GetBoneMemoryPool().SetRootTransform(Vector3::Zero(), Quaternion::Identity(), Vector3::One());
				NewSkeletalMeshComp->GetBoneMemoryPool().GenerateFromSkeleton(SkeletonAsset->GetStoragedResource());
			}

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
			const EStaticMeshAsset* Asset = nullptr;
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineModels/BaseShapes/");
			EString ImportName("Cube");
			EString ImportFileType("obj");
			TryLoadStaticMesh(EBaseSettings::ENGINE_MESH_PATH, "Cube", Asset, &ImportPath, &ImportName, &ImportFileType, TRUE);
			NewStaticMeshComp->SetMeshAsset(Asset);

			New->AddComponent(NewStaticMeshComp);

			FBoxShape* Shape = new FBoxShape(PlaneHalfExtent, 0.f);
			PPhysicsComponent* PhysicsComponent = new PPhysicsComponent(Shape);
			PhysicsComponent->SetOwnerActor(New);
			PhysicsComponent->InitPhysicsComponent();

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

			// Load mesh
			const EStaticMeshAsset* MeshAsset = nullptr;
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineModels/BaseShapes/");
			EString ImportName("Cube");
			EString ImportFileType("obj");
			TryLoadStaticMesh(EBaseSettings::ENGINE_MESH_PATH, "Cube", MeshAsset, &ImportPath, &ImportName, &ImportFileType, TRUE);
			NewStaticMeshComp->SetMeshAsset(MeshAsset);

			// Load and assign material
			const EMaterialAsset* MatAsset = nullptr;
#if _EDITOR_ONLY
			EMaterialAssetManager::GetManagerSingleton()->LoadOrCompileMaterialAsset(
				EEngineSettings::ENGINE_MATERIAL_OUTPUT_DIR,
				"M_DefaultLit",
				EEngineSettings::ENGINE_MATERIAL_SOURCE_DIR,
				EEngineSettings::ENGINE_MATERIAL_SHADER_INCLUDE_DIR,
				MatAsset, TRUE);
#else
			EMaterialAssetManager::GetManagerSingleton()->LoadMaterialAsset(
				EString(EEngineSettings::ENGINE_MATERIAL_OUTPUT_DIR) + "M_DefaultLit/",
				"M_DefaultLit",
				MatAsset);
#endif
			if (MatAsset)
			{
				NewStaticMeshComp->SetMaterialAsset(MatAsset);
			}

			New->AddComponent(NewStaticMeshComp);		// TODO : Fix this that BeginAddToScene need later than material asset setting otherwise will be null for proxy

			New->SetActorLocation(Vector3(250.0f, 50.0f, 0.0f));
			New->SetActorScale(Vector3(100.0f, 100.0f, 100.0f));
		}

		this->GetWorld()->GetController()->SetActorLocation(Vector3(0.0f, 350.0f, -500.0f));
		this->GetWorld()->GetController()->SetActorRotation(MakeQuaternion(Euler(40.0f, 0.0f, 0.0f)));
    }

	void PLevelActor::UserEndPlay()
	{

	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{

	}


}
