#pragma once

#include <CoreMinimal.h>
#include <ShaderAsset/ShaderAsset.h>
#include <RenderResource.h>
#include "PrimitiveSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/BezierGrassComponent.h>

namespace PigeonEngine
{

	class EStaticMeshAsset;
	class EShaderProgram;
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
	class RBezierGrassCBufferParameter : public EConstantBufferParameter
	{
	public:
		// The three world matrices a primitive block starts with. Declared here rather than
		// inherited: the shared primitive block base is gone, and this block is on its own until
		// the grass is rebuilt on the new shader system.
		void			AddPrimitiveMaterialParameter() noexcept;

		virtual void	DoSetupVariable() noexcept override;

		void			AddBezierGrassMaterialParameter() noexcept;

		CLASS_SHADER_PARAMETER(RBezierGrassCBufferParameter)
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
		void			BindPrimitiveParameter()const;
	protected:
		// Resolved from the grass material instance, or from the engine fallback while
		// the component carries none. One program drives the vertex and the pixel stage.
		const EShaderProgram*				GraphicsProgram;
		// The construct pass is a different shader asset than the blade pass.
		const EShaderProgram*				ComputeProgram;
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
		RBezierGrassCBufferParameter		CBufferParameter;
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
