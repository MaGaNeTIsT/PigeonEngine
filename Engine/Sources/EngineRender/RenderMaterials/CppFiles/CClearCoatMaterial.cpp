#include "../Headers/CClearCoatMaterial.h"
#include "../../RenderBase/Headers/CRenderStructCommon.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderBase/Headers/CRenderDevice.h"
#include "../../RenderBase/Headers/CRenderPipeline.h"

const static CustomStruct::CRenderInputLayoutDesc _GDefaultLitMaterialInputLayout[4u] = {
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_NORMAL),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TANGENT),
	CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };

CClearCoatMaterial::CClearCoatMaterial() : CDefaultLitMaterial(typeid(CClearCoatMaterial).name(), MaterialType::MATERIAL_TYPE_OPAQUE_FORWARD, sizeof(CClearCoatMaterial::RenderParams), _GDefaultLitMaterialInputLayout, 4u, ENGINE_DEFAULT_CLEARCOAT_SHADER_FORWARD_VS, ENGINE_DEFAULT_CLEARCOAT_SHADER_FORWARD_PS)
{
#ifdef _DEVELOPMENT_EDITOR
	this->m_ClearCoatStrengthSelect		= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_ClearCoatRoughnessSelect	= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE;
	this->m_ClearCoatNormalSelect		= CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP;
	{
		auto initPathChar = [](CHAR* path, const std::string& str) {
			strcpy_s(path, 512, str.c_str()); };
		initPathChar(this->m_ClearCoatStrengthPath, "Default White");
		initPathChar(this->m_ClearCoatRoughnessPath, "Default White");
		initPathChar(this->m_ClearCoatNormalPath, "Default Bump");
	}
#endif
	this->m_ClearCoatStrengthTexture	= NULL;
	this->m_ClearCoatRoughnessTexture	= NULL;
	this->m_ClearCoatNormalTexture		= NULL;

	if (this->m_RenderParams != nullptr)
	{
		delete (this->m_RenderParams);
		this->m_RenderParams = nullptr;
	}
	this->m_RenderParams = new CClearCoatMaterial::RenderParams();
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness				= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	renderParams->EmissiveAmbientOcclusion			= DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	renderParams->MetallicnessReflectanceIsGlossy	= DirectX::XMFLOAT4(0.f, 1.f, 0.f, 0.f);
	renderParams->ClearCoatStrengthRoughness		= DirectX::XMFLOAT4(0.5f, 1.f, 0.f, 0.f);
}
CClearCoatMaterial::~CClearCoatMaterial()
{
	if (this->m_RenderParams != nullptr)
	{
		delete (this->m_RenderParams);
		this->m_RenderParams = nullptr;
	}
}
void CClearCoatMaterial::SetClearCoatStrengthTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_ClearCoatStrengthTexture = tex;
#ifdef _DEVELOPMENT_EDITOR
		this->m_ClearCoatStrengthSelect = -2;
		strcpy_s(this->m_ClearCoatStrengthPath, 512, tex->GetName().c_str());
#endif
	}
}
void CClearCoatMaterial::SetClearCoatRoughnessTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_ClearCoatRoughnessTexture = tex;
#ifdef _DEVELOPMENT_EDITOR
		this->m_ClearCoatRoughnessSelect = -2;
		strcpy_s(this->m_ClearCoatRoughnessPath, 512, tex->GetName().c_str());
