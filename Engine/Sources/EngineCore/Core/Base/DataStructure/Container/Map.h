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

        V&          operator[](const K& Key);
        
        TMap<K, V>& operator=(TInitList InInitList);
        TMap<K,V>&  operator=(const TMap<K,V>& Other);
        TMap<K,V>&  operator=(TMap<K,V>&& Other) noexcept;

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
        
        template<typename _TOtherSizeType = INT32, TEnableIfType<TIsIntegral<_TOtherSizeType>::value, _TOtherSizeType> = 0>
        PE_NODISCARD _TOtherSizeType Num() const
        {
            return (static_cast<_TOtherSizeType>(Map.size()));
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
        V* FindValueAsPtr(const K& Key)
        {
            auto It = Map.find(Key);
            if (It != Map.end())
            {
                return (&(It->second));
            }
            return nullptr;
        }
        const V* FindValueAsPtr(const K& Key)const
        {
            auto It = Map.find(Key);
            if (It != Map.end())
            {
                return (&(It->second));
            }
            return nullptr;
        }

        void GenerateKeyArray  (TArray<K>& OutKeys);
        void GenerateValueArray(TArray<V>& OutValues);

        void Empty();
        
    private:
        std::map<K,V> Map;
    };

    template<typename TKeyType, typename TValueType>
    class TMapManager
    {
    public:
        TMapManager()
        {
        }
        virtual ~TMapManager()
        {
            Empty();
        }
    public:
        /*
        * Add item into mapped datas. This action may reconstruct whole memories.
        * Params [InReplaceIfHaveKey]: true = If already contain data with InKeyValue will replace it. This action will delete old item. false = If already contain data with InKeyValue will not do anything.
        * Return [UINT32]: 0 = Add failed. value = Mapped datas' size(after add value).
        */
        UINT32 Add(const TKeyType& InKeyValue, TValueType* InDataValue, const BOOL32& InReplaceIfHaveKey = FALSE)
        {
            if (!InDataValue)
            {
                return 0u;
            }
            BOOL32 NeedReplace = FALSE;
            if (SavedDatas.ContainsKey(InKeyValue))
            {
                NeedReplace = TRUE;
                if (!InReplaceIfHaveKey)
                {
                    return 0u;
                }
            }
            if (NeedReplace)
            {
                TValueType* TempData = nullptr;
                if (SavedDatas.FindValue(InKeyValue, TempData))
                {
                    if (TempData && TempData == InDataValue)
                    {
                        return (SavedDatas.Length());
                    }
                    SavedDatas.Remove(InKeyValue);
                    delete TempData;
                }
            }
            SavedDatas.Add(InKeyValue, InDataValue);
            return (SavedDatas.Num<UINT32>());
        }
        /*
        * Remove item with InKeyValue in mapped datas.
        * Return [UINT32]: Mapped datas' size(after remove value).
        */
        UINT32 Remove(const TKeyType& InKeyValue)
        {
            TValueType* TempData = nullptr;
            if (SavedDatas.FindValue(InKeyValue, TempData))
            {
                SavedDatas.Remove(InKeyValue);
                delete TempData;
            }
            return (SavedDatas.Num<UINT32>());
        }
        /*
        * Find item with InKeyValue in mapped datas.
        * Return [TValueType*]: If contain key return item. If not then return nullptr.
        */
        TValueType* Find(const TKeyType& InKeyValue)
        {
            TValueType* TempData = nullptr;
            SavedDatas.FindValue(InKeyValue, TempData);
            return TempData;
        }
        /*
        * Check if contains item with InKeyValue in mapped datas.
        * Return [BOOL32]: If contains return TRUE. If not then return FALSE.
        */
        BOOL32 Contain(const TKeyType& InKeyValue)const
        {
            return (SavedDatas.ContainsKey(InKeyValue));
        }
        /*
        * Clear whole data list.
        */
        void Empty()
        {
            if (SavedDatas.Num() > 0)
            {
                for (auto it = SavedDatas.Begin(); it != SavedDatas.End(); it++)
                {
                    delete (it->second);
                }
                SavedDatas.Empty();
            }
        }
        /*
        * Get size of mapped datas.
        */
        UINT32 Num()const
        {
            return (SavedDatas.Num());
        }
    protected:
        TMap<TKeyType, TValueType*> SavedDatas;
    public:
        TMapManager(const TMapManager&) = delete;
        TMapManager& operator=(const TMapManager&) = delete;
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
        Other.Empty();
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
                return TRUE;
           }
        }
        return FALSE;
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
    void TMap<K, V>::Empty()
    {
        Map.clear();
    }

}

