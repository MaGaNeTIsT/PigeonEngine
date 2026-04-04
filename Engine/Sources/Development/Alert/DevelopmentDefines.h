#pragma once

#include "../../EngineCore/Main/Main.h"
#include <Config/ErrorCaption.h>
#include <Base/DataStructure/BuiltInType.h>

namespace PigeonEngine
{

#if !(_SHIPPING_MODE)

#ifdef _WINDOWS
#define PE_BREAKPOINT	{__debugbreak();}
#else
#define PE_BREAKPOINT	{}
#endif

#define PE_DEBUGDUMP DDebugDump::_Dump_();

	class DDebugDump
	{
	public:
		static void _Dump_();
	};

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
		static bool __Check_(const TExpressionLambdaType& InExpressionFunction, const CHAR* InExpressionString,
			const CHAR* InFile, const UINT32 InLine, const CHAR* InCaption, const CHAR* InMessage, _DDummyArgument)
		{
			if (!(InExpressionFunction()))
			{
				DWindowsMessage::__DummyAlert_(InExpressionString, InMessage, InFile, InLine, InCaption);
				return false;
			}
			return true;
		}

		template<typename TExpressionLambdaType>
		static bool __Check_(const TExpressionLambdaType& InExpressionFunction, const CHAR* InExpressionString,
			const CHAR* InFile, const UINT32 InLine, const CHAR* InCaption, _DDummyArgument)
		{
			if (!(InExpressionFunction()))
			{
				DWindowsMessage::__DummyAlert_(InExpressionString, nullptr, InFile, InLine, InCaption);
				return false;
			}
			return true;
		}

		template<typename TExpressionLambdaType>
		static bool __Check_(const TExpressionLambdaType& InExpressionFunction, const CHAR* InExpressionString,
			const CHAR* InFile, const UINT32 InLine, _DDummyArgument)
		{
			if (!(InExpressionFunction()))
			{
				DWindowsMessage::__DummyAlert_(InExpressionString, nullptr, InFile, InLine, (ENGINE_UNKNOWN_ERROR));
				return false;
			}
			return true;
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


#if _DEBUG_MODE

#define PE_ASSERT(__Expression, ...) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Expression);\
		}, #__Expression, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument())) PE_BREAKPOINT\

#define PE_FAILED(__Caption, __Text) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return FALSE;\
		}, (__Text), (__FILE__), UINT32(__LINE__), (__Caption), DWindowsMessage::_DDummyArgument())) PE_BREAKPOINT\

#define PE_CHECK(__Caption, __Text, __Condition) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, (__Text), (__FILE__), UINT32(__LINE__), (__Caption), DWindowsMessage::_DDummyArgument())) PE_BREAKPOINT\

#define Check(__Condition, ...) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, #__Condition, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument())) PE_BREAKPOINT\

#define CheckSlow(__Condition, ...) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, #__Condition, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument())) PE_BREAKPOINT\

#else

#define PE_ASSERT(__Expression, ...) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Expression);\
		}, #__Expression, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument())) PE_BREAKPOINT\

#define PE_FAILED(__Caption, __Text) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return FALSE;\
		}, (__Text), (__FILE__), UINT32(__LINE__), (__Caption), DWindowsMessage::_DDummyArgument())) { }\

#define PE_CHECK(__Caption, __Text, __Condition) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, (__Text), (__FILE__), UINT32(__LINE__), (__Caption), DWindowsMessage::_DDummyArgument())) { }\

#define Check(__Condition, ...) \
	if (!DWindowsMessage::__Check_(\
		[&]()->BOOL8\
		{\
			return (__Condition);\
		}, #__Condition, (__FILE__), UINT32(__LINE__), ##__VA_ARGS__, DWindowsMessage::_DDummyArgument())) { }\

#define CheckSlow(__Condition, ...)						{}

#endif

#else

#define PE_ASSERT(__Expression, ...)					{}
#define PE_FAILED(__Caption, __Text)					{}
#define PE_CHECK(__Caption, __Text, __Condition)		{}
#define Check(__Condition, ...)							{}
#define CheckSlow(__Condition, ...)						{}

#endif

};