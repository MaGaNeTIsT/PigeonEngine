#include <Editor/Manager/EditorManager.h>
#include <Editor/Panels/EditorGlobalPanel.h>
#include <Editor/Panels/EditorProfilerPanel.h>
#include <Editor/Panels/OutputLogPanel.h>
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
#include <Editor/Panels/WorldOutlinePanel.h>
#include <Editor/Panels/LevelPanel.h>
#include <Editor/Panels/NavigationPanel.h>
#include <Editor/Manager/EditorWorldContext.h>
#include <Editor/Manager/EditorWindowRegistry.h>
#include <Editor/Manager/EditorMenuRegistry.h>
#include <Editor/Manager/EditorSettings.h>
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
		// Drop menu callbacks and window descriptors before the panels they capture are
		// destroyed. The window menu items call back into the window registry by id.
		EEditorMenuRegistry::Clear();
		EEditorWindowRegistry::Clear();
		if (OutputLogPanel)
		{
			delete OutputLogPanel;
			OutputLogPanel = nullptr;
		}
		// The log service flushes after the asset/render teardown in EMainManager::ShutDown,
		// so its shutdown is not tied to the editor lifetime.
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
		if (!OutputLogPanel)
		{
			OutputLogPanel = new FOutputLogPanel();
		}
		AssetManager->EditorInit();
		WorldManager->EditorInit();
		ImportManager->EditorInit();

		// One registration point for the per-class details editors: everything registered
		// here draws on top of the reflection grid for its class chain.
		EWorldDetailsPanel::RegisterBuiltinCustomEditors();

		// Declares the Panels.* schema keys, so the settings Load below sees the tracked
		// default and the user overrides for every optional window. The File/Edit/Build
		// items are registered first so the Window root stays last in the menu bar.
		RegisterEditorMenus();
		RegisterEditorWindows();

		// Editor preferences (optional panels, Display Settings switches) are loaded before
		// any panel draws and applied to the managers that own the runtime flags.
		EEditorSettings::Initialize();
		EditorGlobalPanel->ApplySettings();

		// Reads the persisted visibility for every registered window.
		EEditorWindowRegistry::Initialize();

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

	void EEditorManager::RegisterEditorMenus()
	{
		// File. Paths are Root/Group.../Item; the last segment is the displayed label.
		EEditorMenuRegistry::RegisterItem({
			EString("File/New Level"),
			[this]() -> void { WorldManager->RequestNewLevel(); }
		});
		EEditorMenuRegistry::RegisterItem({
			EString("File/Open Level..."),
			[this]() -> void { WorldManager->RequestOpenLevelPopup(); }
		});
		EEditorMenuRegistry::RegisterItem({
			EString("File/Save Level"),
			[this]() -> void
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
		});
		EEditorMenuRegistry::RegisterItem({
			EString("File/Save Level As..."),
			[this]() -> void { WorldManager->RequestSaveAsPopup(); }
		});
		EEditorMenuRegistry::RegisterItem({
			EString("File/Reload Level"),
			[this]() -> void { WorldManager->RequestReloadLevel(); }
		});

		// Edit. The label and enabled state are resolved while the menu is open.
		EEditorMenuRegistry::RegisterItem({
			EString("Edit/Undo"),
			[this]() -> void { WorldCommands.Undo(); },
			EString("Ctrl+Z"),
			[this]() -> EString
			{
				const EString Label = WorldCommands.GetUndoLabel();
				return (WorldCommands.CanUndo() != FALSE) ? (EString("Undo ") + Label) : EString("Undo");
			},
			nullptr,
			[this]() -> BOOL32 { return WorldCommands.CanUndo(); }
		});
		EEditorMenuRegistry::RegisterItem({
			EString("Edit/Redo"),
			[this]() -> void { WorldCommands.Redo(); },
			EString("Ctrl+Y"),
			[this]() -> EString
			{
				const EString Label = WorldCommands.GetRedoLabel();
				return (WorldCommands.CanRedo() != FALSE) ? (EString("Redo ") + Label) : EString("Redo");
			},
			nullptr,
			[this]() -> BOOL32 { return WorldCommands.CanRedo(); }
		});

		// Build. The two status rows only appear while they describe the current state.
		EEditorMenuRegistry::RegisterItem({
			EString("Build/Navigation/Build Navigation"),
			[this]() -> void
			{
				PWorld* World = WorldManager->GetWorld();
				if (World)
				{
					ENavigationManager::GetManagerSingleton()->RequestWorldBuild(*World
						, ENavigationManager::ENavigationWorldBuildReason::EDITOR);
				}
			},
			EString(),
			nullptr,
			nullptr,
			[this]() -> BOOL32
			{
				PWorld* World = WorldManager->GetWorld();
				return (World && (ENavigationManager::GetManagerSingleton()->IsWorldBuildRunning(*World) == FALSE))
					? TRUE : FALSE;
			}
		});
		EEditorMenuRegistry::RegisterItem({
			EString("Build/Navigation/Building navigation..."),
			nullptr,
			EString(),
			nullptr,
			[this]() -> BOOL32
			{
				PWorld* World = WorldManager->GetWorld();
				return (World && (ENavigationManager::GetManagerSingleton()->IsWorldBuildRunning(*World) != FALSE))
					? TRUE : FALSE;
			},
			nullptr,
			nullptr,
			TRUE
		});
		EEditorMenuRegistry::RegisterItem({
			EString("Build/Navigation/LastBuild"),
			nullptr,
			EString(),
			[this]() -> EString
			{
				PWorld* World = WorldManager->GetWorld();
				if (!World)
				{
					return EString();
				}
				const ENavigationManager::FNavigationWorldBuildReport& Report =
					ENavigationManager::GetManagerSingleton()->GetWorldBuildReport(*World);
				if (Report.bHasRun == FALSE)
				{
					return EString();
				}
				UINT32 Built = 0u;
				for (UINT32 Index = 0u, Num = Report.Profiles.Num<UINT32>(); Index < Num; Index++)
				{
					if (Report.Profiles[Index].bSucceeded != FALSE)
					{
						Built++;
					}
				}
				return EString("Last build: ") + EString::FromInt(Built) + EString("/")
					+ EString::FromInt(Report.Profiles.Num<UINT32>()) + EString(" agents");
			},
			[this]() -> BOOL32
			{
				PWorld* World = WorldManager->GetWorld();
				if (!World)
				{
					return FALSE;
				}
				ENavigationManager* Navigation = ENavigationManager::GetManagerSingleton();
				if (Navigation->IsWorldBuildRunning(*World) != FALSE)
				{
					return FALSE;
				}
				return (Navigation->GetWorldBuildReport(*World).bHasRun != FALSE) ? TRUE : FALSE;
			},
			nullptr,
			nullptr,
			TRUE
		});
	}

	void EEditorManager::RegisterEditorWindows()
	{
		// MenuPath controls the Window-menu section; registration order controls its item
		// order. The draw callbacks only paint the contents: EEditorWindowRegistry owns
		// Begin/End, the X button and the size.
		EEditorWindowRegistry::RegisterWindow({
			EString("Level"), EString("Level"), EString("Window/World/Level"), FALSE, ImVec2(360.0f, 330.0f),
			[this]() -> void
			{
				if (WorldManager && WorldManager->GetWorld())
				{
					ELevelPanel::GenerateLevelPanel(WorldManager);
				}
			}
		});
		EEditorWindowRegistry::RegisterWindow({
			EString("Navigation"), EString("Navigation"), EString("Window/World/Navigation"), FALSE, ImVec2(380.0f, 300.0f),
			[this]() -> void
			{
				if (WorldManager && WorldManager->GetWorld())
				{
					EWorldNavigationPanel::GenerateNavigation(WorldManager->GetWorld());
				}
			}
		});
		EEditorWindowRegistry::RegisterWindow({
			EString("WorldOutline"), EString("World Outline"), EString("Window/World/World Outline"), TRUE, ImVec2(280.0f, 400.0f),
			[this]() -> void
			{
				if (WorldManager && WorldManager->GetWorld())
				{
					EWorldOutlinePanel::GenerateOutline(WorldManager->GetWorld());
				}
			}
		});
		EEditorWindowRegistry::RegisterWindow({
			EString("Details"), EString("Details"), EString("Window/World/Details"), TRUE, ImVec2(280.0f, 400.0f),
			[this]() -> void
			{
				if (WorldManager && WorldManager->GetWorld())
				{
					EWorldDetailsPanel::GenerateDetails(WorldManager->GetWorld());
				}
			}
		});
		EEditorWindowRegistry::RegisterWindow({
			EString("ContentBrowser"), EString("Content Browser"), EString("Window/Assets/Content Browser"), TRUE, ImVec2(900.0f, 420.0f),
			[this]() -> void
			{
				if (AssetManager)
				{
					AssetManager->DrawContentBrowserPanel();
				}
			}
		});
		EEditorWindowRegistry::RegisterWindow({
			EString("OutputLog"), EString("Output Log"), EString("Window/Debug/Output Log"), TRUE, ImVec2(720.0f, 360.0f),
			[this]() -> void
			{
				if (OutputLogPanel)
				{
					OutputLogPanel->DrawContents();
				}
			}
		});
		EEditorWindowRegistry::RegisterWindow({
			EString("Profiler"), EString("Profiler"), EString("Window/Debug/Profiler"), TRUE, ImVec2(424.0f, 360.0f),
			[]() -> void
			{
				EEditorProfilerPanel::GetManagerSingleton()->DrawContents();
			}
		});
		EEditorWindowRegistry::RegisterWindow({
			EString("DisplaySettings"), EString("Display Settings"), EString("Window/Settings/Display Settings"), TRUE, ImVec2(424.0f, 212.0f),
			[]() -> void
			{
				EEditorGlobalPanel::GetManagerSingleton()->DrawDisplaySettingsContents();
			}
		});
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
		EEditorMenuRegistry::DrawMainMenuBar();

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
		LogsManager->Tick();
		EEditorWindowRegistry::DrawWindow(EString(PE_TEXT("OutputLog")));
		ImportManager->EditorUpdate();
		EEditorWindowRegistry::DrawWindow(EString(PE_TEXT("DisplaySettings")));
		if (EditorProfilerPanel)
		{
			EditorProfilerPanel->Tick();
		}
		EEditorWindowRegistry::DrawWindow(EString(PE_TEXT("Profiler")));

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
