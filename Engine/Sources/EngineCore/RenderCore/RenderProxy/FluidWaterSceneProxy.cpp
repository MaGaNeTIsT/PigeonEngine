#include "FluidWaterSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <MaterialAsset/MaterialAsset.h>
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RFluidWaterSceneProxy, RPrimitiveSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RFluidWaterMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddPrimitiveMaterialParameter();
		EndSetupParameter();
		CreateBuffer();
	}

	RFluidWaterSceneProxy::RFluidWaterSceneProxy(PFluidWaterComponent* InComponent)
		: VertexShader(nullptr), PixelShader(nullptr), MaterialAsset(nullptr), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create fluid water scene proxy failed"), (!!Component));
	}

	RFluidWaterSceneProxy::RFluidWaterSceneProxy()
		: VertexShader(nullptr), PixelShader(nullptr), MaterialAsset(nullptr), Component(nullptr)
	{
	}

	RFluidWaterSceneProxy::RFluidWaterSceneProxy(const RFluidWaterSceneProxy& Other)
		: RPrimitiveSceneProxy(Other), VertexShader(Other.VertexShader), PixelShader(Other.PixelShader), MaterialAsset(Other.MaterialAsset), Component(Other.Component)
	{
	}

	RFluidWaterSceneProxy::~RFluidWaterSceneProxy()
	{
	}

	void RFluidWaterSceneProxy::SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EMaterialAsset* InMaterialAsset)
	{
		MaterialAsset = InMaterialAsset;
		SetupShaders();
		SetupMaterialResources();

		SetPrimitiveSettings(InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);

		MaterialParameter.SetupParameters();

	}

	BOOL32 RFluidWaterSceneProxy::IsRenderValid()const
	{
		return TRUE;
	}

	void RFluidWaterSceneProxy::UpdateMaterialAsset(const EMaterialAsset* InMaterialAsset)
	{
		MaterialAsset = InMaterialAsset;
		VertexShader = nullptr;
		PixelShader = nullptr;
		SetupShaders();
		SetupMaterialResources();
	}

	void RFluidWaterSceneProxy::UpdateMaterialCBData(const TArray<TArray<BYTE>>& InCBDatas)
	{

	}

	void RFluidWaterSceneProxy::BindRenderResource()const
	{
		BindVertexShader();
		BindPixelShader();
		BindMaterialParameter(0);
		BindMaterialCBs();
		BindMaterialTextures();
	}

	void RFluidWaterSceneProxy::Draw()const
	{
		// TODO: Implement fluid water drawing logic
	}

	void RFluidWaterSceneProxy::SetupShaders()
	{

	}

	void RFluidWaterSceneProxy::SetupMaterialResources()
	{

	}

	void RFluidWaterSceneProxy::BindVertexShader()const
	{

	}

	void RFluidWaterSceneProxy::BindPixelShader()const
	{

	}

	void RFluidWaterSceneProxy::BindMaterialParameter(const UINT32 InSlot)const
	{

	}

	void RFluidWaterSceneProxy::BindMaterialCBs()const
	{

	}

	void RFluidWaterSceneProxy::BindMaterialTextures()const
	{

	}

}
