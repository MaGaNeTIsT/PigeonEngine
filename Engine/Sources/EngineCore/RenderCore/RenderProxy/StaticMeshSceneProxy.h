#pragma once

#include <CoreMinimal.h>
#include "MeshSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

namespace PigeonEngine
{

	class RStaticMeshSceneProxy : public RMeshSceneProxy
	{
	public:
		RStaticMeshSceneProxy(PStaticMeshComponent* InComponent);
		void			SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset);		
	public:
		void			UpdateMeshAsset(const EStaticMeshAsset* InMeshAsset);
		virtual void	UpdateRenderResource()override;
		virtual void	BindRenderResource()override;
	protected:
		struct RStaticMeshCBufferData
		{
		public:
			RStaticMeshCBufferData() = default;
			RStaticMeshCBufferData(const RStaticMeshCBufferData& Other)
			{
				RENDER_PER_OBJECT_TRANSFORM_COPY(Other)
			}
			RStaticMeshCBufferData& operator=(const RStaticMeshCBufferData& Other)
			{
				RENDER_PER_OBJECT_TRANSFORM_COPY(Other)
				return (*this);
			}
		public:
			RENDER_PER_OBJECT_TRANSFORM
		};
	protected:
		const EStaticMeshAsset*		MeshAsset;
	protected:
		const PStaticMeshComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RStaticMeshSceneProxy)

	};

};