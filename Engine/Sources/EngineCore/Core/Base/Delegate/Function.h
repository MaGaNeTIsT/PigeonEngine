#pragma once

#include "Base/DataStructure/BuiltInType.h"
#include <functional>

namespace PigeonEngine
{

#if 1

    // use std::function for now.
    template <typename FuncType>
    class TFunction;

    template<typename FuncType>
    using TFunctionStorage = std::function<FuncType>;

    template<typename FuncType, typename CallableType>
    using TIsFunctionConstructible = std::is_constructible<TFunctionStorage<FuncType>, CallableType>;

    template <typename Ret, typename... Args>
    class TFunction<Ret (Args...)>
    {
    public:
        TFunction();
        TFunction(std::function<Ret(Args ...)>&& op);
        TFunction(const TFunction<Ret(Args ...)>& Other);
        ~TFunction();
    public:
        TFunction<Ret(Args...)>& operator=(const TFunction<Ret(Args ...)>& Other);
        TFunction<Ret(Args...)>& operator=(std::function<Ret(Args ...)> Other);
        Ret                      operator()(Args... Arguments);
        BOOL32                   operator==(const TFunction<Ret(Args ...)>& Other);
        
    private:
        static ULONGLONG AllocFunctionID()
        {
            static ULONGLONG FunctionIDGenerator = 0u;
            FunctionIDGenerator += 1u;
            return FunctionIDGenerator;
        }

        std::function<Ret(Args...)> Operation = nullptr;
        ULONGLONG FunctionID = 0u;
    };

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction()
    {
        FunctionID = AllocFunctionID();
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(std::function<Ret(Args...)>&& op): Operation(std::move(op))
    {}

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(const TFunction<Ret(Args...)>& Other):Operation(Other.Operation), FunctionID(Other.FunctionID)
    {
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::~TFunction()
    {Operation = nullptr;}

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>& TFunction<Ret(Args...)>::operator=(const TFunction<Ret(Args...)>& Other)
    {
        this->Operation = Other.Operation;
        this->FunctionID = Other.FunctionID;
        return *this;
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>& TFunction<Ret(Args...)>::operator=(std::function<Ret(Args...)> Other)
    {
        this->Operation = std::move(Other);
        this->FunctionID = AllocFunctionID();
        return *this;
    }

    template <typename Ret, typename ... Args>
    Ret TFunction<Ret(Args...)>::operator()(Args... Arguments)
    {
        return Operation(Arguments...);
    }

    template <typename Ret, typename ... Args>
    BOOL32 TFunction<Ret(Args...)>::operator==(const TFunction<Ret(Args...)>& Other)
    {
        return this->FunctionID == Other.FunctionID;
    }

#endif

    // // examples
    // 1, for a normal function,
    // int Test(int any)
    // {
    //     return any;
    // }
    // TFunction<int(int)> Func(Test);
    // int out = Func(5);
    // ---------------------------------
    // 2, for a member function. use lambda
    // class A
    // {
    // public:
    //     int Test(int any)
    //     {
    //         return any;
    //     }
    // };
    // A a;
    // TFunction<int(int)> Func([&a](int m){return a.Test(m);});
    // int out = Func(5);
    // ---------------------------------
}
