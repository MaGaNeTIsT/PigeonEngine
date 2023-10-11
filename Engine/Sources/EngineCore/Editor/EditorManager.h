#pragma once
#include "Base/DataStructure/ObjectBase.h"

namespace PigeonEngine
{
	class EEditorManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();
#if _EDITOR_ONLY
	public:
		void EditorUpdate();
	private:
		BOOL32			m_EditorOpen = FALSE;
#endif
	};
}

