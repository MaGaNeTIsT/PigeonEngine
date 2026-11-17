#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>

namespace PigeonEngine
{

	enum class EEditorSelectionType : UINT8
	{
		NONE = 0,
		MACHINE,
		STATE,
		TRANSITION,
		PARAMETER,
		CONDITION,
		CLIP,
		SECTION,
		NOTIFY,
		SLOT_GROUP,
		CURVE,
		BONE
	};

	/*
	* One typed selection shared by the graph, the track tree and the inspector. ObjectId is
	* the stable name-based identity used by the asset JSON; ObjectIndex is only used for
	* arrays that do not have a unique name (transitions, notify rows).
	*/
	struct FEditorSelection
	{
	public:
		EEditorSelectionType	Type = EEditorSelectionType::NONE;
		EString					ObjectId;
		INT32					ObjectIndex = -1;
		INT32					SubIndex = -1;

	public:
		void Clear()
		{
			Type = EEditorSelectionType::NONE;
			ObjectId = EString();
			ObjectIndex = -1;
			SubIndex = -1;
		}
		PE_NODISCARD BOOL32 Is(const EEditorSelectionType InType)const
		{
			return ((Type == InType) ? TRUE : FALSE);
		}
		PE_NODISCARD BOOL32 IsValid()const
		{
			return ((Type != EEditorSelectionType::NONE) ? TRUE : FALSE);
		}
	};

}

#endif
