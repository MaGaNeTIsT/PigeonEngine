#pragma once

#include "../../../Config/ErrorCaption.h"
#include "../../BuiltInLanguage.h"
#include "../BuiltInType.h"
#include "../../Development/Alert/DevelopmentDefines.h"
#include "String.h"
#include "../Container/Array.h"
#include "../Container/HashMap.h"
#include <deque>
#include <functional>
#include <mutex>

namespace PigeonEngine
{

	class EName
	{
	private:
		static constexpr SIZE_T InvalidIndex = static_cast<SIZE_T>(-1);

		struct EStringPool
		{
			// A deque, not an array: ToString hands out references into this storage, and a
			// concurrent Add from the load thread must not move the strings those references
			// point at. Appending at the end of a deque leaves every existing element in place.
			std::deque<EString> Strings;
			THashMap<SIZE_T, SIZE_T> HashToIndex;
			// One name is interned at a time. The pool is process wide and the render thread
			// interns a derived name while the load thread interns the names its assembly
			// produces; without this, the table and the deque are written by two threads.
			mutable std::mutex PoolMutex;

			SIZE_T Add(const EString& InString)
			{
				const SIZE_T Hash = ComputeHash(InString);

				// The lookup and the insert are one operation: a second thread that interned the
				// same string in between must find that entry rather than append a duplicate.
				std::lock_guard<std::mutex> PoolLock(PoolMutex);
				if (HashToIndex.ContainsKey(Hash))
				{
					const SIZE_T ExistingIndex = HashToIndex[Hash];
					// djb2 -> SIZE_T collision: two distinct strings hashing
					// to the same bucket would silently merge here. Trip the
					// debug-only check so we catch the offender at the call
					// site instead of chasing phantom aggregated profile
					// rows or aliased EName identities later.
					PE_CHECK(
						(ENGINE_STRING_ERROR),
						(PE_TEXT("EName hash collision: distinct strings produced the same hash. Rename one of them.")),
						(Strings[ExistingIndex] == InString));
					return ExistingIndex;
				}

				const SIZE_T TempIndex = static_cast<SIZE_T>(Strings.size());
				Strings.push_back(InString);
				HashToIndex.Add(Hash, TempIndex);
				return TempIndex;
			}

			const EString& Get(const SIZE_T& Index) const
			{
				static EString Empty;
				// The size and the storage are read under the same lock Add writes them under, so
				// an append cannot change the container while this walk is looking at it. The
				// returned reference stays valid after the unlock: a deque never moves an element
				// it already holds when another is appended.
				std::lock_guard<std::mutex> PoolLock(PoolMutex);
				if (Index < static_cast<SIZE_T>(Strings.size()))
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
