/*
#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineGame/Headers/CObjectManager.h"
#include "./CRenderStructCommon.h"

#include "../../../Development/Headers/CReadWriteFile.h"

class CMaterialBase : public CObjectBase
{
public:
	enum MaterialType
	{
		MATERIAL_TYPE_NONE				= 0,
		MATERIAL_TYPE_OPAQUE_DEFERRED	= 1,
		MATERIAL_TYPE_OPAQUE_FORWARD	= 2,
		MATERIAL_TYPE_TRANSPARENT		= 3
	};
public:
	virtual void	Init() = 0;
	virtual void	Bind()const = 0;
#ifdef _DEVELOPMENT_EDITOR
	virtual void	SelectedEditorUpdate() {}
#endif
public:
	virtual const void*			GetConstantData()const = 0;
	const class CVertexShader*	GetVertexShader()const;
	const class CPixelShader*	GetPixelShader()const;
	const MaterialType&			GetMaterialType()const;
	const UINT32&					GetConstantSize()const;
	const std::string&			GetName()const;
	const std::string&			GetVertexShaderName()const;
	const std::string&			GetPixelShaderName()const;
public:
	BOOL32	HasConstantData()const;
	BOOL32	HasInputLayout()const;
	BOOL32	HasVertexShader()const;
	BOOL32	HasPixelShader()const;
protected:
	std::string											m_Name;
	UINT32												m_ConstantSize;
	MaterialType										m_MaterialType;
	std::vector<CustomStruct::CRenderInputLayoutDesc>	m_InputLayout;
	std::string											m_VertexShaderName;
	class CVertexShader*								m_VertexShader;
	std::string											m_PixelShaderName;
	class CPixelShader*									m_PixelShader;
public:
	CMaterialBase(const std::string& name, MaterialType materialType, const UINT32& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT32& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName);
	virtual ~CMaterialBase();
};

class CReadWriteMaterialParamsFile
{
public:
	enum FileMaterialTextureParamsType
	{
		FMTPT_NONE = 0,
		FMTPT_NORMAL,
		FMTPT_ALBEDO,
		FMTPT_EMISSIVE,
		FMTPT_ROUGHNESS,
		FMTPT_AMBIENTOCCLUSION,
		FMTPT_REFLECTANCE,
		FMTPT_METALLICNESS,
		FMTPT_CLOTHSHEEN,
		FMTPT_CLOTHSUBSURFACE,
		FMTPT_ANISOTROPYSTRENGTH,
		FMTPT_ANISOTROPYDIRECTION,
		FMTPT_CLEARCOATSTRENGTH,
		FMTPT_CLEARCOATROUGHNESS,
		FMTPT_CLEARCOATNORMAL,
		FMTPT_COUNT
	};
	enum FileMaterialPropertyParamsType
	{
		FMPPT_NONE = 0,
		FMPPT_BASECOLOR,
		FMPPT_EMISSIVE,
		FMPPT_ROUGHNESS,
		FMPPT_AMBIENTOCCLUSION,
		FMPPT_REFLECTANCE,
		FMPPT_METALLICNESS,
		FMPPT_ISGLOSSY,
		FMPPT_CLOTHSHEEN,
		FMPPT_CLOTHSUBSURFACE,
		FMPPT_ANISOTROPYSTRENGTH,
		FMPPT_ANISOTROPYDIRECTION,
		FMPPT_CLEARCOATSTRENGTH,
		FMPPT_CLEARCOATROUGHNESS,
		FMPPT_COUNT
	};
public:
	void	SaveMaterialParams(const std::string& path, const std::string& name, const class CDefaultLitMaterial* mat);
	void	SaveMaterialParams(const std::string& path, const std::string& name, const class CDefaultLitSkeletonMeshMaterial* mat);
	void	SaveMaterialParams(const std::string& path, const std::string& name, const class CAnisotropicMaterial* mat);
	void	SaveMaterialParams(const std::string& path, const std::string& name, const class CClothMaterial* mat);
	void	SaveMaterialParams(const std::string& path, const std::string& name, const class CClothAnisotropicMaterial* mat);
	void	LoadMaterialParams(const std::string& path, const std::string& name, class CDefaultLitMaterial* mat);
	void	LoadMaterialParams(const std::string& path, const std::string& name, class CDefaultLitSkeletonMeshMaterial* mat);
	void	LoadMaterialParams(const std::string& path, const std::string& name, class CAnisotropicMaterial* mat);
	void	LoadMaterialParams(const std::string& path, const std::string& name, class CClothMaterial* mat);
	void	LoadMaterialParams(const std::string& path, const std::string& name, class CClothAnisotropicMaterial* mat);
public:
	void	SetTexturePath(FileMaterialTextureParamsType type, const std::string& str);
	BOOL32	GetTexturePath(FileMaterialTextureParamsType type, std::string& output)const;
public:
	void	SetPropertyPath(FileMaterialPropertyParamsType type, const std::string& str);
	BOOL32	GetPropertyPath(FileMaterialPropertyParamsType type, std::string& output)const;
public:
	template<typename Type>
	BOOL32 GetPropertyValue(FileMaterialPropertyParamsType type, Type* output, const UINT32& num, const CHAR& separator = CReadWriteMaterialParamsFile::_PropertyValueSeparator)const
	{
		const static std::string _StaticNotExistString = ENGINE_NOT_EXIST_STRING;
		if (type <= FileMaterialPropertyParamsType::FMPPT_NONE || type >= FileMaterialPropertyParamsType::FMPPT_COUNT || num < 1u || this->m_PropertyPath[type] == _StaticNotExistString)
		{
			return FALSE;
		}
		std::string* tempDst = new std::string[num];
		if (!CTempFileHelper::FetchStringIntoNPart(this->m_PropertyPath[type], tempDst, num, separator, "Error"))
		{
			delete[]tempDst;
			return FALSE;
		}
		for (UINT32 i = 0u; i < num; i++)
		{
			Type& refOutput = output[i];
			std::istringstream tempStream(tempDst[i]);
			tempStream >> refOutput;
		}
		delete[]tempDst;
		return TRUE;
	}
	template<typename Type>
	void SetPropertyValue(FileMaterialPropertyParamsType type, const Type* input, const UINT32& num, const CHAR& separator = CReadWriteMaterialParamsFile::_PropertyValueSeparator)
	{
		if (type <= FileMaterialPropertyParamsType::FMPPT_NONE || type >= FileMaterialPropertyParamsType::FMPPT_COUNT || num < 1u)
		{
			return;
		}
		std::string tempStr; UINT32 lastNum = num - 1u;
		for (UINT32 i = 0u; i < num; i++)
		{
			const Type& refOutput = input[i];
			std::ostringstream tempStream;
			tempStream << refOutput;
			tempStr += tempStream.str();
			if (i != lastNum)
			{
				tempStr += separator;
			}
		}
		this->m_PropertyPath[type] = tempStr;
	}
public:
	virtual void	ReadFile(const std::string& fullPath);
	virtual void	WriteFile(const std::string& fullPath);
protected:
	void	InitAllPath();
protected:
	const static CHAR	_PropertySeparator			= '=';
	const static CHAR	_PropertyValueSeparator		= ',';
	const static UINT32	_PropertyStringLengthMax	= 256u;
	const static UINT32	_PathStringLengthMax		= 1024u;
protected:
	std::string		m_TexturePath[FileMaterialTextureParamsType::FMTPT_COUNT];
	std::string		m_PropertyPath[FileMaterialPropertyParamsType::FMPPT_COUNT];
public:
	CReadWriteMaterialParamsFile();
	virtual ~CReadWriteMaterialParamsFile();
};
*/