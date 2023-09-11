#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include "../../../../Core/Config/ErrorCaption.h"
#if _EDITOR_ONLY
#include "../../../../../Development/Alert/DevelopmentDefines.h"
#endif

namespace PigeonEngine
{
    template <typename T>
    class TSharedPtr
    {
    public:
        TSharedPtr();
        TSharedPtr(const TSharedPtr<T>& Other);
        explicit TSharedPtr(const std::shared_ptr<T>& Other);

        T&              operator*()const;
        T*              operator->()const;
        TSharedPtr<T>&  operator=(const TSharedPtr<T>& Other);
        TSharedPtr<T>&  operator=(const std::shared_ptr<T>& Other);
        explicit        operator BOOL32() const;
        BOOL32          operator==(const TSharedPtr<T>& Other);
        BOOL32          operator!=(const TSharedPtr<T>& Other);
        
        PE_NODISCARD BOOL32 IsValid()const;
        
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
    TSharedPtr<T>::operator BOOL32() const
    {
        return IsValid();
    }

    template <typename T>
    BOOL32 TSharedPtr<T>::operator==(const TSharedPtr<T>& Other)
    {
        return this->Shared==Other.Shared;
    }

    template <typename T>
    BOOL32 TSharedPtr<T>::operator!=(const TSharedPtr<T>& Other)
    {
        return !(*this==Other);
    }

    template <typename T>
    BOOL32 TSharedPtr<T>::IsValid() const
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
};

 