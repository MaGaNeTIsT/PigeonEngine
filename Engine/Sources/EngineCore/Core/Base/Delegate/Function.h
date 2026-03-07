#pragma once

#include "Base/DataStructure/BuiltInType.h"
#include <functional>

namespace PigeonEngine
{

#if 1

    // use std::function for now.
    template <typename FuncType>
    class TFunction;

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
        std::function<Ret(Args...)> Operation = nullptr;
    };

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction()
    {
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(std::function<Ret(Args...)>&& op): Operation(std::move(op))
    {}

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(const TFunction<Ret(Args...)>& Other):Operation(Other.Operation)
    {
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::~TFunction()
    {Operation = nullptr;}

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>& TFunction<Ret(Args...)>::operator=(const TFunction<Ret(Args...)>& Other)
    {
        this->Operation = Other.Operation;
        return *this;
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>& TFunction<Ret(Args...)>::operator=(std::function<Ret(Args...)> Other)
    {
        this->Operation = std::move(Other);
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
        return this->Operation == Other.Operation;
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
