#pragma once

#include "Function.h"
#include "Base/Memory/Memory.h"
#include "Base/DataStructure/Container/Array.h"


namespace PigeonEngine
{
	/*------------------ Declaration ------------------*/
	// Stable subscription identity handed out by TDelegateBroadcast::Add and consumed by
	// Remove(FDelegateHandle). 0 is never issued, so it doubles as "no subscription".
	typedef ULONGLONG FDelegateHandle;

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
	private:
		// One row per live subscription. The handle is the stable identity Remove(handle)
		// consumes; the TFunction copy keeps the by-value Remove path (FunctionID
		// equality) that existing subscribers use.
		struct FDelegateEntry
		{
			FDelegateHandle					Handle = 0u;
			TFunction<void (Args...)>		Callback;
		};
	public:
		~TDelegateBroadcast<void (Args...)>() { RemoveAll(); }
	public:
		// The handle may be ignored by subscribers that never detach; keeping the return
		// value non-void breaks no existing call site.
		FDelegateHandle			Add(const TFunction<void (Args...)>& New);
		void					Remove(const TFunction<void (Args...)>& Old);
		BOOL32					Remove(const FDelegateHandle& InHandle);
		void					RemoveAll();
		void					Broadcast(Args... Arguments);
	private:
		static FDelegateHandle	AllocHandle()
		{
			static FDelegateHandle HandleGenerator = 0u;
			HandleGenerator += 1u;
			return HandleGenerator;
		}
	private:
		TArray<FDelegateEntry> ToCall{};
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
	FDelegateHandle TDelegateBroadcast<void(Args...)>::Add(const TFunction<void(Args...)>& New)
	{
		FDelegateEntry NewEntry;
		NewEntry.Handle = AllocHandle();
		NewEntry.Callback = New;
		ToCall.Add(NewEntry);
		return NewEntry.Handle;
	}

	template <typename ... Args>
	void TDelegateBroadcast<void(Args...)>::Remove(const TFunction<void(Args...)>& Old)
	{
		for (UINT32 Index = 0u, Num = ToCall.Num<UINT32>(); Index < Num; Index++)
		{
			if (ToCall[Index].Callback == Old)
			{
				ToCall.RemoveAt(Index);
				return;
			}
		}
	}

	template <typename ... Args>
	BOOL32 TDelegateBroadcast<void(Args...)>::Remove(const FDelegateHandle& InHandle)
	{
		if (InHandle == 0u)
		{
			return FALSE;
		}
		for (UINT32 Index = 0u, Num = ToCall.Num<UINT32>(); Index < Num; Index++)
		{
			if (ToCall[Index].Handle == InHandle)
			{
				ToCall.RemoveAt(Index);
				return TRUE;
			}
		}
		return FALSE;
	}

	template <typename ... Args>
	void TDelegateBroadcast<void(Args...)>::RemoveAll()
	{
		ToCall.Empty();
	}

	template <typename ... Args>
	void TDelegateBroadcast<void(Args...)>::Broadcast(Args... Arguments)
	{
		// The set of listeners this walk covers is fixed at entry - a listener a callback
		// adds fires from the next broadcast on, which also keeps a self-subscribing
		// callback from looping forever - while the upper bound is re-checked against the
		// live count every step, so a removal (even RemoveAll) during the walk cannot read
		// past the end. A removed row may be skipped or fired once more this pass; both
		// are the standard multicast semantics.
		const UINT32 Num = ToCall.Num<UINT32>();
		for (UINT32 Index = 0u; (Index < Num) && (Index < ToCall.Num<UINT32>()); Index++)
		{
			ToCall[Index].Callback(Arguments...);
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
