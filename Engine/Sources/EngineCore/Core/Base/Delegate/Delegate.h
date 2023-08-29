#pragma once
#include "Function.h"
#include "Base/DataStructure/Container/Array.h"


namespace PigeonEngine
{
    template <typename FuncType>
    class TDelegate;
    template <typename Ret, typename... Args>
    class TDelegate<Ret (Args...)>
    {
    public:
        void Add(const TFunction<Ret (Args...)>& New);
        void Excute(Args... Arguments);
    private:
        TArray<TFunction<Ret (Args...)>> ToCall;
    };

    template <typename Ret, typename ... Args>
    void TDelegate<Ret(Args...)>::Add(const TFunction<Ret(Args...)>& New)
    {
        ToCall.Add(New);
    }

    template <typename Ret, typename ... Args>
    void TDelegate<Ret(Args...)>::Excute(Args... Arguments)
    {
        for(const auto& Elem : ToCall)
        {
            Elem(Arguments...);
        }
    }
}
