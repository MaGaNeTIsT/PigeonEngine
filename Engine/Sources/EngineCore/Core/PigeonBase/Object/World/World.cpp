#include "World.h"
#include "../../../../Main/MainManager.h"
#include <Renderer/RenderInterface.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include "PigeonBase/Object/Actor/LevelActor.h"
#include "PigeonBase/Object/Controller/Controller.h"

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PWorld, PObject>();
        // RegisterClassType<ESceneTreeNode, TOctreeNode<PActor*>>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PWorld::PWorld()
    {
        this->RenderScene = nullptr;
    }
    PWorld::~PWorld()
    {
    }

    void PWorld::Init()
    {
        if(Controller)
        {
            Controller->Destroy();
            Controller = nullptr;
        }
        if(LevelScriptor)
        {
            LevelScriptor->Destroy();
            LevelScriptor = nullptr;
        }
        if(RootActor)
        {
            RootActor->Destroy();
            RootActor = nullptr;
        }
		this->RootActor = new PActor();
        this->RootActor->SetIsTickable(TRUE);
		this->RootActor->BeginAddedToScene(this);
        this->RootActor->Init();

        this->Controller = new PController();
        this->Controller->SetIsTickable(TRUE);
        this->Controller->SetName("Controller");
        this->AddActor(Controller);
        // this->Controller->BeginAddedToScene(this);
        this->Controller->Init();

		this->LevelScriptor = new PLevelActor();
        this->LevelScriptor->SetIsTickable(TRUE);
		this->LevelScriptor->BeginAddedToScene(this);
        this->LevelScriptor->Init();
    }

    void PWorld::Uninit()
    {
        // PObject::Uninit();
        this->Destroy();
    }

    void PWorld::Tick(FLOAT deltaTime)
    {
#if _EDITOR_ONLY
        EditorTick(deltaTime);
        return;
#endif
        FixTick(deltaTime);
        
    }

    void PWorld::EditorTick(FLOAT deltaTime)
    {
        RootActor->EditorTick(deltaTime);
        LevelScriptor->EditorTick(deltaTime);
        Controller->EditorTick(deltaTime);
    }

    void PWorld::FixTick(FLOAT deltaTime)
    {
        RootActor->FixedTick(deltaTime);
        LevelScriptor->FixedTick(deltaTime);
        Controller->FixedTick(deltaTime);
    }


    void PWorld::Destroy()
    {
        if(Controller)
        {
            Controller->Destroy();
            Controller = nullptr;
        }
        if(LevelScriptor)
        {
            LevelScriptor->Destroy();
            LevelScriptor = nullptr;
        }
        if(RootActor)
        {
            RootActor->Destroy();
            RootActor = nullptr;
        }
       

        PObject::Destroy();
    }

    void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
    {
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor != nullptr);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding world root to world.", NewActor != RootActor);
        
        
        NewActor->AttachToActor(RootActor);
        NewActor->GetRootComponent()->SetComponentWorldTransform(Trans);
        NewActor->BeginAddedToScene(this);
    }

  
#if _EDITOR_ONLY
    void PWorld::AddSceneToWorld(PScene* NewScene)
    {
        Scenes.Add(NewScene);
    }

    void PWorld::RemoveSceneFromWorld(PScene* Scene)
    {
        Scenes.Remove(Scene);
    }

    void PWorld::SetCurrentScene(PScene* Scene)
    {
        CurrentScene = Scene;
    }
#endif

	PigeonEngine::PController* PWorld::GetController() const
	{
        return this->Controller;
	}

    const EGameTimer* PWorld::GetGameTimer() const
    {
        return EMainManager::GetManagerSingleton()->GetGameTimer();;
    }

    //Render scene state START
    void PWorld::BindRenderScene(RSceneInterface* InScene)
    {
        if (RenderScene)
        {
            RenderScene->ClearAll();
            RenderScene = nullptr;
        }
        RenderScene = InScene;
    }
    RSceneInterface* PWorld::GetRenderScene()
    {
        return RenderScene;
    }
    const RSceneInterface* PWorld::GetRenderScene()const
    {
        return RenderScene;
    }
