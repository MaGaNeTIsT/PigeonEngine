#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include "EditorHelper.h"

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

	private:
		BOOL32 bEditorOpen = FALSE;

		class EWorldManager*		WorldManager		= nullptr;
		class EAssetManager*		AssetManager		= nullptr;
		class EEditorLogManager*	LogsManager		= nullptr;
		class EImportManager*		ImportManager		= nullptr;
		class EEditorGlobalPanel*	EditorGlobalPanel	= nullptr;
		class EEditorProfilerPanel*	EditorProfilerPanel = nullptr;

		// Currently selected component (gizmo target)
		class PSceneComponent* SelectedComponent = nullptr;

		// Gizmo drag state
		EGizmoAxis	DragAxis          = EGizmoAxis::NONE;
		Vector3		DragOriginWorld   = Vector3(0.f, 0.f, 0.f);
		Vector3		ComponentOrigin   = Vector3(0.f, 0.f, 0.f);
		Vector2		LastMousePos      = Vector2(0.f, 0.f);
		BOOL8		bDragging         = FALSE;
	};
#endif
}

