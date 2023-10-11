#pragma once
#include "Base/DataStructure/ObjectBase.h"

namespace PigeonEngine
{
	class EEditorManager : public EManagerBase
	{
		CLASS_VIRTUAL_NOCOPY_BODY(EEditorManager)
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

	public:
		static EEditorManager* GetManagerSingleton()
		{
			static EEditorManager _StaticManagerSingletonObject;
			return (&_StaticManagerSingletonObject);
			return nullptr;
		}
		

	};
}

