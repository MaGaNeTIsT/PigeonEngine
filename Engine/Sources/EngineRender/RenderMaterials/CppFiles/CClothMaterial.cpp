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
	this->m_RenderParams = new CClothMaterial::RenderParams();
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness		= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	renderParams->EmissiveAmbientOcclusion	= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	renderParams->SheenColorIsGlossy		= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.f);
	renderParams->SubsurfaceColor			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.f);
}
CClothMaterial::CClothMaterial(const std::string& name, MaterialType materialType, const UINT& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName) : CMaterialBase(name, materialType, constantSize, inputLayout, inputLayoutNum, vertexShaderName, pixelShaderName)
{
	this->m_VertexShader			= CShaderManager::LoadVertexShader(vertexShaderName, inputLayout, inputLayoutNum);
	this->m_PixelShader				= CShaderManager::LoadPixelShader(pixelShaderName);
	this->m_AlbedoTexture			= NULL;
	this->m_EmissiveTexture			= NULL;
	this->m_SheenColorTexture		= NULL;
	this->m_NormalTexture			= NULL;
	this->m_RoughnessTexture		= NULL;
	this->m_AmbientOcclusionTexture	= NULL;
	this->m_SubsurfaceTexture		= NULL;
	this->m_RenderParams			= nullptr;
#ifdef _DEVELOPMENT_EDITOR
	this->m_IsGlossy = FALSE;
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
}
CClothMaterial::~CClothMaterial()
{
	if (this->m_RenderParams != nullptr)
	{
		delete (this->m_RenderParams);
		this->m_RenderParams = nullptr;
	}
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
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->SheenColorIsGlossy.w = v ? 1.f : 0.f;
}
void CClothMaterial::SetRoughness(const FLOAT& v)
{
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness.w = v;
}
void CClothMaterial::SetAmbientOcclusion(const FLOAT& v)
{
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->EmissiveAmbientOcclusion.w = v;
}
void CClothMaterial::SetBaseColor(const CustomStruct::CColor& clr)
{
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness.x = clr.r;
	renderParams->BaseColorRoughness.y = clr.g;
	renderParams->BaseColorRoughness.z = clr.b;
}
void CClothMaterial::SetEmissiveColor(const CustomStruct::CColor& clr)
{
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->EmissiveAmbientOcclusion.x = clr.r;
	renderParams->EmissiveAmbientOcclusion.y = clr.g;
	renderParams->EmissiveAmbientOcclusion.z = clr.b;
}
void CClothMaterial::SetSheenColor(const CustomStruct::CColor& clr)
{
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->SheenColorIsGlossy.x = clr.r;
	renderParams->SheenColorIsGlossy.y = clr.g;
	renderParams->SheenColorIsGlossy.z = clr.b;
}
void CClothMaterial::SetSubsurfaceColor(const CustomStruct::CColor& clr)
{
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->SubsurfaceColor.x = clr.r;
	renderParams->SubsurfaceColor.y = clr.g;
	renderParams->SubsurfaceColor.z = clr.b;
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
	return (this->m_RenderParams);
}
#ifdef _DEVELOPMENT_EDITOR
void CClothMaterial::HookSelectedTextureUpdate()
{

}
void CClothMaterial::HookApplyTextureUpdate()
{

}
void CClothMaterial::SelectedClothUpdate()
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
		this->HookSelectedTextureUpdate();
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
			this->HookApplyTextureUpdate();
		}
	}
}
void CClothMaterial::SelectedEditorUpdate()
{
	CClothMaterial::RenderParams* renderParams = static_cast<CClothMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT baseColor[3] = { renderParams->BaseColorRoughness.x, renderParams->BaseColorRoughness.y, renderParams->BaseColorRoughness.z };
	FLOAT emissiveColor[3] = { renderParams->EmissiveAmbientOcclusion.x, renderParams->EmissiveAmbientOcclusion.y,renderParams->EmissiveAmbientOcclusion.z };
	FLOAT sheenColor[3] = { renderParams->SheenColorIsGlossy.x, renderParams->SheenColorIsGlossy.y,renderParams->SheenColorIsGlossy.z };
	FLOAT subsurfaceColor[3] = { renderParams->SubsurfaceColor.x, renderParams->SubsurfaceColor.y, renderParams->SubsurfaceColor.z };

	bool isGlossy = this->m_IsGlossy;
	FLOAT glossy = 0.f;
	FLOAT roughness = renderParams->BaseColorRoughness.w;
	FLOAT ambientOcclusion = renderParams->EmissiveAmbientOcclusion.w;

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

	this->SelectedClothUpdate();

	ImGui::EndChild();

	this->m_IsGlossy = isGlossy;
	glossy = isGlossy ? 1.f : 0.f;
	renderParams->BaseColorRoughness = DirectX::XMFLOAT4(baseColor[0], baseColor[1], baseColor[2], roughness);
	renderParams->EmissiveAmbientOcclusion = DirectX::XMFLOAT4(emissiveColor[0], emissiveColor[1], emissiveColor[2], ambientOcclusion);
	renderParams->SheenColorIsGlossy = DirectX::XMFLOAT4(sheenColor[0], sheenColor[1], sheenColor[2], glossy);
	renderParams->SubsurfaceColor = DirectX::XMFLOAT4(subsurfaceColor[0], subsurfaceColor[1], subsurfaceColor[2], 0.f);
}
#endif



