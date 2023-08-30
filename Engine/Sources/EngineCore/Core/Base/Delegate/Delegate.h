#pragma once
#include "Function.h"
#include "Base/DataStructure/Container/Array.h"


namespace PigeonEngine
{
    /*------------------ Declaration ------------------*/
    template <typename FuncType>
    class TDelegate;

    // Prototype, should not be used directly
    template <typename Ret, typename... Args>
    class TDelegate<Ret (Args...)>
    {

    };

    // Delegate no return, multi execution
    template <typename... Args>
    class TDelegateBroadcast<void (Args...)> : public TDelegate<void (Args...)>
    {
    public:
        ~TDelegateBroadcast<void (Args...)>() override {RemoveAll();}
    public:
        void Add(const TFunction<void (Args...)>& New) override;
        void Remove(const TFunction<void (Args...)>& Old) override;
        void RemoveAll() override;
        void Broadcast(Args... Arguments) override;
    private:
        TArray<TFunction<void (Args...)>> ToCall{};
    };
    
    // Delegate with return, single execution
    template <typename Ret, typename... Args>
    class TDelegateSingle<Ret(Args...)> : public TDelegate<Ret (Args...)>
    {
    public:
        ~TDelegateSingle<Ret(Args...)>() {Unbind();}
    public:
        void Bind(const TFunction<Ret (Args...)>& New);
        void Unbind();
        Ret  Execute(Args... Arguments);
    private:
        TFunction<Ret(Args...)> Ops = nullptr;
    };

    /*------------------ Macros ------------------*/
#define MAKE_DELEGATE_MULTI_ONE_PARAM(_DelegateName, _ParamType)\
    class _DelegateName : public PigeonEngine::TDelegateBroadcast<void (_ParamType)>{};
#define MAKE_DELEGATE_MULTI_TWO_PARAM(_DelegateName, _ParamType1, _ParamType2)\
    class _DelegateName : public PigeonEngine::TDelegateBroadcast<void ((_ParamType1), (_ParamType2))>{};
#define MAKE_DELEGATE_MULTI_THREE_PARAM(_DelegateName, _ParamType1, _ParamType2, _ParamType3)\
    class _DelegateName : public PigeonEngine::TDelegateBroadcast<void ((_ParamType1), (_ParamType2), (_ParamType3))>{};
#define MAKE_DELEGATE_MULTI_FOUR_PARAM(_DelegateName, _ParamType1, _ParamType2, _ParamType3, _ParamType4)\
    class _DelegateName : public PigeonEngine::TDelegateBroadcast<void ((_ParamType1), (_ParamType2), (_ParamType3), (_ParamType4))>{};
#define MAKE_DELEGATE_MULTI_FIVE_PARAM(_DelegateName, _ParamType1, _ParamType2, _ParamType3, _ParamType4, _ParamType5)\
    class _DelegateName : public PigeonEngine::TDelegateBroadcast<void ((_ParamType1), (_ParamType2), (_ParamType3), (_ParamType4), (_ParamType5))>{};

#define MAKE_DELEGATE_SINGLE_RET_ONE_PARAM(_DelegateName, _ReturnType, _ParamType)\
    class _DelegateName : public PigeonEngine::TDelegateSingle<_ReturnType((_ParamType))>{};
#define MAKE_DELEGATE_SINGLE_RET_TWO_PARAM(_DelegateName, _ReturnType, _ParamType1, _ParamType2)\
    class _DelegateName : public PigeonEngine::TDelegateSingle<_ReturnType((_ParamType1), (_ParamType2))>{};
#define MAKE_DELEGATE_SINGLE_RET_THREE_PARAM(_DelegateName, _ReturnType, _ParamType1, _ParamType2, _ParamType3)\
    class _DelegateName : public PigeonEngine::TDelegateSingle<_ReturnType((_ParamType1), (_ParamType2), (_ParamType3))>{};
#define MAKE_DELEGATE_SINGLE_RET_FOUR_PARAM(_DelegateName, _ReturnType, _ParamType1, _ParamType2, _ParamType3, _ParamType4)\
    class _DelegateName : public PigeonEngine::TDelegateSingle<_ReturnType((_ParamType1), (_ParamType2), (_ParamType3), (_ParamType4))>{};
#define MAKE_DELEGATE_SINGLE_RET_FIVE_PARAM(_DelegateName, _ReturnType, _ParamType1, _ParamType2, _ParamType3, _ParamType4, _ParamType5)\
    class _DelegateName : public PigeonEngine::TDelegateSingle<_ReturnType((_ParamType1), (_ParamType2), (_ParamType3), (_ParamType4), (_ParamType5))>{};


    /*------------------ Implementation ------------------*/
    template <typename ... Args>
    void TDelegateBroadcast<Args...>::Add(const TFunction<void(Args...)>& New)
    {
        ToCall.Add(New);
        MAKE_DELEGATE_MULTI_ONE_PARAM(FASDAS, int);
        FASDAS in;
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
        if(this->Ops)
  