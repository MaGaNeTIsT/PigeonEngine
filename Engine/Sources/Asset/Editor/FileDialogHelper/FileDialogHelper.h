#pragma once
#include <CoreMinimal.h>

#if _EDITOR_ONLY
typedef struct {
    CHAR* buf;
    size_t* indices; /* byte offsets into buf */
    size_t count;    /* number of indices into buf */
}PathSet;

extern CHAR* GetPath(PathSet* pathSet,size_t index);
extern void FreePathSet(PathSet* pathSet);

extern BOOL OpenDialog(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath);
extern BOOL OpenDialogMultiSelect(const CHAR* filterList, const CHAR* defaultPath, PathSet* outPaths);

#if _WINDOWS

extern BOOL OpenDialogWindows(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath);

extern BOOL OpenDialogMultiSelectWindows(const CHAR* filterList, const CHAR* defaultPath, PathSet* outPaths);
#endif

#endif