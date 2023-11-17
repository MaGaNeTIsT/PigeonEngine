#include "DevelopmentDefines.h"

#include "../../EngineCore/Editor/EditorLogManager.h"
#include "../../EngineCore/Main/MainManager.h"

namespace PigeonEngine
{
#if _EDITOR_ONLY

#define PE_BREAKPOINT	{__debugbreak();}

	INT32 DWindowsMessage::__WindowsMessageBox_(const CHAR* OutText, const CHAR* OutCaption, const UINT32 InType)
	{
		return (::MessageBox(EMainManager::GetManagerSingleton()->GetWindowHandle(), OutText, OutCaption, InType));
	}

	void DWindowsMessage::__DummyAlert_(const CHAR* InExpression, const CHAR* InMessage, const CHAR* InFile, const UINT32 InLine, const CHAR* InCaption)
	{
		{
			PE_LOG_ERROR(InExpression)
			EEditorLogManager* EditorLogManager = EEditorLogManager::GetManagerSingleton();
			EditorLogManager->WriteDownLogs();
		}
#if _DEBUG_MODE
		{
#if 0
			{
				const UINT32 TempType = MB_OK;
				if (DWindowsMessage::__WindowsMessageBox_(InExpression, InCaption, TempType) == IDOK)
				{
					//TODO
				}
			}
#endif

			PE_BREAKPOINT;
		}
#elif _DEVELOP_MODE
		{
			const UINT32 TempType = MB_OK;
			if (DWindowsMessage::__WindowsMessageBox_(InExpression, InCaption, TempType) == IDOK)
			{
				//TODO
			}
		}
#endif
	};

#else

#define PE_BREAKPOINT	{;}

#endif
};