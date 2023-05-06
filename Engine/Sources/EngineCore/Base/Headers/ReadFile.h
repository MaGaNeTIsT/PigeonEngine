#pragma once

#include "../../Core/Headers/Main.h"

namespace PigeonEngine
{

	class EReadFile
	{
	public:

	public:
		EReadFile() = delete;
		EReadFile(const EReadFile&) = delete;
		~EReadFile() {}
	public:
		EReadFile& operator=(const EReadFile&) = delete;

	public:
		static bool ReadFileAsBinary(const std::string FilePath, void*& Return, UINT64& Size);
		static bool ReadFileAsString(const std::string FilePath, std::string& Return);
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
