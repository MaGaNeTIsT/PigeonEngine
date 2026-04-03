#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <RenderDevice/DeviceD3D11.h>
#include <Shader/ShaderParameter.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>

namespace PigeonEngine
{

	class RPrimitiveMaterialParameter : public EMaterialParameter
	{
	public:
		void	AddPrimitiveMaterialParameter();

		CLASS_MATERIAL_PARAMETER(RPrimitiveMaterialParameter)
	};

	class RPrimitiveSceneProxy : public RBaseSceneProxy
	{
	public:
		virtual BOOL32	IsRenderValid()const;
		BOOL32			IsSceneProxyMovable()const;
		BOOL32			IsSceneProxyCastShadow()const;
		BOOL32			IsSceneProxyReceiveShadow()const;
	public:
		void			SetPrimitiveSettings(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
		void			UpdatePrimitiveMatrices(const ERenderPrimitiveMatrices& InMatrices);
	protected:
		BOOL32			IsMovable;
		BOOL32			IsCastShadow;
		BOOL32			IsReceiveShadow;

		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

};