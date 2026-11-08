#include "EditorManager.h"
#include "EditorGlobalPanel.h"
#include "EditorProfilerPanel.h"
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
#if ENGINE_EDITOR_ONLY
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
		LogsManager->ShutDown();
		AssetManager->ShutDown();
		ImportManager->ShutDown();
	}

	void EEditorManager::Init()
	{
	}

	void EEditorManager::Uninit()
	{
	}

	void EEditorManager::EditorInit()
	{
		WorldManager     = EWorldManager::GetManagerSingleton();
		AssetManager     = EAssetManager::GetManagerSingleton();
		LogsManager      = EEditorLogManager::GetManagerSingleton();
		ImportManager    = EImportManager::GetManagerSingleton();
		EditorGlobalPanel = EEditorGlobalPanel::GetManagerSingleton();
		EditorProfilerPanel = EEditorProfilerPanel::GetManagerSingleton();
		LogsManager->EditorInit();
		AssetManager->EditorInit();
		WorldManager->EditorInit();
		ImportManager->EditorInit();

		PEditorController* EditorController = static_cast<PEditorController*>(WorldManager->GetWorld()->GetController());
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
		PWorld* World = WorldManager->GetWorld();
		if (!World) { return; }

		PController* PC = World->GetController();
		if (!PC || !PC->GetCamera()) { return; }

		const ERect ScreenRect(0, 0,
			PigeonEngine::EEngineSettings::ENGINE_SCREEN_WIDTH,
			PigeonEngine::EEngineSettings::ENGINE_SCREEN_HEIGHT);

		// Block clicks when ImGui owns the mouse, but never block an active drag
		if (ImGui::GetIO().WantCaptureMouse && !bDragging) { return; }

		if (Type == IMouse::Event::EType::LPress)
		{
			LastMousePos = MousePos;

			// Try gizmo hit first (only if something is selected)
			if (SelectedComponent)
			{
				Vector3 DragOrigin;
				EGizmoAxis HitAxis = HitTestTranslationGizmo(
					PC->GetCamera(),
					SelectedComponent,
					MousePos.x, MousePos.y,
					ScreenRect,
					DragOrigin
				);
				if (HitAxis != EGizmoAxis::NONE)
				{
					DragAxis        = HitAxis;
					DragOriginWorld = DragOrigin;
					ComponentOrigin = SelectedComponent->GetComponentWorldLocation();
					bDragging       = TRUE;
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
				SelectedComponent = ClickedActor->GetRootComponent();
			}
			else
			{
				World->SetSelectedActor(nullptr);
				SelectedComponent = nullptr;
			}
		}
		else if (Type == IMouse::Event::EType::Move)
		{
			if (bDragging && SelectedComponent)
			{
				Vector3 Delta = ComputeGizmoDragDelta(
					PC->GetCamera(),
					SelectedComponent,
					DragAxis,
					DragOriginWorld,
					MousePos.x, MousePos.y,
					ScreenRect
				);
				SelectedComponent->SetComponentWorldLocation(ComponentOrigin + Delta);
			}
			LastMousePos = MousePos;
		}
		else if (Type == IMouse::Event::EType::LRelease)
		{
			bDragging = FALSE;
			DragAxis  = EGizmoAxis::NONE;
		}
	}

	void EEditorManager::EditorUpdate()
	{
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		WorldManager->EditorUpdate();
		AssetManager->EditorUpdate();
		LogsManager->EditorUpdate();
		ImportManager->EditorUpdate();
		if (EditorGlobalPanel)
		{
			EditorGlobalPanel->DrawDisplaySettings();
		}
		if (EditorProfilerPanel)
		{
			EditorProfilerPanel->DrawPanel();
		}

		// Draw bounds for all actors
		TArray<PActor*> AllActors = WorldManager->GetWorld()->GetAllActors();
		DrawObjectBounds(AllActors);

		// Draw translation gizmo for selected component
		if (SelectedComponent)
		{
			PController* PC = WorldManager->GetWorld()->GetController();
			if (PC && PC->GetCamera())
			{
				DrawTranslationGizmo(PC->GetCamera(), SelectedComponent);
			}
		}
	}
#endif
}
