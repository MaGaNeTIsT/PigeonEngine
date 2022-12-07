#include "../Headers/CClothMaterial.h"
#include "../../RenderBase/Headers/CRenderStructCommon.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../RenderBase/Headers/CRenderDevice.h"
#include "../../RenderBase/Headers/CRenderPipeline.h"

const static CustomStruct::CRenderInputLayoutDesc _GClothMaterialInputLayout[4u] = {
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

CClothMaterial::CClothMaterial() : CMaterialBase(typeid(CClothMaterial).name(), MaterialType::MATERIAL_TYPE_OPAQUE_FORWARD, sizeof(CClothMaterial::RenderParams), _GClothMaterialInputLayout, 4u, ENGINE_DEFAULT_SHADER_CLOTH_VS, ENGINE_DEFAULT_SHADER_CLOTH_PS)
{
	this->m_VertexShader			= CShaderManager::LoadVertexShader(ENGINE_DEFAULT_SHADER_CLOTH_VS, _GClothMaterialInputLayout, 4u);
	this->m_PixelShader				= CShaderManager::LoadPixelShader(ENGINE_DEFAULT_SHADER_CLOTH_PS);
	this->m_AlbedoTexture			= NULL;
	this->m_EmissiveTexture			= NULL;
	this->m_SheenColorTexture		= NULL;
	this->m_NormalTexture			= NULL;
	this->m_RoughnessTexture		= NULL;
	this->m_AmbientOcclusionTexture	= NULL;
	this->m_SubsurfaceTexture		= NULL;
#ifdef _DEVELOPMENT_EDITOR
	this->m_IsGlossy						= FALSE;
	this->m_AlbedoTextureSelect				= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_EmissiveTextureSelect			= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK;
	this->m_SheenColorTextureSelect			= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_NormalTextureSelect				= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP;
	this->m_RoughnessTextureSelect			= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_AmbientOcclusionTextureSelect	= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_SubsurfaceTextureSelect			= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	{
		auto initPathChar = [](CHAR* path, const std::string& str) {
			strcpy_s(path, 512, str.c_str());
		};
		initPathChar(this->m_AlbedoTexturePath, "Default White");
		initPathChar(this->m_EmissiveTexturePath, "Default Black");
		initPathChar(this->m_SheenColorTexturePath, "Default White");
		initPathChar(this->m_NormalTexturePath, "Default Bump");
		initPathChar(this->m_RoughnessTexturePath, "Default White");
		initPathChar(this->m_AmbientOcclusionTexturePath, "Default White");
		initPathChar(this->m_SubsurfaceTexturePath, "Default White");
	}
#endif

	this->m_RenderParams.BaseColorRoughness			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	this->m_RenderParams.EmissiveAmbientOcclusion	= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	this->m_RenderParams.SheenColorIsGlossy			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.f);
	this->m_RenderParams.SubsurfaceColor			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.f);
}
CClothMaterial::~CClothMaterial()
{

}
void CClothMaterial::SetNormalTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_NormalTexture = tex;
	}
}
void CClothMaterial::SetAlbedoTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AlbedoTexture = tex;
	}
}
void CClothMaterial::SetEmissiveTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_EmissiveTexture = tex;
	}
}
void CClothMaterial::SetSheenColorTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_SheenColorTexture = tex;
	}
}
void CClothMaterial::SetRoughnessTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_RoughnessTexture = tex;
	}
}
void CClothMaterial::SetAmbientOcclusionTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AmbientOcclusionTexture = tex;
	}
}
void CClothMaterial::SetSubsurfaceTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_SubsurfaceTexture = tex;
	}
}
void CClothMaterial::SetIsGlossyRoughness(const BOOL& v)
{
	this->m_RenderParams.SheenColorIsGlossy.w = v ? 1.f : 0.f;
}
void CClothMaterial::SetRoughness(const FLOAT& v)
{
	this->m_RenderParams.BaseColorRoughness.w = v;
}
void CClothMaterial::SetAmbientOcclusion(const FLOAT& v)
{
	this->m_RenderParams.EmissiveAmbientOcclusion.w = v;
}
void CClothMaterial::SetBaseColor(const CustomStruct::CColor& clr)
{
	this->m_RenderParams.BaseColorRoughness.x = clr.r;
	this->m_RenderParams.BaseColorRoughness.y = clr.g;
	this->m_RenderParams.BaseColorRoughness.z = clr.b;
}
void CClothMaterial::SetEmissiveColor(const CustomStruct::CColor& clr)
{
	this->m_RenderParams.EmissiveAmbientOcclusion.x = clr.r;
	this->m_RenderParams.EmissiveAmbientOcclusion.y = clr.g;
	this->m_RenderParams.EmissiveAmbientOcclusion.z = clr.b;
}
void CClothMaterial::SetSheenColor(const CustomStruct::CColor& clr)
{
	this->m_RenderParams.SheenColorIsGlossy.x = clr.r;
	this->m_RenderParams.SheenColorIsGlossy.y = clr.g;
	this->m_RenderParams.SheenColorIsGlossy.z = clr.b;
}
void CClothMaterial::SetSubsurfaceColor(const CustomStruct::CColor& clr)
{
	this->m_RenderParams.SubsurfaceColor.x = clr.r;
	this->m_RenderParams.SubsurfaceColor.y = clr.g;
	this->m_RenderParams.SubsurfaceColor.z = clr.b;
}
void CClothMaterial::Init()
{
#ifdef _DEVELOPMENT_EDITOR
	{
		std::map<INT, std::string> baseEngineTextureItems = {
			{ -1, "None" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE, "Default White" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK, "Default Black" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY, "Default Gray" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_RED, "Default Red" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GREEN, "Default Green" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLUE, "Default Blue" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP, "Default Bump" },
			{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_PROPERTY, "Default Property" },
			{ -2, "Custom Path" } };
		auto pathSelect = [&baseEngineTextureItems](const INT& select, CHAR* name, CTexture2D*& texture, const BOOL& sRGB = TRUE) {
			if (select != -2)
			{
				strcpy_s(name, 512, baseEngineTextureItems[select].c_str());
				if (select != -1)
				{
					texture = CRenderPipeline::GetDefaultTexture(static_cast<CustomStruct::CEngineDefaultTexture2DType>(select));
				}
			}
			else
			{
				std::string tempName(name);
				texture = CTextureManager::LoadTexture2D(tempName, sRGB);
			}};
		pathSelect(this->m_AlbedoTextureSelect, this->m_AlbedoTexturePath, this->m_AlbedoTexture);
		pathSelect(this->m_EmissiveTextureSelect, this->m_EmissiveTexturePath, this->m_EmissiveTexture);
		pathSelect(this->m_SheenColorTextureSelect, this->m_SheenColorTexturePath, this->m_SheenColorTexture);
		pathSelect(this->m_NormalTextureSelect, this->m_NormalTexturePath, this->m_NormalTexture, FALSE);
		pathSelect(this->m_RoughnessTextureSelect, this->m_RoughnessTexturePath, this->m_RoughnessTexture, FALSE);
		pathSelect(this->m_AmbientOcclusionTextureSelect, this->m_AmbientOcclusionTexturePath, this->m_AmbientOcclusionTexture, FALSE);
		pathSelect(this->m_SubsurfaceTextureSelect, this->m_SubsurfaceTexturePath, this->m_SubsurfaceTexture);
	}
#endif

	{
		auto nullTextureSelect = [](CTexture2D*& texture, CustomStruct::CEngineDefaultTexture2DType type) {
			if (texture == NULL)
			{
				texture = CRenderPipeline::GetDefaultTexture(type);
			}};
		nullTextureSelect(this->m_AlbedoTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_EmissiveTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK);
		nullTextureSelect(this->m_SheenColorTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_NormalTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP);
		nullTextureSelect(this->m_RoughnessTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_AmbientOcclusionTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_SubsurfaceTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
	}
}
void CClothMaterial::Bind()const
{
	auto bindTexture = [](const CTexture2D* tex, const UINT& slot) {
		if (tex != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(tex->GetShaderResourceView(), slot);
		}};
	bindTexture(this->m_NormalTexture, ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT);
	bindTexture(this->m_AlbedoTexture, ENGINE_TEXTURE2D_CUSTOM_B_START_SLOT);
	bindTexture(this->m_EmissiveTexture, ENGINE_TEXTURE2D_CUSTOM_C_START_SLOT);
	bindTexture(this->m_SheenColorTexture, ENGINE_TEXTURE2D_CUSTOM_D_START_SLOT);
	bindTexture(this->m_RoughnessTexture, 9u);
	bindTexture(this->m_AmbientOcclusionTexture, 10u);
	bindTexture(this->m_SubsurfaceTexture, 11u);
}
const void* CClothMaterial::GetConstantData()const
{
	return (&(this->m_RenderParams));
}
#ifdef _DEVELOPMENT_EDITOR
void CClothMaterial::SelectedEditorUpdate()
{
	FLOAT baseColor[3]			= { this->m_RenderParams.BaseColorRoughness.x, this->m_RenderParams.BaseColorRoughness.y, this->m_RenderParams.BaseColorRoughness.z };
	FLOAT emissiveColor[3]		= { this->m_RenderParams.EmissiveAmbientOcclusion.x, this->m_RenderParams.EmissiveAmbientOcclusion.y, this->m_RenderParams.EmissiveAmbientOcclusion.z };
	FLOAT sheenColor[3]			= { this->m_RenderParams.SheenColorIsGlossy.x, this->m_RenderParams.SheenColorIsGlossy.y, this->m_RenderParams.SheenColorIsGlossy.z };
	FLOAT subsurfaceColor[3]	= { this->m_RenderParams.SubsurfaceColor.x, this->m_RenderParams.SubsurfaceColor.y, this->m_RenderParams.SubsurfaceColor.z };

	bool isGlossy				= this->m_IsGlossy;
	FLOAT glossy				= 0.f;
	FLOAT roughness				= this->m_RenderParams.BaseColorRoughness.w;
	FLOAT ambientOcclusion		= this->m_RenderParams.EmissiveAmbientOcclusion.w;

	std::map<INT, std::string> baseEngineTextureItems = {
		{ -1, "None" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE, "Default White" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLACK, "Default Black" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GRAY, "Default Gray" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_RED, "Default Red" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_GREEN, "Default Green" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BLUE, "Default Blue" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP, "Default Bump" },
		{ CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_PROPERTY, "Default Property" },
		{ -2, "Custom Path" } };

	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(400, 270), false, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

	ImGui::Text("BaseColor");
	ImGui::ColorEdit3("BaseColor", baseColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::Text("EmissiveColor");
	ImGui::ColorEdit3("EmissiveColor", emissiveColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::Text("SheenColor");
	ImGui::ColorEdit3("SheenColor", sheenColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::Text("SubsurfaceColor");
	ImGui::ColorEdit3("SubsurfaceColor", subsurfaceColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::Checkbox("IsGlossy", &isGlossy);
	ImGui::SliderFloat("Roughness", &roughness, 0.f, 1.f);
	ImGui::SliderFloat("AmbientOcclusion", &ambientOcclusion, 0.f, 1.f);

	{
		auto textureCombo = [&baseEngineTextureItems](const std::string& name, INT& select) {
			if (ImGui::BeginCombo(name.c_str(), baseEngineTextureItems[select].c_str()))
			{
				for (const auto& textureItem : baseEngineTextureItems)
				{
					BOOL selected = (select == textureItem.first);
					if (ImGui::Selectable(textureItem.second.c_str(), &selected))
					{
						select = textureItem.first;
					}
					if (selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}};
		auto texturePathText = [](const std::string& name, const INT& select, CHAR* path) {
			if (select == -2)
			{
				ImGui::InputText(name.c_str(), path, 512, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue);
			}};
		textureCombo("BaseTexture", this->m_AlbedoTextureSelect);
		texturePathText("BaseTexturePath", this->m_AlbedoTextureSelect, this->m_AlbedoTexturePath);
		textureCombo("EmissiveTexture", this->m_EmissiveTextureSelect);
		texturePathText("EmissiveTexturePath", this->m_EmissiveTextureSelect, this->m_EmissiveTexturePath);
		textureCombo("SheenTexture", this->m_SheenColorTextureSelect);
		texturePathText("SheenTexturePath", this->m_SheenColorTextureSelect, this->m_SheenColorTexturePath);
		textureCombo("NormalTexture", this->m_NormalTextureSelect);
		texturePathText("NormalTexturePath", this->m_NormalTextureSelect, this->m_NormalTexturePath);
		textureCombo("RoughnessTexture", this->m_RoughnessTextureSelect);
		texturePathText("RoughnessTexturePath", this->m_RoughnessTextureSelect, this->m_RoughnessTexturePath);
		textureCombo("AmbientOcclusionTexture", this->m_AmbientOcclusionTextureSelect);
		texturePathText("AmbientOcclusionTexturePath", this->m_AmbientOcclusionTextureSelect, this->m_AmbientOcclusionTexturePath);
		textureCombo("SubsurfaceTexture", this->m_SubsurfaceTextureSelect);
		texturePathText("SubsurfaceTexturePath", this->m_SubsurfaceTextureSelect, this->m_SubsurfaceTexturePath);
	}

	{
		auto textureUpdate = [&baseEngineTextureItems](const INT& select, CHAR* path, CTexture2D*& tex, const BOOL& sRGB = TRUE) {
			if (select != -2)
			{
				strcpy_s(path, 512, baseEngineTextureItems[select].c_str());
				if (select == -1)
				{
					tex = CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_RED);
				}
				else
				{
					tex = CRenderPipeline::GetDefaultTexture(static_cast<CustomStruct::CEngineDefaultTexture2DType>(select));
				}
			}
			else
			{
				std::string tempName(path);
				tex = CTextureManager::LoadTexture2D(tempName, sRGB);
			}
		};
		if (ImGui::Button("ApplyTextures"))
		{
			textureUpdate(this->m_AlbedoTextureSelect, this->m_AlbedoTexturePath, this->m_AlbedoTexture);
			textureUpdate(this->m_EmissiveTextureSelect, this->m_EmissiveTexturePath, this->m_EmissiveTexture);
			textureUpdate(this->m_SheenColorTextureSelect, this->m_SheenColorTexturePath, this->m_SheenColorTexture);
			textureUpdate(this->m_NormalTextureSelect, this->m_NormalTexturePath, this->m_NormalTexture, FALSE);
			textureUpdate(this->m_RoughnessTextureSelect, this->m_RoughnessTexturePath, this->m_RoughnessTexture, FALSE);
			textureUpdate(this->m_AmbientOcclusionTextureSelect, this->m_AmbientOcclusionTexturePath, this->m_AmbientOcclusionTexture, FALSE);
			textureUpdate(this->m_SubsurfaceTextureSelect, this->m_SubsurfaceTexturePath, this->m_SubsurfaceTexture);
		}
	}

	ImGui::EndChild();

	this->m_IsGlossy = isGlossy;
	glossy = isGlossy ? 1.f : 0.f;
	this->m_RenderParams.BaseColorRoughness			= DirectX::XMFLOAT4(baseColor[0], baseColor[1], baseColor[2], roughness);
	this->m_RenderParams.EmissiveAmbientOcclusion	= DirectX::XMFLOAT4(emissiveColor[0], emissiveColor[1], emissiveColor[2], ambientOcclusion);
	this->m_RenderParams.SheenColorIsGlossy			= DirectX::XMFLOAT4(sheenColor[0], sheenColor[1], sheenColor[2], glossy);
	this->m_RenderParams.SubsurfaceColor			= DirectX::XMFLOAT4(subsurfaceColor[0], subsurfaceColor[1], subsurfaceColor[2], 0.f);
}
#endif