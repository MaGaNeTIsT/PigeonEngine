#include "../Headers/CReadFileString.h"

CTempFileReader::CTempFileReader(const std::string& fullNamePath, std::function<BOOL(std::fstream& const, const CTempFileReader* const)> const& func)
{
	const static CHAR _FilePathBreak[2] = { '/', '\\' };
	this->m_RunState = FALSE;

	std::string failedRead(fullNamePath);
	failedRead = "[" + failedRead + "] is invalid path.";

	INT success = 1;
	{
		std::string tempStr;

		success &= CTempFileReader::FetchPosAndString<TRUE, TRUE>(fullNamePath, this->m_FileType, '.', failedRead, &tempStr);
		success &= CTempFileReader::FetchPosAndString<TRUE, TRUE>(tempStr, this->m_FileName, _FilePathBreak, 2u, failedRead, &tempStr);

		success &= (tempStr.length() > 1) ? 1 : 0;
		this->m_FilePath = (tempStr.length() > 1) ? (tempStr + "/") : failedRead;
	}

	this->m_FullPath = (success == 1) ? fullNamePath : failedRead;

	if (success == 1)
	{
		this->m_FileStream.open(fullNamePath, std::fstream::in | std::fstream::_Nocreate);
	}

	if (this->m_FileStream.is_open())
	{
		this->m_RunState = func(this->m_FileStream, this);
	}
}
CTempFileReader::CTempFileReader(const std::string& path, const std::string& nameWithType, std::function<BOOL(std::fstream& const, const CTempFileReader* const)> const& func)
{
	const static CHAR _FilePathBreak[2] = { '/', '\\' };
	this->m_RunState = FALSE;

	std::string tempFullNamePath(path + nameWithType);
	std::string failedRead(tempFullNamePath);
	failedRead = "[" + failedRead + "] is invalid path.";

	BOOL success = CTempFileReader::FetchPosAndString<TRUE, TRUE>(nameWithType, this->m_FileType, '.', failedRead, &(this->m_FileName));

	this->m_FilePath = (success == 1) ? path : failedRead;
	this->m_FullPath = (success == 1) ? (path + nameWithType) : failedRead;

	if (success == 1)
	{
		this->m_FileStream.open(this->m_FullPath, std::fstream::in | std::fstream::_Nocreate);
	}

	if (this->m_FileStream.is_open())
	{
		this->m_RunState = func(this->m_FileStream, this);
	}
}
CTempFileReader::CTempFileReader(const std::string& path, const std::string& name, const std::string& type, std::function<BOOL(std::fstream& const, const CTempFileReader* const)> const& func)
{
	this->m_RunState = FALSE;
	this->m_FilePath = path;
	this->m_FileName = name;
	this->m_FileType = type;
	this->m_FullPath = path + name + "." + type;

	this->m_FileStream.open(this->m_FullPath, std::fstream::in | std::fstream::_Nocreate);

	if (this->m_FileStream.is_open())
	{
		this->m_RunState = func(this->m_FileStream, this);
	}
}
CTempFileReader::~CTempFileReader()
{
	if (this->m_FileStream.is_open())
	{
		this->m_FileStream.close();
	}
}
const std::string& CTempFileReader::GetFilePath()const
{
	return (this->m_FilePath);
}
const std::string& CTempFileReader::GetFileName()const
{
	return (this->m_FileName);
}
const std::string& CTempFileReader::GetFileType()const
{
	return (this->m_FileType);
}
const std::string& CTempFileReader::GetFullPath()const
{
	return (this->m_FullPath);
}
BOOL CTempFileReader::IsSuccessOpen()const
{
	return (this->m_FileStream.is_open());
}
BOOL CTempFileReader::IsSuccessRun()const
{
	return (this->m_RunState);
}
template<BOOL InvOrder, BOOL RequireClip>
BOOL CTempFileReader::FetchPosAndString(const std::string& src, std::string& dst, const CHAR& signChar, const std::string& failedStr, std::string* clippedSrc)
{
	if (InvOrder == TRUE)
	{
		size_t tempPos = src.find_last_of(signChar);
		if (tempPos == std::string::npos)
		{
			dst = failedStr;
			return FALSE;
		}
		dst = src.substr(tempPos + 1, src.length());
		if (RequireClip == TRUE)
		{
			if (clippedSrc != nullptr)
			{
				(*clippedSrc) = src.substr(0, tempPos);
			}
		}
	}
	else if (InvOrder == FALSE)
	{
		size_t tempPos = src.find_first_of(signChar);
		if (tempPos == std::string::npos)
		{
			dst = failedStr;
			return FALSE;
		}
		dst = src.substr(0, tempPos);
		if (RequireClip == TRUE)
		{
			if (clippedSrc != nullptr)
			{
				(*clippedSrc) = src.substr(tempPos + 1, src.length());
			}
		}
	}
	return TRUE;
}
template<BOOL InvOrder, BOOL RequireClip>
BOOL CTempFileReader::FetchPosAndString(const std::string& src, std::string& dst, const CHAR* signChar, const UINT& signNum, const std::string& failedStr, std::string* clippedSrc = nullptr)
{
	if (InvOrder == TRUE)
	{
		size_t lastPos = std::string::npos;
		for (UINT i = 0u; i < signNum; i++)
		{
			size_t tempPos = src.find_last_of(signChar[i]);
			if (tempPos == std::string::npos)
			{
				continue;
			}
			if (lastPos == std::string::npos)
			{
				lastPos = tempPos;
			}
			else if (tempPos > lastPos)
			{
				lastPos = tempPos;
			}
		}
		if (lastPos == std::string::npos)
		{
			dst = failedStr;
			return FALSE;
		}
		dst = src.substr(lastPos + 1, src.length());
		if (RequireClip == TRUE)
		{
			if (clippedSrc != nullptr)
			{
				(*clippedSrc) = src.substr(0, lastPos);
			}
		}
	}
	else if (InvOrder == FALSE)
	{
		size_t firstPos = std::string::npos;
		for (UINT i = 0u; i < signNum; i++)
		{
			size_t tempPos = src.find_first_of(signChar[i]);
			if (tempPos == std::string::npos)
			{
				continue;
			}
			if (firstPos == std::string::npos)
			{
				firstPos = tempPos;
			}
			else if (tempPos < firstPos)
			{
				firstPos = tempPos;
			}
		}
		if (firstPos == std::string::npos)
		{
			dst = failedStr;
			return FALSE;
		}
		dst = src.substr(0, firstPos);
		if (RequireClip == TRUE)
		{
			if (clippedSrc != nullptr)
			{
				(*clippedSrc) = src.substr(firstPos + 1, src.length());
			}
		}
	}
	return TRUE;
}



CReadObjectMaterialFile::CReadObjectMaterialFile()
{

}
CReadObjectMaterialFile::~CReadObjectMaterialFile()
{

}
void CReadObjectMaterialFile::ReadFile()
{
	CTempFileReader reader("./Engine/Assets/MaterialConfigs/", [](std::fstream& const fileStream, const CTempFileReader* const tempReader)->BOOL {
		CHAR tempStr[1024];
		while (fileStream.getline(tempStr, 1024))
		{
			std::string temp
		}
		});
}