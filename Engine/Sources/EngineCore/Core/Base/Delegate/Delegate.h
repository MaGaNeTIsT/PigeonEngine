#pragma once
#include "Function.h"
#include "Base/DataStructure/Container/Array.h"


namespace PigeonEngine
{
    template <typename FuncType>
    class TDelegate;

    // Prototype, should not be used directly
    template <typename Ret, typename... Args>
    class TDelegate<Ret (Args...)>
    {
    public:
        TDelegate<Ret (Args...)>() = default;
        virtual ~TDelegate() = default;
    public:
        // virtual void Add(const TFunction<Ret (Args...)>& New) = 0;
        // virtual void Remove(const TFunction<Ret (Args...)>& Old) = 0;
        // virtual void RemoveAll() = 0;
        // virtual void Bind(const TFunction<Ret (Args...)>& New) = 0;
        // virtual void Unbind() = 0;
        // virtual void Broadcast(Args... Arguments) = 0;
        // virtual Ret  Execute(Args... Arguments) = 0;
   
    };

    // Delegate no return, multi execution
    template <typename... Args>
    class TDelegateBroadcast<Args...> : public TDelegate<void (Args...)>
    {
    public:
        ~TDelegateBroadcast<Args...>() override {RemoveAll();}
    public:
        void Add(const TFunction<void (Args...)>& New) override;
        void Remove(const TFunction<void (Args...)>& Old) override;
        void RemoveAll() override;
        void Broadcast(Args... Arguments) override;
    private:
        TArray<TFunction<void (Args...)>> ToCall{};
    };
    
    // Delegate no return, multi execution
    template <typename Ret, typename... Args>
    class TDelegateSingle<Ret(Args...)> : public TDelegate<Ret (Args...)>
    {
    public:
        ~TDelegateSingle<Ret(Args...)>() override {Unbind();}
    public:
        virtual void Bind(const TFunction<Ret (Args...)>& New);
        virtual void Unbind();
        virtual Ret  Execute(Args... Arguments);
    private:
        TFunction<Ret(Args...)> Ops = nullptr;
    };

    
    template <typename ... Args>
    void TDelegateBroadcast<Args...>::Add(const TFunction<void(Args...)>& New)
    {
        ToCall.Add(New);
    }

    template <typename ... Args>
    void TDelegateBroadcast<Args...>::Remove(const TFunction<void(Args...)>& Old)
    {
        ToCall.Remove(Old);
    }

    template <typename ... Args>
    void TDelegateBroadcast<Args...>::RemoveAll()
    {
        ToCall.Clear();
    }

    template <typename ... Args>
    void TDelegateBroadcast<Args...>::Broadcast(Args... Arguments)
    {
        for(const auto& elem : ToCall)
        {
            elem(Arguments...);
        }
    }

    template <typename Ret, typename ... Args>
    void TDelegateSingle<Ret, Args...>::Bind(const TFunction<Ret(Args...)>& New)
    {
        this->Ops = New;
    }

    template <typename Ret, typename ... Args>
    void TDelegateSingle<Ret, Args...>::Unbind()
    {
        this->Ops = nullptr;
    }

    template <typename Ret, typename ... Args>
    Ret TDelegateSingle<Ret, Args...>::Execute(Args... Arguments)
    {
        return this->Ops(Arguments);
    }
}
