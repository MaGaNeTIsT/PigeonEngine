#include "EditorManager.h"
#include <Base/RTTI/RTTIManager.h>
#include "PigeonBase/Object/World/WorldManager.h"
#include "AssetManager.h"
#include "EditorHelper.h"
#include "EditorLogManager.h"
#include <Asset/Editor/AssetImporter/ImportManager.h>
#include "PigeonBase/Object/Controller/Controller.h"
#include "PigeonBase/Object/World/World.h"

namespace PigeonEngine
{
#if _EDITOR_ONLY
    static void RegisterClassTypes()
    {
        RegisterClassType<EEditorManager, EManagerBase>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);
    
    EEditorManager::EEditorManager()
    {
    }

    EEditorManager::~EEditorManager()
    {
		
    }

    void EEditorManager::Initialize()
    {

    }

    void EEditorManager::ShutDown()
    {
        m_LogsManager->ShutDown();
        m_AssetManager->ShutDown();
        m_ImportManager->ShutDown();
    }

    void EEditorManager::Init()
    {
        
    }

    void EEditorManager::Uninit()
    {
        
    }

    void EEditorManager::EditorInit()
    {
        m_WorldManager  = EWorldManager::GetManagerSingleton();
        m_AssetManager  = EAssetManager::GetManagerSingleton();
        m_LogsManager   = EEditorLogManager::GetManagerSingleton();
        m_ImportManager = EImportManager::GetManagerSingleton();
        m_LogsManager->EditorInit();
        m_AssetManager->EditorInit();
        m_WorldManager->EditorInit();
        m_ImportManager->EditorInit();

		PEditorController* EditorController = static_cast<PEditorController*>(m_WorldManager->GetWorld()->GetController());
		if (EditorController)
		{
			auto func = [this](IMouse::Event::EType Type, const Vector2& MousePos)
			{
				if (Type == IMouse::Event::EType::LPress)
				{
					TArray<PActor*> AllActors = this->m_WorldManager->GetWorld()->GetAllActors();
					PController* PC = this->m_WorldManager->GetWorld()->GetController();
                    PActor* ClickedActor = SelectObjectInViewport(
                        PC->GetCamera(),
                        AllActors,
                        MousePos.x,
                        MousePos.y,
                        ERect(0, 0, PigeonEngine::EEngineSettings::ENGINE_SCREEN_WIDTH, PigeonEngine::EEngineSettings::ENGINE_SCREEN_HEIGHT)
                    );
					if (ClickedActor)
					{
						this->m_WorldManager->GetWorld()->SetSelectedActor(ClickedActor);
					}
				}
			};
			TFunction<void(IMouse::Event::EType, const Vector2&)> OnMouseBroadcast = func;
			EditorController->OnMouseEvent.Add(OnMouseBroadcast);
		}
    }

    void EEditorManager::EditorUpdate()
    {
        BOOL8 TempEditorOpen = m_EditorOpen;
        ImGui::Begin("EditorRoot", &TempEditorOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Frame Rate Average %.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        
        m_EditorOpen = TempEditorOpen;
        
        m_WorldManager->EditorUpdate();
        m_AssetManager->EditorUpdate();
        m_LogsManager->EditorUpdate();
        m_ImportManager->EditorUpdate();
        BOOL8 bOpen = FALSE;
        ImGui::ShowDemoWindow(&bOpen);
        TArray<PActor*> AllActors = this->m_WorldManager->GetWorld()->GetAllActors();
        DrawObjectBounds(AllActors);
    }
#endif
}