#endif
	}
}
void CClearCoatMaterial::SetClearCoatNormalTexture(CTexture2D* tex)
{
	if (tex != NULL)
	{
		this->m_ClearCoatNormalTexture = tex;
#ifdef _DEVELOPMENT_EDITOR
		this->m_ClearCoatNormalSelect = -2;
		strcpy_s(this->m_ClearCoatNormalPath, 512, tex->GetName().c_str());
#endif
	}
}
CTexture2D* CClearCoatMaterial::GetClearCoatStrengthTexture()const
{
	return (this->m_ClearCoatStrengthTexture);
}
CTexture2D* CClearCoatMaterial::GetClearCoatRoughnessTexture()const
{
	return (this->m_ClearCoatRoughnessTexture);
}
CTexture2D* CClearCoatMaterial::GetClearCoatNormalTexture()const
{
	return (this->m_ClearCoatNormalTexture);
}
void CClearCoatMaterial::SetClearCoatStrength(const FLOAT& v)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->ClearCoatStrengthRoughness.x = v;
}
void CClearCoatMaterial::SetClearCoatRoughness(const FLOAT& v)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->ClearCoatStrengthRoughness.y = v;
}
void CClearCoatMaterial::SetIsGlossyRoughness(const BOOL& v)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->MetallicnessReflectanceIsGlossy.z = v ? 1.f : 0.f;
}
void CClearCoatMaterial::SetRoughness(const FLOAT& v)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness.w = v;
}
void CClearCoatMaterial::SetMetallicness(const FLOAT& v)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->MetallicnessReflectanceIsGlossy.x = v;
}
void CClearCoatMaterial::SetReflectance(const FLOAT& v)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->MetallicnessReflectanceIsGlossy.y = v;
}
void CClearCoatMaterial::SetAmbientOcclusion(const FLOAT& v)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->EmissiveAmbientOcclusion.w = v;
}
void CClearCoatMaterial::SetBaseColor(const CustomStruct::CColor& clr)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->BaseColorRoughness.x = clr.r;
	renderParams->BaseColorRoughness.y = clr.g;
	renderParams->BaseColorRoughness.z = clr.b;
}
void CClearCoatMaterial::SetEmissiveColor(const CustomStruct::CColor& clr)
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	renderParams->EmissiveAmbientOcclusion.x = clr.r;
	renderParams->EmissiveAmbientOcclusion.y = clr.g;
	renderParams->EmissiveAmbientOcclusion.z = clr.b;
}
FLOAT CClearCoatMaterial::GetClearCoatStrength()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT v = renderParams->ClearCoatStrengthRoughness.x;
	return v;
}
FLOAT CClearCoatMaterial::GetClearCoatRoughness()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT v = renderParams->ClearCoatStrengthRoughness.y;
	return v;
}
BOOL CClearCoatMaterial::GetIsGlossyRoughness()const
{
	return (this->m_IsGlossy);
}
FLOAT CClearCoatMaterial::GetRoughness()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT v = renderParams->BaseColorRoughness.w;
	return v;
}
FLOAT CClearCoatMaterial::GetMetallicness()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT v = renderParams->MetallicnessReflectanceIsGlossy.x;
	return v;
}
FLOAT CClearCoatMaterial::GetReflectance()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT v = renderParams->MetallicnessReflectanceIsGlossy.y;
	return v;
}
FLOAT CClearCoatMaterial::GetAmbientOcclusion()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT v = renderParams->EmissiveAmbientOcclusion.w;
	return v;
}
CustomStruct::CColor CClearCoatMaterial::GetBaseColor()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	return (CustomStruct::CColor(renderParams->BaseColorRoughness.x, renderParams->BaseColorRoughness.y, renderParams->BaseColorRoughness.z));
}
CustomStruct::CColor CClearCoatMaterial::GetEmissiveColor()const
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	return (CustomStruct::CColor(renderParams->EmissiveAmbientOcclusion.x, renderParams->EmissiveAmbientOcclusion.y, renderParams->EmissiveAmbientOcclusion.z));
}
void CClearCoatMaterial::Init()
{
	CDefaultLitMaterial::Init();
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
		pathSelect(this->m_ClearCoatStrengthSelect, this->m_ClearCoatStrengthPath, this->m_ClearCoatStrengthTexture, FALSE);
		pathSelect(this->m_ClearCoatRoughnessSelect, this->m_ClearCoatRoughnessPath, this->m_ClearCoatRoughnessTexture, FALSE);
		pathSelect(this->m_ClearCoatNormalSelect, this->m_ClearCoatNormalPath, this->m_ClearCoatNormalTexture, FALSE);
	}
