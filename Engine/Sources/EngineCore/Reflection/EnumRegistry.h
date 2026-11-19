#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{
	/*
	* Stable on-disk names for enum values. Serializers write the name, so inserting a
	* value into the middle of an enum never reinterprets an existing file.
	*
	* Registration is keyed by the enum's runtime type hash (typeid), exactly like the
	* class property registry. Files never store that hash - they store the enum value
	* names - so this table is registration-time only.
	*/
	struct FEnumEntry
	{
		INT32 Value = 0;
		EString Name;
	};

	class EEnumRegistry final
	{
	public:
		static void Register(const size_t& InEnumHash, const CHAR* InEnumName, const TArray<FEnumEntry>& InEntries);

		// TRUE when the value has a registered name; OutName is left untouched otherwise.
		static BOOL32 TryValueToString(const size_t& InEnumHash, INT32 InValue, EString& OutName);
		// TRUE when the name belongs to this enum; OutValue is left untouched otherwise.
		static BOOL32 TryStringToValue(const size_t& InEnumHash, const CHAR* InName, INT32& OutValue);
	};

	template <typename TEnum>
	void RegisterEnumType(const CHAR* InEnumName, const std::initializer_list<FEnumEntry>& InEntries)
	{
		TArray<FEnumEntry> Entries;
		for (const FEnumEntry& Entry : InEntries)
		{
			Entries.Add(Entry);
		}
		EEnumRegistry::Register(GetClassHashCode<TEnum>(), InEnumName, Entries);
	}
}
