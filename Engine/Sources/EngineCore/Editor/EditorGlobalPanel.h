#pragma once

#include <Base/DataStructure/ObjectBase.h>

namespace PigeonEngine
{
#if _EDITOR_ONLY
	class EEditorGlobalPanel : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorGlobalPanel)
	public:
		void Initialize() override;
		void ShutDown() override;
		void DrawResolutionPanel();
	private:
		INT32  m_SelectedResolutionIndex = 0;
		BOOL32 m_bIsFullscreen = FALSE;
	};
#endif
};
