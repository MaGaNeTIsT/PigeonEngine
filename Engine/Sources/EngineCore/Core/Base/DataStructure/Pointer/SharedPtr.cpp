#include "SharedPtr.h"

namespace PigeonEngine
{
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
    TSharedPtr<T>::operator bool() const
    {
        return IsValid();
    }

    template <typename T>
    bool TSharedPtr<T>::operator==(const TSharedPtr<T>& Other)
    {
        return this->Shared==Other.Shared;
    }

    template <typename T>
    bool TSharedPtr<T>::operator!=(const TSharedPtr<T>& Other)
    {
        return !(*this==Other);
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
    template <class ... Types>
    TSharedPtr<T> TSharedPtr<T>::MakeShared(Types&&... Args)
    {
        return TSharedPtr<T>(std::make_shared<T>(Args));
    }


}
