#pragma once

#include <Base/DataStructure/ObjectBase.h>

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
		BOOL32 m_EditorOpen = FALSE;

		class EWorldManager*     m_WorldManager = nullptr;
		class EAssetManager*     m_AssetManager = nullptr;
		class EEditorLogManager* m_LogsManager  = nullptr;

	};
#endif
}

