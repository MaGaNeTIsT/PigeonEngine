#pragma once

#include "../../../Config/ErrorCaption.h"
#include "../../BuiltInLanguage.h"
#include "../BuiltInType.h"
#include "../../Development/Alert/DevelopmentDefines.h"
#include "String.h"
#include "../Container/Array.h"
#include "../Container/HashMap.h"
#include <functional>

namespace PigeonEngine
{

	class EName
	{
	private:
		static constexpr SIZE_T InvalidIndex = static_cast<SIZE_T>(-1);

		struct EStringPool
		{
			TArray<EString> Strings;
			THashMap<SIZE_T, SIZE_T> HashToIndex;

			SIZE_T Add(const EString& InString)
			{
				const SIZE_T Hash = ComputeHash(InString);

				if (HashToIndex.ContainsKey(Hash))
				{
					const SIZE_T ExistingIdx = HashToIndex[Hash];
					// djb2 -> SIZE_T collision: two distinct strings hashing
					// to the same bucket would silently merge here. Trip the
					// debug-only check so we catch the offender at the call
					// site instead of chasing phantom aggregated profile
					// rows or aliased EName identities later.
					PE_CHECK(
						(ENGINE_STRING_ERROR),
						("EName hash collision: distinct strings produced the same hash. Rename one of them."),
						(Strings[ExistingIdx] == InString));
					return ExistingIdx;
				}

				const SIZE_T TempIndex = Strings.Num<SIZE_T>();
				Strings.Add(InString);
				HashToIndex.Add(Hash, TempIndex);
				return TempIndex;
			}

			const EString& Get(const SIZE_T& Index) const
			{
				static EString Empty;
				if (Index < Strings.Num<SIZE_T>())
				{
					return Strings[Index];
				}
				return Empty;
			}

		private:
			SIZE_T ComputeHash(const EString& InString) const
			{
				SIZE_T Hash = 5381;
				const CHAR* Data = *InString;
				while (*Data)
				{
					Hash = ((Hash << 5) + Hash) + static_cast<UINT8>(*Data++);
				}
				return Hash;
			}
		};

		static EStringPool& GetPool()
		{
			static EStringPool Pool;
			return Pool;
		}

	private:
		SIZE_T Index;

	public:
		EName():
			Index(InvalidIndex)
		{
		}

		EName(const CHAR* Other)
		{
			if (Other && *Other)
			{
				Index = GetPool().Add(EString(Other));
			}
			else
			{
				Index = InvalidIndex;
			}
		}

		EName(const EString& Other)
		{
			if (Other.Length() > 0)
			{
				Index = GetPool().Add(Other);
			}
			else
			{
				Index = InvalidIndex;
			}
		}

		EName(const EName& Other):
			Index(Other.Index)
		{
		}

		EName& operator=(const EName& Other)
		{
			Index = Other.Index;
			return *this;
		}

		BOOL32 operator==(const EName& Other) const
		{
			return Index == Other.Index;
		}

		BOOL32 operator!=(const EName& Other) const
		{
			return Index != Other.Index;
		}

		BOOL32 operator<(const EName& Other) const //Is this necessary?
		{
			return Index < Other.Index;
		}

		PE_NODISCARD const EString& ToString() const
		{
			if (Index != InvalidIndex)
			{
				return GetPool().Get(Index);
			}
			static EString Empty;
			return Empty;
		}

		PE_NODISCARD const CHAR* GetData() const
		{
			return *ToString();
		}

		PE_NODISCARD BOOL32 IsValid() const
		{
			return Index != InvalidIndex;
		}

		PE_NODISCARD BOOL32 IsNone() const
		{
			return Index == InvalidIndex;
		}

		PE_NODISCARD SIZE_T GetIndex() const
		{
			return Index;
		}

		static EName None()
		{
			return EName();
		}
	};

};	// namespace PigeonEngine

namespace std
{
	template<>
	struct hash<PigeonEngine::EName>
	{
		std::size_t operator()(const PigeonEngine::EName& InVal) const noexcept
		{
			return (InVal.GetIndex());
		}
	};
};	// namespace std
