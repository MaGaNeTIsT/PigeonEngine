#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{

	class EFileHelper
	{
	public:
		static BOOL ReadFileAsBinary(const EString& FilePath, void*& Return, ULONG& Size);
		static BOOL ReadFileAsString(const EString& FilePath, EString& Return);
		static BOOL SaveBytesToFile (const EString& FilePath, const void* Bytes, const ULONG& Size);
		static BOOL SaveStringToFile(const EString& FilePath, const EString& Str);
	public:
		EFileHelper() = delete;
		EFileHelper(const EFileHelper&) = delete;
		~EFileHelper() {}
	public:
		EFileHelper& operator=(const EFileHelper&) = delete;
	};

};