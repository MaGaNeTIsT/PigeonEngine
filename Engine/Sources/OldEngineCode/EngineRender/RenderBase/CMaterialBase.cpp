/*
#include "../Headers/CMaterialBase.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderMaterials/Headers/CDefaultLitMaterial.h"
#include "../../RenderMaterials/Headers/CClearCoatMaterial.h"
#include "../../RenderMaterials/Headers/CClothMaterial.h"

CMaterialBase::CMaterialBase(const std::string& name, MaterialType materialType, const UINT32& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT32& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName)
{
	this->m_Name				= name;
	this->m_MaterialType		= materialType;
	this->m_ConstantSize		= constantSize;
	if (inputLayoutNum > 0)
	{
		this->m_InputLayout.resize(inputLayoutNum);
		for (UINT32 i = 0u; i < inputLayoutNum; i++)
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
const UINT32& CMaterialBase::GetConstantSize()const
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
BOOL32 CMaterialBase::HasConstantData()const
{
	return (this->m_ConstantSize > 0u && this->GetConstantData() != nullptr);
}
BOOL32 CMaterialBase::HasInputLayout()const
{
	return (this->m_InputLayout.size() > 0);
}
BOOL32 CMaterialBase::HasVertexShader()const
{
	return (this->m_VertexShader != NULL);
}
BOOL32 CMaterialBase::HasPixelShader()const
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
constexpr UINT32 CReadWriteMaterialParamsFile::_PropertyStringLengthMax;
constexpr UINT32 CReadWriteMaterialParamsFile::_PathStringLengthMax;
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
			CustomStruct::CColor matClr(mat->GetBaseColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_BASECOLOR, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetEmissiveColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_EMISSIVE, clr, 3u);
		}

		{
			BOOL32 matV = mat->GetIsGlossyRoughness();
			this->SetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &matV, 1u);
		}

		{
			FLOAT matV = mat->GetRoughness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &matV, 1u);

			matV = mat->GetMetallicness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &matV, 1u);

			matV = mat->GetReflectance();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &matV, 1u);

			matV = mat->GetAmbientOcclusion();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &matV, 1u);
		}
	}

	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->WriteFile(fullPath);
	}
}
void CReadWriteMaterialParamsFile::SaveMaterialParams(const std::string& path, const std::string& name, const class CDefaultLitSkeletonMeshMaterial* mat)
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
			CustomStruct::CColor matClr(mat->GetBaseColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_BASECOLOR, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetEmissiveColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_EMISSIVE, clr, 3u);
		}

		{
			BOOL32 matV = mat->GetIsGlossyRoughness();
			this->SetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &matV, 1u);
		}

		{
			FLOAT matV = mat->GetRoughness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &matV, 1u);

			matV = mat->GetMetallicness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &matV, 1u);

			matV = mat->GetReflectance();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &matV, 1u);

			matV = mat->GetAmbientOcclusion();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &matV, 1u);
		}
	}

	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->WriteFile(fullPath);
	}
}
void CReadWriteMaterialParamsFile::SaveMaterialParams(const std::string& path, const std::string& name, const CAnisotropicMaterial* mat)
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
			saveTexPath(mat->GetAnisotropyStrengthTexture(), FileMaterialTextureParamsType::FMTPT_ANISOTROPYSTRENGTH);
			saveTexPath(mat->GetAnisotropyDirectionTexture(), FileMaterialTextureParamsType::FMTPT_ANISOTROPYDIRECTION);
		}

		{
			CustomStruct::CColor matClr(mat->GetBaseColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_BASECOLOR, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetEmissiveColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_EMISSIVE, clr, 3u);
		}

		{
			BOOL32 matV = mat->GetIsGlossyRoughness();
			this->SetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &matV, 1u);
		}

		{
			FLOAT matV = mat->GetRoughness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &matV, 1u);

			matV = mat->GetMetallicness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &matV, 1u);

			matV = mat->GetReflectance();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &matV, 1u);

			matV = mat->GetAmbientOcclusion();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &matV, 1u);

			matV = mat->GetAnisotropyStrength();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYSTRENGTH, &matV, 1u);

			matV = mat->GetAnisotropyDirection();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYDIRECTION, &matV, 1u);
		}
	}

	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->WriteFile(fullPath);
	}
}
void CReadWriteMaterialParamsFile::SaveMaterialParams(const std::string& path, const std::string& name, const CClothMaterial* mat)
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
			saveTexPath(mat->GetAmbientOcclusionTexture(), FileMaterialTextureParamsType::FMTPT_AMBIENTOCCLUSION);
			saveTexPath(mat->GetSheenColorTexture(), FileMaterialTextureParamsType::FMTPT_CLOTHSHEEN);
			saveTexPath(mat->GetSubsurfaceTexture(), FileMaterialTextureParamsType::FMTPT_CLOTHSUBSURFACE);
		}

		{
			CustomStruct::CColor matClr(mat->GetBaseColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_BASECOLOR, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetEmissiveColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_EMISSIVE, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetSheenColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSHEEN, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetSubsurfaceColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSUBSURFACE, clr, 3u);
		}

		{
			BOOL32 matV = mat->GetIsGlossyRoughness();
			this->SetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &matV, 1u);
		}

		{
			FLOAT matV = mat->GetRoughness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &matV, 1u);

			matV = mat->GetAmbientOcclusion();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &matV, 1u);
		}
	}

	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->WriteFile(fullPath);
	}
}
void CReadWriteMaterialParamsFile::SaveMaterialParams(const std::string& path, const std::string& name, const CClothAnisotropicMaterial* mat)
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
			saveTexPath(mat->GetAmbientOcclusionTexture(), FileMaterialTextureParamsType::FMTPT_AMBIENTOCCLUSION);
			saveTexPath(mat->GetSheenColorTexture(), FileMaterialTextureParamsType::FMTPT_CLOTHSHEEN);
			saveTexPath(mat->GetSubsurfaceTexture(), FileMaterialTextureParamsType::FMTPT_CLOTHSUBSURFACE);
			saveTexPath(mat->GetAnisotropyStrengthTexture(), FileMaterialTextureParamsType::FMTPT_ANISOTROPYSTRENGTH);
			saveTexPath(mat->GetAnisotropyDirectionTexture(), FileMaterialTextureParamsType::FMTPT_ANISOTROPYDIRECTION);
		}

		{
			CustomStruct::CColor matClr(mat->GetBaseColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_BASECOLOR, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetEmissiveColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_EMISSIVE, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetSheenColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSHEEN, clr, 3u);
		}

		{
			CustomStruct::CColor matClr(mat->GetSubsurfaceColor());
			FLOAT clr[3] = { matClr.r, matClr.g, matClr.b };
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSUBSURFACE, clr, 3u);
		}

		{
			BOOL32 matV = mat->GetIsGlossyRoughness();
			this->SetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &matV, 1u);
		}

		{
			FLOAT matV = mat->GetRoughness();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &matV, 1u);

			matV = mat->GetAmbientOcclusion();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &matV, 1u);

			matV = mat->GetAnisotropyStrength();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYSTRENGTH, &matV, 1u);

			matV = mat->GetAnisotropyDirection();
			this->SetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYDIRECTION, &matV, 1u);
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
			auto loadTexFromPath = [&](FileMaterialTextureParamsType type, const BOOL32& sRGB = TRUE)->CTexture2D* {
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
			BOOL32 v = FALSE;
			this->GetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &v, 1u);
			mat->SetIsGlossyRoughness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &v, 1u);
			mat->SetRoughness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &v, 1u);
			mat->SetMetallicness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &v, 1u);
			mat->SetReflectance(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &v, 1u);
			mat->SetAmbientOcclusion(v);
		}
	}
}
void CReadWriteMaterialParamsFile::LoadMaterialParams(const std::string& path, const std::string& name, class CDefaultLitSkeletonMeshMaterial* mat)
{
	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->ReadFile(fullPath);
	}

	{
		{
			auto loadTexFromPath = [&](FileMaterialTextureParamsType type, const BOOL32& sRGB = TRUE)->CTexture2D* {
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
			BOOL32 v = FALSE;
			this->GetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &v, 1u);
			mat->SetIsGlossyRoughness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &v, 1u);
			mat->SetRoughness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &v, 1u);
			mat->SetMetallicness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &v, 1u);
			mat->SetReflectance(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &v, 1u);
			mat->SetAmbientOcclusion(v);
		}
	}
}
void CReadWriteMaterialParamsFile::LoadMaterialParams(const std::string& path, const std::string& name, CAnisotropicMaterial* mat)
{
	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->ReadFile(fullPath);
	}

	{
		{
			auto loadTexFromPath = [&](FileMaterialTextureParamsType type, const BOOL32& sRGB = TRUE)->CTexture2D* {
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
			mat->SetAnisotropyStrengthTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_ANISOTROPYSTRENGTH, FALSE));
			mat->SetAnisotropyDirectionTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_ANISOTROPYDIRECTION, FALSE));
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
			BOOL32 v = FALSE;
			this->GetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &v, 1u);
			mat->SetIsGlossyRoughness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &v, 1u);
			mat->SetRoughness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_METALLICNESS, &v, 1u);
			mat->SetMetallicness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_REFLECTANCE, &v, 1u);
			mat->SetReflectance(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &v, 1u);
			mat->SetAmbientOcclusion(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYSTRENGTH, &v, 1u);
			mat->SetAnisotropyStrength(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYDIRECTION, &v, 1u);
			mat->SetAnisotropyDirection(v);
		}
	}
}
void CReadWriteMaterialParamsFile::LoadMaterialParams(const std::string& path, const std::string& name, CClothMaterial* mat)
{
	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->ReadFile(fullPath);
	}

	{
		{
			auto loadTexFromPath = [&](FileMaterialTextureParamsType type, const BOOL32& sRGB = TRUE)->CTexture2D* {
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
			mat->SetAmbientOcclusionTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_AMBIENTOCCLUSION, FALSE));
			mat->SetSheenColorTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_CLOTHSHEEN, FALSE));
			mat->SetSubsurfaceTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_CLOTHSUBSURFACE, FALSE));
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
			FLOAT clr[3] = { 0.f, 0.f, 0.f };
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSHEEN, clr, 3u);
			mat->SetSheenColor(CustomStruct::CColor(clr[0], clr[1], clr[2]));
		}

		{
			FLOAT clr[3] = { 0.f, 0.f, 0.f };
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSUBSURFACE, clr, 3u);
			mat->SetSubsurfaceColor(CustomStruct::CColor(clr[0], clr[1], clr[2]));
		}

		{
			BOOL32 v = FALSE;
			this->GetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &v, 1u);
			mat->SetIsGlossyRoughness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &v, 1u);
			mat->SetRoughness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &v, 1u);
			mat->SetAmbientOcclusion(v);
		}
	}
}
void CReadWriteMaterialParamsFile::LoadMaterialParams(const std::string& path, const std::string& name, CClothAnisotropicMaterial* mat)
{
	{
		std::string fullPath = path + name + CTempFileHelper::_NameTypeSeparator + _GStaticMaterialConfigTypeString;
		this->ReadFile(fullPath);
	}

	{
		{
			auto loadTexFromPath = [&](FileMaterialTextureParamsType type, const BOOL32& sRGB = TRUE)->CTexture2D* {
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
			mat->SetAmbientOcclusionTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_AMBIENTOCCLUSION, FALSE));
			mat->SetSheenColorTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_CLOTHSHEEN, FALSE));
			mat->SetSubsurfaceTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_CLOTHSUBSURFACE, FALSE));
			mat->SetAnisotropyStrengthTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_ANISOTROPYSTRENGTH, FALSE));
			mat->SetAnisotropyDirectionTexture(loadTexFromPath(FileMaterialTextureParamsType::FMTPT_ANISOTROPYDIRECTION, FALSE));
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
			FLOAT clr[3] = { 0.f, 0.f, 0.f };
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSHEEN, clr, 3u);
			mat->SetSheenColor(CustomStruct::CColor(clr[0], clr[1], clr[2]));
		}

		{
			FLOAT clr[3] = { 0.f, 0.f, 0.f };
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_CLOTHSUBSURFACE, clr, 3u);
			mat->SetSubsurfaceColor(CustomStruct::CColor(clr[0], clr[1], clr[2]));
		}

		{
			BOOL32 v = FALSE;
			this->GetPropertyValue<BOOL32>(FileMaterialPropertyParamsType::FMPPT_ISGLOSSY, &v, 1u);
			mat->SetIsGlossyRoughness(v);
		}

		{
			FLOAT v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ROUGHNESS, &v, 1u);
			mat->SetRoughness(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_AMBIENTOCCLUSION, &v, 1u);
			mat->SetAmbientOcclusion(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYSTRENGTH, &v, 1u);
			mat->SetAnisotropyStrength(v);

			v = 1.f;
			this->GetPropertyValue<FLOAT>(FileMaterialPropertyParamsType::FMPPT_ANISOTROPYDIRECTION, &v, 1u);
			mat->SetAnisotropyDirection(v);
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
BOOL32 CReadWriteMaterialParamsFile::GetTexturePath(FileMaterialTextureParamsType type, std::string& output)const
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
BOOL32 CReadWriteMaterialParamsFile::GetPropertyPath(FileMaterialPropertyParamsType type, std::string& output)const
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

	CTempFileReader reader(FALSE, fullPath, [&](std::fstream& fileStream, const CTempFileReader* const tempReader)->BOOL32 {
		CHAR tempLine[CReadWriteMaterialParamsFile::_PropertyStringLengthMax + CReadWriteMaterialParamsFile::_PathStringLengthMax];
		for (INT32 i = (FileMaterialTextureParamsType::FMTPT_NONE + 1); i < FileMaterialTextureParamsType::FMTPT_COUNT; i++)
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
		for (INT32 i = (FileMaterialPropertyParamsType::FMPPT_NONE + 1); i < FileMaterialPropertyParamsType::FMPPT_COUNT; i++)
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
	CTempFileWriter writer(FALSE, fullPath, [&](std::fstream& fileStream, const CTempFileWriter* const tempWrite)->BOOL32 {
		for (INT32 i = (FileMaterialTextureParamsType::FMTPT_NONE + 1); i < FileMaterialTextureParamsType::FMTPT_COUNT; i++)
		{
			std::string tempWriteStr = _GStaticTextureName[i] + '=';
			if (this->m_TexturePath[i] != _GStaticNotExistString)
			{
				tempWriteStr += this->m_TexturePath[i];
			}
			tempWriteStr += '\n';
			fileStream << tempWriteStr;
		}
		for (INT32 i = (FileMaterialPropertyParamsType::FMPPT_NONE + 1); i < FileMaterialPropertyParamsType::FMPPT_COUNT; i++)
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
	for (INT32 i = (FileMaterialTextureParamsType::FMTPT_NONE + 1); i < FileMaterialTextureParamsType::FMTPT_COUNT; i++)
	{
		this->m_TexturePath[i] = _GStaticNotExistString;
	}
	for (INT32 i = (FileMaterialPropertyParamsType::FMPPT_NONE + 1); i < FileMaterialPropertyParamsType::FMPPT_COUNT; i++)
	{
		this->m_PropertyPath[i] = _GStaticNotExistString;
	}
}
*/