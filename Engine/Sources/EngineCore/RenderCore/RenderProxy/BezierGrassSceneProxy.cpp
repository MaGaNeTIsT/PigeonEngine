#include "BezierGrassSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <RenderDevice/DeviceD3D11.h>
#include <array>
#include <algorithm>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RBezierGrassSceneProxy, RPrimitiveSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RBezierGrassMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddPrimitiveMaterialParameter();
		AddBezierGrassMaterialParameter();
		EndSetupParameter();
		CreateBuffer();
	}
	void RBezierGrassMaterialParameter::AddBezierGrassMaterialParameter()
	{
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_BezierGrassParams1"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_BezierGrassRootColor"));
		AddParameter<Vector4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_BezierGrassTipColor"));
	}

	typedef UINT16 BEZIER_GRASS_INDEX_TYPE;
	struct BEZIER_GRASS_VERTEX_TYPE
	{
		UINT8 X;
		UINT8 Y;
		UINT8 Z;
		UINT8 W;
	};
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

#define SET_BEZIER_GRASS_BUFFER_OFFSET(__LODIndex) \
	VertexOffset[__LODIndex] = RBezierGrassLODData<__LODIndex>::VertexOffset;\
	IndexOffset[__LODIndex] = RBezierGrassLODData<__LODIndex>::IndexOffset;\
	IndexCount[__LODIndex] = RBezierGrassLODData<__LODIndex>::NumIndices;\

	RBezierGrassSceneProxy::RBezierGrassSceneProxy(PBezierGrassComponent* InComponent)
		: VertexShader(nullptr)
		, PixelShader(nullptr)
#if _EDITOR_ONLY
		, DebugComputeShader(nullptr)
		, DebugScreenComputeShader(nullptr)
