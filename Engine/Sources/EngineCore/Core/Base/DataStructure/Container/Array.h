#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{
    template <typename T>
    class TArray
    {
        
    public:
        TArray();
        TArray(const TArray<T>& Other);
        ~TArray();
        
        T& operator[](const UINT& Index);

        UINT Add     (const T& Element);
        T&   Get     (const UINT& Index);
        UINT Find    (const T& Element) const;
        void Resize  (const UINT& NewSize);
        void RemoveAt(const UINT& Index);
        void Remove  (const T& Element);
        void Clear   ();
        
        UINT Length  ()const;
        UINT Last    ()const;
    private:
        class std::vector<T> Elements;
    };

         template <typename T>
    TArray<T>::TArray()
    {
    }

    template <typename T>
    TArray<T>::TArray(const TArray<T>& Other)
    {
        for(UINT i = 0; i < Other.Length() - 1; ++i)
        {
            this->Elements.push_back(Other[i]);
        }
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
        Check(ENGINE_ARRAY_ERROR, "Array has no this index", Index > Length());
        this->Elements.erase(Elements.begin() + Index);
    }

    template <typename T>
    void TArray<T>::Remove(const T& Element)
    {
        const UINT i = Find(Element);
        if(i != -1)
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
    UINT TArray<T>::Find(const T& Element) const
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
    void TArray<T>::Resize(const UINT& NewSize)
    {
        if ((NewSize > 0u) && (this->Elements.size() != NewSize))
        {
            this->Elements.resize(NewSize);
        }
    }

    template <typename T>
    UINT TArray<T>::Length() const
    {
        return this->Elements.size();
    }
};
