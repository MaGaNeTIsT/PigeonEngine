#pragma once

#include <Base/DataStructure/BuiltInType.h>
#include <Config/EngineConfig.h>
#include <typeinfo>
#include <map>
#include <string>
#include <set>

namespace PigeonEngine
{

    class ERTTIManager
    {
    public:
        /// <summary>
        /// Get hash code of [_TType]
        /// </summary>
        /// <typeparam name="_TType"></typeparam>
        /// <returns></returns>
        template<typename _TType>
        size_t GetClassHashCode()const
        {
            return (typeid(_TType).hash_code());
        }

        /// <summary>
        /// Get hash code of class object that [_TType] point to
        /// </summary>
        /// <typeparam name="_TType"></typeparam>
        /// <param name="InTypePtr"></param>
        /// <returns></returns>
        template<typename _TType>
        size_t GetClassHashCode(const _TType* InTypePtr)const
        {
            return (typeid((*InTypePtr)).hash_code());
        }

        /// <summary>
        /// Register [_TType] with its list of [_TParentTypes]
        /// </summary>
        /// <typeparam name="_TType"></typeparam>
        /// <typeparam name="..._TParentTypes"></typeparam>
        template<typename _TType, typename... _TParentTypes>
        void SaveClassInfo()
        {
            ProcessClassInfoInternal<_TType, _TParentTypes...>();
        }

