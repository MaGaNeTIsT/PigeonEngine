#include "Map.h"

namespace PigeonEngine
{
    
    template <typename K, typename V>
    TMap<K, V>::TMap()
    {
    }

    template <typename K, typename V>
    TMap<K, V>::TMap(const TMap<K, V>& Other)
        :
    Map(Other.Map)
    {
        
    }

    template <typename K, typename V>
    TMap<K, V>::TMap(const std::map<K, V>& Other)
        :
    Map(Other)
    {
    }

    template <typename K, typename V>
    TMap<K, V>::TMap(TMap<K, V>&& Other) noexcept
        :
    Map(std::move(Other.Map))
    {
        Other.Clear();
    }

    template <typename K, typename V>
    TMap<K, V>::TMap(std::map<K, V>&& Other)
        :
    Map(std::move(Other))
    {
        Other.clear();
    }

    template <typename K, typename V>
    TMap<K, V>::~TMap()
    {
    }

    template <typename K, typename V>
    V& TMap<K, V>::operator[](const K& Key)
    {
        return Map[Key];
    }

    template <typename K, typename V>
    TMap<K, V>& TMap<K, V>::operator=(const TMap<K, V>& Other)
    {
        Map = Other.Map;
        return *this;
    }

    template <typename K, typename V>
    TMap<K, V>& TMap<K, V>::operator=(TMap<K, V>&& Other) noexcept
    {
        Map = std::move(Other.Map);
        Other.Clear();
        return *this;
    }

    template <typename K, typename V>
    typename std::map<K, V>::iterator TMap<K, V>::begin()
    {
        return Map.begin();
    }

    template <typename K, typename V>
    typename std::map<K, V>::iterator TMap<K, V>::end()
    {
        return Map.end();
    }

    template <typename K, typename V>
    UINT TMap<K, V>::Length() const
    {
        return Map.size();
    }

    template <typename K, typename V>
    void TMap<K, V>::Add(const K& Key, const V& Value)
    {
        Map.insert_or_assign(Key, Value);
    }

    template <typename K, typename V>
    void TMap<K, V>::Remove(const K& Key)
    {
        Map.erase(Map.find(Key));
    }

    template <typename K, typename V>
    BOOL TMap<K, V>::ContainsKey(const K& Key) const
    {
        return Map.contains(Key);
    }

    template <typename K, typename V>
    BOOL TMap<K, V>::FindKey(const V& Value, K& OutKey) const
    {
        for(const auto& elem : Map)
        {
            if(elem.second == Value)
            {
                OutKey = elem.first;
                return true;
           }
        }
        return false;
    }

    template <typename K, typename V>
    BOOL TMap<K, V>::FindValue(const K& Key, V& OutValue) const
    {
        auto It = Map.find(Key);
        if(!(It < Map.end()))
        {
            OutValue = It->second;
            return true;
        }
        return false;
    }

    template <typename K, typename V>
    void TMap<K, V>::GenerateKeyArray(TArray<K>& OutKeys)
    {
        for(const auto& elem : Map)
        {
            OutKeys.Add(elem.first);
        }
    }

    template <typename K, typename V>
    void TMap<K, V>::GenerateValueArray(TArray<V>& OutValues)
    {
        for(const auto& elem : Map)
        {
            OutValues.Add(elem.second);
        }
    }

    template <typename K, typename V>
    void TMap<K, V>::Clear()
    {
        Map.clear();
    }
}
