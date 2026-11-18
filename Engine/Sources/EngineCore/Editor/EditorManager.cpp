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
#include <RenderProxy/RenderSingletonObject.h>
#include "Navigation/NavigationManager.h"
#include <imgui.h>

namespace PigeonEngine
{
#if ENGINE_EDITOR_ONLY
	// The navmesh overlay. The manager caches both segment lists and only rebuilds them when a
	// provider is published, the draw mode changes or a tile was rebuilt. On a rebuild the
	// points go into a fixed size line list primitive once, and every frame is one DrawCustom
	// per list: a single draw call each, independent of the segment count.
	static void DrawNavigationDebug()
	{
		ENavigationManager* NavigationManager = ENavigationManager::GetManagerSingleton();
		if (!NavigationManager->IsDebugDrawEnabled())
		{
			return;
		}

		static const EString PolygonLineListName(PE_TEXT("NavigationDebug.PolygonLines"));
		static const EString TriangleLineListName(PE_TEXT("NavigationDebug.TriangleLines"));
		// Two vertices per segment, so this reserves 65536 segments per list: a 1.5 MB vertex
		// buffer and a 512 KB index buffer. It covers the demo scenes several times over; a
		// larger list is clamped by the primitive, which reports it once.
		PE_CONSTEXPR_STATIC UINT32 LineVertexCapacity = 131072u;
		static UINT32 AppliedSegmentsRevision = 0u;

		const TArray<Vector3>& PolygonSegments = NavigationManager->GetDebugLineSegments();
		const TArray<Vector3>& TriangleSegments = NavigationManager->GetDebugTriangleSegments();
		const UINT32 SegmentsRevision = NavigationManager->GetDebugSegmentsRevision();
		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();

		// The geometry only changes with the revision, so the upload is skipped on the frames in
		// between instead of pushing the whole mesh every frame.
		if (SegmentsRevision != AppliedSegmentsRevision)
		{
			if (PolygonSegments.Num<UINT32>() >= 2u)
			{
				WireframeManager->UpdateCustomLineListPrimitive(PolygonLineListName, LineVertexCapacity
					, PolygonSegments.GetData(), PolygonSegments.Num<UINT32>());
			}
			if (TriangleSegments.Num<UINT32>() >= 2u)
			{
				WireframeManager->UpdateCustomLineListPrimitive(TriangleLineListName, LineVertexCapacity
					, TriangleSegments.GetData(), TriangleSegments.Num<UINT32>());
			}
			AppliedSegmentsRevision = SegmentsRevision;
		}

		// The custom primitives are drawn in name order, not call order, so the two lists are
		// kept apart by height instead: the border sits two units above the triangulation and
		// wins the depth test on every edge they share.
		if ((NavigationManager->IsDebugTrianglesEnabled()) && (TriangleSegments.Num<UINT32>() >= 2u))
		{
			WireframeManager->DrawCustom(TriangleLineListName, Matrix4x4::Identity(), Color4::Blue());
		}
		if (PolygonSegments.Num<UINT32>() >= 2u)
		{
			WireframeManager->DrawCustom(PolygonLineListName
				, MakeMatrix4x4(Vector3(0.f, 2.f, 0.f), Quaternion::Identity(), Vector3(1.f, 1.f, 1.f))
				, Color4::Red());
		}
	}

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

		AttachToCurrentController();
	}

	void EEditorManager::AttachToCurrentController()
	{
		// (Re)subscribe HandleMouseEvent to the world controller's OnMouseEvent. Called
		// from EditorInit and from EditorUpdate: a changed controller pointer means the
		// previous subscription died with the previous controller's delegate, so only the
		// new controller needs a row here.
		PWorld* World = WorldManager ? WorldManager->GetWorld() : nullptr;
		PController* CurrentController = World ? World->GetController() : nullptr;
		if (!CurrentController || (CurrentController == AttachedController))
		{
			return;
		}
		AttachedController = CurrentController;
		auto func = [this](IMouse::Event::EType Type, const Vector2& MousePos)
		{
			this->HandleMouseEvent(Type, MousePos);
		};
		AttachedController->OnMouseEvent.Add(TFunction<void(IMouse::Event::EType, const Vector2&)>(func));
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

		// The world may have been re-initialized since the last frame, replacing the
		// controller our mouse subscription points at.
		AttachToCurrentController();

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

		// Draw the baked navigation mesh, if any volume published one.
		DrawNavigationDebug();

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
