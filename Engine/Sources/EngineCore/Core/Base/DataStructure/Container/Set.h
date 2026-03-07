#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <set>
#include "../../../../Core/Config/ErrorCaption.h"
namespace PigeonEngine
{

    // -------------------   Declarations   -----------------------------
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
        
        T&          operator[](const UINT32& Index);
        const T&    operator[](const UINT32& Index)const;

        BOOL32 Add     (const T& Element);
        T&   GetRef     (const UINT32& Index) const;
        // Find the index of given Element, return set's Length if the Element doesn't exist.
        UINT32 Find    (const T& Element) const;
        BOOL32 Contains(const T& Element) const;
        void RemoveAt(const UINT32& Index);
        void Remove  (const T& Element);
        void Empty   ();
        void Append(const TSet<T>& Other);
        
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

        /**
         * DO NOT USE DIRECTLY
         * STL-like iterators to enable range-based for loop support.
         */
        typename TConstIterator begin()const{return Elements.begin();}
        typename TConstIterator end()const{return Elements.end();}
        typename TIterator      begin(){return Elements.begin();}
        typename TIterator      end(){return Elements.end();}
        
        template<typename _TOtherSizeType = INT32, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_NODISCARD _TOtherSizeType Num()const
        {
            return static_cast<_TOtherSizeType>(Elements.size());
        }
        UINT32 Last    ()const;

        
    private:
        class std::set<T> Elements;
    };

    // -------------------   Implementations   -----------------------------
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
    T& TSet<T>::operator[](const UINT32& Index)
    {
        return Get(Index);
    }

    template <typename T>
    const T& TSet<T>::operator[](const UINT32& Index)const
    {
        return Get(Index);
    }

    template <typename T>
    BOOL32 TSet<T>::Add(const T& Element)
    {
        return Elements.insert(Element).second;
    }

    template <typename T>
    T& TSet<T>::GetRef(const UINT32& Index) const
    {
        PE_CHECK(ENGINE_SET_ERROR, "Set has no such index", Index > Length());
        return Elements[Index];
    }

    template <typename T>
    UINT32 TSet<T>::Find(const T& Element) const
    {
        UINT32 iIndex = 0;
        auto it = Elements.begin();
        while(it!=Elements.end())
        {
            if(*it == Element)
            {
                return iIndex;
            }
            iIndex++;
        }
        return ((UINT32)(-1));
    }

    template <typename T>
    BOOL32 TSet<T>::Contains(const T& Element) const
    {
        for(const auto& elem : Elements)
        {
            if(elem == Element)
            {
                return TRUE;
            }
        }
        return FALSE;
    }

    template <typename T>
    void TSet<T>::RemoveAt(const UINT32& Index)
    {
        if (Index > Num<UINT32>())
        {
            return;
        }
        UINT32 iIndex = 0;
        auto it = Elements.begin();
        while(iIndex < Index)
        {
            ++it;
        }
        this->Elements.erase(it);
    }

    template <typename T>
    void TSet<T>::Remove(const T& Element)
    {
        const UINT32& Index = Find(Element);
        RemoveAt(Index);
    }

    template <typename T>
    void TSet<T>::Empty()
    {
        Elements.clear();
    }

    template <typename T>
    void TSet<T>::Append(const TSet<T>& Other)
    {
        for(const auto& elem: Other)
        {
            Add(elem);
        }
    }

    template <typename T>
    UINT32 TSet<T>::Last() const
    {
        return Length() > 0 ? Length() - 1 : 0;
    }
};