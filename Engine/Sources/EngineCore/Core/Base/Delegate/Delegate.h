#pragma once

#include "Function.h"
#include "Base/Memory/Memory.h"
#include "Base/DataStructure/Container/Array.h"


namespace PigeonEngine
{
	/*------------------ Declaration ------------------*/
	template <typename FuncType>
	class TDelegate;

	// Prototype, should not be used directly
	template <typename Ret, typename... Args>
	class TDelegate<Ret(Args...)>
	{

	};

	template <typename FuncType>
	class TDelegateBroadcast;
	// Delegate no return, multi execution
	template <typename... Args>
	class TDelegateBroadcast<void (Args...)> : public TDelegate<void(Args...)>
	{
	public:
		~TDelegateBroadcast<void(Args...)>() { RemoveAll(); }
	public:
		void Add(const TFunction<void (Args...)>& New);
		void Remove(const TFunction<void (Args...)>& Old);
		void RemoveAll();
		void Broadcast(Args... Arguments);
	private:
		TArray<TFunction<void(Args...)>> ToCall{};
	};

	// Delegate with return, single execution
	template <typename FuncType>
	class TDelegateSingle;

	template <typename Ret, typename... Args>
	class TDelegateSingle<Ret(Args...)> : public TDelegate<Ret(Args...)>
	{
	public:
		~TDelegateSingle<Ret(Args...)>() { Unbind(); }
	public:
		void Bind(const TFunction<Ret(Args...)>& New);
		void Unbind();
		Ret  Execute(Args... Arguments);
	private:
		TFunction<Ret(Args...)> Ops = nullptr;
	};

	/*------------------ Macros ------------------*/
#define MAKE_DELEGATE_MULTI_ONE_PARAM(__DelegateName, __ParamType)\
	class __DelegateName : public PigeonEngine::TDelegateBroadcast<void (__ParamType)>{};
#define MAKE_DELEGATE_MULTI_TWO_PARAM(__DelegateName, __ParamType1, __ParamType2)\
	class __DelegateName : public PigeonEngine::TDelegateBroadcast<void (__ParamType1, __ParamType2)>{};
#define MAKE_DELEGATE_MULTI_THREE_PARAM(__DelegateName, __ParamType1, __ParamType2, __ParamType3)\
	class __DelegateName : public PigeonEngine::TDelegateBroadcast<void (__ParamType1, __ParamType2, __ParamType3)>{};
#define MAKE_DELEGATE_MULTI_FOUR_PARAM(__DelegateName, __ParamType1, __ParamType2, __ParamType3, __ParamType4)\
	class __DelegateName : public PigeonEngine::TDelegateBroadcast<void (__ParamType1, __ParamType2, __ParamType3, __ParamType4)>{};
#define MAKE_DELEGATE_MULTI_FIVE_PARAM(__DelegateName, __ParamType1, __ParamType2, __ParamType3, __ParamType4, __ParamType5)\
	class __DelegateName : public PigeonEngine::TDelegateBroadcast<void (__ParamType1, __ParamType2, __ParamType3, __ParamType4, __ParamType5)>{};

#define MAKE_DELEGATE_SINGLE_RET_ONE_PARAM(__DelegateName, __ReturnType, __ParamType)\
	class __DelegateName : public PigeonEngine::TDelegateSingle<__ReturnType(__ParamType)>{};
#define MAKE_DELEGATE_SINGLE_RET_TWO_PARAM(__DelegateName, __ReturnType, __ParamType1, __ParamType2)\
	class __DelegateName : public PigeonEngine::TDelegateSingle<__ReturnType(__ParamType1, __ParamType2)>{};
#define MAKE_DELEGATE_SINGLE_RET_THREE_PARAM(__DelegateName, __ReturnType, __ParamType1, __ParamType2, __ParamType3)\
	class __DelegateName : public PigeonEngine::TDelegateSingle<__ReturnType(__ParamType1, __ParamType2, __ParamType3)>{};
#define MAKE_DELEGATE_SINGLE_RET_FOUR_PARAM(__DelegateName, __ReturnType, __ParamType1, __ParamType2, __ParamType3, __ParamType4)\
	class __DelegateName : public PigeonEngine::TDelegateSingle<__ReturnType(__ParamType1, __ParamType2, __ParamType3, __ParamType4)>{};
#define MAKE_DELEGATE_SINGLE_RET_FIVE_PARAM(__DelegateName, __ReturnType, __ParamType1, __ParamType2, __ParamType3, __ParamType4, __ParamType5)\
	class __DelegateName : public PigeonEngine::TDelegateSingle<__ReturnType(__ParamType1, __ParamType2, __ParamType3, __ParamType4, __ParamType5)>{};


	/*------------------ Implementation ------------------*/
	template <typename ... Args>
	void TDelegateBroadcast<void(Args...)>::Add(const TFunction<void(Args...)>& New)
	{
		ToCall.Add(New);
	}

	template <typename ... Args>
	void TDelegateBroadcast<void(Args...)>::Remove(const TFunction<void(Args...)>& Old)
	{
		ToCall.Remove(Old);
	}

	template <typename ... Args>
	void TDelegateBroadcast<void(Args...)>::RemoveAll()
	{
		ToCall.Empty();
	}

	template <typename ... Args>
	void TDelegateBroadcast<void(Args...)>::Broadcast(Args... Arguments)
	{
		for (TFunction<void(Args...)>& elem : ToCall)
		{
			elem(Arguments...);
		}
	}

	template <typename Ret, typename ... Args>
	void TDelegateSingle<Ret(Args...)>::Bind(const TFunction<Ret(Args...)>& New)
	{
		this->Ops = New;
	}

	template <typename Ret, typename ... Args>
	void TDelegateSingle<Ret(Args...)>::Unbind()
	{
		this->Ops = nullptr;
	}

	template <typename Ret, typename ... Args>
	Ret TDelegateSingle<Ret(Args...)>::Execute(Args... Arguments)
	{
		if (this->Ops)
			return this->Ops(Arguments);
		return Ret();
	}

}
