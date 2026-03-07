#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include "../../../../Core/Config/ErrorCaption.h"
#include "../../../../../Development/Alert/DevelopmentDefines.h"

namespace PigeonEngine
{

#if 0

    template <typename T>
    class TSharedPtr
    {
    public:
        TSharedPtr();
        TSharedPtr(const TSharedPtr<T>& Other);
        TSharedPtr(const std::shared_ptr<T>& Other);
        TSharedPtr(T* value = nullptr);

        T&              operator*()const;
        T*              operator->()const;
        TSharedPtr<T>&  operator=(T* value);
        TSharedPtr<T>&  operator=(const TSharedPtr<T>& Other);
        TSharedPtr<T>&  operator=(const std::shared_ptr<T>& Other);
        explicit        operator BOOL8() const;
        BOOL8           operator!() const;
        BOOL8           operator==(const TSharedPtr<T>& Other) const;
        BOOL8           operator!=(const TSharedPtr<T>& Other);
        
        PE_NODISCARD BOOL8 IsValid()const;
        
        T* Get() const;

        void Reset();
        
        template <class... Types>
        static TSharedPtr<T> MakeShared(Types&&... Args);
        
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
    TSharedPtr<T>::TSharedPtr(T* value)
        :
    Shared(value ? std::make_shared<T>(*value) : nullptr)
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
    TSharedPtr<T>& TSharedPtr<T>::operator=(T* value)
    {
        Shared = (value ? std::make_shared<T>(*value) : nullptr);
        return *this;
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
    TSharedPtr<T>::operator BOOL8() const
    {
        return IsValid();
    }

    template <typename T>
    BOOL8 TSharedPtr<T>::operator!() const
    {
        return this->Shared == nullptr;
    }

    template <typename T>
    BOOL8 TSharedPtr<T>::operator==(const TSharedPtr<T>& Other) const
    {
        return this->Shared == Other.Shared;
    }

    template <typename T>
    BOOL8 TSharedPtr<T>::operator!=(const TSharedPtr<T>& Other)
    {
        return !(*this==Other);
    }

    template <typename T>
    BOOL8 TSharedPtr<T>::IsValid() const
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
    template <class ... Types>
    TSharedPtr<T> TSharedPtr<T>::MakeShared(Types&&... Args)
    {
        return TSharedPtr<T>(std::make_shared<T>(Args...));
    }

#endif

}