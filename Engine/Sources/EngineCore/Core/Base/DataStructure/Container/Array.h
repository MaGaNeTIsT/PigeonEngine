#pragma once

#include "../../../../Main/Main.h"

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
        T&   Get     (const UINT& Index) const;
        UINT Find    (const T& Element)const;
        void Resize  (const UINT& NewSize);
        void RemoveAt(const UINT& Index);
        void Remove  (const T& Element);
        void Clear   ();
        
        UINT Length  ()const;
        UINT Last    ()const;
    private:
        class std::vector<T> Elements;
    };
};
