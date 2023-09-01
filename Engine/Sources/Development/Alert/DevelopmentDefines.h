#pragma once

#include "../../EngineCore/Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{

#if !(_TEST_MODE || _SHIPPING_MODE)
	class DWindowsMessage
	{
	public:
		enum DWindowsBoxType : UINT8
		{
			WINDOWS_BOX_TYPE_OK = 0,
		};
	public:
		static void _Check_Default(const EString& inCaption, const EString& inText, const BOOL32& condition);

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
#define Check(_Caption,_Text,_Condition)	(DWindowsMessage::_Check_Default((_Caption),(_Text),(_Condition)))
#else
#define PE_FAILED(_Caption,_Text)			(;)
#define Check(_Caption, _Text, _Condition)	(;)
#endif

};