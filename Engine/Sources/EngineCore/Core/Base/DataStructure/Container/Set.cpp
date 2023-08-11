#include "Set.h"

namespace PigeonEngine
{
    template <typename T>
    TSet<T>::TSet()
    {
    }

    template <typename T>
    TSet<T>::TSet(const TSet<T>& Other)
        :
    Elements(Other.Elements)
    {
    }

    template <typename T>
    TSet<T>::TSet(const std::set<T>& Other)
        :
    Elements(Other)
    {
    }

    template <typename T>
    TSet<T>::~TSet()
    {
    }

    template <typename T>
    T& TSet<T>::operator[](const UINT& Index)
    {
        return Get(Index);
    }

    template <typename T>
    UINT TSet<T>::Add(const T& Element)
    {
        return Elements.insert(Element);
    }

    template <typename T>
    T& TSet<T>::Get(const UINT& Index) const
    {
        Check(ENGINE_SET_ERROR, "Set has no such index", Index > Length());
        return Elements[Index];
    }

    template <typename T>
    UINT TSet<T>::Find(const T& Element) const
    {
        return Elements.find(Element) - Elements.begin();
    }

    template <typename T>
    BOOL TSet<T>::Contains(const T& Element) const
    {
        for(const auto& elem : Elements)
        {
            if(elem == Element)
            {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    void TSet<T>::RemoveAt(const UINT& Index)
    {
        if(Index > Length())
        {
            return;
        }
        this->Elements.erase(Elements.begin() + Index );
    }

    template <typename T>
    void TSet<T>::Remove(const T& Element)
    {
        const UINT& Index = Find(Element);
        RemoveAt(Index);
    }

    template <typename T>
    void TSet<T>::Clear()
    {
        Elements.clear();
    }

    template <typename T>
    UINT TSet<T>::Last() const
    {
        return Length() > 0 ? Length() - 1 : 0;
    }
}
