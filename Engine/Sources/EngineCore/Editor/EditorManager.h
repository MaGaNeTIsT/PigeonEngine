#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include "EditorHelper.h"

namespace PigeonEngine
{
#if _EDITOR_ONLY
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
		BOOL32 m_EditorOpen = FALSE;

		class EWorldManager*		m_WorldManager		= nullptr;
		class EAssetManager*		m_AssetManager		= nullptr;
		class EEditorLogManager*	m_LogsManager		= nullptr;
		class EImportManager*		m_ImportManager		= nullptr;
		class EEditorGlobalPanel*	m_EditorGlobalPanel	= nullptr;

		// Currently selected component (gizmo target)
		class PSceneComponent* m_SelectedComponent = nullptr;

		// Gizmo drag state
		EGizmoAxis	m_DragAxis          = EGizmoAxis::NONE;
		Vector3		m_DragOriginWorld   = Vector3(0.f, 0.f, 0.f);
		Vector3		m_ComponentOrigin   = Vector3(0.f, 0.f, 0.f);
		Vector2		m_LastMousePos      = Vector2(0.f, 0.f);
		BOOL8		m_bDragging         = FALSE;
	};
#endif
}

