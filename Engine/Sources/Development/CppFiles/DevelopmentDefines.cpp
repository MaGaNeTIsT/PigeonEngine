#include "../Headers/DevelopmentDefines.h"
#include "../../EngineCore/Core/Headers/Main.h"
#include "../../EngineCore/Core/Headers/Manager.h"

namespace PigeonEngine
{
#ifdef _DEVELOPMENT_EDITOR

	INT WindowsMessageBox(const std::string& outText, const std::string& outCaption, const UINT& type)
	{
		return (::MessageBox(EManager::GetWindowHandle(), outText.c_str(), outCaption.c_str(), type));
	}

	void DWindowsMessage::_Check_Default(const std::string& inCaption, const std::string& inText, const BOOL& condition)
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
		std::string tempText, tempCaption; const UINT tempType = MB_OK;
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
		std::string tempText, tempCaption; UINT tempType = 0u;
		if (!(conditionFunc(tempText, tempCaption, tempType)))
		{
			executeFunc(WindowsMessageBox(tempText, tempCaption, tempType);
		}
	}

#endif
};