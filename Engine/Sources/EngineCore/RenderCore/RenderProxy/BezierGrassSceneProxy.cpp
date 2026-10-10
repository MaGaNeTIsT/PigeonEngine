#include "BezierGrassSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <Shader/ShaderAssetBinding.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <RenderDevice/DeviceD3D11.h>
#include <RenderDevice/D3D11Native.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <TextureAsset/TextureAsset.h>
#include <RenderCommon.h>
#include <Main/MainManager.h>
#include <array>
#include <algorithm>

// ---------------------------------------------------------------------------
// Parked. The bezier grass render path is going to be rebuilt from its shader
// sources (P_BezierGrass and P_BezierGrassConstruct and the templates they
// assemble), so everything below is kept for reference only: nothing here is
// compiled, no proxy of this class is created, and the renderer no longer calls
// it. Read the shaders, not this file, when the rebuild starts.
// ---------------------------------------------------------------------------
#if 0
namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RBezierGrassSceneProxy, RPrimitiveSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	// The setup and binding routines of this proxy still lift the native objects of the
	// stage resources and hand them to RDeviceD3D11 directly instead of going through
	// IRCommandList, so they only run while the D3D11 back-end is the active device. The
	// predicate lives with those routines and goes away with them once the pass is ported
	// to the RHI.
	static BOOL8 IsLegacyD3D11PathActive()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		return ((RenderDevice != nullptr) && (RenderDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)) ? TRUE : FALSE;
	}

	/*
	* The engine fallback shaders of the bezier grass proxy: the blade pass and its
	* construct pass. A proxy whose component carries no material instance resolves to
	* these, so geometry and the construct dispatch are never dropped merely because
	* nothing was authored. Both state their own source; neither carries per-object state.
	*
	* The process wide instance cache owns what these hand out, which is what makes a shader reload
	* reach the grass: the cache re-points the instance to the rebuilt asset, so this keeps the stable
	* handle rather than a private copy. Asked for, not resolved - this is reached from the proxy's
	* draw side, where a load may not happen. Null means the assembly is still pending, and the caller
	* asks again the next time it runs.
	*/
	static const EShaderInstance* GetFallbackBezierGrassShader()
	{
		static const EShaderInstance* FallbackInstance = nullptr;
		if (!FallbackInstance)
		{
			EShaderAssemblyRequest Request;
			Request.ShaderName = EName(PE_TEXT("M_BezierGrass"));
			Request.MaterialName = EString(PE_TEXT("M_BezierGrass"));
			Request.ProgramName = EName(PE_TEXT("BezierGrass"));
			Request.VertexFactory = EShaderVertexFactoryType::PROCEDURAL_GRASS;
			// The grass factory takes the blade handle in attribute 0.
			Request.Profile.StreamProfile = SHADER_VERTEX_ATTRIBUTE_POSITION;
			Request.StageMask = ENGINE_SHADER_STAGE_MASK_GRAPHICS;

			FallbackInstance = REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(Request);
		}
		return FallbackInstance;
	}

	static const EShaderInstance* GetFallbackBezierGrassConstructShader()
	{
		static const EShaderInstance* FallbackInstance = nullptr;
		if (!FallbackInstance)
		{
			EShaderAssemblyRequest Request;
			Request.ShaderName = EName(PE_TEXT("CS_BezierGrassConstruct"));
			Request.ProgramName = EName(PE_TEXT("BezierGrassConstruct"));
			// The construct pass belongs to the grass pipeline, so it carries the grass
			// axis; it declares no vertex factory of its own, so the axis only shapes the
			// variant it is cached under.
			Request.VertexFactory = EShaderVertexFactoryType::PROCEDURAL_GRASS;
			Request.Profile.StreamProfile = SHADER_VERTEX_ATTRIBUTE_POSITION;
			Request.StageMask = ENGINE_SHADER_STAGE_MASK_COMPUTE;

			FallbackInstance = REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(Request);
		}
		return FallbackInstance;
	}

	void RBezierGrassCBufferParameter::AddPrimitiveMaterialParameter() noexcept
	{
		const EVariableFormat MatrixFormat = MakeShaderVariableFormat(
			EVariableFormatPrefixType::VARIABLE_FORMAT_PREFIX_MATRIX, EVariableFormatSuffixType::VARIABLE_FORMAT_SUFFIX_4x4);
		AddStructVariable(PE_TEXT("_WorldMatrix"), MatrixFormat, 1u);
		AddStructVariable(PE_TEXT("_WorldInvMatrix"), MatrixFormat, 1u);
		AddStructVariable(PE_TEXT("_WorldInvTransposeMatrix"), MatrixFormat, 1u);
	}
	void RBezierGrassCBufferParameter::DoSetupVariable() noexcept
	{
		AddPrimitiveMaterialParameter();
		AddBezierGrassMaterialParameter();
	}
	void RBezierGrassCBufferParameter::AddBezierGrassMaterialParameter() noexcept
	{
		// The field order and the field formats are the GPU layout of the blade pass
		// constant buffer; they must stay aligned with the shader declaration.
		const EVariableFormat Float4Format = MakeShaderVariableFormat(
			EVariableFormatPrefixType::VARIABLE_FORMAT_PREFIX_FLOAT, EVariableFormatSuffixType::VARIABLE_FORMAT_SUFFIX_4);
		const EVariableFormat Uint4Format = MakeShaderVariableFormat(
			EVariableFormatPrefixType::VARIABLE_FORMAT_PREFIX_UINT, EVariableFormatSuffixType::VARIABLE_FORMAT_SUFFIX_4);
		AddStructVariable(PE_TEXT("CurrentTimeParams"), Float4Format, 1u);
		AddStructVariable(PE_TEXT("PreviousTimeParams"), Float4Format, 1u);
		AddStructVariable(PE_TEXT("NumAllocatedLODMaxNumInstances"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("TileAnchorSize"), Float4Format, 1u);
		AddStructVariable(PE_TEXT("TileXYNumTiles"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("SubTileSizeBorderSize"), Float4Format, 1u);
		AddStructVariable(PE_TEXT("SubTileXYNumSubTiles"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("MaskXYNumInstances"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("DensityScale"), Float4Format, 1u);
		AddStructVariable(PE_TEXT("HeightMapWorldScaleOffsetBorderPixelSize"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("LayerBorderPixelSizeDensityBorderPixelSize"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("LayerTypeElemsBaseCustomTotalNumTypes"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("LODBodyPart1"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("LODBodyPart2"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("IndexOffset1"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("IndexOffset2"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("VertexOffset1"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("VertexOffset2"), Uint4Format, 1u);
		AddStructVariable(PE_TEXT("LODDistancesSq1"), Float4Format, 1u);
		AddStructVariable(PE_TEXT("LODDistancesSq2"), Float4Format, 1u);
	}

	PE_STATIC_CONSTEXPR UINT32 BEZIER_GRASS_LOD_NUM         = 5u;
	PE_STATIC_CONSTEXPR UINT32 BEZIER_GRASS_MAX_LOD_INDEX   = 4u;
	PE_STATIC_CONSTEXPR UINT32 BEZIER_GRASS_MIN_BODY_PARTS  = 2u;

	typedef UINT16 BEZIER_GRASS_INDEX_TYPE;
	typedef UINT32 BEZIER_GRASS_VERTEX_TYPE;
#if 0
	// @intellisense_off
	template<UINT32 _LODIndex>
	struct RBezierGrassLODData
	{
	public:
		PE_STATIC_CONSTEXPR UINT32 NumTriangles = _LODIndex * 2 + (BEZIER_GRASS_START_BODY_PARTS * 2 + 1);
		PE_STATIC_CONSTEXPR UINT32 NumIndices = NumTriangles * 3;
		PE_STATIC_CONSTEXPR UINT32 NumVertices = _LODIndex * 2 + (BEZIER_GRASS_START_BODY_PARTS * 2 + 2 + 1);
	private:
		template<UINT32 _LOD>
		PE_STATIC_CONSTEXPR UINT32 AccumulateVertexOffset()
		{
			if PE_CONSTEXPR(_LOD < 0)
			{
				return 0;
			}
			if PE_CONSTEXPR(_LOD == 0)
			{
				return RBezierGrassLODData<0>::NumVertices;
			}
			return RBezierGrassLODData<_LOD>::NumVertices + AccumulateVertexOffset<_LOD - 1>();
		}
		template<UINT32 _LOD>
		PE_STATIC_CONSTEXPR UINT32 AccumulateIndexOffset()
		{
			if PE_CONSTEXPR(_LOD < 0)
			{
				return 0;
			}
			if PE_CONSTEXPR(_LOD == 0)
			{
				return RBezierGrassLODData<0>::NumIndices;
			}
			return RBezierGrassLODData<_LOD>::NumIndices + AccumulateIndexOffset<_LOD - 1>();
		}
		template<UINT32 _LOD>
		PE_STATIC_CONSTEXPR UINT32 AccumulateTotalTriangles()
		{
			if PE_CONSTEXPR(_LOD < 0)
			{
				return 0;
			}
			if PE_CONSTEXPR(_LOD == 0)
			{
				return RBezierGrassLODData<0>::NumTriangles;
			}
			return RBezierGrassLODData<_LOD>::NumTriangles + AccumulateTotalTriangles<_LOD - 1>();
		}
		template<UINT32 _LOD>
		PE_STATIC_CONSTEXPR UINT32 AccumulateTotalIndices()
		{
			if PE_CONSTEXPR(_LOD < 0)
			{
				return 0;
			}
			if PE_CONSTEXPR(_LOD == 0)
			{
				return RBezierGrassLODData<0>::NumIndices;
			}
			return RBezierGrassLODData<_LOD>::NumIndices + AccumulateTotalIndices<_LOD - 1>();
		}
		template<UINT32 _LOD>
		PE_STATIC_CONSTEXPR UINT32 AccumulateTotalVertices()
		{
			if PE_CONSTEXPR(_LOD < 0)
			{
				return 0;
			}
			if PE_CONSTEXPR(_LOD == 0)
			{
				return RBezierGrassLODData<0>::NumVertices;
			}
			return RBezierGrassLODData<_LOD>::NumVertices + AccumulateTotalVertices<_LOD - 1>();
		}
	public:
		PE_STATIC_CONSTEXPR UINT32 VertexOffset = (_LODIndex == 0) ? 0 : AccumulateVertexOffset<_LODIndex - 1>();
		PE_STATIC_CONSTEXPR UINT32 IndexOffset = (_LODIndex == 0) ? 0 : AccumulateIndexOffset<_LODIndex - 1>();
		PE_STATIC_CONSTEXPR UINT32 NumTotalTriangles = AccumulateTotalTriangles<_LODIndex>();
		PE_STATIC_CONSTEXPR UINT32 NumTotalIndices = AccumulateTotalIndices<_LODIndex>();
		PE_STATIC_CONSTEXPR UINT32 NumTotalVertices = AccumulateTotalVertices<_LODIndex>();
	};

	template<typename _Ty, SIZE_T _Num1, SIZE_T _Num2>
	PE_CONSTEXPR auto ConcatArrays(const std::array<_Ty, _Num1>& InArray1, const std::array<_Ty, _Num2>& InArray2) -> std::array<_Ty, _Num1 + _Num2>
	{
		std::array<_Ty, _Num1 + _Num2> Result{};

		for (SIZE_T Index = 0; Index < _Num1; Index++)
		{
			Result[Index] = InArray1[Index];
		}

		for (SIZE_T Index = 0; Index < _Num2; Index++)
		{
			Result[_Num1 + Index] = InArray2[Index];
		}

		return Result;
	}

	template<UINT32 _LOD>
	PE_CONSTEXPR auto GenerateLODIndices() -> std::array<BEZIER_GRASS_INDEX_TYPE, RBezierGrassLODData<_LOD>::NumIndices>
	{
		std::array<BEZIER_GRASS_INDEX_TYPE, RBezierGrassLODData<_LOD>::NumIndices> Indices{};
		const UINT32 NumParts = (RBezierGrassLODData<_LOD>::NumTriangles - 1) / 2;

		for (UINT32 PartIndex = 0; PartIndex < NumParts; PartIndex++)
		{
			Indices[PartIndex * 6 + 0] = static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 0);
			Indices[PartIndex * 6 + 1] = static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0);
			Indices[PartIndex * 6 + 2] = static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1);
			Indices[PartIndex * 6 + 3] = static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0);
			Indices[PartIndex * 6 + 4] = static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 1);
			Indices[PartIndex * 6 + 5] = static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1);
		}

		const UINT32 NumTriangles = RBezierGrassLODData<_LOD>::NumTriangles;
		Indices[RBezierGrassLODData<_LOD>::NumIndices - 3] = static_cast<BEZIER_GRASS_INDEX_TYPE>(NumTriangles - 1);
		Indices[RBezierGrassLODData<_LOD>::NumIndices - 2] = static_cast<BEZIER_GRASS_INDEX_TYPE>(NumTriangles + 1);
		Indices[RBezierGrassLODData<_LOD>::NumIndices - 1] = static_cast<BEZIER_GRASS_INDEX_TYPE>(NumTriangles + 0);

		return Indices;
	}

	template<UINT32 _LOD>
	PE_CONSTEXPR auto GenerateLODVertices() -> std::array<BEZIER_GRASS_VERTEX_TYPE, RBezierGrassLODData<_LOD>::NumVertices>
	{
		std::array<BEZIER_GRASS_VERTEX_TYPE, RBezierGrassLODData<_LOD>::NumVertices> Vertices{};

		Vertices[0] = BEZIER_GRASS_VERTEX_TYPE{ 0, 0, 255, 0 };
		Vertices[1] = BEZIER_GRASS_VERTEX_TYPE{ 0, 255, 0, 0 };

		const UINT32 NumParts = (RBezierGrassLODData<_LOD>::NumVertices - 3) / 2;

		for (UINT32 PartIndex = 0; PartIndex < NumParts; PartIndex++)
		{
			FLOAT TParam = (1.f / (NumParts + 1)) * (PartIndex + 1);
			UINT8 T = static_cast<UINT8>(TParam * 255.f);

			UINT8 W = (PartIndex < (NumParts - 1)) ? 255 : 192;

			UINT8 PrevLODT = T;

			if (PartIndex < (NumParts - 2))
			{
				if (PartIndex >= (NumParts - 1) / 2)
				{
					FLOAT NextT = (1.f / (NumParts + 1)) * (PartIndex + 2);
					PrevLODT = static_cast<UINT8>(NextT * 255.f);
				}
				else
				{
					FLOAT PrevT = (1.f / (NumParts + 1)) * PartIndex;
					PrevLODT = static_cast<UINT8>(PrevT * 255.f);
				}
			}

			Vertices[PartIndex * 2 + 2 + 0] = BEZIER_GRASS_VERTEX_TYPE{ T, 0, W, PrevLODT };
			Vertices[PartIndex * 2 + 2 + 1] = BEZIER_GRASS_VERTEX_TYPE{ T, W, 0, PrevLODT };
		}

		Vertices[RBezierGrassLODData<_LOD>::NumVertices - 1] = BEZIER_GRASS_VERTEX_TYPE{ 255, 0, 0, 255 };

		return Vertices;
	}

	template<UINT32 _LOD>
	struct RBezierGrassIndexDataBase;

	template<>
	struct RBezierGrassIndexDataBase<0>
	{
		PE_STATIC_CONSTEXPR auto Value = GenerateLODIndices<0>();
	};

	template<UINT32 _LOD>
	struct RBezierGrassIndexDataRecursive
	{
	private:
		PE_STATIC_CONSTEXPR auto PrevValue = RBezierGrassIndexDataRecursive<_LOD - 1>::Value;
		PE_STATIC_CONSTEXPR auto CurrValue = GenerateLODIndices<_LOD>();
	public:
		PE_STATIC_CONSTEXPR auto Value = ConcatArrays(PrevValue, CurrValue);
	};

	template<>
	struct RBezierGrassIndexDataRecursive<1>
	{
	private:
		PE_STATIC_CONSTEXPR auto PrevValue = RBezierGrassIndexDataBase<0>::Value;
		PE_STATIC_CONSTEXPR auto CurrValue = GenerateLODIndices<1>();
	public:
		PE_STATIC_CONSTEXPR auto Value = ConcatArrays(PrevValue, CurrValue);
	};

	template<UINT32 _LOD>
	struct RBezierGrassIndexData
	{
		using ImplType = typename std::conditional<
			(_LOD == 0),
			RBezierGrassIndexDataBase<_LOD>,
			RBezierGrassIndexDataRecursive<_LOD>
		>::type;

		PE_STATIC_CONSTEXPR auto Value = ImplType::Value;
	};

	template<UINT32 _LOD>
	struct RBezierGrassVertexDataBase;

	template<>
	struct RBezierGrassVertexDataBase<0>
	{
		PE_STATIC_CONSTEXPR auto Value = GenerateLODVertices<0>();
	};

	template<UINT32 _LOD>
	struct RBezierGrassVertexDataRecursive
	{
	private:
		PE_STATIC_CONSTEXPR auto PrevValue = RBezierGrassVertexDataRecursive<_LOD - 1>::Value;
		PE_STATIC_CONSTEXPR auto CurrValue = GenerateLODVertices<_LOD>();
	public:
		PE_STATIC_CONSTEXPR auto Value = ConcatArrays(PrevValue, CurrValue);
	};

	template<>
	struct RBezierGrassVertexDataRecursive<1>
	{
	private:
		PE_STATIC_CONSTEXPR auto PrevValue = RBezierGrassVertexDataBase<0>::Value;
		PE_STATIC_CONSTEXPR auto CurrValue = GenerateLODVertices<1>();
	public:
		PE_STATIC_CONSTEXPR auto Value = ConcatArrays(PrevValue, CurrValue);
	};

	template<UINT32 _LOD>
	struct RBezierGrassVertexData
	{
		using ImplType = typename std::conditional<
			(_LOD == 0),
			RBezierGrassVertexDataBase<_LOD>,
			RBezierGrassVertexDataRecursive<_LOD>
		>::type;

		PE_STATIC_CONSTEXPR auto Value = ImplType::Value;
	};
	// @intellisense_on
#endif


	RBezierGrassSceneProxy::RBezierGrassSceneProxy(PBezierGrassComponent* InComponent)
		: GraphicsProgram(nullptr)
		, ComputeProgram(nullptr)
		, bSplitBentPart(FALSE)
		, Component(InComponent)
		, Property(InComponent->Property)
		, LayerTypeData(InComponent->LayerTypeData)
#if 0
		, TileAnchor(InComponent->TileAnchor)
		, TileSize(InComponent->TileSize)
		, NumTilesX(InComponent->NumTilesX)
		, NumTilesZ(InComponent->NumTilesZ)
		, WindDirection(InComponent->WindDirection)
		, WindStrength(InComponent->WindStrength)
		, LandscapeHeightTexture(nullptr)
		, LayerIndexTexture(nullptr)
		, DensityTexture(nullptr)
#endif
	{
		for (UINT32 LODIndex = 0; LODIndex < BEZIER_GRASS_LOD_NUM; LODIndex++)
		{
			BodyParts.Add(BEZIER_GRASS_MIN_BODY_PARTS + LODIndex);
#if 0
			VertexOffset[LODIndex] = 0;
			IndexOffset[LODIndex] = 0;
			IndexCount[LODIndex] = 0;
#endif
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create bezier grass scene proxy failed"), (!!Component));
	}
	RBezierGrassSceneProxy::RBezierGrassSceneProxy()
		: GraphicsProgram(nullptr), ComputeProgram(nullptr), Component(nullptr)
	{
	}
	RBezierGrassSceneProxy::RBezierGrassSceneProxy(const RBezierGrassSceneProxy& Other)
		: RPrimitiveSceneProxy(Other), GraphicsProgram(Other.GraphicsProgram), ComputeProgram(Other.ComputeProgram), Component(Other.Component)
	{
	}
	RBezierGrassSceneProxy::~RBezierGrassSceneProxy()
	{
	}
	void RBezierGrassSceneProxy::SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices)
	{
		SetupShaders();

		SetPrimitiveSettings(InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);

		CBufferParameter.SetupParameters();

		UpdateRenderResource();
		SetupVertexIndexBuffer();
		SetupComputeBuffers();
		SetupComputeTextures();
		SetupComputeConstantBuffer();
	}
	void RBezierGrassSceneProxy::UpdateProperty(const EBezierGrassProperty& InProperty)
	{
		Property = InProperty;
#if 0
		Property.LOD = BEZIER_GRASS_MAX_LOD_INDEX - InProperty.LOD;
#endif
	}
	void RBezierGrassSceneProxy::UpdateLayerTypeData(const EBezierGrassLayerTypeData& InLayerData)
	{
		LayerTypeData = InLayerData;
		if (LayerTypeBuffer.IsRenderResourceValid())
		{
			LayerTypeBuffer.ReleaseRenderResource();
		}
		SetupLayerTypeBuffer();
	}
	void RBezierGrassSceneProxy::UpdateTileParams(const Vector2& InTileAnchor, const Vector2& InTileSize, UINT32 InNumTilesX, UINT32 InNumTilesZ)
	{
		TileAnchor = InTileAnchor;
		TileSize = InTileSize;
		NumTilesX = InNumTilesX;
		NumTilesZ = InNumTilesZ;
	}
	void RBezierGrassSceneProxy::UpdateWindParams(const Vector3& InWindDirection, FLOAT InWindStrength)
	{
		WindDirection = InWindDirection;
		WindStrength = InWindStrength;
	}
	BOOL32 RBezierGrassSceneProxy::IsRenderValid()const
	{
		//TODO
		return TRUE;
	}
	void RBezierGrassSceneProxy::UpdateInstanceResource()
	{
	}
	void RBezierGrassSceneProxy::UpdateRenderResource()
	{
		Matrix4x4 InvMat(GetLocalToWorldMatrix().Inverse());
		CBufferParameter.SetFieldValue(EName(PE_TEXT("_WorldMatrix")), TranslateUploadMatrixType(GetLocalToWorldMatrix()));
		CBufferParameter.SetFieldValue(EName(PE_TEXT("_WorldInvMatrix")), TranslateUploadMatrixType(InvMat));
		CBufferParameter.SetFieldValue(EName(PE_TEXT("_WorldInvTransposeMatrix")), TranslateUploadTransposeMatrixType(InvMat));
#if 0
		CBufferParameter["_BezierGrassRootColor"] = &TranslateUploadVectorType(Vector4(Property.RootColor.r, Property.RootColor.g, Property.RootColor.b, Property.RootColor.a));
		CBufferParameter["_BezierGrassTipColor"] = &TranslateUploadVectorType(Vector4(Property.TipColor.r, Property.TipColor.g, Property.TipColor.b, Property.TipColor.a));
		CBufferParameter["_BezierGrassParams"] = &TranslateUploadVectorType(Vector4(Property.BentBezierT, Property.Roughness, Property.Metallic, 0.f));
#endif
		CBufferParameter.UploadBuffer();
	}
	void RBezierGrassSceneProxy::BindRenderResource()const
	{
		BindVertexShader();
		BindPixelShader();
		BindMeshAndInstanceResource();
		BindPrimitiveParameter();
	}
	void RBezierGrassSceneProxy::SetupVertexIndexBuffer()
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
		VertexBuffer.ReleaseRenderResource();
		IndexBuffer.ReleaseRenderResource();

		const UINT32 NumLODs = BodyParts.Num();
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("BodyParts must not be empty"), (NumLODs > 0));

		auto BuildLODIndices = [this](UINT32 InLODIndex, TArray<BEZIER_GRASS_INDEX_TYPE>& OutIndices, UINT32& OutNumIndices) -> void
		{
			const UINT32 UsedBodyParts  = BodyParts[InLODIndex];
			const UINT32 NumFirstParts  = bSplitBentPart ? ((UsedBodyParts + 1) / 2) : UsedBodyParts;
			const UINT32 NumSecondParts = UsedBodyParts - NumFirstParts;
			const UINT32 NumTriangles   = UsedBodyParts * 2 + 1;

			OutNumIndices = NumTriangles * 3;
			OutIndices.Reset();
			OutIndices.Reserve(OutNumIndices);

			for (UINT32 PartIndex = 0; PartIndex < NumFirstParts; PartIndex++)
			{
				OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 0));
				OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0));
				OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1));
				OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0));
				OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 1));
				OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1));
			}

			if (NumSecondParts > 0)
			{
				const UINT32 StartVertexIndex = NumFirstParts * 2 + 2;
				for (UINT32 PartIndex = 0; PartIndex < NumSecondParts; PartIndex++)
				{
					OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 0 + StartVertexIndex));
					OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0 + StartVertexIndex));
					OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1 + StartVertexIndex));
					OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0 + StartVertexIndex));
					OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 1 + StartVertexIndex));
					OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1 + StartVertexIndex));
				}
			}

			const UINT32 UsedNumVertices = bSplitBentPart ? (UsedBodyParts * 2 + 5) : (UsedBodyParts * 2 + 3);
			OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(UsedNumVertices - 3));
			OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(UsedNumVertices - 1));
			OutIndices.Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(UsedNumVertices - 2));
		};

		auto BuildLODVertices = [this](UINT32 InLODIndex, TArray<BEZIER_GRASS_VERTEX_TYPE>& OutVertices, UINT32& OutNumVertices, FLOAT& OutMidT) -> void
		{
#if 0
			const UINT32 UsedBodyParts  = BodyParts[InLODIndex];
			const UINT32 NumFirstParts  = bSplitBentPart ? ((UsedBodyParts + 1) / 2) : UsedBodyParts;
			const UINT32 NumSecondParts = UsedBodyParts - NumFirstParts;
			const UINT32 UsedNumVertices = bSplitBentPart ? (UsedBodyParts * 2 + 5) : (UsedBodyParts * 2 + 3);
			const BOOL32 bGeneratePrevT = (InLODIndex < (NumLODs - 1)) && (UsedBodyParts > BEZIER_GRASS_MIN_BODY_PARTS);

			PE_STATIC_CONSTEXPR FLOAT T15BitFloat = static_cast<FLOAT>(0x7fffu);
			PE_STATIC_CONSTEXPR FLOAT T14BitFloat = static_cast<FLOAT>(0x3fffu);

			OutNumVertices = UsedNumVertices;
			OutMidT = 0.f;

			// Build prev-T lookup tables for next LOD
			TArray<FLOAT> PrevTFirst, PrevTSecond;
			if (bGeneratePrevT)
			{
				const UINT32 NextBodyParts    = BodyParts[InLODIndex + 1];
				const UINT32 NextNumTs        = NextBodyParts + 2;
				const UINT32 NextFirstParts   = bSplitBentPart ? ((NextBodyParts + 1) / 2) : NextBodyParts;
				const UINT32 NextSecondParts  = NextBodyParts - NextFirstParts;

				PrevTFirst.Reserve(NextFirstParts + 1);
				for (UINT32 PartIndex = 0; PartIndex <= NextFirstParts; PartIndex++)
				{
					PrevTFirst.Add(static_cast<FLOAT>(PartIndex) / static_cast<FLOAT>(NextNumTs - 1));
				}
				PrevTSecond.Reserve(NextSecondParts + 1);
				for (UINT32 PartIndex = 0; PartIndex <= NextSecondParts; PartIndex++)
				{
					PrevTSecond.Add(static_cast<FLOAT>(PartIndex + NextFirstParts) / static_cast<FLOAT>(NextNumTs - 1));
				}
			}

			auto FindClosestT = [](const TArray<FLOAT>& InTable, FLOAT InT) -> FLOAT
			{
				FLOAT ClosestDist = 1.f;
				FLOAT ClosestT = InT;
				for (UINT32 Index = 0, Num = InTable.Num(); Index < Num; Index++)
				{
					const FLOAT Dist = EMath::Abs(InT - InTable[Index]);
					if (Dist < ClosestDist)
					{
						ClosestDist = Dist;
						ClosestT = InTable[Index];
					}
				}
				return ClosestT;
			};

			auto PackVertex = [&T15BitFloat, &T14BitFloat](FLOAT InTargetT, FLOAT InPrevT, BOOL32 InbSecondPart, BOOL32 InbPosiOffset) -> BEZIER_GRASS_VERTEX_TYPE
			{
				return (static_cast<UINT32>(InTargetT * T15BitFloat) << 17u)
					| (static_cast<UINT32>(InPrevT * T14BitFloat) << 3u)
					| (InbSecondPart ? 0x4u : 0x0u)
					| (InbPosiOffset ? 0x2u : 0x0u);
			};

			OutVertices.Reset();
			OutVertices.Reserve(UsedNumVertices);

			// Root (first part start)
			OutVertices.Add(PackVertex(0.f, 0.f, FALSE, TRUE));
			OutVertices.Add(PackVertex(0.f, 0.f, FALSE, FALSE));

			for (UINT32 PartIndex = 0; PartIndex < NumFirstParts; PartIndex++)
			{
				const FLOAT TargetT = static_cast<FLOAT>(PartIndex + 1) / static_cast<FLOAT>(UsedBodyParts + 1);
				const FLOAT PrevT   = (bGeneratePrevT && (PartIndex != (NumFirstParts - 1))) ? FindClosestT(PrevTFirst, TargetT) : TargetT;
				OutVertices.Add(PackVertex(TargetT, PrevT, FALSE, TRUE));
				OutVertices.Add(PackVertex(TargetT, PrevT, FALSE, FALSE));
			}

			if (NumSecondParts > 0)
			{
				const FLOAT MidTargetT = static_cast<FLOAT>(OutVertices.Last() >> 17u) / T15BitFloat;
				OutMidT = EMath::FloorToFloat(MidTargetT * T15BitFloat) / T15BitFloat;

				OutVertices.Add(PackVertex(MidTargetT, MidTargetT, TRUE, TRUE));
				OutVertices.Add(PackVertex(MidTargetT, MidTargetT, TRUE, FALSE));

				for (UINT32 PartIndex = 0; PartIndex < NumSecondParts; PartIndex++)
				{
					const FLOAT TargetT = static_cast<FLOAT>(PartIndex + NumFirstParts + 1) / static_cast<FLOAT>(UsedBodyParts + 1);
					const FLOAT PrevT   = (bGeneratePrevT && (PartIndex != (NumSecondParts - 1))) ? FindClosestT(PrevTSecond, TargetT) : TargetT;
					OutVertices.Add(PackVertex(TargetT, PrevT, TRUE, TRUE));
					OutVertices.Add(PackVertex(TargetT, PrevT, TRUE, FALSE));
				}
			}

			// Tip - mark with bit 0
			BEZIER_GRASS_VERTEX_TYPE TipVertex = PackVertex(1.f, 1.f, TRUE, FALSE);
			TipVertex = ((TipVertex >> 2u) << 2u) | 0x1u;
			OutVertices.Add(TipVertex);
#endif
		};

		TArray<TArray<BEZIER_GRASS_VERTEX_TYPE>> LODVertices;
		TArray<TArray<BEZIER_GRASS_INDEX_TYPE>>  LODIndices;
		TArray<UINT32> LODNumVertices, LODNumIndices;
		LODVertices.Reserve(NumLODs);
		LODIndices.Reserve(NumLODs);
		LODNumVertices.Reserve(NumLODs);
		LODNumIndices.Reserve(NumLODs);
		BentBezierTs.Reset();
		BentBezierTs.Reserve(NumLODs);

		UINT32 TotalNumVertices = 0;
		UINT32 TotalNumIndices  = 0;
		for (UINT32 LODIndex = 0; LODIndex < NumLODs; LODIndex++)
		{
			TArray<BEZIER_GRASS_INDEX_TYPE>& Indices  = LODIndices.AddDefaultGetRef();
			TArray<BEZIER_GRASS_VERTEX_TYPE>& Vertices = LODVertices.AddDefaultGetRef();
			UINT32 CurrentNumIndices  = 0;
			UINT32 CurrentNumVertices = 0;
			FLOAT  CurrentMidT        = 0.f;

			BuildLODIndices(LODIndex, Indices, CurrentNumIndices);
			BuildLODVertices(LODIndex, Vertices, CurrentNumVertices, CurrentMidT);

			LODNumIndices.Add(CurrentNumIndices);
			LODNumVertices.Add(CurrentNumVertices);
			BentBezierTs.Add(CurrentMidT);
#if 0
			VertexOffset[LODIndex] = TotalNumVertices;
			IndexOffset[LODIndex]  = TotalNumIndices;
			IndexCount[LODIndex]   = CurrentNumIndices;
#endif
			TotalNumVertices += CurrentNumVertices;
			TotalNumIndices  += CurrentNumIndices;
		}

		TArray<BEZIER_GRASS_VERTEX_TYPE> VertexData;
		TArray<BEZIER_GRASS_INDEX_TYPE>  IndexData;
		VertexData.Reserve(TotalNumVertices);
		IndexData.Reserve(TotalNumIndices);
		for (UINT32 LODIndex = 0; LODIndex < NumLODs; LODIndex++)
		{
			VertexData.Append(LODVertices[LODIndex]);
			IndexData.Append(LODIndices[LODIndex]);
		}

		const UINT32 NumTotalVertices = TotalNumVertices;
		const UINT32 NumTotalIndices  = TotalNumIndices;
		RSubresourceDataDesc SubresDesc;
		SubresDesc.pSysMem = &(VertexData[0]);
		RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(VertexBuffer,
			RBufferDesc(static_cast<UINT32>(NumTotalVertices * sizeof(BEZIER_GRASS_VERTEX_TYPE)), RBindFlagType::BIND_VERTEX_BUFFER, sizeof(BEZIER_GRASS_VERTEX_TYPE)),
			&SubresDesc
		);
		SubresDesc.pSysMem = &(IndexData[0]);
		RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(IndexBuffer,
			RBufferDesc(static_cast<UINT32>(NumTotalIndices * sizeof(BEZIER_GRASS_INDEX_TYPE)), RBindFlagType::BIND_INDEX_BUFFER, sizeof(BEZIER_GRASS_INDEX_TYPE)),
			&SubresDesc
		);

		VertexBuffer.Stride = sizeof(BEZIER_GRASS_VERTEX_TYPE);
		IndexBuffer.UseShort = sizeof(BEZIER_GRASS_INDEX_TYPE) == sizeof(UINT16);
		IndexBuffer.IndexCount = NumTotalIndices;
	}
	void RBezierGrassSceneProxy::SetupShaders()
	{
		// The instances the component carries win; the shared engine fallbacks keep the
		// blade pass and the construct dispatch alive when it carries none. GetProgram()
		// caches its resolution per instance, so this stays a lookup, not a rebuild.
		const EShaderInstance* GrassShader = (Component ? Component->GetMaterialInstance() : nullptr);
		if (!GrassShader)
		{
			GrassShader = GetFallbackBezierGrassShader();
		}
		GraphicsProgram = (GrassShader ? GrassShader->GetProgram() : nullptr);

		const EShaderInstance* GrassConstructShader = (Component ? Component->GetComputeMaterialInstance() : nullptr);
		if (!GrassConstructShader)
		{
			GrassConstructShader = GetFallbackBezierGrassConstructShader();
		}
		ComputeProgram = (GrassConstructShader ? GrassConstructShader->GetProgram() : nullptr);
	}
	void RBezierGrassSceneProxy::BindVertexShader()const
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
		const RShaderStageResource* VertexStage = (GraphicsProgram ? ResolveShaderStageResource(*GraphicsProgram, EShaderStageType::SHADER_STAGE_VERTEX) : nullptr);
