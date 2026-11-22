#pragma once

#include <Base/DataStructure/ObjectBase.h>

namespace PigeonEngine
{
#if ENGINE_EDITOR_ONLY
	class EEditorGlobalPanel : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorGlobalPanel)
	public:
		void Initialize() override;
		void ShutDown() override;
		// Pushes the persisted Display Settings switches into the managers that own them.
		void ApplySettings();
		// Draws the contents only; the window registry owns Begin/End and the size.
		void DrawDisplaySettingsContents();
	private:
		INT32	SelectedResolutionIndex	= 0;
		BOOL32	bIsFullscreen			= FALSE;
	};
#endif
};
