#pragma once
#include <CoreMinimal.h>

namespace PigeonEngine
{
#if _EDITOR_ONLY
    typedef struct {
        CHAR* buf;
        size_t* indices; /* byte offsets into buf */
        size_t count;    /* number of indices into buf */
    }PathSet;

    extern CHAR* GetPath(PathSet* pathSet, size_t index);
    extern void FreePathSet(PathSet* pathSet);

    extern BOOL32 OpenDialog(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath);
    extern BOOL32 OpenDialogMultiSelect(const CHAR* filterList, const CHAR* defaultPath, PathSet* outPaths);

#if _WINDOWS

    extern BOOL32 OpenDialogWindows(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath);
    extern BOOL32 OpenDialogMultiSelectWindows(const CHAR* filterList, const CHAR* defaultPath, PathSet* outPaths);
#endif

#endif
};