        /// <summary>
        /// Find if [InTestTypeHash] inherit from [InTypeHash]
        /// [TypeHash] is [typeid(_Type).hash_code()]
        /// </summary>
        /// <param name="InTypeHash"></param>
        /// <param name="InTestTypeHash"></param>
        /// <returns></returns>
        BOOL IsChildClassType(const size_t& InTypeHash, const size_t& InTestTypeHash)const
        {
            BOOL Result = FALSE;
            BOOL NeedContinueRecursion = TRUE;
            const size_t TestHashCode = InTestTypeHash;
            ForwardRecursion(InTypeHash,
                [&TestHashCode, &Result, &NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->void
                {
                    if (InTypeInfo.TypeHashCode == TestHashCode)
                    {
                        Result = TRUE;
                        NeedContinueRecursion = FALSE;
                    }
                },
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL
                {
                    return NeedContinueRecursion;
                });
            return Result;
        }

        /// <summary>
        /// Find if [InTypeHash] inherit from [InTestTypeHash]
        /// [TypeHash] is [typeid(_Type).hash_code()]
        /// </summary>
        /// <param name="InTypeHash"></param>
        /// <param name="InTestTypeHash"></param>
        BOOL IsParentClassType(const size_t& InTypeHash, const size_t& InTestTypeHash)const
        {
            BOOL Result = FALSE;
            BOOL NeedContinueRecursion = TRUE;
            const size_t TestHashCode = InTestTypeHash;
            BackwardRecursion(InTypeHash,
                [&TestHashCode, &Result, &NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->void
                {
                    if (InTypeInfo.TypeHashCode == TestHashCode)
                    {
                        Result = TRUE;
                        NeedContinueRecursion = FALSE;
                    }
                },
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL
                {
                    return NeedContinueRecursion;
                });
            return Result;
        }

        /// <summary>
        /// Find if [_TTestType] inherit from [_TInType]
        /// </summary>
        /// <typeparam name="_TInType"></typeparam>
        /// <typeparam name="_TTestType"></typeparam>
        /// <returns></returns>
        template<typename _TInType, typename _TTestType>
        BOOL IsChildClassType()const
        {
            BOOL Result = FALSE;
            BOOL NeedContinueRecursion = TRUE;
            const size_t TestHashCode = typeid(_TTestType).hash_code();
            ForwardRecursion(typeid(_TInType).hash_code(),
                [&TestHashCode, &Result, &NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->void
                {
                    if (InTypeInfo.TypeHashCode == TestHashCode)
                    {
                        Result = TRUE;
                        NeedContinueRecursion = FALSE;
                    }
                },
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL
                {
                    return NeedContinueRecursion;
                });
            return Result;
        }

        /// <summary>
        /// Find if [_TInType] inherit from [_TTestType]
        /// </summary>
        /// <typeparam name="_TInType"></typeparam>
        /// <typeparam name="_TTestType"></typeparam>
        template<typename _TInType, typename _TTestType>
        BOOL IsParentClassType()const
        {
            BOOL Result = FALSE;
            BOOL NeedContinueRecursion = TRUE;
            const size_t TestHashCode = typeid(_TTestType).hash_code();
            BackwardRecursion(typeid(_TInType).hash_code(),
                [&TestHashCode, &Result, &NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->void
                {
                    if (InTypeInfo.TypeHashCode == TestHashCode)
                    {
                        Result = TRUE;
                        NeedContinueRecursion = FALSE;
                    }
                },
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL
                {
                    return NeedContinueRecursion;
                });
            return Result;
        }
    private:
        struct EClassTypeInfo
        {
            EClassTypeInfo() : TypeName(ENGINE_DEFAULT_NAME), TypeHashCode(-1) {}
            EClassTypeInfo(const std::string& InTypeName, const size_t& InTypeHashCode) : TypeName(InTypeName), TypeHashCode(InTypeHashCode) {}
            EClassTypeInfo(const EClassTypeInfo& Other) : TypeName(Other.TypeName), TypeHashCode(Other.TypeHashCode)
            {
                for (auto It = Other.ParentHashCodes.begin(); It != Other.ParentHashCodes.end(); It++)
                {
                    ParentHashCodes.insert((*It));
                }
                for (auto It = Other.ChildHashCodes.begin(); It != Other.ChildHashCodes.end(); It++)
                {
                    ChildHashCodes.insert((*It));
                }
            }
            EClassTypeInfo& operator=(const EClassTypeInfo& Other)
            {
                TypeName = Other.TypeName;
                TypeHashCode = Other.TypeHashCode;
                if (ParentHashCodes.size() > 0u)
                {
                    ParentHashCodes.clear();
                }
                if (ChildHashCodes.size() > 0u)
                {
                    ChildHashCodes.clear();
                }
                for (auto It = Other.ParentHashCodes.begin(); It != Other.ParentHashCodes.end(); It++)
                {
                    ParentHashCodes.insert((*It));
                }
                for (auto It = Other.ChildHashCodes.begin(); It != Other.ChildHashCodes.end(); It++)
                {
                    ChildHashCodes.insert((*It));
                }
            }

            std::string         TypeName;
            size_t              TypeHashCode;
            std::set<size_t>    ParentHashCodes;
            std::set<size_t>    ChildHashCodes;
        };
    private:
        template<typename _TType>
        void SaveSingleClassInfoInternal()
        {
            size_t TypeHashCode = typeid(_TType).hash_code();
            std::string TypeName = typeid(_TType).name();
            auto Item = ClassTypeInfos.find(TypeHashCode);
            if (Item == ClassTypeInfos.end())
            {
                EClassTypeInfo TempData(TypeName, TypeHashCode);
                ClassTypeInfos.insert_or_assign(TypeHashCode, std::move(TempData));
            }
        }
        template<typename _TType, typename _TParentType>
        void SaveClassAndParentInfoInternal()
        {
            size_t TypeHashCode = typeid(_TType).hash_code();
            if (ClassTypeInfos.find(TypeHashCode) == ClassTypeInfos.end())
            {
                SaveSingleClassInfoInternal<_TType>();
            }
            size_t ParentHashCode = typeid(_TParentType).hash_code();
            if (ClassTypeInfos.find(ParentHashCode) == ClassTypeInfos.end())
            {
                SaveSingleClassInfoInternal<_TParentType>();
            }

            {
                ClassTypeInfos[TypeHashCode].ParentHashCodes.insert(ParentHashCode);
                ClassTypeInfos[ParentHashCode].ChildHashCodes.insert(TypeHashCode);
            }
        }
        template<typename _TType>
        void ProcessClassInfoInternal()
        {
        }
        template<typename _TType, typename _FirstParentType, typename... _RestParentTypes>
        void ProcessClassInfoInternal()
        {
            SaveClassAndParentInfoInternal<_TType, _FirstParentType>();
            ProcessClassInfoInternal<_TType, _RestParentTypes...>();
        }
        template<typename TFunctionType, typename TConditionType>
        void ForwardRecursion(size_t InHashCode, const TFunctionType& InFunc, const TConditionType& InCond)const
        {
            auto CurrentItem = ClassTypeInfos.find(InHashCode);
            if (CurrentItem == ClassTypeInfos.end())
            {
                return;
            }
            const EClassTypeInfo& TempInfo = CurrentItem->second;
            if (InCond(TempInfo))
            {
                InFunc(TempInfo);
                if (TempInfo.ChildHashCodes.size() > 0u)
                {
                    for (auto It = TempInfo.ChildHashCodes.begin(); It != TempInfo.ChildHashCodes.end(); It++)
                    {
                        ForwardRecursion<TFunctionType, TConditionType>(*It, InFunc, InCond);
                    }
                }
            }
        }
        template<typename TFunctionType, typename TConditionType>
        void BackwardRecursion(size_t InHashCode, const TFunctionType& InFunc, const TConditionType& InCond)const
        {
            auto CurrentItem = ClassTypeInfos.find(InHashCode);
            if (CurrentItem == ClassTypeInfos.end())
            {
                return;
            }
            const EClassTypeInfo& TempInfo = CurrentItem->second;
            if (InCond(TempInfo))
            {
                InFunc(TempInfo);
                if (TempInfo.ParentHashCodes.size() > 0u)
                {
                    for (auto It = TempInfo.ParentHashCodes.begin(); It != TempInfo.ParentHashCodes.end(); It++)
                    {
                        BackwardRecursion<TFunctionType, TConditionType>(*It, InFunc, InCond);
                    }
                }
            }
        }
    private:
        std::map<size_t, EClassTypeInfo> ClassTypeInfos;
    private:
        ERTTIManager() {}
        ~ERTTIManager() {}
        ERTTIManager(const ERTTIManager&) = delete;
        ERTTIManager& operator=(const ERTTIManager&) = delete;
    public:
        static ERTTIManager* GetManagerSingleton()
        {
            static ERTTIManager _ManagerSingletonObject;
            return (&_ManagerSingletonObject);
        }
    };

    /// <summary>
    /// Register [_TType] class type with its type list of [_TParentTypes]
    /// </summary>
    template<typename _TType, typename... _TParentTypes>
    extern ENGINE_INLINE void RegisterClassType();

    // Gather hash code of [_TType] class type
    template<typename _TType>
    extern ENGINE_INLINE size_t GetClassHashCode();

    // Gather hash code of class type that [_TType] point to
    template<typename _TType>
    extern ENGINE_INLINE size_t GetClassHashCode(const _TType* InTypePtr);

    // Test if [_TToType] inherit from [_TFromType]
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsChildClass();

    // Test if [_TFromType] inherit from [_TToType]
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsParentClass();

    // Test if [_TToType] inherit from [*InFromPtr]
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsChildClass(const _TFromType* InFromPtr);

    // Test if [*InFromPtr] inherit from [_TToType]
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsParentClass(const _TFromType* InFromPtr);

    // Test if [_TToType] inherit from [InFromTypeHash]
    template<typename _TToType>
    extern ENGINE_INLINE BOOL IsChildClass(const size_t& InFromTypeHash);

    // Test if [InFromTypeHash] inherit from [_TToType]
    template<typename _TToType>
    extern ENGINE_INLINE BOOL IsParentClass(const size_t& InFromTypeHash);

    // Test if [InToTypeHash] inherit from [InFromTypeHash]
    extern ENGINE_INLINE BOOL IsChildClass(const size_t& InFromTypeHash, const size_t& InToTypeHash);

    // Test if [InFromTypeHash] inherit from [InToTypeHash]
    extern ENGINE_INLINE BOOL IsParentClass(const size_t& InFromTypeHash, const size_t& InToTypeHash);

    // Test if [*InToPtr] inherit from [*InFromPtr]
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsChildClass(const _TFromType* InFromPtr, const _TToType* InToPtr);

    // Test if [*InFromPtr] inherit from [*InToPtr]
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsParentClass(const _TFromType* InFromPtr, const _TToType* InToPtr);

    // Test if [InFromHash] class is [_TToType] class
    template<typename _TToType>
    extern ENGINE_INLINE BOOL IsClass(const size_t& InFromHash);

    // Test if [*InFromPtr] class is [_TToType] class
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsClass(const _TFromType* InFromPtr);

    // Test if [InFromHash] class is [InToHash] class
    extern ENGINE_INLINE BOOL IsClass(const size_t& InFromHash, const size_t& InToHash);

    // Test if [InFromHash] class is [*InToPtr] class
    template<typename _TToType>
    extern ENGINE_INLINE BOOL IsClass(const size_t& InFromHash, const _TToType* InToPtr);

    // Test if [*InFromPtr] class is [InToHash] class
    template<typename _TFromType>
    extern ENGINE_INLINE BOOL IsClass(const _TFromType* InFromPtr, const size_t& InToHash);

    // Test if [*InFromPtr] class is [*InToPtr] class
    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE BOOL IsClass(const _TFromType* InFromPtr, const _TToType* InToPtr);

    template<typename _TToType>
    extern ENGINE_INLINE const _TToType* AsClassPtrUnsafe(const void* InFromPtr);

    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE const _TToType* AsClassPtr(const _TFromType* InFromPtr);

    template<typename _TToType>
    extern ENGINE_INLINE _TToType* AsClassPtrUnsafe(void* InFromPtr);

    template<typename _TFromType, typename _TToType>
    extern ENGINE_INLINE _TToType* AsClassPtr(_TFromType* InFromPtr);

};