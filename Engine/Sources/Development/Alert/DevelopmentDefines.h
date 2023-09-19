#pragma once

#include "../../EngineCore/Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>

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
		static void _Check_Default(const CHAR* inCaption, const CHAR* inText, const BOOL8 condition);

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

//Only WindowsPlatform
#define BREAKPOINT		__debugbreak()
using AssertFailedFunc = bool(*)(const CHAR* inExpression, const CHAR* inMessage, const CHAR* inFile, UINT inLine);
__declspec(dllexport) extern AssertFailedFunc AssertFailed;

// Helper functions to pass message on to failed function
struct AssertLastParameter { };
inline bool AssertFailedHelper(const CHAR* inExpression, const CHAR* inFile, UINT inLine, AssertLastParameter) { return AssertFailed(inExpression, nullptr, inFile, inLine); }
inline bool AssertFailedHelper(const CHAR* inExpression, const CHAR* inFile, UINT inLine, const CHAR* inMessage, AssertLastParameter) { return AssertFailed(inExpression, inMessage, inFile, inLine); }
#define ASSERT(inExpression, ...)	do { if (!(inExpression) && AssertFailedHelper(#inExpression, __FILE__, UINT(__LINE__), ##__VA_ARGS__, AssertLastParameter())) BREAKPOINT; } while (false)
#define PE_FAILED(_Caption,_Text)			(DWindowsMessage::_Check_Default((_Caption),(_Text),(FALSE)))
#define Check(_Caption,_Text,_Condition)	(DWindowsMessage::_Check_Default((_Caption),(_Text),(_Condition)))
#else
#define ASSERT(inExpression, ...)			(;)
#define PE_FAILED(_Caption,_Text)			(;)
#define Check(_Caption, _Text, _Condition)	(;)
#endif

};