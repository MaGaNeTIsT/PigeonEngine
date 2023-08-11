#pragma once

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

};