#pragma once
#include <functional>

#include "Base/DataStructure/BuiltInType.h"

namespace PigeonEngine
{
    // use std::function for now.
    template <typename FuncType>
    class TFunction;

    template <typename Ret, typename... Args>
    class TFunction<Ret (Args...)>
    {
    public:
        TFunction();
        explicit TFunction(std::function<Ret(Args ...)>&& op);
        ~TFunction();
    public:
        TFunction<Ret(Args...)>& operator=(const TFunction<Ret(Args ...)>& Other);
        TFunction<Ret(Args...)>& operator=(std::function<Ret(Args ...)> Other);
        Ret                      operator()(Args... Arguments);
        BOOL                     operator==(const TFunction<Ret(Args ...)>& Other);
        
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
    BOOL TFunction<Ret(Args...)>::operator==(const TFunction<Ret(Args...)>& Other)
    {
        return this->Operation == Other.Operation;
    }

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
