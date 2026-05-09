#pragma once

#include "Base/DataStructure/BuiltInType.h"
#include <functional>
#include <type_traits>

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
        TFunction(std::nullptr_t);
        TFunction(std::function<Ret(Args ...)>&& op);
        TFunction(const std::function<Ret(Args ...)>& op);

        // Generic callable constructor (lambdas, function pointers, functors).
        // SFINAE excludes the TFunction copy/move overloads so they win.
        template <typename CallableType, typename = typename std::enable_if<
            !std::is_same<typename std::decay<CallableType>::type, TFunction<Ret(Args...)>>::value &&
            !std::is_same<typename std::decay<CallableType>::type, std::nullptr_t>::value&&
            std::is_constructible<std::function<Ret(Args...)>, CallableType>::value
        >::type>
        TFunction(CallableType&& InCallable);

        TFunction(const TFunction<Ret(Args ...)>& Other);
        TFunction(TFunction<Ret(Args ...)>&& Other) noexcept;
        ~TFunction();
    public:
        TFunction<Ret(Args...)>&    operator=(const TFunction<Ret(Args ...)>& Other);
        TFunction<Ret(Args...)>&    operator=(TFunction<Ret(Args ...)>&& Other) noexcept;
        TFunction<Ret(Args...)>&    operator=(std::nullptr_t);
        Ret                         operator()(Args... Arguments) const;
        BOOL8                       operator==(const TFunction<Ret(Args ...)>& Other) const;
        BOOL8                       operator!=(const TFunction<Ret(Args ...)>& Other) const;

        // True if a callable is bound and safe to invoke.
        BOOL8                       IsValid() const noexcept;
        explicit                    operator BOOL8() const noexcept;

        // Drops the bound callable; subsequent IsValid() returns FALSE.
        void                     Reset();

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
    TFunction<Ret(Args...)>::TFunction(std::nullptr_t)
        : Operation(nullptr)
    {
        FunctionID = AllocFunctionID();
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(std::function<Ret(Args...)>&& op)
        : Operation(std::move(op))
    {
        FunctionID = AllocFunctionID();
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(const std::function<Ret(Args...)>& op)
        : Operation(op)
    {
        FunctionID = AllocFunctionID();
    }

    template <typename Ret, typename ... Args>
    template <typename CallableType, typename>
    TFunction<Ret(Args...)>::TFunction(CallableType&& InCallable)
        : Operation(std::forward<CallableType>(InCallable))
    {
        FunctionID = AllocFunctionID();
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(const TFunction<Ret(Args...)>& Other)
        : Operation(Other.Operation), FunctionID(Other.FunctionID)
    {
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::TFunction(TFunction<Ret(Args...)>&& Other) noexcept
        : Operation(std::move(Other.Operation)), FunctionID(Other.FunctionID)
    {
        Other.Operation = nullptr;
        Other.FunctionID = 0u;
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::~TFunction()
    {
        Operation = nullptr;
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>& TFunction<Ret(Args...)>::operator=(const TFunction<Ret(Args...)>& Other)
    {
        if (this != &Other)
        {
            this->Operation = Other.Operation;
            this->FunctionID = Other.FunctionID;
        }
        return *this;
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>& TFunction<Ret(Args...)>::operator=(TFunction<Ret(Args...)>&& Other) noexcept
    {
        if (this != &Other)
        {
            this->Operation = std::move(Other.Operation);
            this->FunctionID = Other.FunctionID;
            Other.Operation = nullptr;
            Other.FunctionID = 0u;
        }
        return *this;
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>& TFunction<Ret(Args...)>::operator=(std::nullptr_t)
    {
        this->Operation = nullptr;
        this->FunctionID = 0u;
        return *this;
    }

    template <typename Ret, typename ... Args>
    Ret TFunction<Ret(Args...)>::operator()(Args... Arguments) const
    {
        return Operation(Arguments...);
    }

    template <typename Ret, typename ... Args>
    BOOL8 TFunction<Ret(Args...)>::operator==(const TFunction<Ret(Args...)>& Other) const
    {
        return ((this->FunctionID == Other.FunctionID) ? TRUE : FALSE);
    }

    template <typename Ret, typename ... Args>
    BOOL8 TFunction<Ret(Args...)>::operator!=(const TFunction<Ret(Args...)>& Other) const
    {
        return ((this->FunctionID != Other.FunctionID) ? TRUE : FALSE);
    }

    template <typename Ret, typename ... Args>
    BOOL8 TFunction<Ret(Args...)>::IsValid() const noexcept
    {
        return (static_cast<BOOL8>(Operation) ? TRUE : FALSE);
    }

    template <typename Ret, typename ... Args>
    TFunction<Ret(Args...)>::operator BOOL8() const noexcept
    {
        return static_cast<BOOL8>(Operation);
    }

    template <typename Ret, typename ... Args>
    void TFunction<Ret(Args...)>::Reset()
    {
        Operation = nullptr;
        FunctionID = 0u;
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
    // 3, validity check before calling
    // TFunction<void()> Func;
    // if (Func.IsValid())  // or: if (Func)
    // {
    //     Func();
    // }
    // ---------------------------------
}
