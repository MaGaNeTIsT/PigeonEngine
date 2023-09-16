#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>

namespace PigeonEngine
{

	class RPrimitiveSceneProxy : public RBaseSceneProxy
	{
	public:
		virtual BOOL32	IsRenderValid()const;
		BOOL32			IsSceneProxyMovable()const;
		BOOL32			IsSceneProxyCastShadow()const;
		BOOL32			IsSceneProxyReceiveShadow()const;
	public:
		void	SetPrimitiveSettings(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
		void	UpdatePrimitiveMatrices(const ERenderPrimitiveMatrices& InMatrices);
	protected:
		BOOL32	IsMovable;
		BOOL32	IsCastShadow;
		BOOL32	IsReceiveShadow;

		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

};