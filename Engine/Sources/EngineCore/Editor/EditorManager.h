#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include "EditorHelper.h"
#include "WorldEditorCommand.h"

namespace PigeonEngine
{
#if ENGINE_EDITOR_ONLY
	class EEditorManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();

	public:
		void EditorInit();
		void EditorUpdate();

	private:
		void HandleMouseEvent(IMouse::Event::EType Type, const Vector2& MousePos);
		void AttachToCurrentController();

	private:
		BOOL32 bEditorOpen = FALSE;

		class EWorldManager*		WorldManager		= nullptr;
		class EAssetManager*		AssetManager		= nullptr;
		class EEditorLogManager*	LogsManager		= nullptr;
		class EImportManager*		ImportManager		= nullptr;
		class EEditorGlobalPanel*	EditorGlobalPanel	= nullptr;
		class EEditorProfilerPanel*	EditorProfilerPanel = nullptr;

		// Currently selected component (gizmo target), mirrored from PWorld's handle
		// selection every frame; the world remains the authority.
		class PSceneComponent* SelectedComponent = nullptr;

		FWorldEditorCommandStack WorldCommands;
		// Level revision the command stack belongs to; a new/loaded level drops history.
		UINT64 LastLevelRevision = 0u;

		// The controller whose OnMouseEvent currently carries the HandleMouseEvent
		// subscription. The world can be re-initialized at runtime, which replaces the
		// controller and its delegates; AttachToCurrentController re-subscribes whenever
		// the pointer changes, so editor mouse handling survives a world reset.
		class PController* AttachedController = nullptr;

		// Gizmo drag state
		EGizmoAxis	DragAxis          = EGizmoAxis::NONE;
		Vector3		DragOriginWorld   = Vector3(0.f, 0.f, 0.f);
		Vector3		ComponentOrigin   = Vector3(0.f, 0.f, 0.f);
		Vector2		LastMousePos      = Vector2(0.f, 0.f);
		BOOL8		bDragging         = FALSE;
		// Gizmo drag undo record: the component and its local transform at press.
		FObjectHandle	DragComponentHandle;
		ETransform		DragStartTransform;
	};
#endif
}

