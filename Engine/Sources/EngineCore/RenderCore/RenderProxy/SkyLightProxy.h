#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <Shader/ShaderParameter.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>

namespace PigeonEngine
{

	class ETextureCubeAsset;
	class EStaticMeshAsset;
	class EVertexShaderAsset;
	class EPixelShaderAsset;

	class RSkyLightMaterialParameter : public EMaterialParameter
	{
	public:
		virtual void	SetupParameters()override;

		CLASS_MATERIAL_PARAMETER(RSkyLightMaterialParameter)
	};

	class RSkyLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RSkyLightSceneProxy(PSkyLightComponent* InComponent);
		void			SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset);
	public:
		void			UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset);
		void			UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams);
		void			UpdateMatrices(const ERenderSkyLightMatrices& InMatrices);
		void			UpdateRenderResource();
	public:
		BOOL32			IsRenderValid()const;
		void			BindRenderResource()const;
		void			Draw()const;
	protected:
		void			BindVertexShader()const;
		void			BindPixelShader()const;
		void			BindMeshResource()const;
		void			BindMaterialParameter(const UINT32 InSlot)const;
	protected:
		RSkyLightMaterialParameter	SkyLightMaterialParameter;
		const ETextureCubeAsset*	TextureCubeAsset;
		Color3						AdjustColor;
		FLOAT						AdjustIntensity;
	protected:
		const PSkyLightComponent*	Component;
	protected:
		static void		CreateSkyDomeAsset();
	protected:
		static const EStaticMeshAsset*		SkyDomeMeshAsset;
		static const EVertexShaderAsset*	SkyDomeVertexShader;
		static const EPixelShaderAsset*		SkyDomePixelShader;

		RENDER_PROXY_CLASS_BODY(RSkyLightSceneProxy)

	};

};