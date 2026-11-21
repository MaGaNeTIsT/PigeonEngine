#include <Editor/Manager/EditorManager.h>
#include <Editor/Panels/EditorGlobalPanel.h>
#include <Editor/Panels/EditorProfilerPanel.h>
#include <Base/RTTI/RTTIManager.h>
#include "PigeonBase/Object/World/WorldManager.h"
#include <Editor/Asset/AssetManager.h>
#include <Editor/Manager/EditorHelper.h>
#include <Development/EditorLogManager.h>
#include <Asset/Editor/AssetImporter/ImportManager.h>
#include "PigeonBase/Object/Controller/Controller.h"
#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/Component/SceneComponent.h"
#include "PigeonBase/Object/Component/ActorComponent.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#include <Editor/Panels/PropertyEditor.h>
#include <Editor/Panels/WorldDetailsPanel.h>
#include <Editor/Manager/EditorWorldContext.h>
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
		EPropertyEditor::SetPropertyCommittedCallback(EPropertyEditor::FOnPropertyCommitted());
		WorldCommands.Clear();
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

		// One registration point for the per-class details editors: everything registered
		// here draws on top of the reflection grid for its class chain.
		EWorldDetailsPanel::RegisterBuiltinCustomEditors();

		// One registration point for every reflected edit: the Details panel applies the value
		// and hands the before/after pair here, which records the undo command.
		EPropertyEditor::SetPropertyCommittedCallback([this](void* InObject, const size_t& InClassHash,
			const EString& InPropertyName, const FPropertyValue& InBefore, const FPropertyValue& InAfter) -> void
		{
			if (!InObject)
			{
				return;
			}
			EObjectBase* Object = static_cast<EObjectBase*>(InObject);
			const BOOL32 bIsComponent = (dynamic_cast<PActorComponent*>(Object) != nullptr) ? TRUE : FALSE;
			PushSetPropertyCommand(WorldCommands, MakeObjectHandle(Object), bIsComponent, InClassHash,
				InPropertyName, InBefore, InAfter);
		});

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
					DragComponentHandle = MakeObjectHandle(SelectedComponent);
					DragStartTransform = SelectedComponent->GetTransform();
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
				EEditorWorldContext::SetSelectedActor(World, ClickedActor);
				SelectedComponent = dynamic_cast<PSceneComponent*>(EEditorWorldContext::GetSelectedComponent(World));
			}
			else
			{
				EEditorWorldContext::SetSelectedActor(World, nullptr);
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
				// Record a gizmo move as one undoable property edit. The command stores the
				// local transform, which is what the Transform property serializes.
				if (bDragging && DragComponentHandle.IsValid() && SelectedComponent && WorldManager->GetWorld())
				{
					PActorComponent* Resolved = WorldManager->GetWorld()->ResolveComponent(DragComponentHandle);
					if (Resolved == SelectedComponent)
					{
						const Vector3 Moved = SelectedComponent->GetComponentWorldLocation() - ComponentOrigin;
						if (Moved.LengthSquare() > PE_SMALL_NUMBER)
						{
							FPropertyValue Before;
							Before.Type = EPropertyValueType::TRANSFORM;
							Before.TransformValue = DragStartTransform;
							FPropertyValue After;
							After.Type = EPropertyValueType::TRANSFORM;
							After.TransformValue = SelectedComponent->GetTransform();
							PushSetPropertyCommand(WorldCommands, DragComponentHandle, TRUE,
								GetClassHashCode(SelectedComponent), EString(PE_TEXT("Transform")), Before, After);
						}
					}
				}
				DragComponentHandle = FObjectHandle();
				bDragging = FALSE;
				DragAxis  = EGizmoAxis::NONE;
			}
	}

	void EEditorManager::EditorUpdate()
	{
		PWorld* MenuWorld = WorldManager->GetWorld();
		WorldCommands.GetContext().World = MenuWorld;
		if (MenuWorld)
		{
			EEditorWorldContext::SetEditorCommandStack(MenuWorld, &WorldCommands);
		}
		if (WorldManager->GetLevelRevision() != LastLevelRevision)
		{
			LastLevelRevision = WorldManager->GetLevelRevision();
			WorldCommands.Clear();
			LastCommandRevision = WorldCommands.GetHistoryRevision();
		}
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Level"))
				{
					WorldManager->RequestNewLevel();
				}
				if (ImGui::MenuItem("Open Level..."))
				{
					WorldManager->RequestOpenLevelPopup();
				}
				if (ImGui::MenuItem("Save Level"))
				{
					if (WorldManager->GetCurrentLevelPath().Length() > 0u)
					{
						WorldManager->RequestSaveLevel(WorldManager->GetCurrentLevelPath());
					}
					else
					{
						WorldManager->RequestSaveAsPopup();
					}
				}
				if (ImGui::MenuItem("Save Level As..."))
				{
					WorldManager->RequestSaveAsPopup();
				}
				if (ImGui::MenuItem("Reload Level"))
				{
					WorldManager->RequestReloadLevel();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				const EString UndoLabel = WorldCommands.GetUndoLabel();
				const EString RedoLabel = WorldCommands.GetRedoLabel();
				const EString UndoMenuLabel = WorldCommands.CanUndo() ? (EString(PE_TEXT("Undo ")) + UndoLabel) : EString(PE_TEXT("Undo"));
				const EString RedoMenuLabel = WorldCommands.CanRedo() ? (EString(PE_TEXT("Redo ")) + RedoLabel) : EString(PE_TEXT("Redo"));
				if (ImGui::MenuItem(*UndoMenuLabel, "Ctrl+Z", false, WorldCommands.CanUndo() != FALSE))
				{
					WorldCommands.Undo();
				}
				if (ImGui::MenuItem(*RedoMenuLabel, "Ctrl+Y", false, WorldCommands.CanRedo() != FALSE))
				{
					WorldCommands.Redo();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Build"))
			{
				PWorld* EditorWorld = WorldManager->GetWorld();
				ENavigationManager* Navigation = ENavigationManager::GetManagerSingleton();
				const BOOL32 bBuilding = ((EditorWorld) && (Navigation->IsWorldBuildRunning(*EditorWorld) != FALSE)) ? TRUE : FALSE;
				// One action for the whole level: every navmesh agent (default, large, ...) is
				// baked together. No actor has to be selected or clicked.
				if (ImGui::MenuItem("Build Navigation", nullptr, false, ((EditorWorld) && (!bBuilding)) ? true : false))
				{
					Navigation->RequestWorldBuild(*EditorWorld, ENavigationManager::ENavigationWorldBuildReason::EDITOR);
				}
				if (bBuilding)
				{
					ImGui::MenuItem("Building navigation...", nullptr, false, false);
				}
				else if (EditorWorld)
				{
					const ENavigationManager::FNavigationWorldBuildReport& Report = Navigation->GetWorldBuildReport(*EditorWorld);
					if (Report.bHasRun)
					{
						UINT32 Built = 0u;
						for (UINT32 Index = 0u, Num = Report.Profiles.Num<UINT32>(); Index < Num; Index++)
						{
							if (Report.Profiles[Index].bSucceeded)
							{
								Built++;
							}
						}
						const EString Label = EString(PE_TEXT("Last build: ")) + EString::FromInt(Built)
							+ EString(PE_TEXT("/")) + EString::FromInt(Report.Profiles.Num<UINT32>()) + EString(PE_TEXT(" agents"));
						ImGui::MenuItem(*Label, nullptr, false, false);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		ImGuiIO& IO = ImGui::GetIO();
		if ((!IO.WantTextInput) && IO.KeyCtrl)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Z))
			{
				WorldCommands.Undo();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Y))
			{
				WorldCommands.Redo();
			}
		}

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		// The world may have been re-initialized since the last frame, replacing the
		// controller our mouse subscription points at.
		AttachToCurrentController();

		// Selection is owned by the world. Mirror the resolved component for the gizmo, so a
		// destroyed object reads as null immediately and the two selections cannot diverge.
		PWorld* CurrentWorld = WorldManager->GetWorld();
		SelectedComponent = CurrentWorld ? dynamic_cast<PSceneComponent*>(EEditorWorldContext::GetSelectedComponent(CurrentWorld)) : nullptr;
		if (!SelectedComponent)
		{
			bDragging = FALSE;
			DragAxis = EGizmoAxis::NONE;
		}

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

		// Any command pushed by the panels (property edits, structural edits, gizmo release)
		// makes the current level dirty.
		if (WorldCommands.GetHistoryRevision() != LastCommandRevision)
		{
			LastCommandRevision = WorldCommands.GetHistoryRevision();
			if (WorldManager->GetWorld())
			{
				WorldManager->MarkLevelDirty();
			}
		}

		// A Details/Outline action can destroy the selected component while those panels are
		// drawing. Re-read the world selection before the gizmo and bounds passes use the
		// pointer; the old snapshot would be freed memory.
		SelectedComponent = CurrentWorld ? dynamic_cast<PSceneComponent*>(EEditorWorldContext::GetSelectedComponent(CurrentWorld)) : nullptr;
		if (!SelectedComponent)
		{
			bDragging = FALSE;
			DragAxis = EGizmoAxis::NONE;
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
				DrawTranslationGizmo(PC->GetCamera(), SelectedComponent, bDragging ? DragAxis : EGizmoAxis::NONE);
			}
		}
	}
#endif
}
