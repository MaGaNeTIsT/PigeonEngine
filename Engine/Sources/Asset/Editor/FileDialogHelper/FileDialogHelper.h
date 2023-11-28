#pragma once
#include <CoreMinimal.h>

#if _EDITOR_ONLY
extern BOOL OpenDialog(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath);

#if _WINDOWS
extern BOOL OpenDialogWindows(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath);
#endif

#endif