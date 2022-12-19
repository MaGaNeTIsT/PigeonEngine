#include "../Headers/CMaterialBase.h"
#include "../../AssetsManager/Headers/CShader.h"

CMaterialBase::CMaterialBase(const std::string& name, MaterialType materialType, const UINT& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName)
{
	this->m_Name				= name;
	this->m_MaterialType		= materialType;
	this->m_ConstantSize		= constantSize;
	if (inputLayoutNum > 0)
	{
		this->m_InputLayout.resize(inputLayoutNum);
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			this->m_InputLayout[i] = inputLayout[i];
		}
	}
	this->m_VertexShaderName	= vertexShaderName;
	this->m_PixelShaderName		= pixelShaderName;
	this->m_VertexShader		= NULL;
	this->m_PixelShader			= NULL;
}
CMaterialBase::~CMaterialBase()
{
	if (this->m_VertexShader != NULL)
	{
		delete (this->m_VertexShader);
		this->m_VertexShader = NULL;
	}
	if (this->m_PixelShader != NULL)
	{
		delete (this->m_PixelShader);
		this->m_PixelShader = NULL;
	}
}
const CVertexShader* CMaterialBase::GetVertexShader()const
{
	return (this->m_VertexShader);
}
const CPixelShader* CMaterialBase::GetPixelShader()const
{
	return (this->m_PixelShader);
}
const CMaterialBase::MaterialType& CMaterialBase::GetMaterialType()const
{
	return (this->m_MaterialType);
}
const UINT& CMaterialBase::GetConstantSize()const
{
	return (this->m_ConstantSize);
}
const std::string& CMaterialBase::GetName()const
{
	return (this->m_Name);
}
const std::string& CMaterialBase::GetVertexShaderName()const
{
	return (this->m_VertexShaderName);
}
const std::string& CMaterialBase::GetPixelShaderName()const
{
	return (this->m_PixelShaderName);
}
BOOL CMaterialBase::HasConstantData()const
{
	return (this->m_ConstantSize > 0u && this->GetConstantData() != nullptr);
}
BOOL CMaterialBase::HasInputLayout()const
{
	return (this->m_InputLayout.size() > 0);
}
BOOL CMaterialBase::HasVertexShader()const
{
	return (this->m_VertexShader != NULL);
}
BOOL CMaterialBase::HasPixelShader()const
{
	return (this->m_ConstantSize != NULL);
}



const static std::string _GStaticTextureName[CReadMaterialParamsFile::FileMaterialTextureParamsType::FMTPT_COUNT] = {
	"None",
	"NormalTexture",
	"AlbedoTexture",
	"EmissiveTexture",
	"RoughnessTexture",
	"AmbientOcclusionTexture",
	"ReflectanceTexture",
	"MetallicnessTexture",
	"ClothSheenTexture",
	"ClothSubsurfaceTexture",
	"AnisotropyStrengthTexture",
	"AnisotropyDirectionTexture",
	"ClearCoatStrengthTexture",
	"ClearCoatRoughnessTexture",
	"ClearCoatNormalTexture" };
const static std::string _GStaticPropertyName[CReadMaterialParamsFile::FileMaterialPropertyParamsType::FMPPT_COUNT] = {
	"None",
	"BaseColorProperty",
	"EmissiveColorProperty",
	"RoughnessProperty",
	"AmbientOcclusionProperty",
	"ReflectanceProperty",
	"MetallicnessProperty",
	"IsGlossyProperty",
	"SheenColorProperty",
	"SubsurfaceColorProperty",
	"AnisotropyStrengthProperty",
	"AnisotropyDirectionProperty",
	"ClearCoatStrengthProperty",
	"ClearCoatRoughnessProperty" };
