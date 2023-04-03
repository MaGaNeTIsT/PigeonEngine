#pragma once

namespace PigeonEngine
{
#ifdef _DEVELOPMENT_EDITOR
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

#define Check(_Condition)	(DWindowsMessage::_Check_Default("CheckError",(#_Condition),(_Condition)))
#else
#define Check(_Condition)	(;)
#endif
};