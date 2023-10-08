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
        this->RootActor->SetActorScale(Vector3(1,1,1));
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

    static FLOAT QuatX;
    static FLOAT QuatY;
    static FLOAT QuatZ;
    static FLOAT QuatW;

	static FLOAT ScaleX;
	static FLOAT ScaleY;
	static FLOAT ScaleZ;
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

                QuatX = ImguiSelectedActor->GetActorRotation().x;
                QuatY = ImguiSelectedActor->GetActorRotation().y;
                QuatZ = ImguiSelectedActor->GetActorRotation().z;
                QuatW = ImguiSelectedActor->GetActorRotation().w;

                ScaleX = ImguiSelectedActor->GetActorScale().x;
                ScaleY = ImguiSelectedActor->GetActorScale().y;
                ScaleZ = ImguiSelectedActor->GetActorScale().z;
            }
        }

    }

    void PWorld::GenerateDetail()
    {
        
        if(ImguiSelectedActor)
        {
            ImguiSelectedActor->GenerateComponentsImgui();

            static bool inputs_step = true;
            const double f64_one = 1.0f;
            
            {
                ImGui::Text("Location:");
                const BOOL8 bChangedX = ImGui::DragScalar("LocX", ImGuiDataType_Float, &LocX, 0.0005f, NULL, NULL, "%.6f");
                const BOOL8 bChangedY = ImGui::DragScalar("LocY", ImGuiDataType_Float, &LocY, 0.0005f, NULL, NULL, "%.6f"); 
                const BOOL8 bChangedZ = ImGui::DragScalar("LocZ", ImGuiDataType_Float, &LocZ, 0.0005f, NULL, NULL, "%.6f");
                if(bChangedX || bChangedY || bChangedZ)
                {
                   ImguiSelectedActor->SetActorLocation(Vector3(LocX, LocY, LocZ));
                }
				ImGui::Text("Real World Location for debug : ");
				ImGui::Text(*(ImguiSelectedActor->GetActorWorldLocation().AsString()));
            }
            
            {
                ImGui::Text("Rotation:");
                const BOOL8 bChangedX = ImGui::DragScalar("QuatX", ImGuiDataType_Float, &QuatX, 0.0005f, NULL, NULL, "%.6f");
                const BOOL8 bChangedY = ImGui::DragScalar("QuatY", ImGuiDataType_Float, &QuatY, 0.0005f, NULL, NULL, "%.6f");
                const BOOL8 bChangedZ = ImGui::DragScalar("QuatZ", ImGuiDataType_Float, &QuatZ, 0.0005f, NULL, NULL, "%.6f");
                const BOOL8 bChangedW = ImGui::DragScalar("QuatW", ImGuiDataType_Float, &QuatW, 0.0005f, NULL, NULL, "%.6f");

                if (bChangedX || bChangedY || bChangedZ || QuatW)
                {
                    ImguiSelectedActor->SetActorRotation(Quaternion(QuatX, QuatY, QuatZ, QuatW));
                }

            }
            
			{
				ImGui::Text("Scale:");
				BOOL8 bChangedX = ImGui::DragScalar("ScaleX", ImGuiDataType_Float, &ScaleX, 0.0005f, NULL, NULL, "%.6f");
				BOOL8 bChangedY = ImGui::DragScalar("ScaleY", ImGuiDataType_Float, &ScaleY, 0.0005f, NULL, NULL, "%.6f");
				BOOL8 bChangedZ = ImGui::DragScalar("ScaleZ", ImGuiDataType_Float, &ScaleZ, 0.0005f, NULL, NULL, "%.6f");
				
                if (bChangedX || bChangedY || bChangedZ)
				{
					ImguiSelectedActor->SetActorScale(Vector3(ScaleX, ScaleY, ScaleZ));
				}
				ImGui::Text("Real World Scale for debug : ");
				ImGui::Text(*(ImguiSelectedActor->GetActorWorldScale().AsString()));

			}
        }
    }
#endif
};