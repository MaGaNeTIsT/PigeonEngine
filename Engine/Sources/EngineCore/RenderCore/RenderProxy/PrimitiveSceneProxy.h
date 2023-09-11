#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"

namespace PigeonEngine
{

	class PPrimitiveComponent;
	class RScene;

	class RPrimitiveSceneProxy : public RBaseSceneProxy
	{
	public:
		virtual BOOL32	IsRenderValid()const;
		BOOL32			IsSceneProxyMovable()const;
		BOOL32			IsSceneProxyCastShadow()const;
		BOOL32			IsSceneProxyReceiveShadow()const;
	protected:
		void	SetupSceneProxy(PPrimitiveComponent* InComponent, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
	protected:
		BOOL32	IsMovable;
		BOOL32	IsCastShadow;
		BOOL32	IsReceiveShadow;

		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

};