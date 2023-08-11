#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
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
        explicit THashMap(const THashMap<K, V, Pred, Alloc>& Other);
        explicit THashMap(const std::unordered_map<K, V, Pred, Alloc>& Other);

        explicit THashMap(THashMap<K, V, Pred, Alloc>&& Other);
        explicit THashMap(std::unordered_map<K, V, Pred, Alloc>&& Other);

        ~THashMap();

        THashMap<K, V, Hash, Pred, Alloc>& operator= (const THashMap<K, V, Pred, Alloc>& Other);
        THashMap<K, V, Hash, Pred, Alloc>& operator= (THashMap<K, V, Pred, Alloc>&& Other);
        THashMap<K, V, Hash, Pred, Alloc>& operator= (std::unordered_map<K, V, Pred, Alloc>&& Other);

        V&                                 operator[](const K& Key);

        [[nodiscard]] UINT Length() const;
        
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


}

