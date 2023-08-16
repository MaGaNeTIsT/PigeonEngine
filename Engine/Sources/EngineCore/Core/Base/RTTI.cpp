#include "RTTI.h"

namespace PigeonEngine
{

    template<typename _TType, typename... _TParentTypes>
    inline void RegisterClassType()
    {
        ERTTIManager::GetManagerSingleton()->SaveClassInfo<_TType, _TParentTypes...>();
    }

    template<typename _TType>
    inline size_t GetClassHashCode()
    {
        ERTTIManager::GetManagerSingleton()->GetClassHashCode<_TType>();
    }

    template<typename _TFromType, typename _TToType>
    inline BOOL IsChildClass()
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    inline BOOL IsParentClass()
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    inline BOOL IsChildClass(const _TFromType& InRef)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    inline BOOL IsParentClass(const _TFromType& InRef)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsChildClass(const _TFromType* InPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsParentClass(const _TFromType* InPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType<_TFromType, _TToType>());
    }

};