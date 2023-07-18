#pragma once
#include <memory>
#include<CoreMinimal.h>
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
        
        ENGINE_NODISCARD BOOL IsValid()const;
        
        T* Get() const;

        void Reset();
        static TSharedPtr<T> MakeShared();
        
    private:
        std::shared_ptr<T> Shared;
    };

 
};

