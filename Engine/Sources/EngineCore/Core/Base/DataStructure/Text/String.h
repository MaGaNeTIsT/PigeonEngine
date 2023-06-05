#pragma once
#include <CoreMinimal.h>

namespace PigeonEngine
{
    class EString
    {
    public:
        EString();
        EString(const EString& Other);
        EString(EString&& Other) noexcept;

        explicit EString(const std::string& Other);
        explicit EString(std::string&& Other);
        explicit EString(const CHAR* Other);
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
        
        [[nodiscard]] UINT    Length   ()const;
        [[nodiscard]] BOOL    StartWith(const EString& SubString)const;
        [[nodiscard]] UINT    Find     (const EString& SubString, UINT Offset = 0) const;
        [[nodiscard]] UINT    RightFind(const EString& SubString, UINT Offset = 0) const;
        [[nodiscard]] EString Replace  (const EString& From, const EString& To) const;
        [[nodiscard]] EString Left     (const UINT& Count) const;
        [[nodiscard]] EString Right    (const UINT& Count) const;
        [[nodiscard]] EString Substring(const UINT& Start, const UINT& Count) const;

        [[nodiscard]] BOOL    IsNumeric()const;
        [[nodiscard]] INT     AtoI() const;
        [[nodiscard]] DOUBLE  AtoF() const;
    private:
        std::string Str;
    };
};

