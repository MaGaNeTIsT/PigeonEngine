#pragma once

#include "../../EngineCore/Main/Main.h"
#include <Config/ErrorCaption.h>
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
		struct _DDummyArgument {};

		template<typename TExpressionLambdaType>
		static void __Check_(const TExpressionLambdaType& InExpressionFunction, const CHAR* InExpressionString,
			const CHAR* InFile, const UINT32 InLine, const CHAR* InCaption, const CHAR* InMessage, _DDummyArgument)
		{
			if (!(InExpressionFunction()))
			{
				DWindowsMessage::__DummyAlert_(InExpressionString, InMessage, InFile, InLine, InCaption);
			}
		}

		template<typename TExpressionLambdaType>
		static void __Check_(const TExpressionLambdaType& InExpressionFunction, const CHAR* InExpressionString,
			const CHAR* InFile, const UINT32 InLine, const CHAR* InCaption, _DDummyArgument)
		{
			if (!(InExpressionFunction()))
			{
				DWindowsMessage::__DummyAlert_(InExpressionString, nullptr, InFile, InLine, InCaption);
			}
		}

		template<typename TExpressionLambdaType>
		static void __Check_(const TExpressionLambdaType& InExpressionFunction, const CHAR* InExpressionString,
			const CHAR* InFile, const UINT32 InLine, _DDummyArgument)
		{
			if (!(InExpressionFunction()))
			{
				DWindowsMessage::__DummyAlert_(InExpressionString, nullptr, InFile, InLine, (ENGINE_UNKNOWN_ERROR));
			}
		}
	protected:
		static INT32 __WindowsMessageBox_(const CHAR* OutText, const CHAR* OutCaption, const UINT32 InType);
		static void __DummyAlert_(const CHAR* InExpression, const CHAR* InMessage, const CHAR* InFile, const UINT32 InLine, const CHAR* InCaption);
	public:
		DWindowsMessage() = delete;
		DWindowsMessage(const DWindowsMessage&) = delete;
		~DWindowsMessage() {}
	public:
		DWindowsMessage& operator=(const DWindowsMessage&) = delete;
	};


#define PE_ASSERT(__Expression, ...) \
	DWindowsMessage::__Check_(\
		[]()->BOOL8\
		{\
			return (__Expression);\
		}, #__Expression, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument());\

#define PE_FAILED(__Caption, __Text) \
	DWindowsMessage::__Check_(\
		[]()->BOOL8\
		{\
			return FALSE;\
		}, (__Text), (__FILE__), UINT32(__LINE__), (__Caption), DWindowsMessage::_DDummyArgument());\

#define PE_CHECK(__Caption, __Text, __Condition) \
	DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, (__Text), (__FILE__), UINT32(__LINE__), (__Caption), DWindowsMessage::_DDummyArgument());\

#define Check(__Condition, ...) \
	DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, #__Condition, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument());\

#if _DEBUG_MODE

#define CheckSlow(__Condition, ...) \
	DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, #__Condition, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument());\

#else

#define CheckSlow(__Condition, ...)						(;)

#endif

#else

#define PE_ASSERT(__Expression, ...)					(;)
#define PE_FAILED(__Caption, __Text)					(;)
#define PE_CHECK(__Caption, __Text, __Condition)		(;)
#define Check(__Condition, ...)							(;)
#define CheckSlow(__Condition, ...)						(;)

#endif

};