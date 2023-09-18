#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <RenderDevice/DeviceD3D11.h>
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
		void CreatePrimitiveCBuffer()
		{
#if _EDITOR_ONLY
			PrimitiveCBufferSize = sizeof(_TStructType);
#endif
			RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(PrimitiveCBuffer.Buffer,
				RBufferDesc(sizeof(_TStructType), RBindFlagType::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
		}
		template<typename _TStructType>
		void UploadPrimitiveCBuffer(const _TStructType* InStruct)
		{
#if _EDITOR_ONLY
			if (PrimitiveCBufferSize != sizeof(_TStructType) || (!(PrimitiveCBuffer.IsRenderResourceValid())))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Upload primitive constant buffer failed(buffer size is not matched or buffer is null)."));
				return;
			}
#endif
			RDeviceD3D11::GetDeviceSingleton()->UploadBuffer(PrimitiveCBuffer.Buffer, InStruct);
		}
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