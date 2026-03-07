#pragma once

#include "../../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include "String.h"


namespace PigeonEngine
{
    class EPath
    {
    public:
        /// <summary>
        /// change extension of path
        /// </summary>
        /// <param name="Path">:path need change extension.</param>
        /// <param name="Extension">:extension to change.</param>
        static EString ChangeExtension(const EString& Path, const EString& Extension);
        /// <summary>
        /// Combine two path to one.
        /// </summary>
        static EString Combine(const EString& Path1,const EString& Path2);
        /// <summary>
        /// Combine three paths to one.
        /// </summary>
        static EString Combine(const EString& Path1, const EString& Path2, const EString& Path3);
        /// <summary>
        /// Combine a array of paths to one.
        /// </summary>
        static EString Combine(const EString* Paths,INT32 Count);
        /// <summary>
        /// true if path is exists in harddisk.
        /// </summary>
        static BOOL32 Exists(const EString& Path);
        /// <summary>
        /// get directory name of path.
        /// </summary>
        static EString GetDirectoryName(const EString& Path);
        /// <summary>
        /// get extension of path.
        /// </summary>
        static EString GetExtension(const EString& Path);
        /// <summary>
        /// get file name with extension.
        /// </summary>
        static EString GetFileNameWithExtension(const EString& Path);
        /// <summary>
        /// get file name without extension.
        /// </summary>
        static EString GetFileNameWithoutExtension(const EString& Path);
        /// <summary>
        /// get absolute path,
        /// </summary>
        /// <param name="Path">:relative path of asset path</param>
        static EString GetFullAssetPath(const EString& Path);
        /// <summary>
        /// get absolute path,
        /// </summary>
        /// <param name="Path">:relative path of base path</param>
        static EString GetFullPath(const EString& Path, const EString& BasePath);
        /// <summary>
        /// get a string before first '/'.
        /// </summary>
        static EString GetRootPath(const EString& Path);
        /// <summary>
        /// get a string before last '/'.
        /// </summary>
        static EString GetFileFolderPath(const EString& Path);
        /// <summary>
        /// get relative path,
        /// </summary>
        /// <param name="Path">:absolute path</param>
        static EString GetRelativePath(const EString& Path, const EString& BasePath);
        /// <summary>
        /// check if path has extension.
        /// </summary>
        static BOOL32 HasExtension(const EString& Path);

        const static INT32 MAX_PATH_LENGTH = 65535;
    };

    extern UINT32 FindFirstSlashIndex(const EString& Path);
    extern UINT32 FindLastSlashIndex(const EString& Path);
    extern EString ToLower(const EString& String);
    extern EString ToUpper(const EString& String);
};

