#include "../Headers/CReadWriteFile.h"
#include <filesystem>

BOOL CTempFileHelper::FetchStringIntoThreePartInvOrder(const std::string& src, std::string* dst, const CHAR* sign0Char, const UINT& sign0Num, const CHAR* sign1Char, const UINT& sign1Num, const std::string& failedStr, UINT* extra0Index, UINT* extra1Index)
{
	std::string& ptrStr0 = dst[0];
	std::string& ptrStr1 = dst[1];
	std::string* ptrStr2 = &(dst[2]);

	std::string lastStr;

	if (extra0Index == nullptr)
	{
		if (!CTempFileHelper::FetchPosAndString<TRUE, TRUE, FALSE>(src, ptrStr0, sign0Char, sign0Num, failedStr, &lastStr))
		{
			//TODO log.
			return FALSE;
		}
	}
	else
	{
		if (!CTempFileHelper::FetchPosAndString<TRUE, TRUE, TRUE>(src, ptrStr0, sign0Char, sign0Num, failedStr, &lastStr, extra0Index))
		{
			//TODO log.
			return FALSE;
		}
	}
	if (extra1Index == nullptr)
	{
		if (!CTempFileHelper::FetchPosAndString<TRUE, TRUE, FALSE>(lastStr, ptrStr1, sign1Char, sign1Num, failedStr, ptrStr2))
		{
			//TODO log.
			return FALSE;
		}
	}
	else
	{
		if (!CTempFileHelper::FetchPosAndString<TRUE, TRUE, TRUE>(lastStr, ptrStr1, sign1Char, sign1Num, failedStr, ptrStr2, extra1Index))
		{
			//TODO log.
			return FALSE;
		}
	}

	return TRUE;
}
BOOL CTempFileHelper::FetchStringIntoNPart(const std::string& src, std::string* dst, const UINT& numPart, const CHAR** signChar, const UINT* signNum, const std::string& failedStr, UINT* extraIndex)
{
	UINT num_m_1 = numPart - 1;
	std::string lastStr = src;
	for (UINT i = 0u; i < num_m_1; i++)
	{
		const CHAR*& tempSign = signChar[i];
		const UINT& tempSignNum = signNum[i];

		if (extraIndex != nullptr)
		{
			if (!CTempFileHelper::FetchPosAndString<FALSE, TRUE, TRUE>(lastStr, dst[i], tempSign, tempSignNum, failedStr, &lastStr, &(extraIndex[i])))
			{
				//TODO log.
				return FALSE;
			}
		}
		else
		{
			if (!CTempFileHelper::FetchPosAndString<FALSE, TRUE, FALSE>(lastStr, dst[i], tempSign, tempSignNum, failedStr, &lastStr))
			{
				//TODO log.
				return FALSE;
			}
		}
	}
	dst[num_m_1] = lastStr;
	return TRUE;
}
BOOL CTempFileHelper::FetchStringIntoNPart(const std::string& src, std::string* dst, const UINT& numPart, const CHAR& signChar, const std::string& failedStr)
{
	UINT num_m_1 = numPart - 1;
	std::string lastStr = src;
	for (UINT i = 0u; i < num_m_1; i++)
	{
		if (!CTempFileHelper::FetchPosAndString<FALSE, TRUE, FALSE>(lastStr, dst[i], &signChar, 1u, failedStr, &lastStr))
		{
			//TODO log.
			return FALSE;
		}
	}
	dst[num_m_1] = lastStr;
	return TRUE;
}
INT CTempFileHelper::CountFileNumberInFolder(const std::string& path, const std::string& fileType, std::vector<std::string>& fileNames)
{
	if (fileNames.size() != 0)
	{
		fileNames.clear();
	}

	{
		std::filesystem::path pathRoot(path);
		if (!std::filesystem::exists(pathRoot))
		{
			return -1;
		}
		BOOL bRegularFile = std::filesystem::is_regular_file(pathRoot);
		if (!std::filesystem::is_directory(pathRoot) && !bRegularFile)
		{
			return -1;
		}
		if (bRegularFile)
		{
			std::string result[3];
			if (CTempFileHelper::FetchStringIntoThreePartInvOrder(path, result, &(CTempFileHelper::_NameTypeSeparator), 1u, CTempFileHelper::_PathNameSeparator, CTempFileHelper::_PathNameSeparatorNum, "Error"))
			{
				if (result[0] == fileType)
				{
					fileNames.push_back(result[2]);
					return 1;
				}
			}
			return -1;
		}
	}

	INT fileNum = 0;
	std::filesystem::directory_iterator folderIterator(path);
	for (auto& folderItem : folderIterator)
	{
		std::string fileName(folderItem.path().filename().string());

		fileNames.push_back();
		fileNum++;
	}
	return fileNum;
}