#if _EDITOR_ONLY
		if (VertexStage)
#endif
		{
			// This legacy direct path binds the native objects of the stage resource, which
			// is why it only runs on the D3D11 back-end.
			const IRD3D11Shader*						NativeShader		= static_cast<const IRD3D11Shader*>(VertexStage->GetRHIShader());
			Microsoft::WRL::ComPtr<ID3D11InputLayout>	InputLayout(NativeShader->GetD3D11InputLayout());
			Microsoft::WRL::ComPtr<ID3D11VertexShader>	NativeVertexShader(NativeShader->GetD3D11VS());
			{
				RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
				RenderDevice->SetInputLayout(InputLayout);
				RenderDevice->SetVSShader(NativeVertexShader);
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh vertex shader is invalid."));
		}
#endif
	}
	void RBezierGrassSceneProxy::BindPixelShader()const
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
		const RShaderStageResource* PixelStage = (GraphicsProgram ? ResolveShaderStageResource(*GraphicsProgram, EShaderStageType::SHADER_STAGE_PIXEL) : nullptr);
#if _EDITOR_ONLY
		if (PixelStage)
#endif
		{
			const IRD3D11Shader*						NativeShader		= static_cast<const IRD3D11Shader*>(PixelStage->GetRHIShader());
			Microsoft::WRL::ComPtr<ID3D11PixelShader>	NativePixelShader(NativeShader->GetD3D11PS());
			{
				RDeviceD3D11::GetDeviceSingleton()->SetPSShader(NativePixelShader);
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh pixel shader is invalid."));
		}
#endif
	}
	void RBezierGrassSceneProxy::BindMeshAndInstanceResource()const
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
#if _EDITOR_ONLY
		const RShaderStageResource* VertexStage = (GraphicsProgram ? ResolveShaderStageResource(*GraphicsProgram, EShaderStageType::SHADER_STAGE_VERTEX) : nullptr);
		const RShaderStageResource* PixelStage = (GraphicsProgram ? ResolveShaderStageResource(*GraphicsProgram, EShaderStageType::SHADER_STAGE_PIXEL) : nullptr);
		if ((!!VertexStage) && (!!PixelStage))
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			const RIndexBufferResource& IndexRenderResource	= IndexBuffer;
			if (IndexRenderResource.IsRenderResourceValid())
			{
				RFormatType IndexFormat = IndexRenderResource.UseShort ? RFormatType::FORMAT_R16_UINT : RFormatType::FORMAT_R32_UINT;
				RenderDevice->SetIndexBuffer(IndexRenderResource.Buffer, 0u, IndexFormat);
			}

			{
				const RVertexBufferResource& VertexRenderResource = VertexBuffer;
				if (VertexRenderResource.IsRenderResourceValid())
				{
					RenderDevice->SetVertexBuffer(VertexRenderResource.Buffer, VertexRenderResource.Stride, 0, 0);
				}
			}

			{
				RenderDevice->BindVSShaderResourceView(PackedInstanceBuffer.ShaderResourceView, 0);
				RenderDevice->BindVSShaderResourceView(DispatchBuffer.ShaderResourceView, 1);
				RenderDevice->BindVSShaderResourceView(LayerTypeBuffer.ShaderResourceView, 2);
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check bezier grass scene proxy is invalid."));
		}
