#pragma once

#include <Main.h>
namespace PigeonEngine
{
    template <typename T>
    class EArray
    {
        
    public:
        EArray();
        EArray(const EArray<T>& Other);
        ~EArray();
        
        T& operator[](const UINT& Index);

        UINT Add     (const T& Element);
        T&   Get     (const UINT& Index) const;
        UINT Find    (const T& Element)const;
        void RemoveAt(const UINT& Index);
        void Remove  (const T& Element);
        void Clear   ();
        
        UINT Length  ()const;
        UINT Last    ()const;
    private:
        class std::vector<T> Elements;
    };

    
};
