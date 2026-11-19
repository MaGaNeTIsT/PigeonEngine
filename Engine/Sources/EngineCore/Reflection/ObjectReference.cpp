#include "ObjectReference.h"

namespace PigeonEngine
{
	EString MakeObjectReferenceString(const UINT32& InActorFileId, const UINT32& InComponentIndex)
	{
		return EString::FromInt(InActorFileId) + EString(PE_TEXT(":")) + EString::FromInt(InComponentIndex);
	}

	BOOL32 ParseObjectReferenceString(const EString& InReference, UINT32& OutActorFileId, UINT32& OutComponentIndex)
	{
		const INT32 SeparatorIndex = InReference.Find(EString(PE_TEXT(":")));
		if ((SeparatorIndex <= 0) || (static_cast<UINT32>(SeparatorIndex) >= InReference.Length()))
		{
			return FALSE;
		}
		const EString ActorPart = InReference.Left(static_cast<UINT32>(SeparatorIndex));
		const EString ComponentPart = InReference.Right(InReference.Length() - static_cast<UINT32>(SeparatorIndex) - 1u);
		if ((ActorPart.Length() == 0u) || (ComponentPart.Length() == 0u))
		{
			return FALSE;
		}
		if ((!ActorPart.IsNumeric()) || (!ComponentPart.IsNumeric()))
		{
			return FALSE;
		}
		OutActorFileId = static_cast<UINT32>(ActorPart.AtoI());
		OutComponentIndex = static_cast<UINT32>(ComponentPart.AtoI());
		return (OutActorFileId > 0u) ? TRUE : FALSE;
	}
}
