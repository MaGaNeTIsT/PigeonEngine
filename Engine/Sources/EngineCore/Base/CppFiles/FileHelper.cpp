#include "../Headers/FileHelper.h"
#include "../../Core/Headers/ErrorCaption.h"
#include <fstream>

namespace PigeonEngine
{

	BOOL EFileHelper::ReadFileAsBinary(const std::string& FilePath, void*& Return, ULONG& Size)
	{
		using namespace std;
		std::ifstream  fin(FilePath, ios::in | ios::binary);;
		if (!fin)
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				string errorData("Error occured (open file path : ");
				errorData += FilePath + ").";
				PE_FAILED(ENGINE_READ_FILE_ERROR, errorData);
			}
#endif
			return FALSE;
		}
		fin.seekg(0, ios_base::end);
		Size = fin.tellg();
		fin.seekg(0, ios_base::beg);
		Return = new CHAR[Size];
		fin.read((CHAR*)Return, sizeof(CHAR) * Size);
		fin.close();
		return TRUE;
	}
	BOOL EFileHelper::ReadFileAsString(const std::string& FilePath, std::string& Return)
	{
		using namespace std;
		ifstream fin(FilePath, ios::in);
		if (!fin)
		{
#ifdef _DEVELOPMENT_EDITOR
			{
				string errorData("Error occured (open file path : ");
				errorData += FilePath + ").";
				PE_FAILED(ENGINE_READ_FILE_ERROR, errorData);
			}
#endif
			return FALSE;
		}
		istreambuf_iterator<CHAR> beg(fin), end;
		Return = string(beg, end);
		fin.close();
		return TRUE;
	}

};