#endif
		, Component(InComponent)
		, Property(InComponent->Property)
	{
		for (UINT32 LODIndex = 0, NumLODs = BEZIER_GRASS_LOD_NUM; LODIndex < NumLODs; LODIndex++)
		{
			VertexOffset[LODIndex] = 0;
			IndexOffset[LODIndex] = 0;
			IndexCount[LODIndex] = 0;
		}
		InstanceData.Reset();
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create static mesh scene proxy failed"), (!!Component));
	}
	RBezierGrassSceneProxy::RBezierGrassSceneProxy()
		: VertexShader(nullptr), PixelShader(nullptr), Component(nullptr)
	{
	}
	RBezierGrassSceneProxy::RBezierGrassSceneProxy(const RBezierGrassSceneProxy& Other)
		: RPrimitiveSceneProxy(Other), VertexShader(Other.VertexShader), PixelShader(Other.PixelShader), Component(Other.Component)
	{
	}
	RBezierGrassSceneProxy::~RBezierGrassSceneProxy()
	{
	}
	void RBezierGrassSceneProxy::SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices)
	{
		SetupShaders();

		SetPrimitiveSettings(InIsHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);

		MaterialParameter.SetupParameters();

		UpdateRenderResource();
		SetupVertexIndexBuffer();
	}
	void RBezierGrassSceneProxy::UpdateProperty(const EBezierGrassProperty& InProperty)
	{
		Property = InProperty;
		Property.LOD = BEZIER_GRASS_MAX_LOD_INDEX - InProperty.LOD;
	}
	void RBezierGrassSceneProxy::UpdateInstanceData(TArray<EBezierGrassInstanceData>&& InInstanceData)
	{
		if (InInstanceData.Num() > 0)
		{
			InstanceData.MoveFrom(EMemory::Forward<TArray<EBezierGrassInstanceData>>(InInstanceData));
		}
		else
		{
			InstanceData.Empty();
		}
		if (InstanceBuffer.IsRenderResourceValid())
		{
			InstanceBuffer.ReleaseRenderResource();
		}
		if (InstanceData.Num() > 0)
		{
			RSubresourceDataDesc SubresDesc;
			SubresDesc.pSysMem = InstanceData.GetData();
			RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(InstanceBuffer,
				RStructuredBufferDesc(sizeof(EBezierGrassInstanceData), InstanceData.Num<UINT32>(), TRUE),
				&SubresDesc
			);
		}
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
		MaterialParameter["_WorldMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix());
		MaterialParameter["_WorldInvMatrix"] = &TranslateUploadMatrixType(InvMat);
		MaterialParameter["_WorldInvTransposeMatrix"] = &TranslateUploadTransposeMatrixType(InvMat);

		MaterialParameter["_BezierGrassParams1"] = &TranslateUploadVectorType(Vector4(Property.LOD, Property.LeafWidth, 0.f, 0.f));
		MaterialParameter["_BezierGrassRootColor"] = &TranslateUploadVectorType(Vector4(Property.RootColor.r, Property.RootColor.g, Property.RootColor.b, Property.RootColor.a));
		MaterialParameter["_BezierGrassTipColor"] = &TranslateUploadVectorType(Vector4(Property.TipColor.r, Property.TipColor.g, Property.TipColor.b, Property.TipColor.a));

		MaterialParameter.UploadBuffer();
	}
	void RBezierGrassSceneProxy::BindRenderResource()const
	{
		BindVertexShader();
		BindPixelShader();
		BindMeshAndInstanceResource();
		BindMaterialParameter(1u);
	}
	void RBezierGrassSceneProxy::SetupVertexIndexBuffer()
	{
		VertexBuffer.ReleaseRenderResource();
		IndexBuffer.ReleaseRenderResource();

#if 0
		static_assert(BEZIER_GRASS_LOD_NUM > (0));
		static_assert(BEZIER_GRASS_MAX_LOD_INDEX == (4));
		constexpr UINT32 NumTotalVertices = RBezierGrassLODData<4>::NumTotalVertices;
		constexpr UINT32 NumTotalIndices = RBezierGrassLODData<4>::NumTotalIndices;
		constexpr auto VertexData = RBezierGrassVertexData<4>::Value;
		constexpr auto IndexData = RBezierGrassIndexData<4>::Value;

		SET_BEZIER_GRASS_BUFFER_OFFSET(0)
		SET_BEZIER_GRASS_BUFFER_OFFSET(1)
		SET_BEZIER_GRASS_BUFFER_OFFSET(2)
		SET_BEZIER_GRASS_BUFFER_OFFSET(3)
		SET_BEZIER_GRASS_BUFFER_OFFSET(4)
#else
		TArray<BEZIER_GRASS_VERTEX_TYPE> LODVertices[BEZIER_GRASS_LOD_NUM];
		TArray<BEZIER_GRASS_INDEX_TYPE> LODIndices[BEZIER_GRASS_LOD_NUM];
		UINT32 LODNumVertices[BEZIER_GRASS_LOD_NUM];
		UINT32 LODNumIndices[BEZIER_GRASS_LOD_NUM];
		auto GenerateLODIndices = [&LODNumIndices, &LODIndices](UINT32 InLODIndex)->void
		{
			const UINT32 NumParts = InLODIndex + (BEZIER_GRASS_START_BODY_PARTS);
			const UINT32 NumTriangles = NumParts * 2 + 1;

			LODNumIndices[InLODIndex] = NumTriangles * 3;
			LODIndices[InLODIndex].Reset();
			LODIndices[InLODIndex].Reserve(NumTriangles * 3);

			for (UINT32 PartIndex = 0; PartIndex < NumParts; PartIndex++)
			{
				LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 0));
				LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0));
				LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1));
				LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 0));
				LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>((PartIndex + 1) * 2 + 1));
				LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(PartIndex * 2 + 1));
			}
			LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(NumTriangles - 1));
			LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(NumTriangles + 1));
			LODIndices[InLODIndex].Add(static_cast<BEZIER_GRASS_INDEX_TYPE>(NumTriangles + 0));
		};
		auto GenerateLODVertices = [&LODNumVertices, &LODVertices](UINT32 InLODIndex)->void
		{
			const UINT32 NumParts = InLODIndex + (BEZIER_GRASS_START_BODY_PARTS);
			const UINT32 NumVertices = NumParts * 2 + 2 + 1;

			LODNumVertices[InLODIndex] = NumVertices;
			LODVertices[InLODIndex].Reset();
			LODVertices[InLODIndex].Reserve(NumVertices);

			LODVertices[InLODIndex].Add(BEZIER_GRASS_VERTEX_TYPE{ 0, 0, 255, 0 });
			LODVertices[InLODIndex].Add(BEZIER_GRASS_VERTEX_TYPE{ 0, 255, 0, 0 });
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
				LODVertices[InLODIndex].Add(BEZIER_GRASS_VERTEX_TYPE{ T, 0, W, PrevLODT });
				LODVertices[InLODIndex].Add(BEZIER_GRASS_VERTEX_TYPE{ T, W, 0, PrevLODT });
			}
			LODVertices[InLODIndex].Add(BEZIER_GRASS_VERTEX_TYPE{ 255, 0, 0, 255 });
		};
		UINT32 TempNumVertices = 0;
		UINT32 TempNumIndices = 0;
		for (UINT32 LODIndex = 0, NumLODs = BEZIER_GRASS_LOD_NUM; LODIndex < NumLODs; LODIndex++)
		{
			GenerateLODIndices(LODIndex);
			GenerateLODVertices(LODIndex);

			VertexOffset[LODIndex] = TempNumVertices;
			IndexOffset[LODIndex] = TempNumIndices;
			IndexCount[LODIndex] = LODNumIndices[LODIndex];

			TempNumVertices += LODNumVertices[LODIndex];
			TempNumIndices += LODNumIndices[LODIndex];
		}
		TArray<BEZIER_GRASS_VERTEX_TYPE> VertexData;
		TArray<BEZIER_GRASS_INDEX_TYPE> IndexData;
		VertexData.Reserve(TempNumVertices);
		IndexData.Reserve(TempNumIndices);
		for (UINT32 LODIndex = 0, NumLODs = BEZIER_GRASS_LOD_NUM; LODIndex < NumLODs; LODIndex++)
		{
			for (UINT32 LODVertexIndex = 0; LODVertexIndex < LODNumVertices[LODIndex]; LODVertexIndex++)
			{
				VertexData.Add((LODVertices[LODIndex])[LODVertexIndex]);
			}
			for (UINT32 LODIndexIndex = 0; LODIndexIndex < LODNumIndices[LODIndex]; LODIndexIndex++)
			{
				IndexData.Add((LODIndices[LODIndex])[LODIndexIndex]);
			}
		}
		const UINT32 NumTotalVertices = TempNumVertices;
		const UINT32 NumTotalIndices = TempNumIndices;
