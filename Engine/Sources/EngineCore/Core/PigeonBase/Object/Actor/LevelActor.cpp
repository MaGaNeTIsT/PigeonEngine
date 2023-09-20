﻿#include "LevelActor.h"
#include "../Component/SceneComponent.h"
#include "../Component/Primitive/StaticMeshComponent.h"
#include "../World/World.h"
#include "MeshAsset/MeshAsset.h"
#include "../Controller/Controller.h"
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
		
		PActor* New = new PActor();
    	New->SetIsTickable(TRUE);
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
		
		this->GetWorld()->GetController()->SetActorLocation(Vector3(0, 0, -500));
    }

	void PLevelActor::UserEndPlay()
	{
		
	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{

	}


}