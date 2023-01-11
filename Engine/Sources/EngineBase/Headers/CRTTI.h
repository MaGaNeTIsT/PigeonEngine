#pragma once

//Runtime Interface,all types need rtti need to implement from this.
class CRTTI
{
public:
    virtual const size_t TypeIdInstance() const = 0;

    virtual CRTTI* QueryInterface(const size_t)
    {
        return nullptr;
    }
    virtual const CRTTI* QueryInterface(const size_t) const
    {
        return nullptr;
    }

    virtual bool Is(const size_t id) const
    {
        return false;
    }

    virtual bool Is(const char* name) const
    {
        return false;
    }

    template <typename T>
    CRTTI* QueryInterfaceT()
    {
        return QueryInterface(T::TypeIdClass());
    }

    template <typename T>
    const CRTTI* QueryInterfaceT() const
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

#define CClass(Type, ParentType)                                                \
    public:                                                                     \
        static const char* TypeName() { return #Type; }                         \
        virtual const size_t TypeIdInstance() const                             \
        { return Type::TypeIdClass(); }                                         \
        static const size_t TypeIdClass()                                       \
        { static int d = 0; return (size_t) &d; }                               \
        virtual ::CRTTI* QueryInterface( const size_t id )                      \
        {                                                                       \
            if (id == TypeIdClass())                                            \
                { return (CRTTI*)this; }                                        \
            else                                                                \
                { return ParentType::QueryInterface(id); }                      \
        }                                                                       \
        virtual const ::CRTTI* QueryInterface( const size_t id ) const          \
        {                                                                       \
            if (id == TypeIdClass())                                            \
                { return (CRTTI*)this; }                                        \
            else                                                                \
                { return ParentType::QueryInterface(id); }                      \
        }                                                                       \
        virtual bool Is(const size_t id) const                                  \
        {                                                                       \
            if (id == TypeIdClass())                                            \
                { return true; }                                                \
            else                                                                \
                { return ParentType::Is(id); }                                  \
        }                                                                       \
        virtual bool Is(const char* name) const                                 \
        {                                                                       \
            if (name == TypeName())                                             \
                { return true; }                                                \
            else                                                                \
                { return ParentType::Is(name); }                                \
        }     