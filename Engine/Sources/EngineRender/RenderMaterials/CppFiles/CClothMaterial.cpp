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

CClothMaterial::CClothMaterial() : CMaterialBase(typeid(CClothMaterial).name(), MaterialType::MATERIAL_TYPE_OPAQUE_FORWARD, sizeof(RenderParams), _GClothMaterialInputLayout, 4u, ENGINE_SHADER_DEFAULT_CLOTH_VS, ENGINE_SHADER_DEFAULT_CLOTH_PS)
{
	this->m_VertexShader	= CShaderManager::LoadVertexShader(ENGINE_SHADER_DEFAULT_CLOTH_VS, _GClothMaterialInputLayout, 4u);
	this->m_PixelShader		= CShaderManager::LoadPixelShader(ENGINE_SHADER_DEFAULT_CLOTH_PS);
	this->m_AlbedoTexture			= NULL;
	this->m_EmissiveTexture			= NULL;
	this->m_SheenColorTexture		= NULL;
	this->m_NormalTexture			= NULL;
	this->m_RoughnessTexture		= NULL;
	this->m_MetallicnessTexture		= NULL;
	this->m_AmbientOcclusionTexture	= NULL;
#if _DEVELOPMENT_EDITOR
	this->m_AlbedoTextureSelect				= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE;
	this->m_EmissiveTextureSelect			= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE;
	this->m_SheenColorTextureSelect			= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE;
	this->m_NormalTextureSelect				= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP;
	this->m_RoughnessTextureSelect			= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE;
	this->m_MetallicnessTextureSelect		= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE;
	this->m_AmbientOcclusionTextureSelect	= CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE;
	{
		auto initPathChar = [](CHAR* path, const std::string& str) {
			strcpy_s(path, 512, str.c_str());
		};
		initPathChar(this->m_AlbedoTexturePath, "Default White");
		initPathChar(this->m_EmissiveTexturePath, "Default White");
		initPathChar(this->m_SheenColorTexturePath, "Default White");
		initPathChar(this->m_NormalTexturePath, "Default Bump");
		initPathChar(this->m_RoughnessTexturePath, "Default White");
		initPathChar(this->m_MetallicnessTexturePath, "Default White");
		initPathChar(this->m_AmbientOcclusionTexturePath, "Default White");
	}
#endif
}
CClothMaterial::~CClothMaterial()
{

}
void CClothMaterial::Init()
{
#if _DEVELOPMENT_EDITOR
	{
		std::map<INT, std::string> baseEngineTextureItems = {
			{ -1, "None" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE, "Default White" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BLACK, "Default Black" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY, "Default Gray" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_RED, "Default Red" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GREEN, "Default Green" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BLUE, "Default Blue" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP, "Default Bump" },
			{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY, "Default Property" },
			{ -2, "Custom Path" } };
		auto pathSelect = [&baseEngineTextureItems](const INT& select, CHAR* name, CTexture2D*& texture, const BOOL& sRGB = TRUE) {
			if (select != -2)
			{
				strcpy_s(name, 512, baseEngineTextureItems[select].c_str());
				if (select != -1)
				{
					texture = CRenderPipeline::GetDefaultTexture(static_cast<CustomStruct::CEngineDefaultTexture2DEnum>(select));
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
		pathSelect(this->m_NormalTextureSelect, this->m_NormalTexturePath, this->m_NormalTexture);
		pathSelect(this->m_RoughnessTextureSelect, this->m_RoughnessTexturePath, this->m_RoughnessTexture);
		pathSelect(this->m_MetallicnessTextureSelect, this->m_MetallicnessTexturePath, this->m_MetallicnessTexture);
		pathSelect(this->m_AmbientOcclusionTextureSelect, this->m_AmbientOcclusionTexturePath, this->m_AmbientOcclusionTexture);
	}
#endif

	{
		auto nullTextureSelect = [](CTexture2D*& texture, CustomStruct::CEngineDefaultTexture2DEnum type) {
			if (texture == NULL)
			{
				texture = CRenderPipeline::GetDefaultTexture(type);
			}};
		nullTextureSelect(this->m_AlbedoTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY);
		nullTextureSelect(this->m_EmissiveTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE);
		nullTextureSelect(this->m_SheenColorTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE);
		nullTextureSelect(this->m_NormalTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP);
		nullTextureSelect(this->m_RoughnessTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE);
		nullTextureSelect(this->m_MetallicnessTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE);
		nullTextureSelect(this->m_AmbientOcclusionTexture, CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE);
	}

	this->m_RenderParams.BaseColorRoughness			= DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f);
	this->m_RenderParams.EmissiveAmbientOcclusion	= DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	this->m_RenderParams.SheenColorMetallicness		= DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	this->m_RenderParams.SubsurfaceColor			= DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
}
void CClothMaterial::Bind()const
{
	auto bindTexture = [](const CTexture2D* tex, const UINT& slot) {
		if (tex != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(tex->GetShaderResourceView(), slot);
		}};
	bindTexture(this->m_AlbedoTexture, ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
	bindTexture(this->m_NormalTexture, ENGINE_TEXTURE2D_NORMAL_START_SLOT);
	bindTexture(this->m_RoughnessTexture, 2u);
	bindTexture(this->m_EmissiveTexture, 8u);
	bindTexture(this->m_SheenColorTexture, 9u);
	bindTexture(this->m_MetallicnessTexture, 10u);
	bindTexture(this->m_AmbientOcclusionTexture, 11u);
}
const void* CClothMaterial::GetConstantData()const
{
	return (&(this->m_RenderParams));
}
#if _DEVELOPMENT_EDITOR
void CClothMaterial::SelectedEditorUpdate()
{
	FLOAT baseColor[3]			= { this->m_RenderParams.BaseColorRoughness.x, this->m_RenderParams.BaseColorRoughness.y, this->m_RenderParams.BaseColorRoughness.z };
	FLOAT EmissiveColor[3]		= { this->m_RenderParams.EmissiveAmbientOcclusion.x, this->m_RenderParams.EmissiveAmbientOcclusion.y, this->m_RenderParams.EmissiveAmbientOcclusion.z };
	FLOAT SheenColor[3]			= { this->m_RenderParams.SheenColorMetallicness.x, this->m_RenderParams.SheenColorMetallicness.y, this->m_RenderParams.SheenColorMetallicness.z };
	FLOAT SubsurfaceColor[3]	= { this->m_RenderParams.SubsurfaceColor.x, this->m_RenderParams.SubsurfaceColor.y, this->m_RenderParams.SubsurfaceColor.z };

	FLOAT roughness				= this->m_RenderParams.BaseColorRoughness.w;
	FLOAT metallicness			= this->m_RenderParams.SheenColorMetallicness.w;
	FLOAT ambientOcclusion		= this->m_RenderParams.EmissiveAmbientOcclusion.w;

	std::map<INT, std::string> baseEngineTextureItems = {
		{ -1, "None" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_WHITE, "Default White" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BLACK, "Default Black" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GRAY, "Default Gray" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_RED, "Default Red" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_GREEN, "Default Green" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BLUE, "Default Blue" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_BUMP, "Default Bump" },
		{ CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_PROPERTY, "Default Property" },
		{ -2, "Custom Path" } };

	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(400, 270), false, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

	ImGui::Text("BaseColor");
	ImGui::ColorEdit3("BaseColor", baseColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::Text("EmissiveColor");
	ImGui::ColorEdit3("EmissiveColor", EmissiveColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::Text("SheenColor");
	ImGui::ColorEdit3("SheenColor", SheenColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::Text("SubsurfaceColor");
	ImGui::ColorEdit3("SubsurfaceColor", SubsurfaceColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

	ImGui::SliderFloat("Roughness", &roughness, 0.f, 1.f);
	ImGui::SliderFloat("Metallicness", &metallicness, 0.f, 1.f);
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
		textureCombo("MetallicnessTexture", this->m_MetallicnessTextureSelect);
		texturePathText("MetallicnessTexturePath", this->m_MetallicnessTextureSelect, this->m_MetallicnessTexturePath);
		textureCombo("AmbientOcclusionTexture", this->m_AmbientOcclusionTextureSelect);
		texturePathText("AmbientOcclusionTexturePath", this->m_AmbientOcclusionTextureSelect, this->m_AmbientOcclusionTexturePath);
	}

	{
		auto textureUpdate = [&baseEngineTextureItems](const INT& select, CHAR* path, CTexture2D*& tex, const BOOL& sRGB = TRUE) {
			if (select != -2)
			{
				strcpy_s(path, 512, baseEngineTextureItems[select].c_str());
				if (select == -1)
				{
					tex = CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_RED);
				}
				else
				{
					tex = CRenderPipeline::GetDefaultTexture(static_cast<CustomStruct::CEngineDefaultTexture2DEnum>(select));
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
			textureUpdate(this->m_NormalTextureSelect, this->m_NormalTexturePath, this->m_NormalTexture);
			textureUpdate(this->m_RoughnessTextureSelect, this->m_RoughnessTexturePath, this->m_RoughnessTexture);
			textureUpdate(this->m_MetallicnessTextureSelect, this->m_MetallicnessTexturePath, this->m_MetallicnessTexture);
			textureUpdate(this->m_AmbientOcclusionTextureSelect, this->m_AmbientOcclusionTexturePath, this->m_AmbientOcclusionTexture);
		}
	}

	ImGui::EndChild();

	this->m_RenderParams.BaseColorRoughness			= DirectX::XMFLOAT4(baseColor[0], baseColor[1], baseColor[2], roughness);
	this->m_RenderParams.EmissiveAmbientOcclusion	= DirectX::XMFLOAT4(EmissiveColor[0], EmissiveColor[1], EmissiveColor[2], ambientOcclusion);
	this->m_RenderParams.SheenColorMetallicness		= DirectX::XMFLOAT4(SheenColor[0], SheenColor[1], SheenColor[2], metallicness);
	this->m_RenderParams.SubsurfaceColor			= DirectX::XMFLOAT4(SubsurfaceColor[0], SubsurfaceColor[1], SubsurfaceColor[2], 0.f);
}
#endif