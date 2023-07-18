#pragma once

#include <CoreMinimal.h>


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
        TArray(TArray<T>&& Other) noexcept;
        explicit TArray(std::vector<T>&& Other);
        
        ~TArray() = default;
        
        T&          operator[](const UINT& Index);
        const T&    operator[](const UINT& Index)const;
        TArray<T>&  operator= (const TArray<T>& Other);
        TArray<T>&  operator= (TArray<T>&& Other) noexcept;
        TArray<T>&  operator= (std::vector<T>&& Other);
        
        UINT        Add       (const T& Element);
        T&          Get       (const UINT& Index);
        const T&    Get       (const UINT& Index)const;
        // Find the index of given Element, return array's Length if the Element doesn't exist.
        UINT Find      (const T& Element) const;
        BOOL Contains  (const T& Element) const;
        void Resize    (const UINT& NewSize);
        void Recapacity(const UINT& NewCapacity);
        void RemoveAt  (const UINT& Index);
        void Remove    (const T& Element);
        void Pop       ();
        void Clear     ();

        void Append(const TArray<T>& Other);

        void Sort   ();
        void Sort   (bool Predicate(const T&, const T&));
        void Shuffle();
        
        typename std::vector<T>::iterator begin();
        typename std::vector<T>::iterator end();
        typename std::vector<T>::iterator rbegin();
        typename std::vector<T>::iterator rend();

        ENGINE_NODISCARD UINT Length  ()const;
        ENGINE_NODISCARD UINT Last    ()const;
    private:
        class std::vector<T> Elements;
    };

    
};
