#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <string>

namespace PigeonEngine
{
    class EString
    {
    public:
        // This may not be useful. Because we return UINT32 replace size_t.
        static constexpr auto NonePos{ std::string::npos };
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
        CHAR        operator[](const UINT32& Index) const;

        BOOL32      operator==(const EString& Other)const;
        BOOL32      operator!=(const EString& Other)const;
        BOOL32      operator>(const EString& Other)const;
        BOOL32      operator<(const EString& Other)const;
        BOOL32      operator>=(const EString& Other)const;
        BOOL32      operator<=(const EString& Other)const;

        EString&    operator+=(const EString& Other);

        
        PE_NODISCARD UINT32  Length   ()const;
        PE_NODISCARD BOOL32  StartWith(const EString& SubString)const;
        PE_NODISCARD UINT32  Find     (const EString& SubString) const;
        PE_NODISCARD UINT32  RightFind(const EString& SubString) const;
        PE_NODISCARD EString Replace  (const EString& From, const EString& To) const;
        PE_NODISCARD EString Left     (const UINT32& Count) const;
        PE_NODISCARD EString Right    (const UINT32& Count) const;
        PE_NODISCARD EString Substring(const UINT32& Start, const UINT32& Count) const;

        PE_NODISCARD BOOL32  IsNumeric()const;
        PE_NODISCARD INT32   AtoI() const;
        PE_NODISCARD DOUBLE  AtoF() const;
    private:
        std::string Str;
    };
    
    extern PE_INLINE EString ToString(const UINT32& InValue);

    extern PE_INLINE BOOL32 SplitByLastSign(const CHAR InSplitSign, const EString& InOriginStr, EString& OutDotForwardStr, EString& OutDotBackwardStr);

};

