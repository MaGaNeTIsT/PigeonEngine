#pragma once

#include "Name.h"
#include "../Container/Set.h"

namespace PigeonEngine
{
	class ETag
	{
	private:
		EName Name;

	public:
		ETag()
		{
		}

		ETag(const CHAR* Other) :
			Name(Other)
		{
		}

		ETag(const EName& Other) :
			Name(Other)
		{
		}

		ETag(const ETag& Other) :
			Name(Other.Name)
		{
		}

		ETag(const EString& Other) :
			Name(Other)
		{
		}

		PE_NODISCARD const EName& GetName() const
		{
			return Name;
		}

		BOOL32 operator==(const ETag& Other) const
		{
			return Name == Other.Name;
		}

		BOOL32 operator!=(const ETag& Other) const
		{
			return Name != Other.Name;
		}

		BOOL32 operator<(const ETag& Other) const  //Is this necessary?
		{
			return Name < Other.Name;
		}

		PE_NODISCARD BOOL32 IsValid() const
		{
			return Name.IsValid();
		}

		PE_NODISCARD const CHAR* GetData() const
		{
			return Name.GetData();
		}

		PE_NODISCARD const EString& ToString() const
		{
			return Name.ToString();
		}
	};

	class ETagContainer
	{
	public:
		using TConstIterator = typename TSet<ETag>::TConstIterator;

	private:
		TSet<ETag> Tags;

	public:
		ETagContainer()
		{
		}

		void AddTag(const ETag& Other)
		{
			if (Other.IsValid())
			{
				Tags.Add(Other);
			}
		}

		void AddTag(const CHAR* Other)
		{
			AddTag(ETag(Other));
		}

		void AddTag(const EName& Other)
		{
			AddTag(ETag(Other));
		}	

		void AddTag(const EString& Other)
		{
			AddTag(ETag(Other));
		}

		void AddTags(const ETagContainer& Other)
		{
			for (const auto& Tag : Other.Tags)
			{
				AddTag(Tag);
			}
		}

		void RemoveTag(const ETag& Other)
		{
			Tags.Remove(Other);
		}

		void RemoveTag(const CHAR* Other)
		{
			RemoveTag(ETag(Other));
		}

		void RemoveTags(const ETagContainer& Other)
		{
			for (const auto& Tag : Other.Tags)
			{
				RemoveTag(Tag);
			}
		}

		PE_NODISCARD BOOL32 HasTag(const ETag& Other) const
		{
			return Tags.Contains(Other);
		}

		PE_NODISCARD BOOL32 HasTag(const CHAR* Other) const
		{
			return HasTag(ETag(Other));
		}

		PE_NODISCARD BOOL32 HasAny(const ETagContainer& Other) const
		{
			for (const auto& Tag : Other.Tags)
			{
				if (Tags.Contains(Tag))
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		PE_NODISCARD BOOL32 HasAll(const ETagContainer& Other) const
		{
			for (const auto& Tag : Other.Tags)
			{
				if (!Tags.Contains(Tag))
				{
					return FALSE;
				}
			}
			return TRUE;
		}

		PE_NODISCARD BOOL32 MatchesExact(const ETagContainer& Other) const
		{
			if (Tags.Num<UINT32>() != Other.Tags.Num<UINT32>())
			{
				return FALSE;
			}
			return HasAll(Other);
		}

		void Clear()
		{
			Tags.Empty();
		}

		PE_NODISCARD UINT32 Num() const
		{
			return Tags.Num<UINT32>();
		}

		PE_NODISCARD BOOL32 IsEmpty() const
		{
			return Tags.Num<UINT32>() == 0;
		}

		PE_NODISCARD ETagContainer UnionWith(const ETagContainer& Other) const
		{
			ETagContainer Result(*this);
			Result.Tags.Append(Other.Tags);
			return Result;
		}

		PE_NODISCARD ETagContainer Intersection(const ETagContainer& Other) const
		{
			ETagContainer Result;
			for (const auto& Tag : Tags)
			{
				if (Other.Tags.Contains(Tag))
				{
					Result.Tags.Add(Tag);
				}
			}
			return Result;
		}

		PE_NODISCARD ETagContainer Difference(const ETagContainer& Other) const
		{
			ETagContainer Result;
			for (const auto& Tag : Tags)
			{
				if (!Other.Tags.Contains(Tag))
				{
					Result.Tags.Add(Tag);
				}
			}
			return Result;
		}

		PE_NODISCARD TConstIterator Begin() const
		{
			return Tags.Begin();
		}

		PE_NODISCARD TConstIterator End() const
		{
			return Tags.End();
		}
	};
}