CTempFileReaderWriterBase::CTempFileReaderWriterBase(const BOOL& isBinary, const std::string& fullNamePath)
{
	this->m_SeparatorState = FALSE;
	this->m_RunState = FALSE;
	this->m_IsBinary = isBinary;

	std::string failedRead(fullNamePath);
	failedRead = "[" + failedRead + "] is invalid path.";

	INT success = 1;
	{
		std::string tempStr;
		UINT pathNameSeparatorIndex = 0u;

		success &= CTempFileHelper::FetchPosAndString<TRUE, TRUE>(fullNamePath, this->m_FileType, CTempFileHelper::_NameTypeSeparator, failedRead, &tempStr);
		success &= CTempFileHelper::FetchPosAndString<TRUE, TRUE, TRUE>(tempStr, this->m_FileName, CTempFileHelper::_PathNameSeparator, CTempFileHelper::_PathNameSeparatorNum, failedRead, &tempStr, &pathNameSeparatorIndex);

		success &= (tempStr.length() > 1) ? 1 : 0;
		this->m_FilePath = (tempStr.length() > 1) ? (tempStr + CTempFileHelper::_PathNameSeparator[pathNameSeparatorIndex]) : failedRead;
	}

	this->m_FullPath = (success == 1) ? fullNamePath : failedRead;

	if (success == 1)
	{
		this->m_SeparatorState = TRUE;
	}
}
CTempFileReaderWriterBase::CTempFileReaderWriterBase(const BOOL& isBinary, const std::string& path, const std::string& nameWithType)
{
	this->m_SeparatorState = FALSE;
	this->m_RunState = FALSE;
	this->m_IsBinary = isBinary;

	std::string tempFullNamePath(path + nameWithType);
	std::string failedRead(tempFullNamePath);
	failedRead = "[" + failedRead + "] is invalid path.";

	BOOL success = CTempFileHelper::FetchPosAndString<TRUE, TRUE>(nameWithType, this->m_FileType, CTempFileHelper::_NameTypeSeparator, failedRead, &(this->m_FileName));

	this->m_FilePath = (success == 1) ? path : failedRead;
	this->m_FullPath = (success == 1) ? (path + nameWithType) : failedRead;

	if (success == 1)
	{
		this->m_SeparatorState = TRUE;
	}
}
CTempFileReaderWriterBase::CTempFileReaderWriterBase(const BOOL& isBinary, const std::string& path, const std::string& name, const std::string& type)
{
	this->m_SeparatorState = TRUE;
	this->m_RunState = FALSE;
	this->m_IsBinary = isBinary;

	this->m_FilePath = path;
	this->m_FileName = name;
	this->m_FileType = type;
	this->m_FullPath = path + name + CTempFileHelper::_NameTypeSeparator + type;
}
CTempFileReaderWriterBase::~CTempFileReaderWriterBase()
{
	if (this->m_FileStream.is_open())
	{
		this->m_FileStream.close();
	}
}
BOOL CTempFileReaderWriterBase::IsSuccessSeparate()const
{
	return (this->m_SeparatorState);
}
BOOL CTempFileReaderWriterBase::IsSuccessRun()const
{
	return (this->m_RunState);
}
BOOL CTempFileReaderWriterBase::IsFileBinary()const
{
	return (this->m_IsBinary);
}
void CTempFileReaderWriterBase::SetSeparatorState(const BOOL& state)
{
	this->m_SeparatorState = state;
}
void CTempFileReaderWriterBase::SetRunState(const BOOL& state)
{
	this->m_RunState = state;
}
void CTempFileReaderWriterBase::SetFileBinaryState(const BOOL& state)
{
	this->m_IsBinary = state;
}
const std::string& CTempFileReaderWriterBase::GetFilePath()const
{
	return (this->m_FilePath);
}
const std::string& CTempFileReaderWriterBase::GetFileName()const
{
	return (this->m_FileName);
}
const std::string& CTempFileReaderWriterBase::GetFileType()const
{
	return (this->m_FileType);
}
const std::string& CTempFileReaderWriterBase::GetFullPath()const
{
	return (this->m_FullPath);
}
void CTempFileReaderWriterBase::SetFilePath(const std::string& str)
{
	this->m_FilePath = str;
}
void CTempFileReaderWriterBase::SetFileName(const std::string& str)
{
	this->m_FileName = str;
}
void CTempFileReaderWriterBase::SetFileType(const std::string& str)
{
	this->m_FileType = str;
}
void CTempFileReaderWriterBase::SetFullPath(const std::string& str)
{
	this->m_FullPath = str;
}



