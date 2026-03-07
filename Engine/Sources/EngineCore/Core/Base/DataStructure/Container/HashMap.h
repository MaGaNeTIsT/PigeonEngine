#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <unordered_map>
#include "Array.h"

namespace PigeonEngine
{
    // -------------------   Declarations   -----------------------------
    template
    <
        typename K,
        typename V,
        class Hash  = std::hash<K>,         
        class Pred  = std::equal_to<K>,       
        class Alloc = std::allocator<std::pair<const K,V>>
    >
    class THashMap
    {
    public:
        using TIterator         = typename std::unordered_map<K, V, Hash, Pred, Alloc>::iterator;
        using TConstIterator    = typename std::unordered_map<K, V, Hash, Pred, Alloc>::const_iterator;
    public:
        THashMap();
        explicit THashMap(const THashMap<K, V, Pred, Alloc>& Other);
        explicit THashMap(const std::unordered_map<K, V, Pred, Alloc>& Other);

        explicit THashMap(THashMap<K, V, Pred, Alloc>&& Other);
        explicit THashMap(std::unordered_map<K, V, Pred, Alloc>&& Other);

        ~THashMap();

        THashMap<K, V, Hash, Pred, Alloc>& operator= (const THashMap<K, V, Pred, Alloc>& Other);
        THashMap<K, V, Hash, Pred, Alloc>& operator= (THashMap<K, V, Pred, Alloc>&& Other);
        THashMap<K, V, Hash, Pred, Alloc>& operator= (std::unordered_map<K, V, Pred, Alloc>&& Other);

        V&                                 operator[](const K& Key);
        const V&                           operator[](const K& Key)const;

        template<typename _TOtherSizeType = INT32, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_NODISCARD _TOtherSizeType Num() const
        {
            return static_cast<_TOtherSizeType>(HashMap.size());
        }
        
        void Add   (const K& Key, const V& Value);
        void Remove(const K& Key);
        
        BOOL32 ContainsKey(const K& Key) const;
        
        BOOL32 FindKey  (const V& Value, K& OutKey) const;
        BOOL32 FindValue(const K& Key,   V& OutValue) const;

        void GenerateKeyArray  (TArray<K>& OutKeys);
        void GenerateValueArray(TArray<V>& OutValues);

        void Empty();
        
        typename TIterator Begin()
        {
            return HashMap.begin();
        }
        typename TConstIterator Begin() const
        {
            return HashMap.begin();
        }
        typename TIterator End()
        {
            return HashMap.end();
        }
        typename TConstIterator End() const
        {
            return HashMap.end();
        }

        /**
         * DO NOT USE DIRECTLY
         * STL-like iterators to enable range-based for loop support.
         */
        typename TConstIterator begin()const{return HashMap.begin();}
        typename TConstIterator end()const{return HashMap.end();}
        typename TIterator      begin(){return HashMap.begin();}
        typename TIterator      end(){return HashMap.end();}
    private:
        std::unordered_map<K, V, Hash, Pred, Alloc> HashMap;
    };

    // -------------------   Implementations   -----------------------------
    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>::THashMap()
    {
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>::THashMap(const THashMap<K, V, Pred, Alloc>& Other)
        :
    HashMap(Other.HashMap)
    {
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>::THashMap(const std::unordered_map<K, V, Pred, Alloc>& Other)
        :
    HashMap(Other)
    {
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>::THashMap(THashMap<K, V, Pred, Alloc>&& Other)
        :
    HashMap(std::move(Other.HashMap))
    {
        Other.Clear();
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>::THashMap(std::unordered_map<K, V, Pred, Alloc>&& Other)
        :
    HashMap(std::move(Other))
    {
        Other.clear();
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>::~THashMap()
    {
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>& THashMap<K, V, Hash, Pred, Alloc>::operator=(
        const THashMap<K, V, Pred, Alloc>& Other)
    {
        this->HashMap = Other.HashMap;
        return *this;
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>& THashMap<K, V, Hash, Pred, Alloc>::operator=(THashMap<K, V, Pred, Alloc>&& Other)
    {
        HashMap = std::move(Other.HashMap);
        Other.Clear();
        return *this;
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    THashMap<K, V, Hash, Pred, Alloc>& THashMap<K, V, Hash, Pred, Alloc>::operator=(
        std::unordered_map<K, V, Pred, Alloc>&& Other)
    {
        HashMap = std::move(Other);
        Other.clear();
        return *this;
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    V& THashMap<K, V, Hash, Pred, Alloc>::operator[](const K& Key)
    {
        return HashMap[Key];
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    const V& THashMap<K, V, Hash, Pred, Alloc>::operator[](const K& Key)const
    {
        return HashMap[Key];
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    void THashMap<K, V, Hash, Pred, Alloc>::Add(const K& Key, const V& Value)
    {
        HashMap.insert_or_assign(Key, Value);
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    void THashMap<K, V, Hash, Pred, Alloc>::Remove(const K& Key)
    {
        HashMap.erase(HashMap.find(Key));
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    BOOL32 THashMap<K, V, Hash, Pred, Alloc>::ContainsKey(const K& Key) const
    {
        return HashMap.find(Key) < HashMap.end();
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    BOOL32 THashMap<K, V, Hash, Pred, Alloc>::FindKey(const V& Value, K& OutKey) const
    {
        for(const auto& elem : HashMap)
        {
            if(elem.second == Value)
            {
                OutKey = elem.first;
                return TRUE;
            }
        }
        return FALSE;
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    BOOL32 THashMap<K, V, Hash, Pred, Alloc>::FindValue(const K& Key, V& OutValue) const
    {
        const auto it = HashMap.find(Key);
        if(!(it < HashMap.end()))
        {
            return FALSE;
        }
        OutValue = it->second;
        return TRUE;
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    void THashMap<K, V, Hash, Pred, Alloc>::GenerateKeyArray(TArray<K>& OutKeys)
    {
        for(const auto& elem : HashMap)
        {
            OutKeys.Add(elem.first);
        }
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    void THashMap<K, V, Hash, Pred, Alloc>::GenerateValueArray(TArray<V>& OutValues)
    {
        for(const auto& elem : HashMap)
        {
            OutValues.Add(elem.second);
        }
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    void THashMap<K, V, Hash, Pred, Alloc>::Empty()
    {
        HashMap.clear();
    }

}

