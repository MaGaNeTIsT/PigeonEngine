#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <set>

namespace PigeonEngine
{
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
        
        T& operator[](const UINT& Index);

        UINT Add     (const T& Element);
        T&   Get     (const UINT& Index) const;
        // Find the index of given Element, return set's Length if the Element doesn't exist.
        UINT Find    (const T& Element) const;
        BOOL Contains(const T& Element) const;
        void RemoveAt(const UINT& Index);
        void Remove  (const T& Element);
        void Clear   ();

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
        UINT Length()const
        {
            return static_cast<UINT>(Elements.size());
        }
        UINT Last    ()const;
    private:
        class std::set<T> Elements;
    };

   
};