CTempFileReader::CTempFileReader(const BOOL& isBinary, const std::string& fullNamePath, std::function<BOOL(std::fstream&, const CTempFileReader* const)> const& func) : CTempFileReaderWriterBase(isBinary, fullNamePath)
{
	if (this->IsSuccessSeparate())
	{
		this->m_FileStream.open(fullNamePath, (std::fstream::in | std::fstream::_Nocreate) | (isBinary ? std::fstream::binary : 0x00));
	}
	if (this->m_FileStream.is_open())
	{
		this->SetRunState(func(this->m_FileStream, this));
	}
}
CTempFileReader::CTempFileReader(const BOOL& isBinary, const std::string& path, const std::string& nameWithType, std::function<BOOL(std::fstream&, const CTempFileReader* const)> const& func) : CTempFileReaderWriterBase(isBinary, path, nameWithType)
{
	if (this->IsSuccessSeparate())
	{
		this->m_FileStream.open(this->m_FullPath, (std::fstream::in | std::fstream::_Nocreate) | (isBinary ? std::fstream::binary : 0x00));
	}
	if (this->m_FileStream.is_open())
	{
		this->SetRunState(func(this->m_FileStream, this));
	}
}
CTempFileReader::CTempFileReader(const BOOL& isBinary, const std::string& path, const std::string& name, const std::string& type, std::function<BOOL(std::fstream&, const CTempFileReader* const)> const& func) : CTempFileReaderWriterBase(isBinary, path, name, type)
{
	if (this->IsSuccessSeparate())
	{
		this->m_FileStream.open(this->m_FullPath, (std::fstream::in | std::fstream::_Nocreate) | (isBinary ? std::fstream::binary : 0x00));
	}
	if (this->m_FileStream.is_open())
	{
		this->SetRunState(func(this->m_FileStream, this));
	}
}
CTempFileReader::~CTempFileReader()
{

}



CTempFileWriter::CTempFileWriter(const BOOL& isBinary, const std::string& fullNamePath, std::function<BOOL(std::fstream&, const CTempFileWriter* const)> const& func) : CTempFileReaderWriterBase(isBinary, fullNamePath)
{
	if (this->IsSuccessSeparate())
	{
		this->m_FileStream.open(fullNamePath, std::fstream::out | std::fstream::trunc | (isBinary ? std::fstream::binary : 0x00));
	}
	if (this->m_FileStream.is_open())
	{
		this->SetRunState(func(this->m_FileStream, this));
	}
}
CTempFileWriter::CTempFileWriter(const BOOL& isBinary, const std::string& path, const std::string& nameWithType, std::function<BOOL(std::fstream&, const CTempFileWriter* const)> const& func) : CTempFileReaderWriterBase(isBinary, path, nameWithType)
{
	if (this->IsSuccessSeparate())
	{
		this->m_FileStream.open(this->m_FullPath, std::fstream::out | std::fstream::trunc | (isBinary ? std::fstream::binary : 0x00));
	}
	if (this->m_FileStream.is_open())
	{
		this->SetRunState(func(this->m_FileStream, this));
	}
}
CTempFileWriter::CTempFileWriter(const BOOL& isBinary, const std::string& path, const std::string& name, const std::string& type, std::function<BOOL(std::fstream&, const CTempFileWriter* const)> const& func) : CTempFileReaderWriterBase(isBinary, path, name, type)
{
	if (this->IsSuccessSeparate())
	{
		this->m_FileStream.open(this->m_FullPath, std::fstream::out | std::fstream::trunc | (isBinary ? std::fstream::binary : 0x00));
	}
	if (this->m_FileStream.is_open())
	{
		this->SetRunState(func(this->m_FileStream, this));
	}
}
CTempFileWriter::~CTempFileWriter()
{

}