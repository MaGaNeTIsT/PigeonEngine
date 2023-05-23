#pragma once

#include "../../EngineCore/Main/Main.h"

namespace PigeonEngine
{
	// class std::string;
#if !(_TEST_MODE || _SHIPPING_MODE)
	class DWindowsMessage
	{
	public:
		enum DWindowsBoxType
		{
			WINDOWS_BOX_TYPE_OK = 0,
		};
	public:
		static void _Check_Default(const std::string& inCaption, const std::string& inText, const BOOL& condition);

		template<typename TConditionLambdaType>
		static void _Check_Default(const TConditionLambdaType& conditionFunc);

		template<typename TConditionLambdaType, typename TExecuteLambdaType>
		static void _Check_Full(const TConditionLambdaType& conditionFunc, const TExecuteLambdaType& executeFunc);

	public:
		DWindowsMessage() = delete;
		DWindowsMessage(const DWindowsMessage&) = delete;
		~DWindowsMessage() {}
	public:
		DWindowsMessage& operator=(const DWindowsMessage&) = delete;
	};

#define PE_FAILED(_Caption,_Text)			(DWindowsMessage::_Check_Default((_Caption),(_Text),(FALSE)))
// #define Check(_Condition)					(DWindowsMessage::_Check_Default(("CheckError"),(#_Condition),(_Condition)))
#define Check(_Caption,_Text,_Condition)	(DWindowsMessage::_Check_Default((_Caption),(_Text),(_Condition)))
#else
#define PE_FAILED(_Caption,_Text)			(;)
#define Check(_Condition)					(;)
#define Check(_Caption, _Text, _Condition)	(;)
#endif
};