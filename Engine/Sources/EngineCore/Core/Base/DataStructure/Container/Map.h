#pragma once
#include <CoreMinimal.h>
#include <map>
#include "Array.h"
namespace PigeonEngine
{
    template <typename K, typename V>
    class TMap
    {
    public:
        using TValueType = std::pair<const K, V>;
        using TInitList = std::initializer_list<TValueType>;
    public:
        TMap();
        TMap(const TMap<K,V>& Other);
        TMap(TInitList InInitList);
        explicit TMap(const std::map<K,V>& Other);
        TMap(TMap<K,V>&& Other) noexcept;
        explicit TMap(std::map<K,V>&& Other);
        ~TMap();

        V& operator[](const K& Key);
        
        TMap<K, V>& operator=(TInitList InInitList);
        TMap<K,V>& operator=(const TMap<K,V>& Other);
        TMap<K,V>& operator=(TMap<K,V>&& Other) noexcept;

        typename std::map<K,V>::iterator begin();
        typename std::map<K,V>::iterator end();

        ENGINE_NODISCARD UINT Length() const;
        
        void Add   (const K& Key, const V& Value);
        void Remove(const K& Key);
        
        BOOL ContainsKey(const K& Key) const;
        
        BOOL FindKey  (const V& Value, K& OutKey) const;
        BOOL FindValue(const K& Key,   V& OutValue) const;

        void GenerateKeyArray  (TArray<K>& OutKeys);
        void GenerateValueArray(TArray<V>& OutValues);

        void Clear();
        
    private:
        std::map<K,V> Map;
    };

}

