#include "String.h"
namespace PigeonEngine
{
    PString::PString()
    {
    }

    PString::PString(const PString& Other)
        :
    Str(Other.Str)
    {
    }

    PString::PString(PString&& Other)
    {
        Str = std::move(Other.Str);
        Other = "";
    }

    PString::PString(const std::string& Other)
        :
    Str(Other)
    {
    }

    PString::PString(std::string&& Other)
        :
    Str(std::move(Other))
    {
        Other="";
    }

    PString::PString(const CHAR* Other)
    {
        Str = Other;
    }

    PString& PString::operator=(const PString& Other)
    {
        Str = Other.Str;
        return *this;
    }

    PString& PString::operator=(const std::string& Other)
    {
        Str = Other;
        return *this;
    }

    PString& PString::operator=(const CHAR* Other)
    {
        Str = Other;
        return *this;
    }

    PString& PString::operator=(PString&& Other) noexcept
    {
        Str = std::move(Other.Str);
        Other = "";
        return *this;
    }

    PString& PString::operator+(const PString& Other)
    {
        Str += Other.Str;
        return *this;
    }

    PString& PString::operator+(const std::string& Other)
    {
        Str += Other;
        return *this;
    }

    PString& PString::operator+(const CHAR* Other)
    {
        Str += Other;
        return *this;
    }

    const CHAR* PString::operator*() const
    {
        return Str.c_str();
    }

    CHAR PString::operator[](const UINT& Index) const
    {
        return Str[Index];
    }

    BOOL PString::operator==(const PString& Other) const
    {
        return Str == Other.Str;
    }

    BOOL PString::operator!=(const PString& Other) const
    {
        return Str != Other.Str;
    }

    UINT PString::Length() const
    {
        return Str.length();
    }

    BOOL PString::StartWith(const PString& SubString) const
    {
        return Find(SubString) == 0;
    }

    UINT PString::Find(const PString& SubString, UINT Offset) const
    {
        return Str.find(*SubString, Offset);
    }

    UINT PString::RightFind(const PString& SubString, UINT Offset) const
    {
        return Str.rfind(*SubString, Offset);
    }

    PString PString::Replace(const PString& From, const PString& To) const
    {
        const UINT FromLength = From.Length();
        PString Temp = *this;
        UINT FromStart  = Temp.Find(From);
        while(FromStart < Temp.Length())
        {
            Temp.Str.replace(Temp.Str.begin() + FromStart, Temp.Str.begin() + FromStart + FromLength - 1, *To);
            FromStart  = Temp.Find(From);
        }
        return Temp;
    }

    PString PString::Left(const UINT& Count) const
    {
        return PString(Str.substr(0, EMath::Min(Length(), Count)));
    }

    PString PString::Right(const UINT& Count) const
    {
        return PString(Str.substr(EMath::Max(Length() - Count, 0U), Count));
    }

    BOOL PString::IsNumeric() const
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

    INT PString::AtoI() const
    {
        if(!IsNumeric())
        {
            return -1;
        }
        return atoi(Str.c_str());
    }

    DOUBLE PString::AtoF() const
    {
        if(!IsNumeric())
        {
            return -1;
        }
        return atof(Str.c_str());
    }

  
};
