#pragma once

#include <CoreMinimal.h>
#include "PrimitiveSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/FluidWaterComponent.h>

namespace PigeonEngine
{

	class EVertexShaderAsset;
	class EPixelShaderAsset;
	class EMaterialAsset;

	class RFluidWaterMaterialParameter : public RPrimitiveMaterialParameter
	{
	public:
		virtual void	SetupParameters()override;

		CLASS_MATERIAL_PARAMETER(RFluidWaterMaterialParameter)
	};

	class RFluidWaterSceneProxy : public RPrimitiveSceneProxy
	{
	public:
		RFluidWaterSceneProxy(PFluidWaterComponent* InComponent);
		void			SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EMaterialAsset* InMaterialAsset);
	public:
		virtual BOOL32	IsRenderValid()const override;
		void			UpdateMaterialAsset(const EMaterialAsset* InMaterialAsset);
		void			UpdateMaterialCBData(const TArray<TArray<BYTE>>& InCBDatas);
		void			BindRenderResource()const;
		void			Draw()const;
	protected:
		void			SetupShaders();
		void			SetupMaterialResources();
		void			BindVertexShader()const;
		void			BindPixelShader()const;
		void			BindMaterialParameter(const UINT32 InSlot)const;
		void			BindMaterialCBs()const;
		void			BindMaterialTextures()const;
	protected:
		const EVertexShaderAsset*		VertexShader;
		const EPixelShaderAsset*		PixelShader;
		const EMaterialAsset*			MaterialAsset;
		RFluidWaterMaterialParameter	MaterialParameter;
		TArray<RBufferResource>			MaterialCBBuffers;
	protected:
		const PFluidWaterComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RFluidWaterSceneProxy)

	};

};
