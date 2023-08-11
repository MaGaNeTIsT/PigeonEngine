#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>

namespace PigeonEngine
{
    class EString
    {
    public:
        EString();
        EString(const EString& Other);
        EString(EString&& Other) noexcept;
        EString(const CHAR* Other);

        explicit EString(const std::string& Other);
        explicit EString(std::string&& Other);
        ~EString() = default;

        EString&    operator=(const EString& Other);
        EString&    operator=(const std::string& Other);
        EString&    operator=(const CHAR* Other);
        EString&    operator=(EString&& Other) noexcept;
        EString&    operator+(const EString& Other);
        EString&    operator+(const std::string& Other);
        EString&    operator+(const CHAR* Other);
        const CHAR* operator*()const;
        CHAR        operator[](const UINT& Index) const;

        BOOL        operator==(const EString& Other)const;
        BOOL        operator!=(const EString& Other)const;

        EString&    operator+=(const EString& Other);

        
        ENGINE_NODISCARD UINT    Length   ()const;
        ENGINE_NODISCARD BOOL    StartWith(const EString& SubString)const;
        ENGINE_NODISCARD UINT    Find     (const EString& SubString, UINT Offset = 0) const;
        ENGINE_NODISCARD UINT    RightFind(const EString& SubString, UINT Offset = 0) const;
        ENGINE_NODISCARD EString Replace  (const EString& From, const EString& To) const;
        ENGINE_NODISCARD EString Left     (const UINT& Count) const;
        ENGINE_NODISCARD EString Right    (const UINT& Count) const;
        ENGINE_NODISCARD EString Substring(const UINT& Start, const UINT& Count) const;

        ENGINE_NODISCARD BOOL    IsNumeric()const;
        ENGINE_NODISCARD INT     AtoI() const;
        ENGINE_NODISCARD DOUBLE  AtoF() const;
    private:
        std::string Str;
    };
    
    extern inline EString ToString(const UINT& InValue);
};

