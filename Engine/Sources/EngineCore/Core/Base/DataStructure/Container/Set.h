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
        explicit TSet(const std::set<T>& Other);
        ~TSet();
        
        T& operator[](const UINT& Index);

        UINT Add     (const T& Element);
        T&   Get     (const UINT& Index) const;
        // Find the index of given Element, return set's Length if the Element doesn't exist.
        UINT Find    (const T& Element) const;
        BOOL Contains(const T& Element) const;
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

   
};