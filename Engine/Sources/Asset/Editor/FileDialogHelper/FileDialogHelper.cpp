#include "FileDialogHelper.h"
#if _EDITOR_ONLY
using namespace PigeonEngine;
//#include <Development\Alert\DevelopmentDefines.h>
#if _WINDOWS
#include <shobjidl.h>
#define FILE_DIALOG_HELPER_MAX_STRLEN 256
#endif

CHAR* GetPath(PathSet* pathSet, size_t index)
{
    return pathSet->buf + pathSet->indices[index];
}

void FreePathSet(PathSet* pathSet)
{
    free(pathSet->indices);
    free(pathSet->buf);
}

BOOL OpenDialog(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath)
{
#if _WINDOWS
	return OpenDialogWindows(filterList, defaultPath, outPath);
#else // _WINDOWS
	return FALSE;
#endif
}
BOOL OpenDialogMultiSelect(const CHAR* filterList, const CHAR* defaultPath, PathSet* outPaths)
{
#if _WINDOWS
    return OpenDialogMultiSelectWindows(filterList, defaultPath, outPaths);
#else // _WINDOWS
    return FALSE;
#endif
}
#if _WINDOWS
static BOOL CopyCharToWChar(const CHAR* inStr, WCHAR** outStr)
{
    INT inStrByteCount = static_cast<INT>(strlen(inStr));
    INT charsNeeded = MultiByteToWideChar(CP_UTF8, 0,
        inStr, inStrByteCount,
        NULL, 0);
    if (!charsNeeded)
        return FALSE;
    assert(!*outStr);
    charsNeeded += 1; // terminator

    *outStr = (WCHAR*)malloc(charsNeeded * sizeof(WCHAR));
    if (!*outStr)
        return FALSE;

    INT ret = MultiByteToWideChar(CP_UTF8, 0,
        inStr, inStrByteCount,
        *outStr, charsNeeded);
    (*outStr)[charsNeeded - 1] = '\0';

    INT inStrCharacterCount = static_cast<INT>(strlen(inStr));
    if (ret != inStrCharacterCount)
        return FALSE;
    return TRUE;
}

static void CopyWCharToChar(const WCHAR* inStr, CHAR** outStr)
{
    int inStrCharacterCount = static_cast<int>(wcslen(inStr));
    int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0,
        inStr, inStrCharacterCount,
        NULL, 0, NULL, NULL);
    assert(bytesNeeded);
    bytesNeeded += 1;

    *outStr = (CHAR*)malloc(bytesNeeded);
    if (!*outStr)
        return;

    int bytesWritten = WideCharToMultiByte(CP_UTF8, 0,
        inStr, -1,
        *outStr, bytesNeeded,
        NULL, NULL);
    assert(bytesWritten);
}

static BOOL SetDefaultPath(IFileDialog* dialog, const CHAR* defaultPath)
{
    if (!defaultPath || strlen(defaultPath) == 0)
        return TRUE;

    WCHAR* defaultPathW = { 0 };
    CopyCharToWChar(defaultPath, &defaultPathW);

    IShellItem* folder;
    HRESULT result = SHCreateItemFromParsingName(defaultPathW, NULL, IID_PPV_ARGS(&folder));

    // Valid non results.
    if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE))
    {
        free(defaultPathW);
        return TRUE;
    }

    if (!SUCCEEDED(result))
    {
        free(defaultPathW);
        return FALSE;
    }

    // Could also call SetDefaultFolder(), but this guarantees defaultPath -- more consistency across API.
    dialog->SetFolder(folder);

    free(defaultPathW);
    folder->Release();

    return TRUE;
}