const static std::string _GStaticNotExistString = ENGINE_NOT_EXIST_STRING;
constexpr UINT CReadMaterialParamsFile::_PropertyStringLengthMax;
constexpr UINT CReadMaterialParamsFile::_PathStringLengthMax;
CReadMaterialParamsFile::CReadMaterialParamsFile()
{
	this->m_TexturePath[FileMaterialTextureParamsType::FMTPT_NONE] = _GStaticNotExistString;
	this->m_PropertyPath[FileMaterialPropertyParamsType::FMPPT_NONE] = _GStaticNotExistString;
	this->InitAllPath();
}
CReadMaterialParamsFile::~CReadMaterialParamsFile()
{

}
void CReadMaterialParamsFile::SetTexturePath(FileMaterialTextureParamsType type, const std::string& str)
{
	if (type <= FileMaterialTextureParamsType::FMTPT_NONE || type >= FileMaterialTextureParamsType::FMTPT_COUNT)
	{
		return;
	}
	this->m_TexturePath[type] = str;
}
BOOL CReadMaterialParamsFile::GetTexturePath(FileMaterialTextureParamsType type, std::string& output)const
{
	if (type <= FileMaterialTextureParamsType::FMTPT_NONE || type >= FileMaterialTextureParamsType::FMTPT_COUNT || this->m_TexturePath[type] == _GStaticNotExistString)
	{
		output = this->m_TexturePath[FileMaterialTextureParamsType::FMTPT_NONE];
		return FALSE;
	}
	output = this->m_TexturePath[type];
	return TRUE;
}
void CReadMaterialParamsFile::SetPropertyPath(FileMaterialPropertyParamsType type, const std::string& str)
{
	if (type <= FileMaterialPropertyParamsType::FMPPT_NONE || type >= FileMaterialPropertyParamsType::FMPPT_COUNT)
	{
		return;
	}
	this->m_PropertyPath[type] = str;
}
BOOL CReadMaterialParamsFile::GetPropertyPath(FileMaterialPropertyParamsType type, std::string& output)const
{
	if (type <= FileMaterialPropertyParamsType::FMPPT_NONE || type >= FileMaterialPropertyParamsType::FMPPT_COUNT || this->m_PropertyPath[type] == _GStaticNotExistString)
	{
		output = this->m_PropertyPath[FileMaterialPropertyParamsType::FMPPT_NONE];
		return FALSE;
	}
	output = this->m_PropertyPath[type];
	return TRUE;
}
void CReadMaterialParamsFile::ReadFile(const std::string& fullPath)
{
	this->InitAllPath();

	CTempFileReader reader(FALSE, fullPath, [&](std::fstream& fileStream, const CTempFileReader* const tempReader)->BOOL {
		CHAR tempLine[CReadMaterialParamsFile::_PropertyStringLengthMax + CReadMaterialParamsFile::_PathStringLengthMax];
		for (INT i = (FileMaterialTextureParamsType::FMTPT_NONE + 1); i < FileMaterialTextureParamsType::FMTPT_COUNT; i++)
		{
			if (!fileStream.getline(tempLine, CReadMaterialParamsFile::_PropertyStringLengthMax + CReadMaterialParamsFile::_PathStringLengthMax))
			{
				continue;
			}
			std::string tempStr(tempLine); std::string tempProp, tempPath;
			if (!CTempFileHelper::FetchPosAndString<FALSE, TRUE>(tempStr, tempProp, CReadMaterialParamsFile::_PropertySeparator, "Error", &tempPath))
			{
				continue;
			}
			if (tempProp != _GStaticTextureName[i] || tempPath.length() < 2)
			{
				continue;
			}
			this->m_TexturePath[i] = tempPath;
		}
		for (INT i = (FileMaterialPropertyParamsType::FMPPT_NONE + 1); i < FileMaterialPropertyParamsType::FMPPT_COUNT; i++)
		{
			if (!fileStream.getline(tempLine, CReadMaterialParamsFile::_PropertyStringLengthMax + CReadMaterialParamsFile::_PathStringLengthMax))
			{
				continue;
			}
			std::string tempStr(tempLine); std::string tempProp, tempPath;
			if (!CTempFileHelper::FetchPosAndString<FALSE, TRUE>(tempStr, tempProp, CReadMaterialParamsFile::_PropertySeparator, "Error", &tempPath))
			{
				continue;
			}
			if (tempProp != _GStaticPropertyName[i] || tempPath.length() < 2)
			{
				continue;
			}
			this->m_PropertyPath[i] = tempPath;
		}
		return TRUE; });
}
void CReadMaterialParamsFile::WriteFile(const std::string& fullPath)
{
	CTempFileWriter writer(FALSE, fullPath, [&](std::fstream& fileStream, const CTempFileWriter* const tempWrite)->BOOL {
		for (INT i = (FileMaterialTextureParamsType::FMTPT_NONE + 1); i < FileMaterialTextureParamsType::FMTPT_COUNT; i++)
		{
			std::string tempWriteStr = _GStaticTextureName[i] + '=';
			if (this->m_TexturePath[i] != _GStaticNotExistString)
			{
				tempWriteStr += this->m_TexturePath[i];
			}
			tempWriteStr += '\n';
			fileStream << tempWriteStr;
		}
		for (INT i = (FileMaterialPropertyParamsType::FMPPT_NONE + 1); i < FileMaterialPropertyParamsType::FMPPT_COUNT; i++)
		{
			std::string tempWriteStr = _GStaticPropertyName[i] + '=';
			if (this->m_PropertyPath[i] != _GStaticNotExistString)
			{
				tempWriteStr += this->m_PropertyPath[i];
			}
			tempWriteStr += '\n';
			fileStream << tempWriteStr;
		}
		return TRUE; });
}
void CReadMaterialParamsFile::InitAllPath()
{
	for (INT i = (FileMaterialTextureParamsType::FMTPT_NONE + 1); i < FileMaterialTextureParamsType::FMTPT_COUNT; i++)
	{
		this->m_TexturePath[i] = _GStaticNotExistString;
	}
	for (INT i = (FileMaterialPropertyParamsType::FMPPT_NONE + 1); i < FileMaterialPropertyParamsType::FMPPT_COUNT; i++)
	{
		this->m_PropertyPath[i] = _GStaticNotExistString;
	}
}