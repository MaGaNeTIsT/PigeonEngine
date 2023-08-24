#include "String.h"
#include <Config/ErrorCaption.h>
#include <Base/Math/Math.h>
#include <Config/EngineConfig.h>
#include "../../../../../Development/Alert/DevelopmentDefines.h"

namespace PigeonEngine
{
    EString::EString()
    {
    }

    EString::EString(const EString& Other)
        :
    Str(Other.Str)
    {
    }

    EString::EString(EString&& Other) noexcept
    {
        Str = std::move(Other.Str);
        Other = "";
    }

    EString::EString(const std::string& Other)
        :
    Str(Other)
    {
    }

    EString::EString(std::string&& Other)
        :
    Str(std::move(Other))
    {
        Other="";
    }

    EString::EString(const CHAR* Other)
    {
        Str = Other;
    }

    EString& EString::operator=(const EString& Other)
    {
        Str = Other.Str;
        return *this;
    }

    EString& EString::operator=(const std::string& Other)
    {
        Str = Other;
        return *this;
    }

    EString& EString::operator=(const CHAR* Other)
    {
        Str = Other;
        return *this;
    }

    EString& EString::operator=(EString&& Other) noexcept
    {
        Str = std::move(Other.Str);
        Other = "";
        return *this;
    }

    EString& EString::operator+(const EString& Other)
    {
        Str += Other.Str;
        return *this;
    }

    EString& EString::operator+(const std::string& Other)
    {
        Str += Other;
        return *this;
    }

    EString& EString::operator+(const CHAR* Other)
    {
        Str += Other;
        return *this;
    }

    const CHAR* EString::operator*() const
    {
        return Str.c_str();
    }

    CHAR EString::operator[](const UINT& Index) const
    {
        return Str[Index];
    }

    BOOL EString::operator==(const EString& Other) const
    {
        return Str == Other.Str;
    }

    BOOL EString::operator!=(const EString& Other) const
    {
        return Str != Other.Str;
    }

    BOOL EString::operator>(const EString& Other) const
    {
        return this->Str > Other.Str;
    }

    BOOL EString::operator<(const EString& Other) const
    {
        return this->Str < Other.Str;
    }

    BOOL EString::operator>=(const EString& Other) const
    {
        return this->Str >= Other.Str;
    }

    BOOL EString::operator<=(const EString& Other) const
    {
        return this->Str <= Other.Str;
    }

    EString& EString::operator+=(const EString& Other)
    {
        Str += Other.Str;
        return *this;
    }

    UINT EString::Length() const
    {
        return static_cast<UINT>(Str.length());
    }

    BOOL EString::StartWith(const EString& SubString) const
    {
        return Find(SubString) == 0;
    }

    UINT EString::Find(const EString& SubString, UINT Offset) const
    {
        return static_cast<UINT>(Str.find(*SubString, Offset));
    }

    UINT EString::RightFind(const EString& SubString, UINT Offset) const
    {
        return static_cast<UINT>(Str.rfind(*SubString, Offset));
    }

    EString EString::Replace(const EString& From, const EString& To) const
    {
        const UINT FromLength = From.Length();
        EString Temp = *this;
        UINT FromStart  = Temp.Find(From);
        while(FromStart < Temp.Length())
        {
            Temp.Str.replace(Temp.Str.begin() + FromStart, Temp.Str.begin() + FromStart + FromLength - 1, *To);
            FromStart  = Temp.Find(From);
        }
        return Temp;
    }

    EString EString::Left(const UINT& Count) const
    {
        return EString(Str.substr(0, EMath::Min(Length(), Count)));
    }

    EString EString::Right(const UINT& Count) const
    {
        return EString(Str.substr(EMath::Max(Length() - Count, 0U), Count));
    }

    EString EString::Substring(const UINT& Start, const UINT& Count) const
    {
        return EString(Str.substr(EMath::Max(0u, Start), EMath::Min(Length() - Start, Count)));
    }

    BOOL EString::IsNumeric() const
    {
        for(const auto& elem : Str)
        {
            if(!(elem < '9' && elem > '0'))
            {
                return false;
            }
        }
        return true;
    }

    INT EString::AtoI() const
    {
        if(!IsNumeric())
        {
            return -1;
        }
        return atoi(Str.c_str());
    }

    DOUBLE EString::AtoF() const
    {
        if(!IsNumeric())
        {
            return -1;
        }
        return atof(Str.c_str());
    }

    ENGINE_INLINE EString ToString(const UINT& InValue)
    {
        return EString(std::to_string(InValue).c_str());
    }

    ENGINE_INLINE BOOL SplitByLastSign(const CHAR InSplitSign, const EString& InOriginStr, EString& OutDotForwardStr, EString& OutDotBackwardStr)
    {
        const UINT OriginStrLen = InOriginStr.Length();
        if (OriginStrLen <= 3u)
        {
            OutDotForwardStr    = ENGINE_DEFAULT_NAME;
            OutDotBackwardStr   = ENGINE_DEFAULT_NAME;
            return FALSE;
        }
        const CHAR TempCharSign[1] = { InSplitSign };
        const UINT LastDot = InOriginStr.RightFind(TempCharSign);
        if (LastDot >= OriginStrLen)
        {
            OutDotForwardStr    = ENGINE_DEFAULT_NAME;
            OutDotBackwardStr   = ENGINE_DEFAULT_NAME;
            return FALSE;
        }
        Check((ENGINE_STRING_ERROR), ("Check string rest length error."), (OriginStrLen >= (LastDot + 1u)));
        OutDotForwardStr    = InOriginStr.Substring(0u, LastDot);
        OutDotBackwardStr   = InOriginStr.Substring((LastDot + 1u), OriginStrLen - (LastDot + 1u));
        return TRUE;
    }

};
