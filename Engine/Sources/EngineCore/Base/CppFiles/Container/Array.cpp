#include "../../Headers/Container/Array.h"
#include <vector>

namespace PigeonEngine
{
    template <typename T>
    EArray<T>::EArray()
    {
    }

    template <typename T>
    EArray<T>::EArray(const EArray<T>& Other)
    {
        for(UINT i = 0; i < Other.Length() - 1; ++i)
        {
            this->Elements.push_back(Other[i]);
        }
    }

    template <typename T>
    EArray<T>::~EArray()
    {
        
    }

    template <typename T>
    T& EArray<T>::operator[](const UINT& Index)
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        return Elements[Index];
    }

    template <typename T>
    UINT EArray<T>::Last() const
    {
        return Length() > 0 ? Length() - 1 : 0;
    }

    template <typename T>
    UINT EArray<T>::Add(const T& Element)
    {
        return this->Elements.push_back(Element);
    }

    template <typename T>
    T& EArray<T>::Get(const UINT& Index) const
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        return Elements[Index];
    }

    template <typename T>
    void EArray<T>::RemoveAt(const UINT& Index)
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        this->Elements.erase(Elements.begin() + Index - 1);
    }

    template <typename T>
    void EArray<T>::Remove(const T& Element)
    {
        if(const UINT i = Find(Element) != -1)
        {
            RemoveAt(i);
        }
    }

    template <typename T>
    void EArray<T>::Clear()
    {
        Elements.clear();
    }

    template <typename T>
    UINT EArray<T>::Find(const T& Element) const
    {
        for(UINT i = 0; i < Elements.size();++i)
        {
            if(Elements[i] == Element)
            {
                return i;
            }
        }
        return -1;
    }

    template <typename T>
    UINT EArray<T>::Length() const
    {
        return this->Elements.size();
    }
};
