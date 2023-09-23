#include "FileHelper.h"
#include <CoreMinimal.h>
#include <fstream>
#include <filesystem>
namespace PigeonEngine
{

	BOOL8 EFileHelper::ReadFileAsBinary(const EString& FilePath, void*& Return, ULONG& Size)
	{
		std::ifstream file(*FilePath, std::ios::binary);
		// PE_CHECK(ENGINE_FILE_ERROR, "EFileHelper::ReadFileAsBinary : Load file failed", file.is_open())
		if(!file.is_open())
		{
			file.close();
			return FALSE;
		}
		// 获取文件长度
		file.seekg(0, std::ios::end);
		Size = static_cast<ULONG>(file.tellg());
		file.seekg(0, std::ios::beg);

		// 分配内存并读取文件数据
		Return = new char[Size];
		file.read(static_cast<char*>(Return), Size);

		file.close();
		return true;
	}
	
	BOOL8 EFileHelper::ReadFileAsString(const EString& FilePath, EString& Return)
	{
		std::ifstream file(*FilePath);
		if(!file.is_open())
		{
			file.close();
			return FALSE;
		}
		// 使用流操作符将文件内容读取到字符串中
		std::string line;
		Return = "";
		while (std::getline(file, line)) {
			Return += line + '\n';
		}
		file.close();
		return true;
	}
	
	BOOL8 EFileHelper::SaveStringToFile(const EString& FilePath, const EString& Str, const BOOL8& bCreateDirectory)
	{
		std::filesystem::path directoryPath = std::filesystem::path(*FilePath).parent_path();
		if (!std::filesystem::exists(directoryPath))
		{
			if(bCreateDirectory)
			{
				PE_CHECK(ENGINE_FILE_ERROR, "EFileHelper::SaveStringToFile : Create directory failed when save string to file", std::filesystem::create_directories(directoryPath))
			}
			else
			{
				return FALSE;
			}
		}
		std::ofstream out(*FilePath);
		PE_CHECK(ENGINE_FILE_ERROR, "EFileHelper::SaveStringToFile : save string to file failed", out.is_open())
		out << (*Str);
		out.close();
		return TRUE;
	}
	
	BOOL8 EFileHelper::SaveBytesToFile(const EString& FilePath, const void* Bytes, const ULONG& Size, const BOOL8& bCreateDirectory)
	{
		std::filesystem::path directoryPath = std::filesystem::path(*FilePath).parent_path();
		if (!std::filesystem::exists(directoryPath))
		{
			if(bCreateDirectory)
			{
				PE_CHECK(ENGINE_FILE_ERROR, "EFileHelper::SaveBytesToFile : Create directory failed when save string to file", std::filesystem::create_directories(directoryPath))
			}
			else
			{
				return FALSE;
			}
		}
		
		std::ofstream out(*FilePath, std::ios::binary);
		PE_CHECK(ENGINE_FILE_ERROR, "EFileHelper::SaveBytesToFile : Save bytes to file failed", out.is_open())
		out.write(static_cast<const CHAR*>(Bytes), Size);
		out.close();
		return TRUE;
	}
	
	BOOL8 EFileHelper::IsFileExists(const EString& FilePath)
	{
		return static_cast<BOOL8>(std::filesystem::exists(*FilePath));
	}

	BOOL8 EFileHelper::IsDirectoryExists(const EString& DirectoryPath)
	{
		return static_cast<BOOL8>(std::filesystem::exists(*DirectoryPath) && std::filesystem::is_directory(*DirectoryPath));
	}

	BOOL8 EFileHelper::MakeDirectory(const EString& DirectoryPath)
	{
		return static_cast<BOOL8>(std::filesystem::create_directories(*DirectoryPath));
	}

	

};