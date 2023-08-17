#include "RTTIManager.h"

namespace PigeonEngine
{

    template<typename _TType, typename... _TParentTypes>
    ENGINE_INLINE void RegisterClassType()
    {
        ERTTIManager::GetManagerSingleton()->SaveClassInfo<_TType, _TParentTypes...>();
    }

    template<typename _TType>
    ENGINE_INLINE size_t GetClassHashCode()
    {
        return (ERTTIManager::GetManagerSingleton()->GetClassHashCode<_TType>());
    }

    template<typename _TType>
    ENGINE_INLINE size_t GetClassHashCode(const _TType* InTypePtr)
    {
        return (ERTTIManager::GetManagerSingleton()->GetClassHashCode<_TType>(InTypePtr));
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsChildClass()
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsParentClass()
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsChildClass(const _TFromType* InFromPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>()));
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsParentClass(const _TFromType* InFromPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>()));
    }

    template<typename _TToType>
    ENGINE_INLINE BOOL IsChildClass(const size_t& InFromTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(InFromTypeHash, GetClassHashCode<_TToType>()));
    }

    template<typename _TToType>
    ENGINE_INLINE BOOL IsParentClass(const size_t& InFromTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(InFromTypeHash, GetClassHashCode<_TToType>()));
    }

    ENGINE_INLINE BOOL IsChildClass(const size_t& InFromTypeHash, const size_t& InToTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(InFromTypeHash, InToTypeHash));
    }

    ENGINE_INLINE BOOL IsParentClass(const size_t& InFromTypeHash, const size_t& InToTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(InFromTypeHash, InToTypeHash));
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsChildClass(const _TFromType* InFromPtr, const _TToType* InToPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsParentClass(const _TFromType* InFromPtr, const _TToType* InToPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TToType>
    ENGINE_INLINE BOOL IsClass(const size_t& InFromHash)
    {
        return (InFromHash == (GetClassHashCode<_TToType>()));
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsClass(const _TFromType* InFromPtr)
    {
        return ((GetClassHashCode<_TFromType>(InFromPtr)) == (GetClassHashCode<_TToType>()));
    }

    ENGINE_INLINE BOOL IsClass(const size_t& InFromHash, const size_t& InToHash)
    {
        return (InFromHash == InToHash);
    }

    template<typename _TToType>
    ENGINE_INLINE BOOL IsClass(const size_t& InFromHash, const _TToType* InToPtr)
    {
        return (InFromHash == (GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TFromType>
    ENGINE_INLINE BOOL IsClass(const _TFromType* InFromPtr, const size_t& InToHash)
    {
        return ((GetClassHashCode<_TFromType>(InFromPtr)) == InToHash);
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE BOOL IsClass(const _TFromType* InFromPtr, const _TToType* InToPtr)
    {
        return ((GetClassHashCode<_TFromType>(InFromPtr)) == (GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TToType>
    ENGINE_INLINE const _TToType* AsClassPtrUnsafe(const void* InFromPtr)
    {
        return (reinterpret_cast<const _TToType*>(InFromPtr));
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE const _TToType* AsClassPtr(const _TFromType* InFromPtr)
    {
        const _TToType* Result = nullptr;
        if (IsClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = InFromPtr;
        }
        else if (IsParentClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = static_cast<const _TToType*>(InFromPtr);
        }
        return Result;
    }

    template<typename _TToType>
    ENGINE_INLINE _TToType* AsClassPtrUnsafe(void* InFromPtr)
    {
        return (reinterpret_cast<_TToType*>(InFromPtr));
    }

    template<typename _TFromType, typename _TToType>
    ENGINE_INLINE _TToType* AsClassPtr(_TFromType* InFromPtr)
    {
        _TToType* Result = nullptr;
        if (IsClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = InFromPtr;
        }
        else if (IsParentClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = static_cast<_TToType*>(InFromPtr);
        }
        return Result;
    }

};