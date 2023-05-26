#pragma once
#include <memory>
#include<CoreMinimal.h>
namespace PigeonEngine
{
    template <typename T>
    class TSharedPtr
    {
    public:
        TSharedPtr();
        TSharedPtr(const TSharedPtr<T>& Other);
        explicit TSharedPtr(const std::shared_ptr<T>& Other);

        T&             operator*()const;
        T*             operator->()const;
        TSharedPtr<T>& operator=(const TSharedPtr<T>& Other);
        TSharedPtr<T>& operator=(const std::shared_ptr<T>& Other);
        
        [[nodiscard]] BOOL IsValid()const;
        
        T* Get() const;

        void Reset();
        static TSharedPtr<T> MakeShared();
        
    private:
        std::shared_ptr<T> Shared;
    };

    template <typename T>
    TSharedPtr<T>::TSharedPtr()
    {
    }

    template <typename T>
    TSharedPtr<T>::TSharedPtr(const TSharedPtr<T>& Other)
        :
    Shared(Other.Shared)
    {
    }

    template <typename T>
    TSharedPtr<T>::TSharedPtr(const std::shared_ptr<T>& Other)
        :
    Shared(Other)
    {
    }

    template <typename T>
    T& TSharedPtr<T>::operator*() const
    {
        return *Shared;
    }

    template <typename T>
    T* TSharedPtr<T>::operator->() const
    {
        return Shared.get();
    }

    template <typename T>
    TSharedPtr<T>& TSharedPtr<T>::operator=(const TSharedPtr<T>& Other)
    {
        Shared = Other.Shared;
        return *this;
    }

    template <typename T>
    TSharedPtr<T>& TSharedPtr<T>::operator=(const std::shared_ptr<T>& Other)
    {
        Shared = Other;
        return *this;
    }

    template <typename T>
    BOOL TSharedPtr<T>::IsValid() const
    {
        return Shared.get() == nullptr;
    }

    template <typename T>
    T* TSharedPtr<T>::Get() const
    {
        return Shared.get();
    }

    template <typename T>
    void TSharedPtr<T>::Reset()
    {
        Shared.reset();
    }

    template <typename T>
    TSharedPtr<T> TSharedPtr<T>::MakeShared()
    {
        return TSharedPtr<T>(std::make_shared<T>());
    }
};

