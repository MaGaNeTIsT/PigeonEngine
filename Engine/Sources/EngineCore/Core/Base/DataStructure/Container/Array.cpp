#include "Array.h"
#include <algorithm>

namespace PigeonEngine
{
    template <typename T>
    TArray<T>::TArray()
    {
    }

    template <typename T>
    TArray<T>::TArray(const TArray<T>& Other)
        :
    Elements(Other.Elements)
    {
        
    }

    template <typename T>
    TArray<T>::TArray(const std::vector<T>& Other)
        :
    Elements(Other)
    {
    }

    template <typename T>
    TArray<T>::TArray(TArray<T>&& Other) noexcept
        :
    Elements(std::move(Other.Elements))
    {
        Other.Clear();
    }

    template <typename T>
    TArray<T>::TArray(std::vector<T>&& Other)
        :
    Elements(std::move(Other))
    {
        Other.clear();
    }

    template <typename T>
    T& TArray<T>::operator[](const UINT& Index)
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        return Elements[Index];
    }

    template <typename T>
    const T& TArray<T>::operator[](const UINT& Index)const
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        return Elements[Index];
    }

    template <typename T>
    TArray<T>& TArray<T>::operator=(const TArray<T>& Other)
    {
         Elements = Other.Elements;
         return *this;
    }

    template <typename T>
    TArray<T>& TArray<T>::operator=(TArray<T>&& Other) noexcept
    {
        Elements = std::move(Other.Elements);
        Other.Clear();
        return *this;
    }

    template <typename T>
    TArray<T>& TArray<T>::operator=(std::vector<T>&& Other)
    {
         Elements = std::move(Other);
         Other.clear();
         return *this;
    }


    template <typename T>
    UINT TArray<T>::LastIndex() const
    {
        return Length() > 0 ? Length() - 1 : 0;
    }

    template <typename T>
    UINT TArray<T>::Add(const T& Element)
    {
         this->Elements.push_back(Element);
         return LastIndex();
    }

    template <typename T>
    T& TArray<T>::GetRef(const UINT& Index)
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        return this->Elements[Index];
    }

    template <typename T>
    const T& TArray<T>::Get(const UINT& Index)const
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        return this->Elements[Index];
    }

    template <typename T>
    const T& TArray<T>::Last()
    {
        return Elements[LastIndex()];
    }

    template <typename T>
    void TArray<T>::RemoveAt(const UINT& Index)
    {
        if(Index > Length())
        {
            return;
        }
        this->Elements.erase(Elements.begin() + Index);
    }

    template <typename T>
    void TArray<T>::Remove(const T& Element)
    {
        const UINT& i = Find(Element);
        if(i != Length())
        {
            RemoveAt(i);
        }
    }

    template <typename T>
    void TArray<T>::Pop()
    {
        Elements.pop_back();
    }

    template <typename T>
    void TArray<T>::Clear()
    {
        Elements.clear();
    }

    template <typename T>
    void TArray<T>::Append(const TArray<T>& Other)
    {
         const UINT Size = Length() + Other.Length();
         Recapacity(Size * 2);
         for(const auto& elem:Other)
         {
             Add(elem);
         }
    }

    template <typename T>
    void TArray<T>::Sort()
    {
        std::sort(Elements.begin(), Elements.end());
    }

    template <typename T>
    void TArray<T>::Sort(bool Predicate(const T&, const T&))
    {
        std::sort(Elements.begin(), Elements.end(),Predicate);
    }
    
    template <typename T>
    void TArray<T>::Shuffle()
    {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::shuffle(Elements.begin(), Elements.end(),rng);
    }

    template <typename T>
    TArray<T> TArray<T>::Reverse() const
    {
        std::vector<T> Temp = Elements;
        std::reverse(Temp.begin(), Temp.end());
        return TArray<T>(Temp);
    }

    template <typename T>
    TArray<T>::TIterator TArray<T>::Begin()
    {
        return Elements.begin();
    }

    template <typename T>
    TArray<T>::TConstIterator TArray<T>::Begin()const
    {
        return Elements.begin();
    }

    template <typename T>
    TArray<T>::TIterator TArray<T>::End()
    {
        return Elements.end();
    }

    template <typename T>
    TArray<T>::TConstIterator TArray<T>::End()const
    {
        return Elements.end();
    }

    template <typename T>
    TArray<T>::TIterator TArray<T>::Rbegin()
    {
        return Elements.rbegin();
    }

    template <typename T>
    TArray<T>::TIterator TArray<T>::Rend()
    {
        return Elements.rend();
    }

    template <typename T>
    UINT TArray<T>::Find(const T& Element) const
    {
        for(UINT i = 0; i < Elements.size();++i)
        {
            if(Elements[i] == Element)
            {
                return i;
            }
        }
        return Length();
    }

    template <typename T>
    BOOL TArray<T>::Contains(const T& Element) const
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
    void TArray<T>::Resize(const UINT& NewSize)
    {
        if ((NewSize > 0u) && (this->Elements.size() != NewSize))
        {
            this->Elements.resize(NewSize);
        }
    }

    template <typename T>
    void TArray<T>::Recapacity(const UINT& NewCapacity)
    {
         if ((NewCapacity > 0u) && (this->Elements.capacity() != NewCapacity))
         {
             this->Elements.reserve(NewCapacity);
         }
    }

    template <typename T>
    UINT TArray<T>::Length() const
    {
        return this->Elements.size();
    }
    
}