#if _EDITOR_ONLY
    static Quaternion Quat;
    static Vector3 Scale;
    static Vector3 Location;
    static FLOAT LocX;
    static FLOAT LocY;
    static FLOAT LocZ;
    
    void PWorld::GenerateWorldOutline()
    {
        
        
        for(const auto& elem : RootActor->GetAllActorsAttached())
        {
            if(elem->GenerateImgui(elem == ImguiSelectedActor))
            {
                if(elem == ImguiSelectedActor)
                {
                    continue;
                }
                ImguiSelectedActor = elem;
                // Location =  ImguiSelectedActor->GetActorLocation();
                // LocX = Location.x;
                // LocY = Location.y;
                // LocZ = Location.z;
                // Quat     =  ImguiSelectedActor->GetActorRotation();
                // Scale    =  ImguiSelectedActor->GetActorScale();
            }
        }

    }

    void PWorld::GenerateDetail()
    {
        ImGui::Text("Detail");
        if(ImguiSelectedActor)
        {
            ImguiSelectedActor->GenerateComponentsImgui();

            static bool inputs_step = true;
            const double f64_one = 1.0f;
            
            {
                ImGui::Text("Location:");
                BOOL8 bLocationChanged = FALSE;
                BOOL8 bLocationChangedX = ImGui::DragScalar("X", ImGuiDataType_Float, &LocX, 0.0005f, NULL, NULL, "%.6f");
               // ImGui::SameLine(); 
                BOOL8 bLocationChangedY = ImGui::DragScalar("Y", ImGuiDataType_Float, &LocY, 0.0005f, NULL, NULL, "%.6f");
               // ImGui::SameLine(); 
                BOOL8 bLocationChangedZ = ImGui::DragScalar("Z", ImGuiDataType_Float, &LocZ, 0.0005f, NULL, NULL, "%.6f");
                bLocationChanged = bLocationChangedX || bLocationChangedY || bLocationChangedZ;
                if(bLocationChanged)
                {
                   //ImguiSelectedActor->SetActorLocation(Location);
                }
            }
           
            // {
            //     ImGui::Text("Rotation:");
            //     BOOL bRotationChanged = ImGui::InputScalar("X",  ImGuiDataType_Double, &Quat.x, inputs_step ? &f64_one : NULL);
            //     ImGui::SameLine(); 
            //     bRotationChanged = bRotationChanged || ImGui::InputScalar("Y",  ImGuiDataType_Double, &Quat.y, inputs_step ? &f64_one : NULL);
            //     ImGui::SameLine(); 
            //     bRotationChanged = bRotationChanged || ImGui::InputScalar("Z",  ImGuiDataType_Double, &Quat.z, inputs_step ? &f64_one : NULL);
            //     ImGui::SameLine(); 
            //     bRotationChanged = bRotationChanged || ImGui::InputScalar("W",  ImGuiDataType_Double, &Quat.w, inputs_step ? &f64_one : NULL);
            //     if(bRotationChanged)
            //     {
            //         ImguiSelectedActor->SetActorRotation(Quat);
            //     }
            // }
            //
            // {
            //     ImGui::Text("Scale:");
            //     BOOL bScaleChanged = ImGui::InputScalar("X",  ImGuiDataType_Double, &Scale.x, inputs_step ? &f64_one : NULL);
            //     ImGui::SameLine(); 
            //     bScaleChanged = bScaleChanged || ImGui::InputScalar("Y",  ImGuiDataType_Double, &Scale.y, inputs_step ? &f64_one : NULL);
            //     ImGui::SameLine(); 
            //     bScaleChanged = bScaleChanged || ImGui::InputScalar("Z",  ImGuiDataType_Double, &Scale.z, inputs_step ? &f64_one : NULL);
            //     if(bScaleChanged)
            //     {
            //         ImguiSelectedActor->SetActorScale(Scale);
            //     }
            // }
          

         
        }
    }
#endif
};