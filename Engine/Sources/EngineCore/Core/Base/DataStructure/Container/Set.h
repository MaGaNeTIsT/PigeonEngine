#pragma once
#include <CoreMinimal.h>
#include <set>

namespace PigeonEngine
{
    template <typename T>
    class TSet
    {
    public:
        TSet();
        TSet(const TSet<T>& Other);
        TSet(const std::set<T>& Other);
        ~TSet();
        
        T& operator[](const UINT& Index);

        UINT Add     (const T& Element);
        T&   Get     (const UINT& Index) const;
        UINT Find    (const T& Element) const;
        void RemoveAt(const UINT& Index);
        void Remove  (const T& Element);
        void Clear   ();

        typename std::set<T>::iterator begin();
        typename std::set<T>::iterator end();
        
        UINT Length  ()const;
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
    void TSet<T>::RemoveAt(const UINT& Index)
    {
        Check(ENGINE_SET_ERROR, "Set has no such index", Index > Length());
        this->Elements.erase(Elements.begin() + Index - 1);
    }

    template <typename T>
    void TSet<T>::Remove(const T& Element)
    {
        UINT Index = Find(Element);
        RemoveAt(Index);
    }

    template <typename T>
    void TSet<T>::Clear()
    {
        Elements.clear();
    }

    template <typename T>
    typename std::set<T>::iterator TSet<T>::begin()
    {
        return Elements.begin();
    }

    template <typename T>
    typename std::set<T>::iterator TSet<T>::end()
    {
        return Elements.end();
    }

    template <typename T>
    UINT TSet<T>::Length() const
    {
        return Elements.size();
    }

    template <typename T>
    UINT TSet<T>::Last() const
    {
        return Length() > 0 ? Length() - 1 : 0;
    }
   
};