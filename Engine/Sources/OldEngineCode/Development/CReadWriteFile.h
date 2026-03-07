/*
#pragma once

#include "../../EngineCore/Core/Headers/Main.h"
#include <iostream>
#include <fstream>
#include <sstream>

class CTempFileHelper
{
public:
	const static CHAR		_NameTypeSeparator		= '.';
	const static UINT32		_PathNameSeparatorNum	= 2u;
	constexpr static CHAR	_PathNameSeparator[2u]	= { '/', '\\' };
public:
	template<BOOL32 InvOrder, BOOL32 RequireClip>
	static BOOL32 FetchPosAndString(const std::string& src, std::string& dst, const CHAR& signChar, const std::string& failedStr, std::string* clippedSrc = nullptr)
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
				if (clippedSrc != nullptr && tempPos > 0)
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
				if (clippedSrc != nullptr && (tempPos + 1) < src.length())
				{
					(*clippedSrc) = src.substr(tempPos + 1, src.length());
				}
			}
		}
		return TRUE;
	}
	template<BOOL32 InvOrder, BOOL32 RequireClip, BOOL32 RequireSignIndex>
	static BOOL32 FetchPosAndString(const std::string& src, std::string& dst, const CHAR* signChar, const UINT32& signNum, const std::string& failedStr, std::string* clippedSrc = nullptr, UINT32* extraIndex = nullptr)
	{
		if (InvOrder == TRUE)
		{
			size_t lastPos = std::string::npos;
			for (UINT32 i = 0u; i < signNum; i++)
			{
				size_t tempPos = src.find_last_of(signChar[i]);
				if (tempPos == std::string::npos)
				{
					continue;
				}
				if (lastPos == std::string::npos)
				{
					lastPos = tempPos;
					if (RequireSignIndex)
					{
						if (extraIndex != nullptr)
						{
							(*extraIndex) = i;
						}
					}
				}
				else if (tempPos > lastPos)
				{
					lastPos = tempPos;
					if (RequireSignIndex)
					{
						if (extraIndex != nullptr)
						{
							(*extraIndex) = i;
						}
					}
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
				if (clippedSrc != nullptr && lastPos > 0)
				{
					(*clippedSrc) = src.substr(0, lastPos);
				}
			}
		}
		else if (InvOrder == FALSE)
		{
			size_t firstPos = std::string::npos;
			for (UINT32 i = 0u; i < signNum; i++)
			{
				size_t tempPos = src.find_first_of(signChar[i]);
				if (tempPos == std::string::npos)
				{
					continue;
				}
				if (firstPos == std::string::npos)
				{
					firstPos = tempPos;
					if (RequireSignIndex)
					{
						if (extraIndex != nullptr)
						{
							(*extraIndex) = i;
						}
					}
				}
				else if (tempPos < firstPos)
				{
					firstPos = tempPos;
					if (RequireSignIndex)
					{
						if (extraIndex != nullptr)
						{
							(*extraIndex) = i;
						}
					}
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
				if (clippedSrc != nullptr && (firstPos + 1) < src.length())
				{
					(*clippedSrc) = src.substr(firstPos + 1, src.length());
				}
			}
		}
		return TRUE;
	}
public:
	static BOOL32 FetchStringIntoThreePartInvOrder(const std::string& src, std::string* dst, const CHAR* sign0Char, const UINT32& sign0Num, const CHAR* sign1Char, const UINT32& sign1Num, const std::string& failedStr, UINT32* extra0Index = nullptr, UINT32* extra1Index = nullptr);
	static BOOL32 FetchStringIntoNPart(const std::string& src, std::string* dst, const UINT32& numPart, const CHAR** signChar, const UINT32* signNum, const std::string& failedStr, UINT32* extraIndex = nullptr);
	static BOOL32 FetchStringIntoNPart(const std::string& src, std::string* dst, const UINT32& numPart, const CHAR& signChar, const std::string& failedStr);
public:
	/*
	* Return value : If -1, invalid path. If 0, there is no file. If > 0, file's number.
	*/
	static INT32 CountFileNumberInFolder(const std::string& path, const std::string& fileType, std::vector<std::string>& fileNames);
};

class CTempFileReaderWriterBase
{
public:
	CTempFileReaderWriterBase(const BOOL32& isBinary, const std::string& fullNamePath);
	CTempFileReaderWriterBase(const BOOL32& isBinary, const std::string& path, const std::string& nameWithType);
	CTempFileReaderWriterBase(const BOOL32& isBinary, const std::string& path, const std::string& name, const std::string& type);
public:
	BOOL32	IsSuccessSeparate()const;
	BOOL32	IsSuccessRun()const;
	BOOL32	IsFileBinary()const;
protected:
	void	SetSeparatorState(const BOOL32& state);
	void	SetRunState(const BOOL32& state);
	void	SetFileBinaryState(const BOOL32& state);
protected:
	BOOL32	m_SeparatorState;
	BOOL32	m_RunState;
	BOOL32	m_IsBinary;
public:
	const std::string&	GetFilePath()const;
	const std::string&	GetFileName()const;
	const std::string&	GetFileType()const;
	const std::string&	GetFullPath()const;
protected:
	void	SetFilePath(const std::string& str);
	void	SetFileName(const std::string& str);
	void	SetFileType(const std::string& str);
	void	SetFullPath(const std::string& str);
protected:
	std::fstream	m_FileStream;
	std::string		m_FullPath;
	std::string		m_FilePath;
	std::string		m_FileName;
	std::string		m_FileType;
public:
	virtual ~CTempFileReaderWriterBase();
};

class CTempFileReader : public CTempFileReaderWriterBase
{
public:
	CTempFileReader(const BOOL32& isBinary, const std::string& fullNamePath, std::function<BOOL32(std::fstream&, const CTempFileReader* const)> const& func);
	CTempFileReader(const BOOL32& isBinary, const std::string& path, const std::string& nameWithType, std::function<BOOL32(std::fstream&, const CTempFileReader* const)> const& func);
	CTempFileReader(const BOOL32& isBinary, const std::string& path, const std::string& name, const std::string& type, std::function<BOOL32(std::fstream&, const CTempFileReader* const)> const& func);
public:
	virtual ~CTempFileReader();
};

class CTempFileWriter : public CTempFileReaderWriterBase
{
public:
	CTempFileWriter(const BOOL32& isBinary, const std::string& fullNamePath, std::function<BOOL32(std::fstream&, const CTempFileWriter* const)> const& func);
	CTempFileWriter(const BOOL32& isBinary, const std::string& path, const std::string& nameWithType, std::function<BOOL32(std::fstream&, const CTempFileWriter* const)> const& func);
	CTempFileWriter(const BOOL32& isBinary, const std::string& path, const std::string& name, const std::string& type, std::function<BOOL32(std::fstream&, const CTempFileWriter* const)> const& func);
public:
	virtual ~CTempFileWriter();
};
*/