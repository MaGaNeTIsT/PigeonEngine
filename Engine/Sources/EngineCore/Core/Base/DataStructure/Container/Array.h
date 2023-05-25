#pragma once

#include <CoreMinimal.h>

#include <algorithm>
#include <random>



namespace PigeonEngine
{
    template <typename T>
    class TArray
    {
        
    public:
        TArray();
        TArray(const TArray<T>& Other);
        explicit TArray(const std::vector<T>& Other);

        ~TArray();
        
        T&      operator[](const UINT& Index);
        TArray& operator= (const TArray<T>& Other);
        
        UINT Add       (const T& Element);
        T&   Get       (const UINT& Index);
        // Find the index of given Element, return array's Length if the Element doesn't exist.
        UINT Find      (const T& Element) const;
        BOOL Contains  (const T& Element) const;
        void Resize    (const UINT& NewSize);
        void Recapacity(const UINT& NewCapacity);
        void RemoveAt  (const UINT& Index);
        void Remove    (const T& Element);
        void Clear     ();

        void Append(const TArray<T>& Other);

        void Sort   ();
        void Sort   (bool Predicate(const T&, const T&));
        void Shuffle();
        
        typename std::vector<T>::iterator begin();
        typename std::vector<T>::iterator end();
        typename std::vector<T>::iterator rbegin();
        typename std::vector<T>::iterator rend();

        [[nodiscard]] UINT Length  ()const;
        [[nodiscard]] UINT Last    ()const;
    private:
        class std::vector<T> Elements;
    };

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
    TArray<T>::~TArray()
    {
        
    }

    template <typename T>
    T& TArray<T>::operator[](const UINT& Index)
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
    UINT TArray<T>::Last() const
    {
        return Length() > 0 ? Length() - 1 : 0;
    }

    template <typename T>
    UINT TArray<T>::Add(const T& Element)
    {
         this->Elements.push_back(Element);
         return Last();
    }

    template <typename T>
    T& TArray<T>::Get(const UINT& Index)
    {
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        return this->Elements[Index];
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
    typename std::vector<T>::iterator TArray<T>::begin()
     {
         return Elements.begin();
     }

    template <typename T>
    typename std::vector<T>::iterator TArray<T>::end()
     {
         return Elements.end(); 
     }

    template <typename T>
    typename std::vector<T>::iterator TArray<T>::rbegin()
    {
        return Elements.rbegin();
    }

    template <typename T>
    typename std::vector<T>::iterator TArray<T>::rend()
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
};
