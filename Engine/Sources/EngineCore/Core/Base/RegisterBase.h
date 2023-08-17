#pragma once

#include <Base/DataStructure/BuiltInType.h>
#include <vector>
#include <functional>

namespace PigeonEngine
{

	class ERegisterBase
	{
	public:
		typedef std::function<void()> _FunctionType;
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
	private:
		std::vector<_FunctionType> FunctionRegistry;
	public:
		ERegisterBase() {}
		~ERegisterBase() {}
		ERegisterBase(const ERegisterBase&) {}
		ERegisterBase& operator=(const ERegisterBase&) {}
	};

};