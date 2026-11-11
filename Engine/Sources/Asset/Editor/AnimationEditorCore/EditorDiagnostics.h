#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include "EditorSelection.h"

namespace PigeonEngine
{

	enum class EEditorDiagnosticSeverity : UINT8
	{
		DIAGNOSTIC_INFO = 0,
		DIAGNOSTIC_WARNING,
		DIAGNOSTIC_ERROR
	};

	struct FEditorDiagnostic
	{
	public:
		EEditorDiagnosticSeverity	Severity = EEditorDiagnosticSeverity::DIAGNOSTIC_INFO;
		EString						Message;
		// Stable object identity used by the inspector/graph to focus the reported object.
		EEditorSelectionType		ObjectType = EEditorSelectionType::NONE;
		EString						ObjectId;
		INT32						ObjectIndex = -1;
		INT32						SubIndex = -1;
		EString						JsonPath;
	};

	/*
	* Diagnostics are owned by a document instead of being a single error string. The panel
	* can show every problem at once and the caller decides how to focus an object.
	*/
	class FEditorDiagnostics
	{
	public:
		void Clear();
		void Add(EEditorDiagnosticSeverity InSeverity, const EString& InMessage
			, const EString& InObjectId = EString(), INT32 InObjectIndex = -1
			, INT32 InSubIndex = -1, const EString& InJsonPath = EString()
			, EEditorSelectionType InObjectType = EEditorSelectionType::NONE);
		PE_NODISCARD BOOL32 HasErrors()const;
		PE_NODISCARD BOOL32 HasWarnings()const;
		PE_NODISCARD INT32 Num()const;
		PE_NODISCARD const TArray<FEditorDiagnostic>& GetItems()const;
	private:
		TArray<FEditorDiagnostic> Items;
	};

}

#endif