static BOOL AddFiltersToDialog(::IFileDialog* fileOpenDialog, const CHAR* filterList)
{
    const WCHAR WILDCARD[] = L"*.*";

    if (!filterList || strlen(filterList) == 0)
        return TRUE;

    // Count rows to alloc
    UINT filterCount = 1; /* guaranteed to have one filter on a correct, non-empty parse */
    const CHAR* p_filterList;
    for (p_filterList = filterList; *p_filterList; ++p_filterList)
    {
        if (*p_filterList == ';')
            ++filterCount;
    }

    assert(filterCount);
    if (!filterCount)
    {
        return FALSE;
    }

    /* filterCount plus 1 because we hardcode the *.* wildcard after the while loop */
    COMDLG_FILTERSPEC* specList = (COMDLG_FILTERSPEC*)malloc(sizeof(COMDLG_FILTERSPEC) * ((size_t)filterCount + 1));
    if (!specList)
    {
        return FALSE;
    }
    for (UINT i = 0; i < filterCount + 1; ++i)
    {
        specList[i].pszName = NULL;
        specList[i].pszSpec = NULL;
    }

    size_t specIdx = 0;
    p_filterList = filterList;
    CHAR typebuf[FILE_DIALOG_HELPER_MAX_STRLEN] = { 0 };  /* one per comma or semicolon */
    CHAR* p_typebuf = typebuf;

    CHAR specbuf[FILE_DIALOG_HELPER_MAX_STRLEN] = { 0 }; /* one per semicolon */

    while (TRUE)
    {
        if ((*p_filterList) == ',' || (*p_filterList) == ';' || (*p_filterList) == '\0')
        {
            /* append a type to the specbuf (pending filter) */

            size_t specBufLen = FILE_DIALOG_HELPER_MAX_STRLEN;

            const CHAR SEP[] = ";";
            PE_CHECK((ENGINE_ASSET_ERROR), ("Filter count must be less than 256!."), (specBufLen > strlen(typebuf) + 3));

            if (strlen(specbuf) > 0)
            {
                strncat_s(specbuf, SEP, specBufLen - strlen(specbuf) - 1);
                specBufLen += strlen(SEP);
            }

            CHAR extWildcard[FILE_DIALOG_HELPER_MAX_STRLEN];
            INT bytesWritten = sprintf_s(extWildcard, FILE_DIALOG_HELPER_MAX_STRLEN, "*.%s", typebuf);
            assert(bytesWritten == (INT)(strlen(typebuf) + 2));

            strncat_s(specbuf, extWildcard, specBufLen - strlen(specbuf) - 1);

            p_typebuf = typebuf;
            memset(typebuf, 0, sizeof(CHAR) * FILE_DIALOG_HELPER_MAX_STRLEN);
        }

        if (*p_filterList == ';' || *p_filterList == '\0')
        {
            /* end of filter -- add it to specList */
            if (!CopyCharToWChar(specbuf, (WCHAR**)&specList[specIdx].pszName))
                return FALSE;
            if (!CopyCharToWChar(specbuf, (WCHAR**)&specList[specIdx].pszSpec))
                return FALSE;

            memset(specbuf, 0, sizeof(CHAR) * FILE_DIALOG_HELPER_MAX_STRLEN);
            ++specIdx;
            if (specIdx == filterCount)
                break;
        }
        BOOL tempbool = (*p_filterList) == ',' || (*p_filterList) == ';' || (*p_filterList) == '\0';
        if (!tempbool)
        {
            *p_typebuf = *p_filterList;
            ++p_typebuf;
        }

        ++p_filterList;
    }

    /* Add wildcard */
    specList[specIdx].pszSpec = WILDCARD;
    specList[specIdx].pszName = WILDCARD;

    fileOpenDialog->SetFileTypes(filterCount + 1, specList);

    /* free speclist */
    for (size_t i = 0; i < filterCount; ++i)
    {
        if ((void*)specList[i].pszSpec)
        {
#pragma warning(once : 6001)
            free((void*)specList[i].pszSpec);
//#pragma warning(restore : 6001)
        }
    }
    if(specList)
        free(specList);

    return TRUE;
}

static size_t GetUTF8ByteCountForWChar(const WCHAR* str)
{
    size_t bytesNeeded = WideCharToMultiByte(CP_UTF8, 0,
        str, -1,
        NULL, 0, NULL, NULL);
    return bytesNeeded + 1;
}

