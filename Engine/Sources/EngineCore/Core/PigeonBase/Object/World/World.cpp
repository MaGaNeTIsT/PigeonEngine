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
       
        if(RootActor)
        {
            RootActor->Destroy();
            RootActor = nullptr;
        }
		this->RootActor = new PLevelActor();
		
        this->Controller = new PController();
        this->Controller->SetIsTickable(TRUE);
        this->Controller->SetName("Controller");
        this->AddActor(Controller);
        // this->Controller->BeginAddedToScene(this);

        this->Controller->Init();
        this->RootActor->Init();
        this->RootActor->BeginAddedToScene(this);
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
        Controller->EditorTick(deltaTime);
    }

    void PWorld::FixTick(FLOAT deltaTime)
    {
        RootActor->FixedTick(deltaTime);
        Controller->FixedTick(deltaTime);
    }

    void PWorld::Destroy()
    {
        if(RootActor)
        {
            // RootActor->DestroyActorsAttached();
            RootActor->Destroy();
            RootActor = nullptr;
        }
        // AllActors.Clear();
        PObject::Destroy();
    }

    void PWorld::AddActor(PActor* NewActor, const ETransform& Trans)
    {
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding nullptr to world.", NewActor != nullptr);
		PE_CHECK(ENGINE_WORLD_ERROR, "PWorld::AddActor : Adding world root to world.", NewActor != RootActor);
        
        
        NewActor->AttachToActor(RootActor);
        NewActor->GetRootComponent()->SetComponentWorldTransform(Trans);
        NewActor->BeginAddedToScene(this);
        // AllActors.Add(NewActor);
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
				LocX = ImguiSelectedActor->GetActorLocation().x;
				LocY = ImguiSelectedActor->GetActorLocation().y;
				LocZ = ImguiSelectedActor->GetActorLocation().z;

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
                BOOL8 bLocationChangedX = ImGui::DragScalar("X", ImGuiDataType_Float, &LocX, 0.0005f, NULL, NULL, "%.6f");
                BOOL8 bLocationChangedY = ImGui::DragScalar("Y", ImGuiDataType_Float, &LocY, 0.0005f, NULL, NULL, "%.6f"); 
                BOOL8 bLocationChangedZ = ImGui::DragScalar("Z", ImGuiDataType_Float, &LocZ, 0.0005f, NULL, NULL, "%.6f");
                if(bLocationChangedX || bLocationChangedY || bLocationChangedZ)
                {
                   ImguiSelectedActor->SetActorLocation(Vector3(LocX, LocY, LocZ));
                }
				ImGui::Text("RealLocation for debug : ");

                ImGui::Text(*(ImguiSelectedActor->GetActorLocation().AsString()));
            }
        }
    }
#endif
};