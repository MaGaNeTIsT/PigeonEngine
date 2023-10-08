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
    static FLOAT LocX = 0.0f;
    static FLOAT LocY = 0.0f;
    static FLOAT LocZ = 0.0f;

    static FLOAT ScaleX = 0.0f;
    static FLOAT ScaleY = 0.0f;
    static FLOAT ScaleZ = 0.0f;
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
                LocX = ImguiSelectedActor->GetActorLocation().x;
                LocY = ImguiSelectedActor->GetActorLocation().y;
                LocZ = ImguiSelectedActor->GetActorLocation().z;
                Quat     =  ImguiSelectedActor->GetActorRotation();
                ScaleX = ImguiSelectedActor->GetActorScale().x;
                ScaleY = ImguiSelectedActor->GetActorScale().y;
                ScaleZ = ImguiSelectedActor->GetActorScale().z;
            }
        }

    }

    void PWorld::GenerateDetail()
    {
        ImGui::Text("Detail");
        if(ImguiSelectedActor)
        {
            ImguiSelectedActor->GenerateComponentsImgui();

            
            {
                ImGui::Text("Location:");
                const BOOL8 bLocationChangedX = ImGui::DragScalar("LocX", ImGuiDataType_Float, &LocX, 0.05f, NULL, NULL, "X %.6f");
                const BOOL8 bLocationChangedY = ImGui::DragScalar("LocY", ImGuiDataType_Float, &LocY, 0.05f, NULL, NULL, "Y %.6f");
                const BOOL8 bLocationChangedZ = ImGui::DragScalar("LocZ", ImGuiDataType_Float, &LocZ, 0.05f, NULL, NULL, "Z %.6f");

                if(bLocationChangedX || bLocationChangedY || bLocationChangedZ)
                {
                    ImguiSelectedActor->SetActorLocation(Vector3(LocX, LocY, LocZ));
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

            {
                ImGui::Text("Scale:");
                const BOOL8 bScaleChangedX = ImGui::DragScalar("ScaleX", ImGuiDataType_Float, &ScaleX, 0.05f, NULL, NULL, "X %.6f");
                const BOOL8 bScaleChangedY = ImGui::DragScalar("ScaleY", ImGuiDataType_Float, &ScaleY, 0.05f, NULL, NULL, "Y %.6f");
                const BOOL8 bScaleChangedZ = ImGui::DragScalar("ScaleZ", ImGuiDataType_Float, &ScaleZ, 0.05f, NULL, NULL, "Z %.6f");
                if(bScaleChangedX || bScaleChangedY || bScaleChangedZ)
                {
                    ImguiSelectedActor->SetActorScale(Vector3(ScaleX, ScaleY, ScaleZ));
                }
            }

         
        }
    }
#endif
};