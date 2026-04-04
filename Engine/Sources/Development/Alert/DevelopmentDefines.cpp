#include "DevelopmentDefines.h"

#include "../../EngineCore/Editor/EditorLogManager.h"
#include "../../EngineCore/Main/MainManager.h"
#include "../../EngineCore/Core/Base/DataStructure/Text/String.h"

#ifdef _WINDOWS
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif

namespace PigeonEngine
{
#if !(_SHIPPING_MODE)

	INT32 DWindowsMessage::__WindowsMessageBox_(const CHAR* OutText, const CHAR* OutCaption, const UINT32 InType)
	{
		return (::MessageBox(EMainManager::GetManagerSingleton()->GetWindowHandle(), OutText, OutCaption, InType));
	}

	void DWindowsMessage::__DummyAlert_(const CHAR* InExpression, const CHAR* InMessage, const CHAR* InFile, const UINT32 InLine, const CHAR* InCaption)
	{
#if _EDITOR_ONLY
		{
			PE_LOG_ERROR(InExpression)
			EEditorLogManager* EditorLogManager = EEditorLogManager::GetManagerSingleton();
			EditorLogManager->WriteDownLogs();
		}
#endif

#if _DEBUG_MODE
		{
			//const UINT32 TempType = MB_OK;
			//if (DWindowsMessage::__WindowsMessageBox_(InExpression, InCaption, TempType) == IDOK)
			//{
			//	//TODO
			//}
			// breakpoint is triggered at the assertion call site via the macro
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
	void DDebugDump::_Dump_()
	{

#if _EDITOR_ONLY
#ifdef _WINDOWS
		const int MaxFrames = 62;
		void* stack[MaxFrames];
		HANDLE process = GetCurrentProcess();

		SymInitialize(process, NULL, TRUE);
		WORD frames = CaptureStackBackTrace(0, MaxFrames, stack, NULL);
		
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
		if (symbol)
		{
			symbol->MaxNameLen = 255;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

			PE_LOG_ERROR("===== Stack Trace Start =====");
			for (int i = 1; i < frames; i++)//1 start, skip self.
			{
				SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
				
				IMAGEHLP_LINE64 line;
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				DWORD displacement = 0;
				
				if (SymGetLineFromAddr64(process, (DWORD64)(stack[i]), &displacement, &line))
				{
					PE_LOG_ERROR(*EString(EString::FromInt(frames - i - 1) + ": " + symbol->Name + " - " + line.FileName + ":" + EString::FromInt((UINT32)line.LineNumber)));
				}
				else
				{
					PE_LOG_ERROR(*EString(EString::FromInt(frames - i - 1) + ": " + symbol->Name));
				}
			}
			PE_LOG_ERROR("===== Stack Trace End =====");
			free(symbol);
		}
#else
		PE_LOG_ERROR("Stack trace not implemented for this platform.");
#endif
		{
			EEditorLogManager* EditorLogManager = EEditorLogManager::GetManagerSingleton();
			EditorLogManager->WriteDownLogs();
		}
#endif
	}
#endif
};