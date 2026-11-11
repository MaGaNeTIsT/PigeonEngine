#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{
	class EFileHelper
	{
	public:
		static BOOL8 ReadFileAsBinary(const EString& FilePath, void*& Return, ULONG& Size);
		static BOOL8 ReadFileAsString(const EString& FilePath, EString& Return);
		static BOOL8 SaveBytesToFile (const EString& FilePath, const void* Bytes, const ULONG& Size, const BOOL8& bCreateDirectory = TRUE);
		/*
		* Writes to a sibling .tmp file first and only replaces FilePath after the write
		* completed. A failed save therefore leaves the previous file intact instead of
		* truncating it, which is what an editor save has to guarantee.
		*/
		static BOOL8 SaveBytesToFileAtomic(const EString& FilePath, const void* Bytes, const ULONG& Size, const BOOL8& bCreateDirectory = TRUE);
		static BOOL8 SaveStringToFile(const EString& FilePath, const EString& Str, const BOOL8& bCreateDirectory = TRUE, const BOOL8& bAppend = FALSE);

		static BOOL8 IsFileExists(const EString& FilePath);
		static BOOL8 IsDirectoryExists(const EString& DirectoryPath);

		static BOOL8 MakeDirectory(const EString& DirectoryPath);

		static BOOL8 ScanDirectory(const EString& DirectoryPath, TArray<EString>& OutFolders, TArray<EString>& OutFiles, const BOOL8& OutWithFullPath);

		static BOOL8 ReadFirstNumberOfBytesInFile(const EString& FilePath, void*& Return, const ULONG& Size);
		static BOOL8 CopyFileToNewPath(const EString& FilePath, const EString& NewFilePath);
		// Editor asset management: move/delete a single file without touching other files.
		static BOOL8 MoveFile(const EString& FilePath, const EString& NewFilePath);
		static BOOL8 DeleteFile(const EString& FilePath);
	public:
		EFileHelper() = delete;
		EFileHelper(const EFileHelper&) = delete;
		~EFileHelper() = default;
	public:
		EFileHelper& operator=(const EFileHelper&) = delete;
	};

};