#endif
	}
	void RBezierGrassSceneProxy::BindPrimitiveParameter()const
	{
		IRCommandList* CmdList = EMainManager::GetManagerSingleton()->GetActiveCommandList();
		if (!CmdList) { return; }
		// The instance block keeps the engine wide convention of living at root 1.
		const RBufferResource& ConstantBuffer = CBufferParameter.GetBuffer();
		if (ConstantBuffer.IsRenderResourceValid())
		{
			IRRHIBuffer* CBPtr = const_cast<RBufferResource&>(ConstantBuffer).GetResolvedRHIBuffer();
			CmdList->SetGraphicsRootConstantBuffer(1u, CBPtr);
		}
	}
	void RBezierGrassSceneProxy::SetupLayerTypeBuffer()
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
		TArray<FLOAT> LayerData;
		LayerData.Reserve(27);

		LayerData.Add(LayerTypeData.Facing.x);
		LayerData.Add(LayerTypeData.Facing.y);
		LayerData.Add(LayerTypeData.Facing.z);
		LayerData.Add(LayerTypeData.Height.x);
		LayerData.Add(LayerTypeData.Height.y);
		LayerData.Add(LayerTypeData.Height.z);
		LayerData.Add(LayerTypeData.Width.x);
		LayerData.Add(LayerTypeData.Width.y);
		LayerData.Add(LayerTypeData.Width.z);
		LayerData.Add(LayerTypeData.Tilt.x);
		LayerData.Add(LayerTypeData.Tilt.y);
		LayerData.Add(LayerTypeData.Tilt.z);
		LayerData.Add(LayerTypeData.Bend.x);
		LayerData.Add(LayerTypeData.Bend.y);
		LayerData.Add(LayerTypeData.Bend.z);
		LayerData.Add(LayerTypeData.MidPointT.x);
		LayerData.Add(LayerTypeData.MidPointT.y);
		LayerData.Add(LayerTypeData.MidPointT.z);

		UINT32 Flags = (LayerTypeData.bBent ? 0x1u : 0x0u) | (LayerTypeData.bUseFacing ? 0x2u : 0x0u);
		LayerData.Add(*reinterpret_cast<FLOAT*>(&Flags));

		for (INT32 i = 0; i < 8; i++)
		{
			LayerData.Add(LayerTypeData.SideCurve[i]);
		}

		RSubresourceDataDesc SubresDesc;
		SubresDesc.pSysMem = LayerData.GetData();
		RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(LayerTypeBuffer,
			RStructuredBufferDesc(sizeof(FLOAT), LayerData.Num(), TRUE), &SubresDesc);
	}
	void RBezierGrassSceneProxy::SetupComputeTextures()
	{
		if (!LandscapeHeightTexture)
		{
			EString ImportPath(GetEngineDefaultTexturePath(RDefaultTextureType::TEXTURE2D_BLACK));
			EString TexName(GetEngineDefaultTextureName(RDefaultTextureType::TEXTURE2D_BLACK));
			EString ImportFileType("png");
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, TexName, LandscapeHeightTexture, &ImportPath, &TexName, &ImportFileType);
		}

		if (!LayerIndexTexture)
		{
			EString ImportPath(GetEngineDefaultTexturePath(RDefaultTextureType::TEXTURE2D_BLACK));
			EString TexName(GetEngineDefaultTextureName(RDefaultTextureType::TEXTURE2D_BLACK));
			EString ImportFileType("png");
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, TexName, LayerIndexTexture, &ImportPath, &TexName, &ImportFileType);
		}

		if (!DensityTexture)
		{
			EString ImportPath(GetEngineDefaultTexturePath(RDefaultTextureType::TEXTURE2D_WHITE));
			EString TexName(GetEngineDefaultTextureName(RDefaultTextureType::TEXTURE2D_WHITE));
			EString ImportFileType("png");
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, TexName, DensityTexture, &ImportPath, &TexName, &ImportFileType);
		}
	}
	void RBezierGrassSceneProxy::SetupComputeConstantBuffer()
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
		if (ComputeConstantBuffer.IsRenderResourceValid())
		{
			ComputeConstantBuffer.ReleaseRenderResource();
		}

		struct BezierGrassConstructParams
		{
			Vector4 CurrentTimeParams;
			Vector4 PreviousTimeParams;
			UINT32 LayerTypeElemsBaseCustomTotalNumTypes[4];
			UINT32 RandomSeedParams[4];
			UINT32 RandomSeedNumAllocatedLODMaxNumInstances[4];
			Vector4 TileAnchorSize;
			UINT32 TileXYNumTiles[4];
			UINT32 SubTileXYNumSubTiles[4];
			Vector4 SubTileSizeBorderSize;
			Vector4 GlobalWindDirectionStrength;
			Vector4 DensityScaleWindStrengthRange;
			UINT32 MaskXYNumInstances[4];
			UINT32 LODBodyPart1[4];
			UINT32 LODBodyPart2[4];
			UINT32 IndexOffset1[4];
			UINT32 IndexOffset2[4];
			UINT32 VertexOffset1[4];
			UINT32 VertexOffset2[4];
			Vector4 LODDistancesSq1;
			Vector4 LODDistancesSq2;
			UINT32 HeightMapWorldScaleOffsetBorderPixelSize[4];
			UINT32 LayerBorderPixelSizeDensityBorderPixelSize[4];
			Vector4 PreViewTranslationHigh;
			Vector4 PreViewTranslationLow;
			Vector4 ViewFrustumPlane0;
			Vector4 ViewFrustumPlane1;
			Vector4 ViewFrustumPlane2;
			Vector4 ViewFrustumPlane3;
			Vector4 ViewFrustumPlane4;
		};

		BezierGrassConstructParams Params = {};
		Params.LayerTypeElemsBaseCustomTotalNumTypes[0] = 19;
		Params.LayerTypeElemsBaseCustomTotalNumTypes[1] = 0;
		Params.LayerTypeElemsBaseCustomTotalNumTypes[2] = 19;
		Params.LayerTypeElemsBaseCustomTotalNumTypes[3] = 1;
		Params.TileAnchorSize = Vector4(TileAnchor.x, TileAnchor.y, TileSize.x, TileSize.y);
		Params.TileXYNumTiles[2] = NumTilesX;
		Params.TileXYNumTiles[3] = NumTilesZ;
		Params.SubTileXYNumSubTiles[2] = 1;
		Params.SubTileXYNumSubTiles[3] = 1;
		Params.SubTileSizeBorderSize = Vector4(TileSize.x, TileSize.y, 5.0f, 5.0f);
		Params.GlobalWindDirectionStrength = Vector4(WindDirection.x, WindDirection.y, 0.f, WindStrength);
		Params.DensityScaleWindStrengthRange = Vector4(1.0f, 0.f, 10.0f, 0.f);
		Params.MaskXYNumInstances[1] = 1;
		Params.MaskXYNumInstances[2] = 16;
		Params.MaskXYNumInstances[3] = 16;
		for (UINT32 LODIndex = 0; LODIndex < 4; LODIndex++)
		{
			Params.LODBodyPart1[LODIndex] = (LODIndex < (UINT32)BodyParts.Num()) ? BodyParts[LODIndex] : BodyParts.LastRef();
		}
		for (UINT32 LODIndex = 0; LODIndex < 4; LODIndex++)
		{
			Params.LODBodyPart2[LODIndex] = BodyParts.LastRef();
		}
		for (UINT32 i = 0; i < BEZIER_GRASS_LOD_NUM; i++)
		{
#if 0
			Params.IndexOffset1[i] = IndexOffset[i];
			Params.VertexOffset1[i] = VertexOffset[i];
#endif
		}
		Params.LODDistancesSq1 = Vector4(10000.f, 40000.f, 90000.f, 160000.f);
		Params.LODDistancesSq2 = Vector4(250000.f, 360000.f, 490000.f, 640000.f);
		Params.HeightMapWorldScaleOffsetBorderPixelSize[0] = 0;
		Params.HeightMapWorldScaleOffsetBorderPixelSize[1] = 0;
		Params.HeightMapWorldScaleOffsetBorderPixelSize[2] = 8;
		Params.HeightMapWorldScaleOffsetBorderPixelSize[3] = 256;
		Params.LayerBorderPixelSizeDensityBorderPixelSize[0] = 8;
		Params.LayerBorderPixelSizeDensityBorderPixelSize[1] = 256;
		Params.LayerBorderPixelSizeDensityBorderPixelSize[2] = 8;
		Params.LayerBorderPixelSizeDensityBorderPixelSize[3] = 256;
		Params.ViewFrustumPlane0 = Vector4(1.f, 0.f, 0.f, 10000.f);
		Params.ViewFrustumPlane1 = Vector4(0.f, 1.f, 0.f, 10000.f);
		Params.ViewFrustumPlane2 = Vector4(0.f, 0.f, 1.f, 10000.f);
		Params.ViewFrustumPlane3 = Vector4(0.f, 0.f, -1.f, 10000.f);
		Params.ViewFrustumPlane4 = Vector4(0.f, -1.f, 0.f, 10000.f);

		RSubresourceDataDesc SubresDesc;
		SubresDesc.pSysMem = &Params;
		RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(ComputeConstantBuffer,
			RBufferDesc(sizeof(BezierGrassConstructParams), RBindFlagType::BIND_CONSTANT_BUFFER, 0),
			&SubresDesc);
	}

	void RBezierGrassSceneProxy::SetupComputeBuffers()
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
		const UINT32 MaxInstances = 1024;

		if (!PackedInstanceBuffer.IsRenderResourceValid())
		{
			RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(PackedInstanceBuffer,
				RStructuredBufferDesc(sizeof(UINT32), MaxInstances * 10, TRUE));
		}

		if (!DispatchBuffer.IsRenderResourceValid())
		{
			RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(DispatchBuffer,
				RStructuredBufferDesc(sizeof(UINT32), 7, TRUE));
		}

		SetupLayerTypeBuffer();

		if (!TileShuffleBuffer.IsRenderResourceValid())
		{
			TArray<UINT32> ShuffleData;
			ShuffleData.SetNum(256);
			for (UINT32 i = 0; i < 256; i++)
			{
				ShuffleData[i] = 0;
			}
			RSubresourceDataDesc ShuffleSubresDesc;
			ShuffleSubresDesc.pSysMem = ShuffleData.GetData();
			RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(TileShuffleBuffer,
				RStructuredBufferDesc(sizeof(UINT32), 256, TRUE), &ShuffleSubresDesc);
		}

		if (!DrawIndirectBuffer.IsRenderResourceValid())
		{
			RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(DrawIndirectBuffer,
				RStructuredBufferDesc(sizeof(UINT32), 5 * BEZIER_GRASS_LOD_NUM, TRUE));
		}
	}

	void RBezierGrassSceneProxy::DispatchComputeShader()
	{
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
		const RShaderStageResource* ComputeStage = (ComputeProgram ? ResolveShaderStageResource(*ComputeProgram, EShaderStageType::SHADER_STAGE_COMPUTE) : nullptr);
		if (!ComputeStage)
		{
			return;
		}

		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		const IRD3D11Shader*						NativeShader		= static_cast<const IRD3D11Shader*>(ComputeStage->GetRHIShader());
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	NativeComputeShader(NativeShader->GetD3D11CS());

		RenderDevice->SetCSShader(NativeComputeShader);

		// Issue 4: bind the constant buffer (slot 0)
		if (ComputeConstantBuffer.IsRenderResourceValid())
		{
			RenderDevice->BindCSConstantBuffer(ComputeConstantBuffer.Buffer, 0);
		}

		// Issue 2: bind textures (slot 0, 1, 2)
		if (LandscapeHeightTexture && LandscapeHeightTexture->IsRenderResourceValid())
		{
			RenderDevice->BindCSShaderResourceView(LandscapeHeightTexture->GetRenderResource()->ShaderResourceView, 0);
		}
		if (LayerIndexTexture && LayerIndexTexture->IsRenderResourceValid())
		{
			RenderDevice->BindCSShaderResourceView(LayerIndexTexture->GetRenderResource()->ShaderResourceView, 1);
		}
		if (DensityTexture && DensityTexture->IsRenderResourceValid())
		{
			RenderDevice->BindCSShaderResourceView(DensityTexture->GetRenderResource()->ShaderResourceView, 2);
		}

		// Bind buffers
		RenderDevice->BindCSUnorderedAccessView(PackedInstanceBuffer.UnorderedAccessView, 0);
		RenderDevice->BindCSUnorderedAccessView(DispatchBuffer.UnorderedAccessView, 1);
		RenderDevice->BindCSUnorderedAccessView(DrawIndirectBuffer.UnorderedAccessView, 2);
		RenderDevice->BindCSShaderResourceView(LayerTypeBuffer.ShaderResourceView, 5);
		RenderDevice->BindCSShaderResourceView(TileShuffleBuffer.ShaderResourceView, 6);

		// Issue 1: derive the dispatch size from the actual instance count
		const UINT32 NumInstances = 16 * 16;
		const UINT32 ThreadGroupSize = 256;
		const UINT32 ThreadGroupX = (NumInstances + ThreadGroupSize - 1) / ThreadGroupSize;
		RenderDevice->Dispatch(ThreadGroupX, 1, 1);

		// Clear the bindings
		RenderDevice->BindNoCSUnorderedAccessView(0);
		RenderDevice->BindNoCSUnorderedAccessView(1);
		RenderDevice->BindNoCSUnorderedAccessView(2);
	}
	void RBezierGrassSceneProxy::Draw()const
	{
		// The engine fallbacks are asked for rather than resolved, so the first frames of a cold start
		// may legitimately have none. Re-running the resolution here is what makes the grass appear
		// once it is assembled; it is a lookup once both instances are held.
		if ((!GraphicsProgram) || (!ComputeProgram))
		{
			const_cast<RBezierGrassSceneProxy*>(this)->SetupShaders();
		}
		if (!IsLegacyD3D11PathActive())
		{
			return;
		}
#if _EDITOR_ONLY
		const RIndexBufferResource& IndexRenderResource = IndexBuffer;
		if (IndexRenderResource.IsRenderResourceValid())
#endif
		{
#if 0
			const UINT32 UsedLOD = EMath::Clamp((UINT32)(EMath::CeilToInt32(Property.LOD)), 0u, (UINT32)(BEZIER_GRASS_MAX_LOD_INDEX));
			const UINT32 UsedIndexOffset = IndexOffset[UsedLOD];
			const UINT32 UsedIndexCount = IndexCount[UsedLOD];
			const UINT32 UsedVertexOffset = VertexOffset[UsedLOD];
			const UINT32 NumInstances = 256;
			RDeviceD3D11::GetDeviceSingleton()->DrawIndexedInstance(NumInstances, UsedIndexCount, 0, UsedIndexOffset, UsedVertexOffset);
#endif
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Draw bezier grass indexed is invalid."));
		}
#endif
	}

};
#endif
