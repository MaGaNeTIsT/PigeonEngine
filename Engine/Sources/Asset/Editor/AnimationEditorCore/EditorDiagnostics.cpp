#include "EditorDiagnostics.h"

#if _EDITOR_ONLY

namespace PigeonEngine
{

	void FEditorDiagnostics::Clear()
	{
		Items.Empty();
	}

	void FEditorDiagnostics::Add(EEditorDiagnosticSeverity InSeverity, const EString& InMessage
		, const EString& InObjectId, INT32 InObjectIndex, INT32 InSubIndex, const EString& InJsonPath
		, EEditorSelectionType InObjectType)
	{
		FEditorDiagnostic Diagnostic;
		Diagnostic.Severity = InSeverity;
		Diagnostic.Message = InMessage;
		Diagnostic.ObjectType = InObjectType;
		Diagnostic.ObjectId = InObjectId;
		Diagnostic.ObjectIndex = InObjectIndex;
		Diagnostic.SubIndex = InSubIndex;
		Diagnostic.JsonPath = InJsonPath;
		Items.Add(Diagnostic);
	}

	BOOL32 FEditorDiagnostics::HasErrors()const
	{
		for (INT32 Index = 0; Index < Items.Num(); Index++)
		{
			if (Items[Index].Severity == EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	BOOL32 FEditorDiagnostics::HasWarnings()const
	{
		for (INT32 Index = 0; Index < Items.Num(); Index++)
		{
			if (Items[Index].Severity == EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	INT32 FEditorDiagnostics::Num()const
	{
		return Items.Num();
	}

	const TArray<FEditorDiagnostic>& FEditorDiagnostics::GetItems()const
	{
		return Items;
	}

}

#endif
