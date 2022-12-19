#include "../Headers/CMaterialBase.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../RenderMaterials/Headers/CClearCoatMaterial.h"
#include "../../RenderMaterials/Headers/CClothMaterial.h"

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



const static std::string _GStaticTextureName[CReadWriteMaterialParamsFile::FileMaterialTextureParamsType::FMTPT_COUNT] = {
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
const static std::string _GStaticPropertyName[CReadWriteMaterialParamsFile::FileMaterialPropertyParamsType::FMPPT_COUNT] = {
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
const static std::string _GStaticMaterialConfigTypeString = "mat_tex_cfg";
constexpr UINT CReadWriteMaterialParamsFile::_PropertyStringLengthMax;
constexpr UINT CReadWriteMaterialParamsFile::_PathStringLengthMax;
CReadWriteMaterialParamsFile::CReadWriteMaterialParamsFile()
{
	this->m_TexturePath[FileMaterialTextureParamsType::FMTPT_NONE] = _GStaticNotExistString;
	this->m_PropertyPath[FileMaterialPropertyParamsType::FMPPT_NONE] = _GStaticNotExistString;
	this->InitAllPath();
}
CReadWriteMaterialParamsFile::~CReadWriteMaterialParamsFile()
{

}
void CReadWriteMaterialParamsFile::SaveMaterialParams(const std::string& path, const std::string& name, const CDefaultLitMaterial* mat)
{
	this->InitAllPath();

	{
		{
			auto saveTexPath = [&](CTexture2D* tex, FileMaterialTextureParamsType type) {
				if (tex != NULL)
				{
					this->SetTexturePath(type, tex->GetName());
				}};

			saveTexPath(mat->GetNormalTexture(), FileMaterialTextureParamsType::FMTPT_NORMAL);
			saveTexPath(mat->GetAlbedoTexture(), FileMaterialTextureParamsType::FMTPT_ALBEDO);
			saveTexPath(mat->GetEmissiveTexture(), FileMaterialTextureParamsType::FMTPT_EMISSIVE);
			saveTexPath(mat->GetRoughnessTexture(), FileMaterialTextureParamsType::FMTPT_ROUGHNESS);
			saveTexPath(mat->GetMetallicnessTexture(), FileMaterialTextureParamsType::FMTPT_METALLICNESS);
			saveTexPath(mat->GetAmbientOcclusionTexture(), FileMaterialTextureParamsType::FMTPT_AMBIENTOCCLUSION);
			saveTexPath(mat->GetReflectanceTexture(), FileMaterialTextureParamsType::FMTPT_REFLECTANCE);
		}

		{
			CustomStruct::CColor matBaseColor(mat->GetBaseColor());
			FLOAT baseColor[3] = { matBaseColor.r, matBaseColor.g, matBaseColor.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_BASECOLOR, baseColor, 3u);
		}

		{
			CustomStruct::CColor matEmissiveColor(mat->GetEmissiveColor());
			FLOAT emissiveColor[3] = { matEmissiveColor.r, matEmissiveColor.g, matEmissiveColor.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_EMISSIVE, emissiveColor, 3u);
		}

		{
			BOOL matGlossy = mat->GetIsGlossyRoughness();
			this->SetPropertyValue<BOOL>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &matGlossy, 1u);
		}

		{
			FLOAT matRoughness = mat->GetRoughness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &matRoughness, 1u);
		}

		{
			FLOAT matMetallicness = mat->GetMetallicness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &matMetallicness, 1u);
		}

		{
			FLOAT matReflectance = mat->GetReflectance();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &matReflectance, 1u);
		}

		{
			FLOAT matAmbientOcclusion = mat->GetAmbientOcclusion();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &matAmbientOcclusion, 1u);
		}
	}

	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->WriteFile(fullPath);
	}
}
void CReadWriteMaterialParamsFile::LoadMaterialParams(const std::string& path, const std::string& name, CDefaultLitMaterial* mat)
{
	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->ReadFile(fullPath);
	}

	{
		{
			auto loadTexFromPath = [&](FileMaterialTextureParamsType type, const BOOL& sRGB = TRUE)->CTexture2D* {
				std::string outputStr;
				if (this->GetTexturePath(type, outputStr))
				{
					return (CTextureManager::LoadTexture2D(outputStr, sRGB));
				}
				return NULL; };

			mat->SetNormalTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_NORMAL, FALSE));
			mat->SetAlbedoTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_ALBEDO));
			mat->SetEmissiveTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_EMISSIVE));
			mat->SetRoughnessTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_ROUGHNESS, FALSE));
			mat->SetMetallicnessTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_METALLICNESS, FALSE));
			mat->SetAmbientOcclusionTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_AMBIENTOCCLUSION, FALSE));
			mat->SetReflectanceTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_REFLECTANCE, FALSE));
		}

		{
			FLOAT clr[3] = { 1.f, 1.f, 1.f };
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_BASECOLOR, clr, 3u);
			mat->SetBaseColor(CustomStruct::CColor(clr[0], clr[1], clr[2]));
		}

		{
			FLOAT clr[3] = { 0.f, 0.f, 0.f };
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_EMISSIVE, clr, 3u);
			mat->SetEmissiveColor(CustomStruct::CColor(clr[0], clr[1], clr[2]));
		}

		{
			BOOL v = FALSE;
			this->GetPropertyValue<BOOL>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &v, 1u);
			mat->SetIsGlossyRoughness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &v, 1u);
			mat->SetRoughness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &v, 1u);
			mat->SetMetallicness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &v, 1u);
			mat->SetReflectance(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &v, 1u);
			mat->SetAmbientOcclusion(v);
		}
	}
}
void CReadWriteMaterialParamsFile::SetTexturePath(FileMaterialTextureParamsType type, const std::string& str)
{
	if (type <= FileMaterialTextureParamsType::FMTPT_NONE || type >= FileMaterialTextureParamsType::FMTPT_COUNT)
	{
		return;
	}
	this->m_TexturePath[type] = str;
}
BOOL CReadWriteMaterialParamsFile::GetTexturePath(FileMaterialTextureParamsType type, std::string& output)const
{
	if (type <= FileMaterialTextureParamsType::FMTPT_NONE || type >= FileMaterialTextureParamsType::FMTPT_COUNT || this->m_TexturePath[type] == _GStaticNotExistString)
	{
		output = this->m_TexturePath[FileMaterialTextureParamsType::FMTPT_NONE];
		return FALSE;
	}
	output = this->m_TexturePath[type];
	return TRUE;
}
void CReadWriteMaterialParamsFile::SetPropertyPath(FileMaterialPropertyParamsType type, const std::string& str)
{
	if (type <= FileMaterialPropertyParamsType::FMPPT_NONE || type >= FileMaterialPropertyParamsType::FMPPT_COUNT)
	{
		return;
	}
	this->m_PropertyPath[type] = str;
}
BOOL CReadWriteMaterialParamsFile::GetPropertyPath(FileMaterialPropertyParamsType type, std::string& output)const
{
	if (type <= FileMaterialPropertyParamsType::FMPPT_NONE || type >= FileMaterialPropertyParamsType::FMPPT_COUNT || this->m_PropertyPath[type] == _GStaticNotExistString)
	{
		output = this->m_PropertyPath[FileMaterialPropertyParamsType::FMPPT_NONE];
		return FALSE;
	}
	output = this->m_PropertyPath[type];
	return TRUE;
}
void CReadWriteMaterialParamsFile::ReadFile(const std::string& fullPath)
{
	this->InitAllPath();

	CTempFileReader reader(FALSE, fullPath, [&](std::fstream& fileStream, const CTempFileReader* const tempReader)->BOOL {
		CHAR tempLine[CReadWriteMaterialParamsFile::_PropertyStringLengthMax + CReadWriteMaterialParamsFile::_PathStringLengthMax];
		for (INT i = (FileMaterialTextureParamsType::FMTPT_NONE + 1); i < FileMaterialTextureParamsType::FMTPT_COUNT; i++)
		{
			if (!fileStream.getline(tempLine, CReadWriteMaterialParamsFile::_PropertyStringLengthMax + CReadWriteMaterialParamsFile::_PathStringLengthMax))
			{
				continue;
			}
			std::string tempStr(tempLine); std::string tempProp, tempPath;
			if (!CTempFileHelper::FetchPosAndString<FALSE, TRUE>(tempStr, tempProp, CReadWriteMaterialParamsFile::_PropertySeparator, "Error", &tempPath))
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
			if (!fileStream.getline(tempLine, CReadWriteMaterialParamsFile::_PropertyStringLengthMax + CReadWriteMaterialParamsFile::_PathStringLengthMax))
			{
				continue;
			}
			std::string tempStr(tempLine); std::string tempProp, tempPath;
			if (!CTempFileHelper::FetchPosAndString<FALSE, TRUE>(tempStr, tempProp, CReadWriteMaterialParamsFile::_PropertySeparator, "Error", &tempPath))
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
void CReadWriteMaterialParamsFile::WriteFile(const std::string& fullPath)
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
void CReadWriteMaterialParamsFile::InitAllPath()
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