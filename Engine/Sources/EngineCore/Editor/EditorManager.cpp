#include "EditorManager.h"
#include "EditorGlobalPanel.h"
#include <Base/RTTI/RTTIManager.h>
#include "PigeonBase/Object/World/WorldManager.h"
#include "AssetManager.h"
#include "EditorHelper.h"
#include "EditorLogManager.h"
#include <Asset/Editor/AssetImporter/ImportManager.h>
#include "PigeonBase/Object/Controller/Controller.h"
#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/Component/SceneComponent.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#include <imgui.h>

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
        m_WorldManager     = EWorldManager::GetManagerSingleton();
        m_AssetManager     = EAssetManager::GetManagerSingleton();
        m_LogsManager      = EEditorLogManager::GetManagerSingleton();
        m_ImportManager    = EImportManager::GetManagerSingleton();
        m_EditorGlobalPanel = EEditorGlobalPanel::GetManagerSingleton();
        m_LogsManager->EditorInit();
        m_AssetManager->EditorInit();
        m_WorldManager->EditorInit();
        m_ImportManager->EditorInit();

        PEditorController* EditorController = static_cast<PEditorController*>(m_WorldManager->GetWorld()->GetController());
        if (EditorController)
        {
            auto func = [this](IMouse::Event::EType Type, const Vector2& MousePos)
            {
                this->HandleMouseEvent(Type, MousePos);
            };
            TFunction<void(IMouse::Event::EType, const Vector2&)> OnMouseBroadcast = func;
            EditorController->OnMouseEvent.Add(OnMouseBroadcast);
        }
    }

    void EEditorManager::HandleMouseEvent(IMouse::Event::EType Type, const Vector2& MousePos)
    {
        PWorld* World = m_WorldManager->GetWorld();
        if (!World) { return; }

        PController* PC = World->GetController();
        if (!PC || !PC->GetCamera()) { return; }

        const ERect ScreenRect(0, 0,
            PigeonEngine::EEngineSettings::ENGINE_SCREEN_WIDTH,
            PigeonEngine::EEngineSettings::ENGINE_SCREEN_HEIGHT);

        // Block clicks when ImGui owns the mouse, but never block an active drag
        if (ImGui::GetIO().WantCaptureMouse && !m_bDragging) { return; }

        if (Type == IMouse::Event::EType::LPress)
        {
            m_LastMousePos = MousePos;

            // Try gizmo hit first (only if something is selected)
            if (m_SelectedComponent)
            {
                Vector3 DragOrigin;
                EGizmoAxis HitAxis = HitTestTranslationGizmo(
                    PC->GetCamera(),
                    m_SelectedComponent,
                    MousePos.x, MousePos.y,
                    ScreenRect,
                    DragOrigin
                );
                if (HitAxis != EGizmoAxis::NONE)
                {
                    m_DragAxis        = HitAxis;
                    m_DragOriginWorld = DragOrigin;
                    m_ComponentOrigin = m_SelectedComponent->GetComponentWorldLocation();
                    m_bDragging       = TRUE;
                    return; // consumed by gizmo
                }
            }

            // Actor selection
            TArray<PActor*> AllActors = World->GetAllActors();
            PActor* ClickedActor = SelectObjectInViewport(
                PC->GetCamera(), AllActors,
                MousePos.x, MousePos.y, ScreenRect
            );
            if (ClickedActor)
            {
                World->SetSelectedActor(ClickedActor);
                m_SelectedComponent = ClickedActor->GetRootComponent();
            }
            else
            {
                World->SetSelectedActor(nullptr);
                m_SelectedComponent = nullptr;
            }
        }
        else if (Type == IMouse::Event::EType::Move)
        {
            if (m_bDragging && m_SelectedComponent)
            {
                Vector3 Delta = ComputeGizmoDragDelta(
                    PC->GetCamera(),
                    m_SelectedComponent,
                    m_DragAxis,
                    m_DragOriginWorld,
                    MousePos.x, MousePos.y,
                    ScreenRect
                );
                m_SelectedComponent->SetComponentWorldLocation(m_ComponentOrigin + Delta);
            }
            m_LastMousePos = MousePos;
        }
        else if (Type == IMouse::Event::EType::LRelease)
        {
            m_bDragging = FALSE;
            m_DragAxis  = EGizmoAxis::NONE;
        }
    }

    void EEditorManager::EditorUpdate()
    {
        m_WorldManager->EditorUpdate();
        m_AssetManager->EditorUpdate();
        m_LogsManager->EditorUpdate();
        m_ImportManager->EditorUpdate();
        if (m_EditorGlobalPanel)
        {
            m_EditorGlobalPanel->DrawResolutionPanel();
        }

        // Draw bounds for all actors
        TArray<PActor*> AllActors = m_WorldManager->GetWorld()->GetAllActors();
        DrawObjectBounds(AllActors);

        // Draw translation gizmo for selected component
        if (m_SelectedComponent)
        {
            PController* PC = m_WorldManager->GetWorld()->GetController();
            if (PC && PC->GetCamera())
            {
                DrawTranslationGizmo(PC->GetCamera(), m_SelectedComponent);
            }
        }
    }
#endif
}
