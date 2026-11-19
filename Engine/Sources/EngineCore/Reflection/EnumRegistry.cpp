#include "EnumRegistry.h"

namespace PigeonEngine
{
	namespace
	{
		struct FEnumInfo
		{
			EString Name;
			TArray<FEnumEntry> Entries;
		};

		TMap<size_t, FEnumInfo>& GetEnumMap()
		{
			static TMap<size_t, FEnumInfo> Enums;
			return Enums;
		}
	}

	void EEnumRegistry::Register(const size_t& InEnumHash, const CHAR* InEnumName, const TArray<FEnumEntry>& InEntries)
	{
		FEnumInfo Info;
		Info.Name = InEnumName;
		Info.Entries = InEntries;
		GetEnumMap().Add(InEnumHash, Info);
	}

	BOOL32 EEnumRegistry::TryValueToString(const size_t& InEnumHash, INT32 InValue, EString& OutName)
	{
		const FEnumInfo* Info = GetEnumMap().FindValueAsPtr(InEnumHash);
		if (!Info)
		{
			return FALSE;
		}
		for (UINT32 Index = 0u, Num = Info->Entries.Num<UINT32>(); Index < Num; Index++)
		{
			if (Info->Entries[Index].Value == InValue)
			{
				OutName = Info->Entries[Index].Name;
				return TRUE;
			}
		}
		return FALSE;
	}

	BOOL32 EEnumRegistry::TryStringToValue(const size_t& InEnumHash, const CHAR* InName, INT32& OutValue)
	{
		const FEnumInfo* Info = GetEnumMap().FindValueAsPtr(InEnumHash);
		if (!Info || !InName)
		{
			return FALSE;
		}
		for (UINT32 Index = 0u, Num = Info->Entries.Num<UINT32>(); Index < Num; Index++)
		{
			if (Info->Entries[Index].Name == InName)
			{
				OutValue = Info->Entries[Index].Value;
				return TRUE;
			}
		}
		return FALSE;
	}
}
