#pragma once

#include "RTTIManager.h"
#include <Base/RegisterBase.h>
#include <Base/TemplateHeaders.h>

namespace PigeonEngine
{

    class ERTTIObject
    {
    public:
        /// <summary>
        /// Determine that [*this] class is or is not [_Type] class
        /// </summary>
        /// <typeparam name="_Type"></typeparam>
        /// <returns></returns>
        template<typename _Type>
        BOOL32 IsTypeOf()const
        {
            return (IsClass(this->GetTypeHash(), GetClassHashCode<_Type>()));
        }

        /// <summary>
        /// Determine that [*this] class is child of [_Type] class
        /// </summary>
        /// <typeparam name="_Type"></typeparam>
        /// <returns></returns>
        template<typename _Type>
        BOOL32 IsChildOf()const
        {
            return (IsParentClass<_Type>(this->GetTypeHash()));
        }

        /// <summary>
        /// Determine that [*this] class is parent of [_Type] class
        /// </summary>
        /// <typeparam name="_Type"></typeparam>
        /// <returns></returns>
        template<typename _Type>
        BOOL32 IsParentOf()const
        {
            return (IsChildClass<_Type>(this->GetTypeHash()));
        }

        /// <summary>
        /// This is very slow operator that can cast child pointer or parent pointer into the other one.
        /// This operator will check class type is or is not fitted.
        /// </summary>
        /// <typeparam name="_Type"></typeparam>
        /// <returns></returns>
        template<typename _Type>
        _Type* AsType()
        {
            _Type* Result = nullptr;
            if (this->IsTypeOf<_Type>())
            {
                Result = dynamic_cast<_Type*>(this);
            }
            else if (this->IsChildOf<_Type>())
            {
                Result = dynamic_cast<_Type*>(this);
            }
            return Result;
        }

        /// <summary>
        /// This is very slow operator that can cast child pointer or parent pointer into the other one.
        /// This operator will check class type is or is not fitted.
        /// </summary>
        /// <typeparam name="_Type"></typeparam>
        /// <returns></returns>
        template<typename _Type>
        const _Type* AsType()const
        {
            const _Type* Result = nullptr;
            if (this->IsTypeOf<_Type>())
            {
                Result = dynamic_cast<const _Type*>(this);
            }
            else if (this->IsChildOf<_Type>())
            {
                Result = dynamic_cast<const _Type*>(this);
            }
            return Result;
        }

        virtual const CHAR* StaticClassName()const
        {
            return (EngineStaticClassName(this));
        }
    protected:
        /// <summary>
        /// Get hash code of [*this] type
        /// </summary>
        /// <returns></returns>
        virtual size_t GetTypeHash()const
        {
            return (GetClassHashCode(this));
        }
    public:
        ERTTIObject() {}
        ERTTIObject(const ERTTIObject&) {}
        virtual ~ERTTIObject() {}
        ERTTIObject& operator=(const ERTTIObject&) { return (*this); }
    };

    PE_REGISTER_CLASS_DECLARE(EClassTypeRegisterManager, EVoidFunctionType);

#define PE_REGISTER_CLASS_TYPE(__Func) \
    static PigeonEngine::EClassTypeRegisterManager::ERegisterObject ClassTypeRegisterObject(__Func);\

    PE_REGISTER_CLASS_DECLARE(EClassFactoryRegisterManager, EVoidFunctionType);

#define PE_REGISTER_CLASS_FACTORY(__Func) \
    static PigeonEngine::EClassFactoryRegisterManager::ERegisterObject ClassFactoryRegisterObject(__Func);\

};