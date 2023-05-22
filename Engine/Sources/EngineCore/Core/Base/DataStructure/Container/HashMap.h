#pragma once
#include <CoreMinimal.h>
#include <unordered_map>

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
        ~THashMap();
        
    private:
        std::unordered_map<K, V, Hash, Pred, Alloc> HashMap;
    };
}