CClothAnisotropicMaterial::CClothAnisotropicMaterial() : CClothMaterial(typeid(CClothAnisotropicMaterial).name(), MaterialType::MATERIAL_TYPE_OPAQUE_FORWARD, sizeof(CClothAnisotropicMaterial::RenderParams), _GClothMaterialInputLayout, 4u, ENGINE_SHADER_CLOTH_ANISOTROPIC_VS, ENGINE_SHADER_CLOTH_ANISOTROPIC_PS)
{
	this->m_AnisotropyStrengthTexture	= NULL;
	this->m_AnisotropyDirectionTexture	= NULL;
	this->m_RenderParams = new CClothAnisotropicMaterial::RenderParams();
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	renderParams->EmissiveAmbientOcclusion		= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	renderParams->SheenColorIsGlossy			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.f);
	renderParams->SubsurfaceColor				= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.f);
	renderParams->AnisotropyStrengthDirection	= DirectX::XMFLOAT4(1.f, 1.f, 0.f, 0.f);
#ifdef _DEVELOPMENT_EDITOR
	this->m_IsGlossy = FALSE;
	this->m_AnisotropyStrengthSelect	= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_AnisotropyDirectionSelect	= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	{
		auto initPathChar = [](CHAR* path, const std::string& str) {
			strcpy_s(path, 512, str.c_str());
		};
		initPathChar(this->m_AnisotropyStrengthPath, "Default White");
		initPathChar(this->m_AnisotropyDirectionPath, "Default White");
	}
#endif
}
CClothAnisotropicMaterial::~CClothAnisotropicMaterial()
{
	if (this->m_RenderParams != nullptr)
	{
		delete (this->m_RenderParams);
		this->m_RenderParams = nullptr;
	}
}
void CClothAnisotropicMaterial::SetAnisotropyStrengthTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AnisotropyStrengthTexture = tex;
	}
}
void CClothAnisotropicMaterial::SetAnisotropyDirectionTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_AnisotropyDirectionTexture = tex;
	}
}
void CClothAnisotropicMaterial::SetAnisotropyStrength(const FLOAT& v)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->AnisotropyStrengthDirection.x = v;
}
void CClothAnisotropicMaterial::SetAnisotropyDirection(const FLOAT& v)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->AnisotropyStrengthDirection.y = v;
}
void CClothAnisotropicMaterial::SetIsGlossyRoughness(const BOOL& v)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->SheenColorIsGlossy.w = v ? 1.f : 0.f;
}
void CClothAnisotropicMaterial::SetRoughness(const FLOAT& v)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness.w = v;
}
void CClothAnisotropicMaterial::SetAmbientOcclusion(const FLOAT& v)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->EmissiveAmbientOcclusion.w = v;
}
void CClothAnisotropicMaterial::SetBaseColor(const CustomStruct::CColor& clr)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness.x = clr.r;
	renderParams->BaseColorRoughness.y = clr.g;
	renderParams->BaseColorRoughness.z = clr.b;
}
void CClothAnisotropicMaterial::SetEmissiveColor(const CustomStruct::CColor& clr)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->EmissiveAmbientOcclusion.x = clr.r;
	renderParams->EmissiveAmbientOcclusion.y = clr.g;
	renderParams->EmissiveAmbientOcclusion.z = clr.b;
}
void CClothAnisotropicMaterial::SetSheenColor(const CustomStruct::CColor& clr)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->SheenColorIsGlossy.x = clr.r;
	renderParams->SheenColorIsGlossy.y = clr.g;
	renderParams->SheenColorIsGlossy.z = clr.b;
}
void CClothAnisotropicMaterial::SetSubsurfaceColor(const CustomStruct::CColor& clr)
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->SubsurfaceColor.x = clr.r;
	renderParams->SubsurfaceColor.y = clr.g;
	renderParams->SubsurfaceColor.z = clr.b;
}
void CClothAnisotropicMaterial::Init()
{
	CClothMaterial::Init();
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
		pathSelect(this->m_AnisotropyStrengthSelect, this->m_AnisotropyStrengthPath, this->m_AnisotropyStrengthTexture, FALSE);
		pathSelect(this->m_AnisotropyDirectionSelect, this->m_AnisotropyDirectionPath, this->m_AnisotropyDirectionTexture, FALSE);
	}
