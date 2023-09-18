#include "DevelopmentDefines.h"
#include "../../EngineCore/Main/MainManager.h"

namespace PigeonEngine
{
#if _EDITOR_ONLY

	INT32 WindowsMessageBox(const CHAR* outText, const CHAR* outCaption, const UINT32& type)
	{
		return (::MessageBox(EMainManager::GetManagerSingleton()->GetWindowHandle(), outText, outCaption, type));
	}

	void DWindowsMessage::_Check_Default(const CHAR* inCaption, const CHAR* inText, const BOOL8 condition)
	{
		if (!condition)
		{
			const UINT32 tempType = MB_OK;
			if (WindowsMessageBox(inText, inCaption, tempType) == IDOK)
			{
				//TODO
			}
		}
	}

	template<typename TConditionLambdaType>
	void DWindowsMessage::_Check_Default(const TConditionLambdaType& conditionFunc)
	{
		EString tempText, tempCaption; const UINT32 tempType = MB_OK;
		if (!(conditionFunc(tempText, tempCaption)))
		{
			if (WindowsMessageBox(tempText, tempCaption, tempType) == IDOK)
			{
				//TODO
			}
		}
	}

	template<typename TConditionLambdaType, typename TExecuteLambdaType>
	void DWindowsMessage::_Check_Full(const TConditionLambdaType& conditionFunc, const TExecuteLambdaType& executeFunc)
	{
		EString tempText, tempCaption; UINT32 tempType = 0u;
		if (!(conditionFunc(tempText, tempCaption, tempType)))
		{
			executeFunc(WindowsMessageBox(tempText, tempCaption, tempType));
		}
	}

#endif
};