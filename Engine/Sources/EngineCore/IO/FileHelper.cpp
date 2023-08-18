﻿#include "FileHelper.h"
#include <CoreMinimal.h>
#include <fstream>

namespace PigeonEngine
{

	BOOL EFileHelper::ReadFileAsBinary(const EString& FilePath, void*& Return, ULONG& Size)
	{
		using namespace std;
		ifstream  fin(*FilePath, ios::in | ios::binary);
		if (!fin)
		{
#ifdef _EDITOR_ONLY
			{
				EString ErrorData("Error occured when calling EFileHelper::ReadFileAsBinary(open file path : ");
				ErrorData += FilePath;
				ErrorData += ").";
				PE_FAILED((ENGINE_IO_FILE_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		fin.seekg(0, ios_base::end);
		Size = static_cast<ULONG>(fin.tellg());
		fin.seekg(0, ios_base::beg);
		Return = new CHAR[Size];
		fin.read((CHAR*)Return, sizeof(CHAR) * Size);
		fin.close();
		return TRUE;
	}
	BOOL EFileHelper::ReadFileAsString(const EString& FilePath, EString& Return)
	{
		using namespace std;
		ifstream fin(*FilePath, ios::in);
		if (!fin)
		{
#ifdef _EDITOR_ONLY
			{
				EString ErrorData("Error occured when calling EFileHelper::ReadFileAsString(open file path : ");
				ErrorData += FilePath;
				ErrorData += ").";
				PE_FAILED((ENGINE_IO_FILE_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		istreambuf_iterator<CHAR> beg(fin), end;
		Return = string(beg, end);
		fin.close();
		return TRUE;
	}
	BOOL EFileHelper::SaveStringToFile(const EString& FilePath, const EString& Str)
	{
		using namespace std;
		ofstream out(*FilePath, ios::out);
		if (!out)
		{
#ifdef _EDITOR_ONLY
			{
				EString ErrorData("Error occured when calling EFileHelper::SaveStringToFile (open file path : ");
				ErrorData += FilePath;
				ErrorData += ").";
				PE_FAILED((ENGINE_IO_FILE_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		out << (*Str);
		out.close();
		return TRUE;
	}
	BOOL EFileHelper::SaveBytesToFile(const EString& FilePath, const void* Bytes, const ULONG& Size)
	{
		using namespace std;
		ofstream out(*FilePath, ios::out | ios::binary);
		if (!out)
		{
#ifdef _EDITOR_ONLY
			{
				EString ErrorData("Error occured when calling EFileHelper::SaveBytesToFile (open file path : ");
				ErrorData += FilePath;
				ErrorData += ").";
				PE_FAILED((ENGINE_IO_FILE_ERROR), (ErrorData));
			}
#endif
			return FALSE;
		}
		out.write(reinterpret_cast<const CHAR*>(Bytes), Size);
		out.close();
		return TRUE;
	}

};