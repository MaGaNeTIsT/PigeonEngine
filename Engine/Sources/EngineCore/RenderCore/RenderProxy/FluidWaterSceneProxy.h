#pragma once

#include <CoreMinimal.h>
#include "PrimitiveSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/FluidWaterComponent.h>

namespace PigeonEngine
{

	class EShaderInstance;
	class EShaderProgram;

	class RFluidWaterCBufferParameter : public EConstantBufferParameter
	{
	public:
		// The three world matrices a primitive block starts with. Declared here rather than
		// inherited: the shared primitive block base is gone, and this block is on its own until
		// the water is rebuilt on the new shader system.
		void	AddPrimitiveMaterialParameter() noexcept;
		virtual void	DoSetupVariable() noexcept override;

		CLASS_SHADER_PARAMETER(RFluidWaterCBufferParameter)
	};

	class RFluidWaterSceneProxy : public RPrimitiveSceneProxy
	{
	public:
		RFluidWaterSceneProxy(PFluidWaterComponent* InComponent);
		void			SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EShaderInstance* InMaterialAsset);
	public:
		virtual BOOL32	IsRenderValid()const override;
		void			UpdateMaterialAsset(const EShaderInstance* InMaterialAsset);
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
		// The material of this water surface: one shader instance, never owned by the proxy, so two
		// proxies can share one instance.
		const EShaderInstance*			MaterialAsset;
		/*
		* The program the material resolved to, read through the instance instead of cached here: the
		* instance is the stable handle across a shader reload (the process wide cache re-points it to
		* the rebuilt asset), so asking it for the program is what keeps this proxy on the current one.
		* A cached pointer would strand the proxy on the object that was replaced.
		*/
		PE_NODISCARD const EShaderProgram*	GetMaterialProgram()const;
		RFluidWaterCBufferParameter		CBufferParameter;
	protected:
		const PFluidWaterComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RFluidWaterSceneProxy)

	};

};
