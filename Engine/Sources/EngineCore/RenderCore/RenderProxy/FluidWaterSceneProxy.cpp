#include "FluidWaterSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <Shader/ShaderAssetBinding.h>
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RFluidWaterSceneProxy, RPrimitiveSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RFluidWaterCBufferParameter::AddPrimitiveMaterialParameter() noexcept
	{
		const EVariableFormat MatrixFormat = MakeShaderVariableFormat(
			EVariableFormatPrefixType::VARIABLE_FORMAT_PREFIX_MATRIX, EVariableFormatSuffixType::VARIABLE_FORMAT_SUFFIX_4x4);
		AddStructVariable(PE_TEXT("_WorldMatrix"), MatrixFormat, 1u);
		AddStructVariable(PE_TEXT("_WorldInvMatrix"), MatrixFormat, 1u);
		AddStructVariable(PE_TEXT("_WorldInvTransposeMatrix"), MatrixFormat, 1u);
	}
	void RFluidWaterCBufferParameter::DoSetupVariable() noexcept
	{
	AddPrimitiveMaterialParameter();
	}

	RFluidWaterSceneProxy::RFluidWaterSceneProxy(PFluidWaterComponent* InComponent)
	: MaterialAsset(nullptr), Component(InComponent)
	{
	PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create fluid water scene proxy failed"), (!!Component));
	}

	RFluidWaterSceneProxy::RFluidWaterSceneProxy()
	: MaterialAsset(nullptr), Component(nullptr)
	{
	}

	RFluidWaterSceneProxy::RFluidWaterSceneProxy(const RFluidWaterSceneProxy& Other)
		: RPrimitiveSceneProxy(Other), MaterialAsset(Other.MaterialAsset), Component(Other.Component)
	{
	}

	RFluidWaterSceneProxy::~RFluidWaterSceneProxy()
	{
	}

	void RFluidWaterSceneProxy::SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EShaderInstance* InMaterialAsset)
	{
		UpdateMaterialAsset(InMaterialAsset);

		SetPrimitiveSettings(InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);

		CBufferParameter.SetupParameters();

	}

	BOOL32 RFluidWaterSceneProxy::IsRenderValid()const
	{
		return TRUE;
	}

	void RFluidWaterSceneProxy::UpdateMaterialAsset(const EShaderInstance* InMaterialAsset)
	{
		MaterialAsset = InMaterialAsset;
		// The program is not cached here any more: it is read through the instance, so a shader
		// reload re-points the instance and this proxy follows it without a refresh entry point.
		SetupShaders();
		SetupMaterialResources();
	}

	const EShaderProgram* RFluidWaterSceneProxy::GetMaterialProgram()const
	{
		// const_cast for the same reason EShaderInstance::GetProgram() const does it: the resolution
		// is a cache inside the instance, not observable state of this proxy.
		return (MaterialAsset ? const_cast<EShaderInstance*>(MaterialAsset)->GetProgram() : nullptr);
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
