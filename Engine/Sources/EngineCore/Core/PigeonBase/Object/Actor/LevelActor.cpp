#include "LevelActor.h"
#include <Base/Timer/Timer.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include "../Component/SceneComponent.h"
#include "../World/World.h"
#include "../Controller/Controller.h"
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

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
    		New->SetName("MeshActor");
    		this->GetWorld()->AddActor(New);
            Vector3 S = New->GetActorWorldScale();
    		PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
    		NewStaticMeshComp->SetIsTickable(TRUE);
    		New->AddComponent(NewStaticMeshComp);
            S = New->GetActorWorldScale();
            New->SetActorScale(Vector3(10, 10, 10));
            S = New->GetActorWorldScale();
    		const EStaticMeshAsset* Asset = nullptr;
    		EString ImportPath("./Engine/Assets/EngineModels/SceneModels/Robot/");
    		EString ImportName("Robot");
    		EString ImportFileType("obj");
    		TryLoadStaticMesh(ESettings::ENGINE_MESH_PATH, "Robot", Asset, &ImportPath, &ImportName, &ImportFileType);

    		NewStaticMeshComp->SetMeshAsset(Asset);
    		NewStaticMeshComp->SetName("MeshComponent");
    	}
		//
    	
    	{
    		PActor* New = new PActor();
    		New->SetName("LightActor");

    		New->SetIsTickable(TRUE);
    		this->GetWorld()->AddActor(New);
			PDirectionalLightComponent* LightComp = new PDirectionalLightComponent();
            LightComp->SetLightColor(Color3(1.0f, 1.0f, 1.0f));
    		LightComp->SetLightIntensity(1.f);
    		New->AddComponent(LightComp);
            LightComp->SetName("DirectLightComponent");
            Quaternion Quat = Quaternion(0.5, 0.5, 0, 1);
            Quat.Normalize();
            New->SetActorRotation(Quat);
    	}

        {
            PActor* New = new PActor();
            New->SetName("SkyLightActor");

            New->SetIsTickable(TRUE);
            this->GetWorld()->AddActor(New);
            PSkyLightComponent* SkyLightComp = new PSkyLightComponent();

            const ETextureCubeAsset* CubeMap = nullptr;
            TArray<EString> ImportPaths;
            TArray<EString> ImportNames;
            TArray<EString> ImportFileTypes;
            for (UINT32 i = 0u; i < 6u; i++)
            {
                ImportPaths.Add("./Engine/Assets/EngineTextures/DefaultSkyBox/Sky_001/");
                ImportFileTypes.Add("png");
            }
            ImportNames.Add("Sky_001_Right");
            ImportNames.Add("Sky_001_Left");
            ImportNames.Add("Sky_001_Top");
            ImportNames.Add("Sky_001_Bottom");
            ImportNames.Add("Sky_001_Forward");
            ImportNames.Add("Sky_001_Back");
            TryLoadTextureCube(ESettings::ENGINE_TEXTURE_PATH, "SkyBox001", CubeMap, &ImportPaths, &ImportNames, &ImportFileTypes);

            SkyLightComp->SetCubeMapAsset(CubeMap);
            SkyLightComp->SetLightAdjust(Color3(1.f, 1.f, 1.f));
            SkyLightComp->SetIntensity(1.f);
            New->AddComponent(SkyLightComp);
            SkyLightComp->SetName("SkyLightComponent");
            New->SetActorScale(Vector3(1000.f, 1000.f, 1000.f));
        }

		this->GetWorld()->GetController()->SetActorLocation(Vector3(0, 0, -160));
    }

	void PLevelActor::UserEndPlay()
	{
		
	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{
    	
        FLOAT t = static_cast<FLOAT>(EMath::Sin(this->GetWorld()->GetGameTimer()->GetClockTime()));
		Vector3 newPos = Vector3(-10.f * t -10.f * t, -10.f * t);
    	this->GetWorld()->GetController()->SetActorLocation(newPos);
	}


}
