#include "SkyLightProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSkyLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RSkyLightMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_WorldMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_WorldInvMatrix"));
		AddParameter<Vector3, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_LightAdjustColor"));
		AddParameter<FLOAT, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT1>(("_LightAdjustIntensity"));
		EndSetupParameter();
		CreateBuffer();
	}

	RSkyLightSceneProxy::RSkyLightSceneProxy(PSkyLightComponent* InComponent)
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create directional light scene proxy failed"), (!!Component));
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy()
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f), Component(nullptr)
	{
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy(const RSkyLightSceneProxy& Other)
		: RBaseSceneProxy(Other), TextureCubeAsset(Other.TextureCubeAsset), AdjustColor(Other.AdjustColor), AdjustIntensity(Other.AdjustIntensity), Component(Other.Component)
	{
	}
	RSkyLightSceneProxy::~RSkyLightSceneProxy()
	{
	}
	void RSkyLightSceneProxy::SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset)
	{
		UpdateTextureCubeAsset(InTextureCubeAsset);
		UpdateSkyLightParams(InSkyLightParams);
		UpdateMatrices(InMatrices);

		SkyLightMaterialParameter.SetupParameters();

		UpdateRenderResource();
	}
	void RSkyLightSceneProxy::UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset)
	{
		TextureCubeAsset = InTextureCubeAsset;
	}
	void RSkyLightSceneProxy::UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams)
	{
		AdjustColor		= InSkyLightParams.LightAdjustColor;
		AdjustIntensity	= InSkyLightParams.LightAdjustIntensity;
	}
	void RSkyLightSceneProxy::UpdateMatrices(const ERenderSkyLightMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, InMatrices.WorldScaling);
	}
	void RSkyLightSceneProxy::UpdateRenderResource()
	{
		FLOAT LightAdjustIntensity = TranslateUploadBaseType(AdjustIntensity);
		SkyLightMaterialParameter["_WorldMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix());
		SkyLightMaterialParameter["_WorldInvMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix().Inverse());
		SkyLightMaterialParameter["_LightAdjustColor"] = &TranslateUploadColorType(AdjustColor);
		SkyLightMaterialParameter["_LightAdjustIntensity"] = &LightAdjustIntensity;
		SkyLightMaterialParameter.UploadBuffer();
	}

};