#endif
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
		const EString ImportPath(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
		const EString ImportVSName = EString("BezierGrass") + EEngineSettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
		const RInputLayoutDesc TempShaderInputLayouts[] =
		{
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0, sizeof(UINT8), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT)
		};
		if (!VertexShader)
		{
			constexpr UINT32 TempShaderInputLayoutNum = PE_ARRAYSIZE(TempShaderInputLayouts);
			TryLoadVertexShader(EBaseSettings::ENGINE_SHADER_PATH, ImportVSName,
				VertexShader,
				&ImportPath, &ImportVSName,
				TempShaderInputLayouts, &TempShaderInputLayoutNum);
		}
		if (!PixelShader)
		{
			const EString ImportPSName = EString("BezierGrass") + EEngineSettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(EBaseSettings::ENGINE_SHADER_PATH, ImportPSName,
				PixelShader,
				&ImportPath, &ImportPSName);
		}
	}
	void RBezierGrassSceneProxy::BindVertexShader()const
	{
#if _EDITOR_ONLY
		if ((!!VertexShader) && (VertexShader->IsRenderResourceValid()))
#endif
		{
			const RVertexShaderResource* VSResource = VertexShader->GetRenderResource();
			{
				RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
				RenderDevice->SetInputLayout(VSResource->InputLayout);
				RenderDevice->SetVSShader(VSResource->Shader);
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
#if _EDITOR_ONLY
		if ((!!PixelShader) && (PixelShader->IsRenderResourceValid()))
#endif
		{
			const RPixelShaderResource* PSResource = PixelShader->GetRenderResource();
			{
				RDeviceD3D11::GetDeviceSingleton()->SetPSShader(PSResource->Shader);
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
#if _EDITOR_ONLY
		if ((!!VertexShader) && (VertexShader->IsRenderResourceValid()) && (!!PixelShader) && (PixelShader->IsRenderResourceValid()))
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
				RenderDevice->BindVSShaderResourceView(InstanceBuffer.ShaderResourceView, 1);
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check static mesh scene proxy is invalid."));
		}
#endif
	}
	void RBezierGrassSceneProxy::BindMaterialParameter(const UINT32 InSlot)const
	{
		const RBufferResource& ConstantBuffer = MaterialParameter.GetConstantBuffer();
#if _EDITOR_ONLY
		if (((InSlot >= 0u) && (InSlot < 8u)) && (ConstantBuffer.IsRenderResourceValid()))
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			RenderDevice->BindVSConstantBuffer(ConstantBuffer.Buffer, InSlot);
			RenderDevice->BindPSConstantBuffer(ConstantBuffer.Buffer, InSlot);
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh constant buffer is invalid."));
		}
#endif
	}
	void RBezierGrassSceneProxy::Draw()const
	{
		if (InstanceData.Num() > 0)
		{
#if _EDITOR_ONLY
			const RIndexBufferResource& IndexRenderResource = IndexBuffer;
			if (IndexRenderResource.IsRenderResourceValid())
#endif
			{
				const UINT32 UsedLOD = EMath::Clamp((UINT32)(EMath::CeilToInt32(Property.LOD)), 0u, (UINT32)(BEZIER_GRASS_MAX_LOD_INDEX));
				const UINT32 UsedIndexOffset = IndexOffset[UsedLOD];
				const UINT32 UsedIndexCount = IndexCount[UsedLOD];
				const UINT32 UsedVertexOffset = VertexOffset[UsedLOD];
				RDeviceD3D11::GetDeviceSingleton()->DrawIndexedInstance(InstanceData.Num(), UsedIndexCount, 0, UsedIndexOffset, UsedVertexOffset);
			}
#if _EDITOR_ONLY
			else
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Draw static mesh indexed is invalid."));
			}
#endif
		}
	}

};