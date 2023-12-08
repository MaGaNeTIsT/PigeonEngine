#include "LevelActor.h"
#include <Base/Timer/Timer.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include "../Component/SceneComponent.h"
#include "../World/World.h"
#include "../Controller/Controller.h"
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>

//Physics Test
#include <PhysicsTest/PhysicsTest.h>
#include <PigeonBase/Object/Component/Physics/PhysicsComponent.h>
#include "../../../../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"
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
    	// add a static mesh actor
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

			New->SetActorLocation(Vector3(0.0f, 0.0f, 0.0f));
			PE_LOG_LOG(NewStaticMeshComp->GetComponentWorldLocation().AsString());
			PE_LOG_LOG(New->GetBounds().AsString());
			New->SetActorLocation(Vector3(100.0f, 0.0f, 0.0f));
			PE_LOG_LOG(New->GetBounds().AsString());

		}
		// // Cube
	 //    {
		// 	PActor* New = new PActor();
		// 	New->SetIsTickable(TRUE);
		// 	POBJ_DEBUGNAME_SET(New, "StaticMeshActor");
		// 	this->GetWorld()->AddActor(New);
		// 	PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
		// 	NewStaticMeshComp->SetIsTickable(TRUE);
		// 	New->AddComponent(NewStaticMeshComp);
		// 	const EStaticMeshAsset* Asset = nullptr;
		// 	EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
		// 	EString ImportPath(AssetBasePath + "EngineModels/SceneModels/Robot/");
		// 	EString ImportName("Robot");
		// 	EString ImportFileType("obj");
		// 	TryLoadStaticMesh(EBaseSettings::ENGINE_MESH_PATH, "Robot", Asset, &ImportPath, &ImportName, &ImportFileType, TRUE);
	 //
		// 	NewStaticMeshComp->SetMeshAsset(Asset);
	 //
		// 	New->SetActorLocation(Vector3(100.0f, 0.0f, 0.0f));
	 //    }
		
         //add a skeletal mesh actor
		 //{
		 //	PActor* New = new PActor();
		 //	New->SetIsTickable(TRUE);
		 //	POBJ_DEBUGNAME_SET(New, "SkeletalMeshActor");
		 //	this->GetWorld()->AddActor(New);
		 //	PSkeletalMeshComponent* NewSkeletalMeshComp = new PSkeletalMeshComponent();
		 //	NewSkeletalMeshComp->SetIsTickable(TRUE);
		 //	New->AddComponent(NewSkeletalMeshComp);
		
		 //	const ESkinnedMeshAsset* MeshAsset = nullptr;
		 //	const ESkeletonAsset* SkeletonAsset = nullptr;
		 //	EString ImportPath("./Engine/Assets/EngineModels/SceneModels/UnrealCharacter/");
		 //	EString ImportName("SK_Mannequin_UE4_WithWeapon");
		 //	EString ImportFileType("FBX");
		 //	EString MeshAssetName("SK_Mannequin_UE4_WithWeapon_Mesh");
		 //	EString SkeletonAssetName("SK_Mannequin_UE4_WithWeapon_Skeleton");
		 // EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
		 //	//EString ImportPath(AssetBasePath + "EngineModels/SceneModels/D.Va/");
		 //	//EString ImportName("Model");
		 //	//EString ImportFileType("FBX");
		 //	//EString MeshAssetName("DVa_Mesh");
		 //	//EString SkeletonAssetName("DVa_Skeleton");
		 //	TryLoadSkinnedMesh(EEngineSettings::ENGINE_MESH_PATH, MeshAssetName, MeshAsset, &ImportPath, &ImportName, &ImportFileType, TRUE);
		 //	TryLoadSkeleton(EEngineSettings::ENGINE_SKELETON_PATH, SkeletonAssetName, SkeletonAsset, &ImportPath, &ImportName, &ImportFileType);
		
		 //	NewSkeletalMeshComp->SetMeshAsset(MeshAsset);
		 //	NewSkeletalMeshComp->SetSkeletonAsset(SkeletonAsset);
		 //	{
		 //		Quaternion DefaultRot(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.0f)));
		 //		NewSkeletalMeshComp->GetBoneMemoryPool().SetRootTransform(Vector3::Zero(), DefaultRot, Vector3::One());
		 //		NewSkeletalMeshComp->GetBoneMemoryPool().GenerateFromSkeleton(SkeletonAsset->GetStoragedResource());
		 //	}
		
		 //	New->SetActorLocation(Vector3(-100.0f, -90.0f, 0.0f));
		 //}
		{
		
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
			New->SetActorScale(Vector3(1000.0f, 1000.0f, 1000.0f));
		}

        {
            
        }

		// add a Physics CharacterTest Actor
		{
			PPhysicsTestCharacter* New = new PPhysicsTestCharacter();
			New->SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(New, "Physics Test Character");
			FCharacterSettings* Settings = new FCharacterSettings();
			Settings->Layer = Layers::MOVING;
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
		 		Quaternion DefaultRot(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.0f)));
		 		NewSkeletalMeshComp->GetBoneMemoryPool().SetRootTransform(Vector3::Zero(), DefaultRot, Vector3::One());
		 		NewSkeletalMeshComp->GetBoneMemoryPool().GenerateFromSkeleton(SkeletonAsset->GetStoragedResource());
		 	}

			New->SkeletalMeshComponent = NewSkeletalMeshComp;

			//New->StaticMeshComponent = new PStaticMeshComponent();
			//New->StaticMeshComponent->SetIsTickable(TRUE);
			//const EStaticMeshAsset* Asset = nullptr;
			//EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			//EString ImportPath(AssetBasePath + "EngineModels/SceneModels/Robot/");
			//EString ImportName("Robot");
			//EString ImportFileType("obj");
			//TryLoadStaticMesh(EEngineSettings::ENGINE_MESH_PATH, "Robot", Asset, &ImportPath, &ImportName, &ImportFileType, TRUE);
			//New->StaticMeshComponent->SetMeshAsset(Asset);
			New->AddComponent(New->SkeletalMeshComponent, ETransform());

			New->SetActorLocation(Vector3(-20.0f, 0.0f, 50.0f));
		}

		{
			PActor* New = new PActor();
			POBJ_DEBUGNAME_SET(New, "Plane Actor");

			New->SetIsTickable(TRUE);
			this->GetWorld()->AddActor(New);

			FBoxShape* Shape = new FBoxShape(Vector3(500.f, 10.f, 500.f), 0.f);
			PPhysicsComponent* PhysicsComponent = new PPhysicsComponent(Shape);
			PhysicsComponent->SetOwnerActor(New);
			New->SetActorLocation(Vector3(0.0f, -10.0f, 0.0f));
			PhysicsComponent->InitPhysicsComponent();
			New->AddComponent(PhysicsComponent);

		}

		this->GetWorld()->GetController()->SetActorLocation(Vector3(0.0f, 0.0f, -200.0f));
    }

	void PLevelActor::UserEndPlay()
	{
		
	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{
    	
        //FLOAT t = static_cast<FLOAT>(EMath::Sin(this->GetWorld()->GetGameTimer()->GetClockTime()));
        //Vector3 newPos = Vector3(-10.0f * t - 10.0f * t, -10.0f * t);
        //this->GetWorld()->GetController()->SetActorLocation(newPos);
	}


}
