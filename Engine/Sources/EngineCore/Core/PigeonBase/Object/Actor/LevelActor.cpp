#include "LevelActor.h"
#include "../Component/SceneComponent.h"
#include "../Component/Primitive/StaticMeshComponent.h"
#include "../World/World.h"
#include "MeshAsset/MeshAsset.h"
#include "../Controller/Controller.h"
#include "Base/Timer/Timer.h"
#include "PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h"

namespace PigeonEngine
{
    PLevelActor::PLevelActor()
    {
       
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
    	
    		PStaticMeshComponent* NewStaticMeshComp = new PStaticMeshComponent();
    		NewStaticMeshComp->SetIsTickable(TRUE);
    		New->AddComponent(NewStaticMeshComp);

    		const EStaticMeshAsset* Asset = nullptr;
    		EString ImportPath("./Engine/Assets/EngineModels/SceneModels/Camera/");
    		EString ImportName("Camera");
    		EString ImportFileType("obj");
    		TryLoadStaticMesh(ESettings::ENGINE_MESH_PATH, "CameraMesh", Asset, &ImportPath, &ImportName, &ImportFileType);

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
    		LightComp->SetLightIntensity(1000.f);
    		New->AddComponent(LightComp);
    		LightComp->SetName("DirectLightComponent");
    		Quaternion Quat = Quaternion(0.5,0.5,0,1);
    		Quat.Normalize();
    		New->SetActorRotation(Quat);
    	}
		this->GetWorld()->GetController()->SetActorLocation(Vector3(0, 0, -10));
    }

	void PLevelActor::UserEndPlay()
	{
		
	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{
    	
    	FLOAT t = EMath::Sin(this->GetWorld()->GetGameTimer()->GetClockTime());
		Vector3 newPos = Vector3(-10.f * t -10.f * t, -10.f * t);
    	this->GetWorld()->GetController()->SetActorLocation(newPos);
	}


}
