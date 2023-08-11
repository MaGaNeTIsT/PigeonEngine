#include "DevelopmentDefines.h"
#include "../../EngineCore/Main/Main.h"
#include "../../EngineCore/Main/MainManager.h"

namespace PigeonEngine
{
#ifdef _EDITOR_ONLY

	INT WindowsMessageBox(const EString& outText, const EString& outCaption, const UINT& type)
	{
		return (::MessageBox(EMainManager::GetManagerSingleton()->GetWindowHandle(), *outText, *outCaption, type));
	}

	void DWindowsMessage::_Check_Default(const EString& inCaption, const EString& inText, const BOOL& condition)
	{
		if (!condition)
		{
			const UINT tempType = MB_OK;
			if (WindowsMessageBox(inText, inCaption, tempType) == IDOK)
			{
				//TODO
			}
		}
	}

	template<typename TConditionLambdaType>
	void DWindowsMessage::_Check_Default(const TConditionLambdaType& conditionFunc)
	{
		EString tempText, tempCaption; const UINT tempType = MB_OK;
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
		EString tempText, tempCaption; UINT tempType = 0u;
		if (!(conditionFunc(tempText, tempCaption, tempType)))
		{
			executeFunc(WindowsMessageBox(tempText, tempCaption, tempType));
		}
	}

#endif
};