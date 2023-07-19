#pragma once
#include<CoreMinimal.h>
#include <memory>
namespace PigeonEngine
{
    template <typename T>
    class TSharedPtr
    {
    public:
        TSharedPtr();
        TSharedPtr(const TSharedPtr<T>& Other);
        explicit TSharedPtr(const std::shared_ptr<T>& Other);

        T&             operator*()const;
        T*             operator->()const;
        TSharedPtr<T>& operator=(const TSharedPtr<T>& Other);
        TSharedPtr<T>& operator=(const std::shared_ptr<T>& Other);
        explicit       operator bool() const;
        bool           operator==(const TSharedPtr<T>& Other);
        bool           operator!=(const TSharedPtr<T>& Other);
        
        ENGINE_NODISCARD BOOL IsValid()const;
        
        T* Get() const;

        void Reset();
        
        template <class... Types>
        static TSharedPtr<T> MakeShared(Types&&... Args);
        
    private:
        std::shared_ptr<T> Shared;
    };

   
 
};

 