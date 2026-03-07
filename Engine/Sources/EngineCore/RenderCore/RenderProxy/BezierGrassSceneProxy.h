#pragma once

#include <CoreMinimal.h>
#include <ShaderAsset/ShaderAsset.h>
#include <RenderResource.h>
#include "PrimitiveSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/BezierGrassComponent.h>

namespace PigeonEngine
{

	class EStaticMeshAsset;
	class EVertexShaderAsset;
	class EPixelShaderAsset;

	class RBezierGrassMaterialParameter : public RPrimitiveMaterialParameter
	{
	public:
		virtual void	SetupParameters()override;

		void			AddBezierGrassMaterialParameter();

		CLASS_MATERIAL_PARAMETER(RBezierGrassMaterialParameter)
	};

	class RBezierGrassSceneProxy : public RPrimitiveSceneProxy
	{
	public:
		RBezierGrassSceneProxy(PBezierGrassComponent* InComponent);
		void			SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices);
		void			UpdateProperty(const EBezierGrassProperty& InProperty);
		void			UpdateInstanceData(TArray<EBezierGrassInstanceData>&& InInstanceData);
	public:
		virtual BOOL32	IsRenderValid()const override;
		void			UpdateInstanceResource();
		void			UpdateRenderResource();
		void			BindRenderResource()const;
		void			Draw()const;
	protected:
		void			SetupVertexIndexBuffer();
		void			SetupShaders();
		void			BindVertexShader()const;
		void			BindPixelShader()const;
		void			BindMeshAndInstanceResource()const;
		void			BindMaterialParameter(const UINT32 InSlot)const;
	protected:
		const EVertexShaderAsset*			VertexShader;
		const EPixelShaderAsset*			PixelShader;
#if _EDITOR_ONLY
		const EComputeShaderAsset*			DebugComputeShader;
		const EComputeShaderAsset*			DebugScreenComputeShader;
#endif
		UINT32								VertexOffset[BEZIER_GRASS_LOD_NUM];
		UINT32								IndexOffset[BEZIER_GRASS_LOD_NUM];
		UINT32								IndexCount[BEZIER_GRASS_LOD_NUM];
		RVertexBufferResource				VertexBuffer;
		RIndexBufferResource				IndexBuffer;
		RBezierGrassMaterialParameter		MaterialParameter;
		EBezierGrassProperty				Property;
		TArray<EBezierGrassInstanceData>	InstanceData;
		RStructuredBuffer					InstanceBuffer;
	protected:
		const PBezierGrassComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RBezierGrassSceneProxy)

	private:
		friend class RSceneRenderer;

	};

};