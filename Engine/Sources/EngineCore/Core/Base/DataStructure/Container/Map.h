#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <map>
#include "Array.h"

namespace PigeonEngine
{
    // -------------------   Declarations   -----------------------------
    template <typename K, typename V>
    class TMap
    {
    public:
        using TValueType        = std::pair<const K, V>;
        using TInitList         = std::initializer_list<TValueType>;
        using TIterator         = typename std::map<K, V>::iterator;
        using TConstIterator    = typename std::map<K, V>::const_iterator;
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

        typename TIterator Begin()
        {
            return Map.begin();
        }
        typename TConstIterator Begin()const
        {
            return Map.begin();
        }
        typename TIterator End()
        {
            return Map.end();
        }
        typename TConstIterator End()const
        {
            return Map.end();
        }

        /**
         * DO NOT USE DIRECTLY
         * STL-like iterators to enable range-based for loop support.
         */
        typename TConstIterator begin()const{return Map.begin();}
        typename TConstIterator end()const{return Map.end();}
        typename TIterator      begin(){return Map.begin();}
        typename TIterator      end(){return Map.end();}
        
        PE_NODISCARD UINT32 Length() const
        {
            return (static_cast<UINT32>(Map.size()));
        }
        
        void Add   (const K& Key, const V& Value);
        void Remove(const K& Key);
        
        BOOL32 ContainsKey(const K& Key) const;
        
        BOOL32 FindKey  (const V& Value, K& OutKey) const;
        BOOL32 FindValue(const K& Key,   V& OutValue) const
        {
            auto It = Map.find(Key);
            if (It != Map.end())
            {
                OutValue = It->second;
                return TRUE;
            }
            return FALSE;
        }

        void GenerateKeyArray  (TArray<K>& OutKeys);
        void GenerateValueArray(TArray<V>& OutValues);

        void Clear();
        
    private:
        std::map<K,V> Map;
    };

    // -------------------   Implementations   -----------------------------
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
    TMap<K, V>::TMap(TInitList InInitList)
        : Map(InInitList)
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
    TMap<K, V>& TMap<K, V>::operator=(TInitList InInitList)
    {
        Clear();
        Map.insert(InInitList);
        return *this;
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
    BOOL32 TMap<K, V>::ContainsKey(const K& Key) const
    {
        return Map.find(Key) != Map.end();
    }

    template <typename K, typename V>
    BOOL32 TMap<K, V>::FindKey(const V& Value, K& OutKey) const
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

