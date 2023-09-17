#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
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
		void			SetPrimitiveSettings(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
		void			UpdatePrimitiveMatrices(const ERenderPrimitiveMatrices& InMatrices);
		virtual void	UpdateRenderResource();
		virtual void	BindRenderResource();
	public:
		const RBufferResource&	GetPrimitiveCBuffer()const;
	protected:
		template<typename _TStructType>
		void CreatePrimitiveCBuffer();
		template<typename _TStructType>
		void UploadPrimitiveCBuffer(const _TStructType* InStruct);
		void BindPrimitiveCBuffer(const UINT32 InSlot);
	protected:
		BOOL32			IsMovable;
		BOOL32			IsCastShadow;
		BOOL32			IsReceiveShadow;
#if _EDITOR_ONLY
		UINT32			PrimitiveCBufferSize;
#endif
		RBufferResource	PrimitiveCBuffer;

		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

};