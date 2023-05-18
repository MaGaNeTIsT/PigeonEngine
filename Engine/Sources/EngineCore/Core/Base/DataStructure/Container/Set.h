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
        ~TSet();
        
        T& operator[](const UINT& Index);

        UINT Add     (const T& Element);
        T&   Get     (const UINT& Index) const;
        UINT Find    (const T& Element) const;
        void RemoveAt(const UINT& Index);
        void Remove  (const T& Element);
        void Clear   ();
        
        UINT Length  ()const;
        UINT Last    ()const;
    private:
        class std::set<T> Elements;
    };

   
};