static int CopyWCharToExistingCharBuffer(const WCHAR* inStr, CHAR* outPtr)
{
    int bytesNeeded = static_cast<int>(GetUTF8ByteCountForWChar(inStr));

    /* invocation copies null term */
    int bytesWritten = WideCharToMultiByte(CP_UTF8, 0,
        inStr, -1,
        outPtr, bytesNeeded,
        NULL, 0);

    return bytesWritten;

}

static BOOL AllocPathSet(IShellItemArray* shellItems, PathSet* pathSet)
{
    //const char ERRORMSG[] = "Error allocating pathset.";

    assert(shellItems);
    assert(pathSet);

    // How many items in shellItems?
    DWORD numShellItems;
    HRESULT result = shellItems->GetCount(&numShellItems);
    if (!SUCCEEDED(result))
    {
        //TODO:Log Error.
        return FALSE;
    }

    pathSet->count = static_cast<size_t>(numShellItems);
    assert(pathSet->count > 0);

    pathSet->indices = (size_t*)malloc(sizeof(size_t) * pathSet->count);
    if (!pathSet->indices)
    {
        return FALSE;
    }

    /* count the total bytes needed for buf */
    size_t bufSize = 0;
    for (DWORD i = 0; i < numShellItems; ++i)
    {
        ::IShellItem* shellItem;
        result = shellItems->GetItemAt(i, &shellItem);
        if (!SUCCEEDED(result))
        {
            //TODO:Log Error.
            return FALSE;
        }

        // Confirm SFGAO_FILESYSTEM is true for this shellitem, or ignore it.
        SFGAOF attribs;
        result = shellItem->GetAttributes(SFGAO_FILESYSTEM, &attribs);
        if (!SUCCEEDED(result))
        {
            //TODO:Log Error.
            return FALSE;
        }
        if (!(attribs & SFGAO_FILESYSTEM))
            continue;

        LPWSTR name;
        shellItem->GetDisplayName(SIGDN_FILESYSPATH, &name);

        // Calculate length of name with UTF-8 encoding
        bufSize += GetUTF8ByteCountForWChar(name);

        CoTaskMemFree(name);
    }

    assert(bufSize);

    pathSet->buf = (CHAR*)malloc(sizeof(CHAR) * bufSize);
    memset(pathSet->buf, 0, sizeof(CHAR) * bufSize);

    /* fill buf */
    CHAR* p_buf = pathSet->buf;
    for (DWORD i = 0; i < numShellItems; ++i)
    {
        ::IShellItem* shellItem;
        result = shellItems->GetItemAt(i, &shellItem);
        if (!SUCCEEDED(result))
        {
            //TODO:Log Error.
            return FALSE;
        }

        // Confirm SFGAO_FILESYSTEM is true for this shellitem, or ignore it.
        SFGAOF attribs;
        result = shellItem->GetAttributes(SFGAO_FILESYSTEM, &attribs);
        if (!SUCCEEDED(result))
        {
            //TODO:Log Error.
            return FALSE;
        }
        if (!(attribs & SFGAO_FILESYSTEM))
            continue;

        LPWSTR name;
        shellItem->GetDisplayName(SIGDN_FILESYSPATH, &name);

        int bytesWritten = CopyWCharToExistingCharBuffer(name, p_buf);
        CoTaskMemFree(name);

        ptrdiff_t index = p_buf - pathSet->buf;
        assert(index >= 0);
        pathSet->indices[i] = static_cast<size_t>(index);

        p_buf += bytesWritten;
    }

    return TRUE;
}

