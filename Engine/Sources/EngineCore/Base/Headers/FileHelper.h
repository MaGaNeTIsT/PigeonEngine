#pragma once

#include "../../Core/Headers/Main.h"

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


 /* Test Code ReadFileAsBinary */
 //const std::string FilePath = "C:/Users/Administrator/Desktop/[ファミ通書籍編集部] 白夜極光　公式アート＆シナリオブック/OEBPS/Images/embed0000.jpg";
 //void* File = nullptr;
 //unsigned long FileSize = 0;
 //if (ReadFileAsBinary(FilePath, File, FileSize))
 //{
 //	for (unsigned long i = 0; i < FileSize; ++i)
 //	{
 //		if (i % 16 != 0)
 //		{
 //			printf(" ");
 //			printf("%02x", (unsigned char)((char*)File)[i]);
 //		}
 //		else
 //		{
 //			printf("\n");
 //		}
 //	}
 //}

 /* Test Code ReadFileAsString */
//const std::string FilePath = "C:/Users/Administrator/Desktop/12312/readme.txt";
//unsigned long FileSize = 0;
//std::string File;
//if (ReadFileAsString(FilePath, File))
//{
//	std::cout << File << std::endl;
//}
