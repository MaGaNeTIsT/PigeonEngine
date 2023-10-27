#pragma once

#include <Base/DataStructure/BuiltInType.h>
#include <Base/Delegate/Function.h>
#include <vector>

namespace PigeonEngine
{

	typedef		std::function<void(void)>		EVoidFunctionType;
	typedef		std::function<void* (void)>		ERawPtrFunctionType;

	template<typename _TFunctionType>
	class ERegisterBase
	{
	public:
		template<typename _TRegisterFunctionType>
		void AddRegisterFunction(_TRegisterFunctionType InRegisterFunction)
		{
			FunctionRegistry.push_back(InRegisterFunction);
		}
		template<typename _TRegisterFunctionType, typename... _TArguments>
		void AddRegisterFunction(_TRegisterFunctionType InRegisterFunction, _TArguments... InArguments)
		{
			FunctionRegistry.push_back(std::bind(InRegisterFunction, InArguments...));
		}
	public:
		void DoRegister()
		{
			for (auto& It : FunctionRegistry)
			{
				It();
			}
		}
		void ClearRegister()
		{
			FunctionRegistry.clear();
		}
		UINT32 GetRegisterNum()const
		{
			return static_cast<UINT32>(FunctionRegistry.size());
		}
	private:
		std::vector<_TFunctionType> FunctionRegistry;
	public:
		ERegisterBase() {}
		~ERegisterBase() {}
		ERegisterBase(const ERegisterBase&) {}
		ERegisterBase& operator=(const ERegisterBase&) { return (*this); }
	};

#define PE_REGISTER_CLASS_DECLARE(__ClassName, __FunctionType) \
    class __ClassName final : public ERegisterBase<__FunctionType>\
    {\
	public:\
		class ERegisterObject final\
		{\
		public:\
			ERegisterObject(__FunctionType InRegisterFunction)\
			{\
				__ClassName::GetManagerSingleton()->AddRegisterFunction<__FunctionType>(InRegisterFunction);\
			}\
		public:\
			~ERegisterObject() {}\
			ERegisterObject() = delete;\
			ERegisterObject(const ERegisterObject&) = delete;\
			ERegisterObject& operator=(const ERegisterObject&) = delete;\
		};\
	public:\
		__ClassName(const __ClassName&) = delete;\
		__ClassName& operator=(const __ClassName&) = delete;\
	private:\
		__ClassName() {}\
		virtual ~__ClassName() {}\
	public:\
		static __ClassName* GetManagerSingleton()\
		{\
			static __ClassName _StaticManagerSingletonObject;\
			return (&_StaticManagerSingletonObject);\
		}\
	};\

};