BOOL OpenDialogWindows(const CHAR* filterList, const CHAR* defaultPath, CHAR** outPath)
{
    BOOL RESULT = FALSE;
    HRESULT coResult = ::CoInitializeEx(NULL, ::COINIT_APARTMENTTHREADED | ::COINIT_DISABLE_OLE1DDE);
    if (!((coResult == RPC_E_CHANGED_MODE) || SUCCEEDED(coResult)))
    {
        return FALSE;
    }

	// Create dialog
	::IFileOpenDialog* fileOpenDialog(NULL);
	HRESULT result = ::CoCreateInstance(::CLSID_FileOpenDialog, NULL,
		CLSCTX_ALL, ::IID_IFileOpenDialog,
		reinterpret_cast<void**>(&fileOpenDialog));

	if (!SUCCEEDED(result))
	{
        //TODO:Log Error open failed;
        goto End;
	}

    if (!AddFiltersToDialog(fileOpenDialog, filterList))
    {
        goto End;
    }

    if (!SetDefaultPath(fileOpenDialog, defaultPath))
    {
        goto End;
    }

    // Show the dialog.
    result = fileOpenDialog->Show(NULL);
    if (SUCCEEDED(result))
    {
        // Get the file name
        ::IShellItem* shellItem(NULL);
        result = fileOpenDialog->GetResult(&shellItem);
        if (!SUCCEEDED(result))
        {
            goto End;
        }
        WCHAR* filePath(NULL);
        result = shellItem->GetDisplayName(::SIGDN_FILESYSPATH, &filePath);
        if (!SUCCEEDED(result))
        {
            shellItem->Release();
            goto End;
        }

        CopyWCharToChar(filePath, outPath);
        CoTaskMemFree(filePath);
        if (!*outPath)
        {
            /* error is malloc-based, error message would be redundant */
            shellItem->Release();
            goto End;
        }

        shellItem->Release();
        RESULT = TRUE;
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        //LOG cancel
    }
    else
    {
        //LOG("File dialog box show failed.");
    }

End:
    if (fileOpenDialog)
        fileOpenDialog->Release();

    if (SUCCEEDED(coResult))
        ::CoUninitialize();

    return RESULT;
}

BOOL OpenDialogMultiSelectWindows(const CHAR* filterList, const CHAR* defaultPath, PathSet* outPaths)
{
    BOOL RESULT = FALSE;

    HRESULT coResult = ::CoInitializeEx(NULL, ::COINIT_APARTMENTTHREADED | ::COINIT_DISABLE_OLE1DDE);
    if (!((coResult == RPC_E_CHANGED_MODE) || SUCCEEDED(coResult)))
    {
        return FALSE;
    }

    // Create dialog
    ::IFileOpenDialog* fileOpenDialog(NULL);
    HRESULT result = ::CoCreateInstance(::CLSID_FileOpenDialog, NULL,
        CLSCTX_ALL, ::IID_IFileOpenDialog,
        reinterpret_cast<void**>(&fileOpenDialog));

    if (!SUCCEEDED(result))
    {
        //TODO:Log Error open failed;
        goto End;
    }

    if (!AddFiltersToDialog(fileOpenDialog, filterList))
    {
        goto End;
    }

    if (!SetDefaultPath(fileOpenDialog, defaultPath))
    {
        goto End;
    }

    // Set a flag for multiple options
    DWORD dwFlags;
    result = fileOpenDialog->GetOptions(&dwFlags);
    if (!SUCCEEDED(result))
    {
        //TODO:Log error
        RESULT = FALSE;
        goto End;
    }
    result = fileOpenDialog->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
    if (!SUCCEEDED(result))
    {
        //TODO:Log error
        RESULT = FALSE;
        goto End;
    }

    // Show the dialog.
    result = fileOpenDialog->Show(NULL);
    if (SUCCEEDED(result))
    {
        IShellItemArray* shellItems;
        result = fileOpenDialog->GetResults(&shellItems);
        if (!SUCCEEDED(result))
        {
            //TODO:Log error
            RESULT = FALSE;
            goto End;
        }

        if (!AllocPathSet(shellItems, outPaths))
        {
            shellItems->Release();
            goto End;
        }

        shellItems->Release();
        RESULT = TRUE;
    }
    else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        RESULT = FALSE;
    }
    else
    {
        //TODO:Log error
        RESULT = FALSE;
    }

End:
    if (fileOpenDialog)
        fileOpenDialog->Release();

    if (SUCCEEDED(coResult))
        ::CoUninitialize();

    return RESULT;
}

#endif
#endif