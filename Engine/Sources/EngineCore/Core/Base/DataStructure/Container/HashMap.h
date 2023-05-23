#pragma once
#include <CoreMinimal.h>
#include <unordered_map>

#include "Array.h"

namespace PigeonEngine
{
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
        THashMap();
        THashMap(const THashMap<K, V, Pred, Alloc>& Other);
        THashMap(const std::unordered_map<K, V, Pred, Alloc>& Other);
        ~THashMap();

        THashMap<K, V, Hash, Pred, Alloc>& operator= (const THashMap<K, V, Pred, Alloc>& Other);
        V&                                 operator[](const K& Key);

        UINT Length() const;
        
        void Add   (const K& Key, const V& Value);
        void Remove(const K& Key);
        
        BOOL ContainsKey(const K& Key) const;
        
        BOOL FindKey  (const V& Value, K& OutKey) const;
        BOOL FindValue(const K& Key,   V& OutValue) const;

        void GenerateKeyArray  (TArray<K>& OutKeys);
        void GenerateValueArray(TArray<V>& OutValues);

        void Clear();
        
        typename std::unordered_map<K, V, Hash, Pred, Alloc>::iterator begin();
        typename std::unordered_map<K, V, Hash, Pred, Alloc>::iterator end();
    private:
        std::unordered_map<K, V, Hash, Pred, Alloc> HashMap;
    };

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
    V& THashMap<K, V, Hash, Pred, Alloc>::operator[](const K& Key)
    {
        return HashMap[Key];
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    UINT THashMap<K, V, Hash, Pred, Alloc>::Length() const
    {
        return HashMap.size();
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
    BOOL THashMap<K, V, Hash, Pred, Alloc>::ContainsKey(const K& Key) const
    {
        return HashMap.find(Key) < HashMap.end();
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    BOOL THashMap<K, V, Hash, Pred, Alloc>::FindKey(const V& Value, K& OutKey) const
    {
        for(const auto& elem : HashMap)
        {
            if(elem.second == Value)
            {
                OutKey = elem.first;
                return true;
            }
        }
        return false;
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    BOOL THashMap<K, V, Hash, Pred, Alloc>::FindValue(const K& Key, V& OutValue) const
    {
        const auto it = HashMap.find(Key);
        if(!(it < HashMap.end()))
        {
            return false;
        }
        OutValue = it->second;
        return true;
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
    void THashMap<K, V, Hash, Pred, Alloc>::Clear()
    {
        HashMap.clear();
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    typename std::unordered_map<K, V, Hash, Pred, Alloc>::iterator THashMap<K, V, Hash, Pred, Alloc>::begin()
    {
        return HashMap.begin();
    }

    template <typename K, typename V, class Hash, class Pred, class Alloc>
    typename std::unordered_map<K, V, Hash, Pred, Alloc>::iterator THashMap<K, V, Hash, Pred, Alloc>::end()
    {
        return HashMap.end();
    }
}

