#pragma once

#include "../../../../Main/Main.h"
#include "String.h"
#include "../Container/HashMap.h"

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
					return HashToIndex[Hash];
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
} // namespace PigeonEngine
