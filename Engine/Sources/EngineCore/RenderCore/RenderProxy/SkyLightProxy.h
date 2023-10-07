#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>

namespace PigeonEngine
{

	class RSkyLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RSkyLightSceneProxy(PSkyLightComponent* InComponent);
		void			SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset);
	public:
		void			UpdateMeshAsset(const ETextureCube* InTextureCubeAsset);
		void			UpdateRenderResource();
		void			BindRenderResource()const;
		void			Draw()const;
	protected:
		void			SetupShaders();
		void			BindVertexShader()const;
		void			BindPixelShader()const;
		void			BindMeshResource()const;
		void			BindMaterialParameter(const UINT32 InSlot)const;
	protected:
		const ETextureCube*			TextureCubeAsset;
	protected:
		const PSkyLightComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RSkyLightSceneProxy)

	};

};