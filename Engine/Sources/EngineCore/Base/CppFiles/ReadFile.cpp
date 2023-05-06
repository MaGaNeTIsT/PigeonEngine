#include "../Headers/ReadFile.h"
#include <fstream>

bool PigeonEngine::EReadFile::ReadFileAsBinary(const std::string FilePath, void*& Return, UINT64& Size)
{
	using namespace std;
	std::ifstream  fin(FilePath, ios::in | ios::binary);;
	if (!fin)
	{
		// cout << "Error Occured" << endl;
		return false;
	}
	fin.seekg(0, ios_base::end);
	Size = fin.tellg();
	fin.seekg(0, ios_base::beg);
	Return = new char[Size];
	fin.read((char*)Return, sizeof(char) * Size);
	fin.close();
	return true;
}

bool PigeonEngine::EReadFile::ReadFileAsString(const std::string FilePath, std::string& Return)
{
	using namespace std;
	ifstream fin(FilePath, ios::in);
	if (!fin)
	{
		// cout << "Error Occured" << endl;
		return false;
	}
	istreambuf_iterator<char> beg(fin), end;
	Return = string(beg, end);
	fin.close();
	return true;
}
