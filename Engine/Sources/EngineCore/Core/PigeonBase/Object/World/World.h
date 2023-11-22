#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Pointer/SharedPtr.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Object.h>
#include <PigeonBase/Object/Actor.h>

namespace PigeonEngine
{

    class PScene;
    class EGameTimer;
    class PActorComponent;
    class PStaticMeshComponent;
    class PSkeletalMeshComponent;
    class RSceneInterface;
    class PLevelActor;
    enum EWorldType : UINT8
    {
        EWT_GAME,
        EWT_EDITOR,
        EWT_RUN_IN_EDITOR, // maybe useless
    };
    
    /*
     * actual world in runtime
     */
    class PWorld final : public PObject
    {
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PWorld)
        
    // Begin PObject Interface
    public:
        void	Init() override;
        void	Uninit() override;
        void	Tick(FLOAT deltaTime) override;
    protected:

        void FixTick(FLOAT deltaTime);
    public:
        void Destroy() override; 
    // End PObject Interface

    // Actors in world management
    public:
        PE_NODISCARD const PActor*          GetActorByUniqueID(const ObjectIdentityType& UniqueID, const BOOL8& bIncludeChildActor) const ;
        PE_NODISCARD const PActorComponent* GetComponentByUniqueID(const ObjectIdentityType& UniqueID, const BOOL8& bIncludeChildComponent) const ;
        void AddActor(PActor* NewActor, const ETransform& Trans = ETransform());
        //void DestroyActor();
    private:
        PLevelActor* RootActor = nullptr;
        TSet<PActor*> AllActors;
#if _EDITOR_ONLY
    public:
        void AddSceneToWorld(PScene* NewScene);
        void RemoveSceneFromWorld(PScene* Scene);
        void SetCurrentScene(PScene* Scene);
    private:
        // Current operating scene.(in editor)
        PScene* CurrentScene    = nullptr;
        // Like unreal's persistent scene.
        PScene* PersistentScene = nullptr;
        TArray<PScene*> Scenes;
#endif

    public:
        class PController* GetController()const;
    private:
        class PController* Controller = nullptr;
    public:
        const EGameTimer* GetGameTimer() const;
  
        //Render scene state START
    public:
        void                    BindRenderScene(RSceneInterface* InScene);
        RSceneInterface*        GetRenderScene();
        const RSceneInterface*  GetRenderScene()const;
    private:
        RSceneInterface*    RenderScene = nullptr;
        //Render scene state END

#if _EDITOR_ONLY
        // IMGUI
    public:
        void SetSelectedActor(PActor* Selected);
        void SetSelectedComponent(PActorComponent* Selected);
    public:
        void GenerateWorldOutline();
        void GenerateDetail();
    private:
        PActor* ImguiSelectedActor = nullptr;
        PActorComponent* ImguiSelectedComponent = nullptr;
    protected:
        void EditorTick(FLOAT deltaTime) override;
    private:
        class PEditorController* EditorController = nullptr;
#endif
        
    };

   
};
