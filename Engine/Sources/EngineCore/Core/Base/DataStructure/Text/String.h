#pragma once
#include <CoreMinimal.h>

namespace PigeonEngine
{
    class PString
    {
    public:
        PString();
        PString(const PString& Other);

        explicit PString(const std::string& Other);
        explicit PString(std::string Other);
        explicit PString(const CHAR* Other);
        ~PString();

        PString&    operator=(const PString& Other);
        PString&    operator=(const std::string& Other);
        PString&    operator=(const CHAR* Other);
        PString&    operator+(const PString& Other);
        PString&    operator+(const std::string& Other);
        PString&    operator+(const CHAR* Other);
        const CHAR* operator*()const;
        CHAR        operator[](const UINT& Index) const;

        BOOL        operator==(const PString& Other)const;
        BOOL        operator!=(const PString& Other)const;
        
        [[nodiscard]] UINT    Length()const;
        [[nodiscard]] UINT    Find (const PString& SubString, UINT Offset = 0) const;
        [[nodiscard]] UINT    RightFind(const PString& SubString, UINT Offset = 0) const;
        [[nodiscard]] PString Replace(const PString& From, const PString& To) const;

        [[nodiscard]] BOOL   IsNumeric()const;
        [[nodiscard]] INT    AtoI() const;
        [[nodiscard]] FLOAT  AtoF() const;
    private:
        std::string Str;
    };
};

