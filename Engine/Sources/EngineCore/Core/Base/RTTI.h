#pragma once

#include <Base/DataStructure/BuiltInType.h>
#include <Config/EngineConfig.h>
#include <map>
#include <string>
#include <set>

namespace PigeonEngine
{

    class ERTTI;

#define EClass(Type, ParentType) \
    public:\
        static const char* TypeName() { return #Type; }\
        virtual const size_t TypeIdInstance() const override { return Type::TypeIdClass(); }\
        static const size_t TypeIdClass() { static int d = 0; return (size_t) &d; }\
        virtual ERTTI* QueryInterface( const size_t id ) override\
        {\
            if (id == TypeIdClass()) { return (ERTTI*)this; }\
            else { return ParentType::QueryInterface(id); }\
        }\
        virtual const ERTTI* QueryInterface( const size_t id ) const override\
        {\
            if (id == TypeIdClass()) { return (ERTTI*)this; }\
            else { return ParentType::QueryInterface(id); }\
        }\
        virtual bool Is(const size_t id) const override\
        {\
            if (id == TypeIdClass()) { return true; }\
            else { return ParentType::Is(id); }\
        }\
        virtual bool Is(const CHAR* name) const override\
        {\
            if (name == TypeName()) { return true; }\
            else { return ParentType::Is(name); }\
        }\
        using Super = ParentType;\

    //Runtime Interface,all types need rtti need to implement from this.
    class ERTTI
    {
    public:
        virtual const size_t TypeIdInstance() const = 0;

        virtual ERTTI* QueryInterface(const size_t)
        {
            return nullptr;
        }
        virtual const ERTTI* QueryInterface(const size_t) const
        {
            return nullptr;
        }
        virtual bool Is(const size_t id) const
        {
            return false;
        }

        /// <summary>
        /// This function CAN NOT used for those class that inherited from template type class.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        virtual bool Is(const CHAR* name) const
        {
            return false;
        }

        template <typename T>
        ERTTI* QueryInterfaceT()
        {
            return QueryInterface(T::TypeIdClass());
        }

        template <typename T>
        const ERTTI* QueryInterfaceT() const
        {
            return QueryInterface(T::TypeIdClass());
        }

        template <typename T>
        bool IsT() const
        {
            return Is(T::TypeIdClass());
        }

        template <typename T>
        T* As()
        {
            if (Is(T::TypeIdClass()))
            {
                return (T*)this;
            }

            return nullptr;
        }
        template <typename T>
        const T* As() const
        {
            if (Is(T::TypeIdClass()))
            {
                return (T*)this;
            }

            return nullptr;
        }
    };

    template<typename _TType, typename... _TParentTypes>
    extern inline void RegisterClassType();

    template<typename _TType>
    extern inline size_t GetClassHashCode();

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsChildClass();

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsParentClass();

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsChildClass(const _TFromType& InRef);

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsParentClass(const _TFromType& InRef);

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsChildClass(const _TFromType* InPtr);

    template<typename _TFromType, typename _TToType>
    extern inline BOOL IsParentClass(const _TFromType* InPtr);

    class ERTTIObject
    {
    public:
        template<typename _Type>
        BOOL IsChildOf()
        {
            return 
        }
    };

    class ERTTIManager
    {
    public:
        template<typename _TType>
        size_t GetClassHashCode()
        {
            return (typeid(_TType).hash_code());
        }
        template<typename _TType, typename... _TParentTypes>
        void SaveClassInfo()
        {
            ProcessClassInfoInternal<_TType, _TParentTypes...>();
        }

        /// <summary>
        /// Find if [_TTestType] inherit from [_TInType]
        /// </summary>
        /// <typeparam name="_TInType"></typeparam>
        /// <typeparam name="_TTestType"></typeparam>
        /// <returns></returns>
        template<typename _TInType, typename _TTestType>
        BOOL IsChildClassType()
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
        void IsParentClassType()
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
        void ForwardRecursion(size_t InHashCode, const TFunctionType& InFunc, const TConditionType& InCond)
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
        void BackwardRecursion(size_t InHashCode, const TFunctionType& InFunc, const TConditionType& InCond)
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
    public:
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

};