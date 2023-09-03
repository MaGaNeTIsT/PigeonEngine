#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{

	class EFileHelper
	{
	public:
		static BOOL32 ReadFileAsBinary(const EString& FilePath, void*& Return, ULONG& Size);
		static BOOL32 ReadFileAsString(const EString& FilePath, EString& Return);
		static BOOL32 SaveBytesToFile (const EString& FilePath, const void* Bytes, const ULONG& Size);
		static BOOL32 SaveStringToFile(const EString& FilePath, const EString& Str);
	public:
		EFileHelper() = delete;
		EFileHelper(const EFileHelper&) = delete;
		~EFileHelper() {}
	public:
		EFileHelper& operator=(const EFileHelper&) = delete;
	};

};