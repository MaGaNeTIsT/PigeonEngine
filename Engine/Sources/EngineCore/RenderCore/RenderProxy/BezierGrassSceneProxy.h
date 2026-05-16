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
#if 0
	struct EBezierGrassGlobalProxyData
	{
		float4          GlobalWindDirectionStrength;
		float4          WindStrengthRange;
		float4          CurrentTimeParams;
		float4          PreviousTimeParams;
		uint4           NumAllocatedLODMaxNumInstances;
		float4          TileAnchorSize;
		uint4           TileXYNumTiles;
		float4          SubTileSizeBorderSize;
		uint4           SubTileXYNumSubTiles;
		uint4           MaskXYNumInstances;
		float4          DensityScale;
		uint4           HeightMapWorldScaleOffsetBorderPixelSize;
		uint4           LayerBorderPixelSizeDensityBorderPixelSize;
		uint4           LayerTypeElemsBaseCustomTotalNumTypes;
		uint4           LODBodyPart1;
		uint4           LODBodyPart2;
		uint4           IndexOffset1;
		uint4           IndexOffset2;
		uint4           VertexOffset1;
		uint4           VertexOffset2;
		float4          LODDistancesSq1;
		float4          LODDistancesSq2;
	};
#endif
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
		void			SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices);
		void			UpdateProperty(const EBezierGrassProperty& InProperty);
		void			UpdateLayerTypeData(const EBezierGrassLayerTypeData& InLayerData);
		void			UpdateTileParams(const Vector2& InTileAnchor, const Vector2& InTileSize, UINT32 InNumTilesX, UINT32 InNumTilesZ);
		void			UpdateWindParams(const Vector3& InWindDirection, FLOAT InWindStrength);
	public:
		virtual BOOL32	IsRenderValid()const override;
		void			UpdateInstanceResource();
		void			UpdateRenderResource();
		void			BindRenderResource()const;
		void			DispatchComputeShader();
		void			Draw()const;
	protected:
		void			SetupVertexIndexBuffer();
		void			SetupComputeBuffers();
		void			SetupComputeTextures();
		void			SetupComputeConstantBuffer();
		void			SetupLayerTypeBuffer();
		void			SetupShaders();
		void			BindVertexShader()const;
		void			BindPixelShader()const;
		void			BindMeshAndInstanceResource()const;
		void			BindMaterialParameter(const UINT32 InSlot)const;
	protected:
		const EVertexShaderAsset*			VertexShader;
		const EPixelShaderAsset*			PixelShader;
		const EComputeShaderAsset*			ComputeShader;
		BOOL32								bSplitBentPart;
		TArray<UINT32>						BodyParts;
		TArray<FLOAT>						BentBezierTs;
#if 0
		UINT32								VertexOffset[BEZIER_GRASS_LOD_NUM];
		UINT32								IndexOffset[BEZIER_GRASS_LOD_NUM];
		UINT32								IndexCount[BEZIER_GRASS_LOD_NUM];
#endif
		RVertexBufferResource				VertexBuffer;
		RIndexBufferResource				IndexBuffer;
		RBezierGrassMaterialParameter		MaterialParameter;
		EBezierGrassProperty				Property;
		EBezierGrassLayerTypeData			LayerTypeData;
		Vector2								TileAnchor;
		Vector2								TileSize;
		UINT32								NumTilesX;
		UINT32								NumTilesZ;
		Vector3								WindDirection;
		FLOAT								WindStrength;
		RStructuredBuffer					PackedInstanceBuffer;
		RStructuredBuffer					DispatchBuffer;
		RStructuredBuffer					LayerTypeBuffer;
		RStructuredBuffer					TileShuffleBuffer;
		RStructuredBuffer					DrawIndirectBuffer;
		RBufferResource						ComputeConstantBuffer;
		const class ETexture2DAsset*		LandscapeHeightTexture;
		const class ETexture2DAsset*		LayerIndexTexture;
		const class ETexture2DAsset*		DensityTexture;
	protected:
		const PBezierGrassComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RBezierGrassSceneProxy)

	private:
		friend class RSceneRenderer;

	};

};