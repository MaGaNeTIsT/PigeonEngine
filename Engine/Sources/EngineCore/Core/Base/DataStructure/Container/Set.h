#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <set>
#include "../../../../Core/Config/ErrorCaption.h"
namespace PigeonEngine
{
    template <typename T>
    class TSet
    {
    public:
        using TIterator         = typename std::set<T>::iterator;
        using TConstIterator    = typename std::set<T>::const_iterator;
    public:
        TSet();
        TSet(const TSet<T>& Other);
        explicit TSet(const std::set<T>& Other);
        ~TSet();
        
        T& operator[](const UINT& Index);

        BOOL Add     (const T& Element);
        T&   Get     (const UINT& Index) const;
        // Find the index of given Element, return set's Length if the Element doesn't exist.
        UINT Find    (const T& Element) const;
        BOOL Contains(const T& Element) const;
        void RemoveAt(const UINT& Index);
        void Remove  (const T& Element);
        void Clear   ();

        typename TIterator Begin()
        {
            return Elements.begin();
        }
        typename TConstIterator Begin() const
        {
            return Elements.begin();
        }
        typename TIterator End()
        {
            return Elements.end();
        }
        typename TConstIterator End() const
        {
            return Elements.end();
        }
        UINT Length()const
        {
            return static_cast<UINT>(Elements.size());
        }
        UINT Last    ()const;
    private:
        class std::set<T> Elements;
    };

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
    BOOL TSet<T>::Add(const T& Element)
    {
        return Elements.insert(Element).second;
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
};