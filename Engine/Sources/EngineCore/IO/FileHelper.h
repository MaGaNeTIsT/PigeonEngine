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
		static BOOL8 SaveStringToFile(const EString& FilePath, const EString& Str, const BOOL8& bCreateDirectory = TRUE);

		static BOOL8 IsFileExists(const EString& FilePath);
		static BOOL8 IsDirectoryExists(const EString& DirectoryPath);

		static BOOL8 MakeDirectory(const EString& DirectoryPath);

		static BOOL8 ScanDirectory(const EString& DirectoryPath, TArray<EString>& OutFolders, TArray<EString>& OutFiles, const BOOL8& OutWithFullPath);

		static BOOL8 ReadFirstNumberOfBytesInFile(const EString& FilePath, void*& Return, ULONG& Size);
	public:
		EFileHelper() = delete;
		EFileHelper(const EFileHelper&) = delete;
		~EFileHelper() = default;
	public:
		EFileHelper& operator=(const EFileHelper&) = delete;
	};

};