#endif

	{
		auto nullTextureSelect = [](CTexture2D*& texture, CustomStruct::CEngineDefaultTexture2DType type) {
			if (texture == NULL)
			{
				texture = CRenderPipeline::GetDefaultTexture(type);
			}};
		nullTextureSelect(this->m_ClearCoatStrengthTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_ClearCoatRoughnessTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
		nullTextureSelect(this->m_ClearCoatNormalTexture, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP);
	}
}
void CClearCoatMaterial::Bind()const
{
	CDefaultLitMaterial::Bind();
	auto bindTexture = [](const CTexture2D* tex, const UINT& slot, CustomStruct::CEngineDefaultTexture2DType type) {
		if (tex != NULL)
		{
			CRenderDevice::BindPSShaderResourceView(tex->GetShaderResourceView(), slot);
		}
		else
		{
			CRenderDevice::BindPSShaderResourceView(CRenderPipeline::GetDefaultTexture(type)->GetShaderResourceView(), slot);
		}};
	bindTexture(this->m_ClearCoatStrengthTexture, 12u, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
	bindTexture(this->m_ClearCoatRoughnessTexture, 13u, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE);
	bindTexture(this->m_ClearCoatNormalTexture, 14u, CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_BUMP);
}
#ifdef _DEVELOPMENT_EDITOR
void CClearCoatMaterial::HookSelectedTextureUpdate()
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
	textureCombo("ClearCoatStrengthTexture", this->m_ClearCoatStrengthSelect);
	texturePathText("ClearCoatStrengthTexturePath", this->m_ClearCoatStrengthSelect, this->m_ClearCoatStrengthPath);
	textureCombo("ClearCoatRoughnessTexture", this->m_ClearCoatRoughnessSelect);
	texturePathText("ClearCoatRoughnessTexturePath", this->m_ClearCoatRoughnessSelect, this->m_ClearCoatRoughnessPath);
	textureCombo("ClearCoatNormalTexture", this->m_ClearCoatNormalSelect);
	texturePathText("ClearCoatNormalTexturePath", this->m_ClearCoatNormalSelect, this->m_ClearCoatNormalPath);
}
void CClearCoatMaterial::HookApplyTextureUpdate()
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
		}};
	textureUpdate(this->m_ClearCoatStrengthSelect, this->m_ClearCoatStrengthPath, this->m_ClearCoatStrengthTexture, FALSE);
	textureUpdate(this->m_ClearCoatRoughnessSelect, this->m_ClearCoatRoughnessPath, this->m_ClearCoatRoughnessTexture, FALSE);
	textureUpdate(this->m_ClearCoatNormalSelect, this->m_ClearCoatNormalPath, this->m_ClearCoatNormalTexture, FALSE);
}
void CClearCoatMaterial::SelectedEditorUpdate()
{
	CClearCoatMaterial::RenderParams* renderParams = static_cast<CClearCoatMaterial::RenderParams*>(this->m_RenderParams);
	FLOAT baseColor[3]		= { renderParams->BaseColorRoughness.x, renderParams->BaseColorRoughness.y, renderParams->BaseColorRoughness.z };
	FLOAT EmissiveColor[3]	= { renderParams->EmissiveAmbientOcclusion.x, renderParams->EmissiveAmbientOcclusion.y, renderParams->EmissiveAmbientOcclusion.z };

	bool isGlossy				= this->m_IsGlossy;
	FLOAT glossy				= renderParams->MetallicnessReflectanceIsGlossy.z;
	FLOAT roughness				= renderParams->BaseColorRoughness.w;
	FLOAT ambientOcclusion		= renderParams->EmissiveAmbientOcclusion.w;
	FLOAT metallicness			= renderParams->MetallicnessReflectanceIsGlossy.x;
	FLOAT reflectance			= renderParams->MetallicnessReflectanceIsGlossy.y;
	FLOAT clearCoatStrength		= renderParams->ClearCoatStrengthRoughness.x;
	FLOAT clearCoatRoughness	= renderParams->ClearCoatStrengthRoughness.y;

	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(400, 270), false, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);

	{
		ImGui::Text("BaseColor");
		ImGui::ColorEdit3("BaseColor", baseColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

		ImGui::Text("EmissiveColor");
		ImGui::ColorEdit3("EmissiveColor", EmissiveColor, ImGuiColorEditFlags_::ImGuiColorEditFlags_NoLabel);

		ImGui::Checkbox("IsGlossy", &isGlossy);
		ImGui::SliderFloat("Roughness", &roughness, 0.f, 1.f);
		ImGui::SliderFloat("Metallicness", &metallicness, 0.f, 1.f);
		ImGui::SliderFloat("Reflectance", &reflectance, 0.f, 1.f);
		ImGui::SliderFloat("AmbientOcclusion", &ambientOcclusion, 0.f, 1.f);
		ImGui::SliderFloat("ClearCoatStrength", &clearCoatStrength, 0.f, 1.f);
		ImGui::SliderFloat("ClearCoatRoughness", &clearCoatRoughness, 0.f, 1.f);
	}
	this->SelectedDefaultLitUpdate();

	ImGui::EndChild();

	this->m_IsGlossy = isGlossy;
	glossy = isGlossy ? 1.f : 0.f;
	renderParams->BaseColorRoughness				= DirectX::XMFLOAT4(baseColor[0], baseColor[1], baseColor[2], roughness);
	renderParams->EmissiveAmbientOcclusion			= DirectX::XMFLOAT4(EmissiveColor[0], EmissiveColor[1], EmissiveColor[2], ambientOcclusion);
	renderParams->MetallicnessReflectanceIsGlossy	= DirectX::XMFLOAT4(metallicness, reflectance, glossy, 0.f);
	renderParams->MetallicnessReflectanceIsGlossy	= DirectX::XMFLOAT4(clearCoatStrength, clearCoatRoughness, 0.f, 0.f);
}
#endif