#endif

	{
		auto nullTextureSelect = [](CTexture2D*& texture, CustomStruct::CEngineDefaultTexture2DType type) {
			if (texture == NULL)
			{
				texture = CRenderPipeline::GetDefaultTexture(type);
			}};
		nullTextureSelect(this->m_AlbedoTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_EmissiveTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
	}
}
void CClothAnisotropicMaterial::Bind()const
{
	CClothMaterial::Bind();
	auto bindTexture = [](const CTexture2D* tex, const UINT& slot) {
		if (tex != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(tex->GetShaderResourceView(), slot);
		}};
	bindTexture(this->m_AnisotropyStrengthTexture, 12u);
	bindTexture(this->m_AnisotropyDirectionTexture, 13u);
}
#ifdef _DEVELOPMENT_EDITOR
void CClothAnisotropicMaterial::HookSelectedTextureUpdate()
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
	textureCombo("AnisotropyStrengthTexture", this->m_AnisotropyStrengthSelect);
	texturePathText("AnisotropyStrengthTexturePath", this->m_AnisotropyStrengthSelect, this->m_AnisotropyStrengthPath);
	textureCombo("AnisotropyDirectionTexture", this->m_AnisotropyDirectionSelect);
	texturePathText("AnisotropyDirectionTexturePath", this->m_AnisotropyDirectionSelect, this->m_AnisotropyDirectionPath);
}
void CClothAnisotropicMaterial::HookApplyTextureUpdate()
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
	textureUpdate(this->m_AnisotropyStrengthSelect, this->m_AnisotropyStrengthPath, this->m_AnisotropyStrengthTexture);
	textureUpdate(this->m_AnisotropyDirectionSelect, this->m_AnisotropyDirectionPath, this->m_AnisotropyDirectionTexture);
}
void CClothAnisotropicMaterial::SelectedEditorUpdate()
{
	CClothAnisotropicMaterial::RenderParams* renderParams = static_cast<CClothAnisotropicMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT baseColor[3]			= { renderParams->BaseColorRoughness.x, renderParams->BaseColorRoughness.y, renderParams->BaseColorRoughness.z };
	FLOAT emissiveColor[3]		= { renderParams->EmissiveAmbientOcclusion.x, renderParams->EmissiveAmbientOcclusion.y,renderParams->EmissiveAmbientOcclusion.z };
	FLOAT sheenColor[3]			= { renderParams->SheenColorIsGlossy.x, renderParams->SheenColorIsGlossy.y,renderParams->SheenColorIsGlossy.z };
	FLOAT subsurfaceColor[3]	= { renderParams->SubsurfaceColor.x, renderParams->SubsurfaceColor.y, renderParams->SubsurfaceColor.z };

	bool isGlossy				= this->m_IsGlossy;
	FLOAT glossy				= 0.f;
	FLOAT roughness				= renderParams->BaseColorRoughness.w;
	FLOAT ambientOcclusion		= renderParams->EmissiveAmbientOcclusion.w;
	FLOAT anisotropyStrength	= renderParams->AnisotropyStrengthDirection.x;
	FLOAT anisotropyDirection	= renderParams->AnisotropyStrengthDirection.y;

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
	ImGui::SliderFloat("AnisotropyStrength", &anisotropyStrength, 0.f, 1.f);
	ImGui::SliderFloat("AnisotropyDirection", &anisotropyDirection, 0.f, 1.f);

	this->SelectedClothUpdate();

	ImGui::EndChild();

	this->m_IsGlossy = isGlossy;
	glossy = isGlossy ? 1.f : 0.f;
	renderParams->BaseColorRoughness = DirectX::XMFLOAT4(baseColor[0], baseColor[1], baseColor[2], roughness);
	renderParams->EmissiveAmbientOcclusion = DirectX::XMFLOAT4(emissiveColor[0], emissiveColor[1], emissiveColor[2], ambientOcclusion);
	renderParams->SheenColorIsGlossy = DirectX::XMFLOAT4(sheenColor[0], sheenColor[1], sheenColor[2], glossy);
	renderParams->SubsurfaceColor = DirectX::XMFLOAT4(subsurfaceColor[0], subsurfaceColor[1], subsurfaceColor[2], 0.f);
	renderParams->AnisotropyStrengthDirection = DirectX::XMFLOAT4(anisotropyStrength, anisotropyDirection, 0.f, 0.f);
}
#endif