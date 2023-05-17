#pragma once

#include "../Main/Main.h"

namespace PigeonEngine
{

	class EFileHelper
	{
	public:
		static BOOL ReadFileAsBinary(const std::string& FilePath, void*& Return, ULONG& Size);
		static BOOL ReadFileAsString(const std::string& FilePath, std::string& Return);
		static BOOL SaveBytesToFile (const std::string& FilePath, const void* Bytes, const ULONG& Size);
		static BOOL SaveStringToFile(const std::string& FilePath, const std::string& Str);
	public:
		EFileHelper() = delete;
		EFileHelper(const EFileHelper&) = delete;
		~EFileHelper() {}
	public:
		EFileHelper& operator=(const EFileHelper&) = delete;
	};

};