#pragma once

#include "../../../../Entry/EngineMain.h"
#include <iostream>
#include <fstream>

class CTempFileReader
{
public:
	CTempFileReader(const std::string& fullNamePath, std::function<BOOL(std::fstream& const, const CTempFileReader* const)> const& func);
	CTempFileReader(const std::string& path, const std::string& nameWithType, std::function<BOOL(std::fstream& const, const CTempFileReader* const)> const& func);
	CTempFileReader(const std::string& path, const std::string& name, const std::string& type, std::function<BOOL(std::fstream& const, const CTempFileReader* const)> const& func);
public:
	const std::string&	GetFilePath()const;
	const std::string&	GetFileName()const;
	const std::string&	GetFileType()const;
	const std::string&	GetFullPath()const;
	BOOL	IsSuccessOpen()const;
	BOOL	IsSuccessRun()const;
private:
	BOOL			m_RunState;
	std::fstream	m_FileStream;
	std::string		m_FullPath;
	std::string		m_FilePath;
	std::string		m_FileName;
	std::string		m_FileType;
public:
	template<BOOL InvOrder, BOOL RequireClip>
	static BOOL	FetchPosAndString(const std::string& src, std::string& dst, const CHAR& signChar, const std::string& failedStr, std::string* clippedSrc = nullptr);
	template<BOOL InvOrder, BOOL RequireClip>
	static BOOL	FetchPosAndString(const std::string& src, std::string& dst, const CHAR* signChar, const UINT& signNum, const std::string& failedStr, std::string* clippedSrc = nullptr);
public:
	~CTempFileReader();
};

class CReadObjectMaterialFile
{
public:
	virtual void ReadFile();
protected:
	std::string		m_NormalPath;
	std::string		m_AlbedoPath;
	std::string		m_EmissivePath;
	std::string		m_RoughnessPath;
	std::string		m_AmbientOcclusionPath;
	std::string		m_ReflectancePath;
	std::string		m_MetallicnessPath;
	std::string		m_ClothSheenPath;
	std::string		m_ClothSubsurfacePath;
	std::string		m_AnisotropyStrengthPath;
	std::string		m_AnisotropyDirectionPath;
	std::string		m_ClearCoatStrengthPath;
	std::string		m_ClearCoatRoughnessPath;
	std::string		m_ClearCoatNormalPath;
public:
	CReadObjectMaterialFile();
	virtual ~CReadObjectMaterialFile();
};