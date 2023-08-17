#pragma once

#include "RTTIManager.h"

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
        BOOL IsTypeOf()const
        {
            return (IsClass(this->GetTypeHash(), GetClassHashCode<_Type>()));
        }

        /// <summary>
        /// Determine that [*this] class is child of [_Type] class
        /// </summary>
        /// <typeparam name="_Type"></typeparam>
        /// <returns></returns>
        template<typename _Type>
        BOOL IsChildOf()const
        {
            return (IsParentClass<_Type>(this->GetTypeHash()));
        }

        /// <summary>
        /// Determine that [*this] class is parent of [_Type] class
        /// </summary>
        /// <typeparam name="_Type"></typeparam>
        /// <returns></returns>
        template<typename _Type>
        BOOL IsParentOf()const
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
                Result = this;
            }
            else if (this->IsChildOf<_Type>())
            {
                Result = static_cast<_Type*>(this);
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
                Result = this;
            }
            else if (this->IsChildOf<_Type>())
            {
                Result = static_cast<const _Type*>(this);
            }
            return Result;
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
    };

};