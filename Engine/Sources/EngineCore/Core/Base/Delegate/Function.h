#pragma once
#include <functional>

namespace PigeonEngine
{
    // use std::function for now.
    template <typename FuncType>
    class TFunction;

    template <typename Ret, typename... Args>
    class TFunction<Ret (Args...)>
    {
    public:
        TFunction(std::function<Ret(Args...)> Op) : Operation(Op) {}

        Ret operator()(Args... Arguments){return Operation(Arguments...);}
    private:
        std::function<Ret(Args...)> Operation;
    };

    // // examples
    // 1, for a normal function,
    // int Test(int any)
    // {
    //     return any;
    // }
    // TFunction<int(int)> Func(Test);
    // int out = Func(5);
    // ---------------------------------
    // 2, for a member function.
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
