#pragma once

#include <Base/DataStructure/BuiltInType.h>
#include <Base/RegisterBase.h>
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
        /// Get hash code [typeid.hash_code] of class by class name [typeid.name]
        /// </summary>
        /// <param name="InClassName"></param>
        /// <returns></returns>
        size_t GetClassHashCodeByName(const CHAR* InClassName)const
        {
            if (auto It = ClassNameToHash.find(InClassName); It != ClassNameToHash.end())
            {
                return (It->second);
            }
            return (static_cast<size_t>(-1));
        }

        /// <summary>
        /// Get name [typeid.name] of class by hash code [typeid.hash_code]
        /// </summary>
        /// <param name="InClassHashCode"></param>
        /// <returns></returns>
        const CHAR* GetClassNameByHashCode(const size_t& InClassHashCode)const
        {
            if (auto It = ClassHashToName.find(InClassHashCode); It != ClassHashToName.end())
            {
                return (It->second.c_str());
            }
            return "EngineUnknownClassName";
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
        /// Register factory function [void* __Func(void)] of [_TType] class
        /// </summary>
        /// <typeparam name="_TType"></typeparam>
        /// <param name="__Function"></param>
        template<typename _TType>
        void SaveClassFactoryFunction(ERawPtrFunctionType __Function)
        {
            if (ClassFactories.find(GetClassHashCode<_TType>()) == ClassFactories.end())
            {
                ClassFactories.insert_or_assign(GetClassHashCode<_TType>(), __Function);
            }
        }

        /// <summary>
        /// Use class hash code to finding registed factory function then create object class
        /// </summary>
        /// <param name="InHashCode"></param>
        /// <returns></returns>
        void* CreateClassObject(const size_t& InHashCode)
        {
            if (auto It = ClassFactories.find(InHashCode); It != ClassFactories.end())
            {
                void* NewObject = (It->second)();
                return (NewObject);
            }
            return nullptr;
        }

        /// <summary>
        /// Use class name to finding registed factory function then create object class
        /// </summary>
        /// <param name="InHashCode"></param>
        /// <returns></returns>
        void* CreateClassObject(const CHAR* InClassName)
        {
            if (auto It = ClassFactories.find(GetClassHashCodeByName(InClassName)); It != ClassFactories.end())
            {
                void* NewObject = (It->second)();
                return (NewObject);
            }
            return nullptr;
        }

        /// <summary>
        /// Use class hash code to finding registed factory function then create object class then cast pointer into [_TType](must be parent class of create object class or the same class)
        /// </summary>
        /// <param name="InHashCode"></param>
        /// <returns></returns>
        template<typename _TType>
        _TType* CreateClassObject(const size_t& InHashCode)
        {
            if (auto It = ClassFactories.find(InHashCode); It != ClassFactories.end())
            {
                class ERTTIObject* NewObject = (class ERTTIObject*)((It->second)());
                if (IsChildClassType(GetClassHashCode<_TType>(), typeid(*NewObject).hash_code()))
                {
                    return ((_TType*)NewObject);
                }
                delete NewObject;
            }
            return nullptr;
        }

        /// <summary>
        /// Use class name to finding registed factory function then create object class then cast pointer into [_TType](must be parent class of create object class or the same class)
        /// </summary>
        /// <param name="InHashCode"></param>
        /// <returns></returns>
        template<typename _TType>
        _TType* CreateClassObject(const CHAR* InClassName)
        {
            if (auto It = ClassFactories.find(GetClassHashCodeByName(InClassName)); It != ClassFactories.end())
            {
                class ERTTIObject* NewObject = (class ERTTIObject*)((It->second)());
                if (IsChildClassType(GetClassHashCode<_TType>(), typeid(*NewObject).hash_code()))
                {
                    return ((_TType*)NewObject);
                }
                delete NewObject;
            }
            return nullptr;
        }

        /// <summary>
        /// Find if [InTestTypeHash] inherit from [InTypeHash]
        /// [TypeHash] is [typeid(_Type).hash_code()]
        /// </summary>
        /// <param name="InTypeHash"></param>
        /// <param name="InTestTypeHash"></param>
        /// <returns></returns>
        BOOL32 IsChildClassType(const size_t& InTypeHash, const size_t& InTestTypeHash)const
        {
            BOOL32 Result = FALSE;
            BOOL32 NeedContinueRecursion = TRUE;
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
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL32
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
        BOOL32 IsParentClassType(const size_t& InTypeHash, const size_t& InTestTypeHash)const
        {
            BOOL32 Result = FALSE;
            BOOL32 NeedContinueRecursion = TRUE;
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
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL32
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
        BOOL32 IsChildClassType()const
        {
            BOOL32 Result = FALSE;
            BOOL32 NeedContinueRecursion = TRUE;
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
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL32
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
        BOOL32 IsParentClassType()const
        {
            BOOL32 Result = FALSE;
            BOOL32 NeedContinueRecursion = TRUE;
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
                [&NeedContinueRecursion](const EClassTypeInfo& InTypeInfo)->BOOL32
                {
                    return NeedContinueRecursion;
                });
            return Result;
        }

        template<typename _TClassType>
        const CHAR* GetStaticClassName()const
        {
            if (auto It = ClassTypeInfos.find(GetClassHashCode<_TClassType>()); It != ClassTypeInfos.end())
            {
                return (It->second.TypeName.c_str());
            }
            return ("EngineUnknownClassName");
        }

        const CHAR* GetStaticClassName(const size_t& InClassTypeHash)const
        {
            if (auto It = ClassTypeInfos.find(InClassTypeHash); It != ClassTypeInfos.end())
            {
                return (It->second.TypeName.c_str());
            }
            return ("EngineUnknownClassName");
        }

        template<typename _TClassType>
        const CHAR* GetStaticClassName(const _TClassType* InClassTypePtr)const
        {
            if (auto It = ClassTypeInfos.find(GetClassHashCode<_TClassType>(InClassTypePtr)); It != ClassTypeInfos.end())
            {
                return (It->second.TypeName.c_str());
            }
            return ("EngineUnknownClassName");
        }
    private:
        struct EClassTypeInfo
        {
            EClassTypeInfo() : TypeName(ESettings::ENGINE_DEFAULT_NAME), TypeHashCode(-1) {}
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
                return (*this);
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
                ClassHashToName.insert_or_assign(TypeHashCode, TypeName);
                ClassNameToHash.insert_or_assign(TypeName, TypeHashCode);
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
        std::map<size_t, EClassTypeInfo>        ClassTypeInfos;
        std::map<size_t, std::string>           ClassHashToName;
        std::map<std::string, size_t>           ClassNameToHash;
        std::map<size_t, ERawPtrFunctionType>   ClassFactories;
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

    // Register [_TType] class type with its type list of [_TParentTypes]
    template<typename _TType, typename... _TParentTypes>
    extern PE_INLINE void RegisterClassType();

    // Gather hash code of [_TType] class type
    template<typename _TType>
    extern PE_INLINE size_t GetClassHashCode();

    // Gather hash code of class type that [_TType] point to
    template<typename _TType>
    extern PE_INLINE size_t GetClassHashCode(const _TType* InTypePtr);

    // Test if [_TToType] inherit from [_TFromType]
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsChildClass();

    // Test if [_TFromType] inherit from [_TToType]
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsParentClass();

    // Test if [_TToType] inherit from [*InFromPtr]
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsChildClass(const _TFromType* InFromPtr);

    // Test if [*InFromPtr] inherit from [_TToType]
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsParentClass(const _TFromType* InFromPtr);

    // Test if [_TToType] inherit from [InFromTypeHash]
    template<typename _TToType>
    extern PE_INLINE BOOL32 IsChildClass(const size_t& InFromTypeHash);

    // Test if [InFromTypeHash] inherit from [_TToType]
    template<typename _TToType>
    extern PE_INLINE BOOL32 IsParentClass(const size_t& InFromTypeHash);

    // Test if [InToTypeHash] inherit from [InFromTypeHash]
    extern PE_INLINE BOOL32 IsChildClass(const size_t& InFromTypeHash, const size_t& InToTypeHash);

    // Test if [InFromTypeHash] inherit from [InToTypeHash]
    extern PE_INLINE BOOL32 IsParentClass(const size_t& InFromTypeHash, const size_t& InToTypeHash);

    // Test if [*InToPtr] inherit from [*InFromPtr]
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsChildClass(const _TFromType* InFromPtr, const _TToType* InToPtr);

    // Test if [*InFromPtr] inherit from [*InToPtr]
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsParentClass(const _TFromType* InFromPtr, const _TToType* InToPtr);

    // Test if [InFromHash] class is [_TToType] class
    template<typename _TToType>
    extern PE_INLINE BOOL32 IsClass(const size_t& InFromHash);

    // Test if [*InFromPtr] class is [_TToType] class
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsClass(const _TFromType* InFromPtr);

    // Test if [InFromHash] class is [InToHash] class
    extern PE_INLINE BOOL32 IsClass(const size_t& InFromHash, const size_t& InToHash);

    // Test if [InFromHash] class is [*InToPtr] class
    template<typename _TToType>
    extern PE_INLINE BOOL32 IsClass(const size_t& InFromHash, const _TToType* InToPtr);

    // Test if [*InFromPtr] class is [InToHash] class
    template<typename _TFromType>
    extern PE_INLINE BOOL32 IsClass(const _TFromType* InFromPtr, const size_t& InToHash);

    // Test if [*InFromPtr] class is [*InToPtr] class
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE BOOL32 IsClass(const _TFromType* InFromPtr, const _TToType* InToPtr);

    // Gather [_TClassType] class static name
    template<typename _TClassType>
    extern PE_INLINE const CHAR* EngineStaticClassName();

    // Gather class static name by class hash [InClassHash]
    extern PE_INLINE const CHAR* EngineStaticClassName(const size_t& InClassHash);

    // Gather class static name of [InClassPtr] point to
    template<typename _TClassType>
    extern PE_INLINE const CHAR* EngineStaticClassName(const _TClassType* InClassPtr);

    // Cast class ptr [InFromPtr] to class ptr [_TToType] without check
    template<typename _TToType>
    extern PE_INLINE const _TToType* AsClassPtrUnsafe(const void* InFromPtr);

    // Cast class ptr [InFromPtr] to class ptr [_TToType] with check
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE const _TToType* AsClassPtr(const _TFromType* InFromPtr);

    // Cast class ptr [InFromPtr] to class ptr [_TToType] without check
    template<typename _TToType>
    extern PE_INLINE _TToType* AsClassPtrUnsafe(void* InFromPtr);

    // Cast class ptr [InFromPtr] to class ptr [_TToType] with check
    template<typename _TFromType, typename _TToType>
    extern PE_INLINE _TToType* AsClassPtr(_TFromType* InFromPtr);

    // Get class register name by class hash code
    extern PE_INLINE size_t GetClassHashCodeByName(const CHAR* InClassName);

    // Get class hash code by class register name
    extern PE_INLINE const CHAR* GetClassNameByHashCode(const size_t& InClassHashCode);

    // Save factory function [void* _Func(void)] for class
    template<typename _TType>
    extern PE_INLINE void SaveClassFactoryFunction(ERawPtrFunctionType __Function);

    // Create class object memory by class hash code from registed factory function [void* _Func(void)]
    extern PE_INLINE void* CreateClassObject(const size_t& InHashCode);

    // Create class object memory by class register name from registed factory function [void* _Func(void)]
    extern PE_INLINE void* CreateClassObject(const CHAR* InClassName);

    // Use class hash code to finding registed factory function then create object class then cast pointer into [_TType](must be parent class of create object class or the same class)
    template<typename _TType>
    extern PE_INLINE _TType* CreateClassObject(const size_t& InHashCode);

    // Use class name to finding registed factory function then create object class then cast pointer into [_TType](must be parent class of create object class or the same class)
    template<typename _TType>
    extern PE_INLINE _TType* CreateClassObject(const CHAR* InClassName);

    template<typename _TType, typename... _TParentTypes>
    PE_INLINE void RegisterClassType()
    {
        ERTTIManager::GetManagerSingleton()->SaveClassInfo<_TType, _TParentTypes...>();
    }

    template<typename _TType>
    PE_INLINE size_t GetClassHashCode()
    {
        return (ERTTIManager::GetManagerSingleton()->GetClassHashCode<_TType>());
    }

    template<typename _TType>
    PE_INLINE size_t GetClassHashCode(const _TType* InTypePtr)
    {
        return (ERTTIManager::GetManagerSingleton()->GetClassHashCode<_TType>(InTypePtr));
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsChildClass()
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsParentClass()
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType<_TFromType, _TToType>());
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsChildClass(const _TFromType* InFromPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>()));
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsParentClass(const _TFromType* InFromPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>()));
    }

    template<typename _TToType>
    PE_INLINE BOOL32 IsChildClass(const size_t& InFromTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(InFromTypeHash, GetClassHashCode<_TToType>()));
    }

    template<typename _TToType>
    PE_INLINE BOOL32 IsParentClass(const size_t& InFromTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(InFromTypeHash, GetClassHashCode<_TToType>()));
    }

    PE_INLINE BOOL32 IsChildClass(const size_t& InFromTypeHash, const size_t& InToTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(InFromTypeHash, InToTypeHash));
    }

    PE_INLINE BOOL32 IsParentClass(const size_t& InFromTypeHash, const size_t& InToTypeHash)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(InFromTypeHash, InToTypeHash));
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsChildClass(const _TFromType* InFromPtr, const _TToType* InToPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsChildClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsParentClass(const _TFromType* InFromPtr, const _TToType* InToPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->IsParentClassType(GetClassHashCode<_TFromType>(InFromPtr), GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TToType>
    PE_INLINE BOOL32 IsClass(const size_t& InFromHash)
    {
        return (InFromHash == (GetClassHashCode<_TToType>()));
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsClass(const _TFromType* InFromPtr)
    {
        return ((GetClassHashCode<_TFromType>(InFromPtr)) == (GetClassHashCode<_TToType>()));
    }

    PE_INLINE BOOL32 IsClass(const size_t& InFromHash, const size_t& InToHash)
    {
        return (InFromHash == InToHash);
    }

    template<typename _TToType>
    PE_INLINE BOOL32 IsClass(const size_t& InFromHash, const _TToType* InToPtr)
    {
        return (InFromHash == (GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TFromType>
    PE_INLINE BOOL32 IsClass(const _TFromType* InFromPtr, const size_t& InToHash)
    {
        return ((GetClassHashCode<_TFromType>(InFromPtr)) == InToHash);
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE BOOL32 IsClass(const _TFromType* InFromPtr, const _TToType* InToPtr)
    {
        return ((GetClassHashCode<_TFromType>(InFromPtr)) == (GetClassHashCode<_TToType>(InToPtr)));
    }

    template<typename _TClassType>
    PE_INLINE const CHAR* EngineStaticClassName()
    {
        return (ERTTIManager::GetManagerSingleton()->GetStaticClassName<_TClassType>());
    }

    PE_INLINE const CHAR* EngineStaticClassName(const size_t& InClassHash)
    {
        return (ERTTIManager::GetManagerSingleton()->GetStaticClassName(InClassHash));
    }

    template<typename _TClassType>
    PE_INLINE const CHAR* EngineStaticClassName(const _TClassType* InClassPtr)
    {
        return (ERTTIManager::GetManagerSingleton()->GetStaticClassName<_TClassType>(InClassPtr));
    }

    template<typename _TToType>
    PE_INLINE const _TToType* AsClassPtrUnsafe(const void* InFromPtr)
    {
        return (reinterpret_cast<const _TToType*>(InFromPtr));
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE const _TToType* AsClassPtr(const _TFromType* InFromPtr)
    {
        const _TToType* Result = nullptr;
        if (IsClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = dynamic_cast<const _TToType*>(InFromPtr);
        }
        else if (IsParentClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = dynamic_cast<const _TToType*>(InFromPtr);
        }
        return Result;
    }

    template<typename _TToType>
    PE_INLINE _TToType* AsClassPtrUnsafe(void* InFromPtr)
    {
        return (reinterpret_cast<_TToType*>(InFromPtr));
    }

    template<typename _TFromType, typename _TToType>
    PE_INLINE _TToType* AsClassPtr(_TFromType* InFromPtr)
    {
        _TToType* Result = nullptr;
        if (IsClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = dynamic_cast<_TToType*>(InFromPtr);
        }
        else if (IsParentClass<_TFromType, _TToType>(InFromPtr))
        {
            Result = dynamic_cast<_TToType*>(InFromPtr);
        }
        return Result;
    }

    PE_INLINE size_t GetClassHashCodeByName(const CHAR* InClassName)
    {
        return (ERTTIManager::GetManagerSingleton()->GetClassHashCodeByName(InClassName));
    }

    PE_INLINE const CHAR* GetClassNameByHashCode(const size_t& InClassHashCode)
    {
        return (ERTTIManager::GetManagerSingleton()->GetClassNameByHashCode(InClassHashCode));
    }

    template<typename _TType>
    PE_INLINE void SaveClassFactoryFunction(ERawPtrFunctionType __Function)
    {
        ERTTIManager::GetManagerSingleton()->SaveClassFactoryFunction<_TType>(__Function);
    }

    PE_INLINE void* CreateClassObject(const size_t& InHashCode)
    {
        return (ERTTIManager::GetManagerSingleton()->CreateClassObject(InHashCode));
    }

    PE_INLINE void* CreateClassObject(const CHAR* InClassName)
    {
        return (ERTTIManager::GetManagerSingleton()->CreateClassObject(InClassName));
    }

    template<typename _TType>
    PE_INLINE _TType* CreateClassObject(const size_t& InHashCode)
    {
        return (ERTTIManager::GetManagerSingleton()->CreateClassObject<_TType>(InHashCode));
    }

    template<typename _TType>
    PE_INLINE _TType* CreateClassObject(const CHAR* InClassName)
    {
        return (ERTTIManager::GetManagerSingleton()->CreateClassObject<_TType>(InClassName));
    }

};