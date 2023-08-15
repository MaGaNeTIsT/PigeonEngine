#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <RenderDevice/DeviceD3D11.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	enum EMeshType
	{
		MESH_TYPE_STATIC	= 0,
		MESH_TYPE_SKIN		= 1,
		MESH_TYPE_COUNT
	};

	// 32 bit number                    31 30| 29    26| 25    22| 21    18| 17    14| 13    10| 9     6 | 5     2 | 1 0
	// Vertex layout by bit (32 bits) :  0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0
	// Vertex layout layer :           NotUse|   Skin  |Bitangent|  Color  | Tangent |  Normal | TexCoord|  Vertex | Index
	enum EVertexLayoutType : UINT32
	{
		MESH_INDEX_FULL		= (1 << 0),
		MESH_INDEX_HALF		= (1 << 1),
		MESH_VERTEX			= (1 << 2),
		MESH_TEXTURECOORD	= (1 << 6),
		MESH_NORMAL			= (1 << 10),
		MESH_TANGENT		= (1 << 14),
		MESH_COLOR			= (1 << 18),
		MESH_BITANGENT		= (1 << 22),
		MESH_SKIN			= (1 << 26)
	};
	extern UINT GetMeshVertexLayoutTypeStartBitIndex(EVertexLayoutType InType);
	extern EVertexLayoutType TranslateSemanticBaseTypeToVertexBaseLayout(RShaderSemanticType InBaseType);
	extern UINT TranslateVertexBaseLayoutToSemanticBaseType(EVertexLayoutType InBaseType);

	struct EVertexDescriptor
	{
		EVertexDescriptor() noexcept : PartType(0u), ElementNum(0u), Stride(0u) {}
		EVertexDescriptor(const EVertexDescriptor& Other) noexcept : PartType(Other.PartType), ElementNum(Other.ElementNum), Stride(Other.Stride) {}
		constexpr EVertexDescriptor(EVertexLayoutType InType, UINT InStride = 0u, UINT InElementNum = 0u) noexcept : PartType(InType), ElementNum(InElementNum), Stride(InStride) {}

		UINT	PartType;
		UINT	ElementNum;
		UINT	Stride;
	};
	struct EIndexData : public EVertexDescriptor
	{
		EIndexData() noexcept : EVertexDescriptor(), Indices(nullptr) {}
		EIndexData(UINT InStride, UINT InElementNum = 0u) noexcept
			: EVertexDescriptor(), Indices(nullptr)
		{
#if _EDITOR_ONLY
			if ((InStride != 2u) && (InStride != 4u))
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Check index data error(stride is not 4 bytes or 2 bytes)."));
			}
			else
#endif
			{
				PartType = (InStride == 4u) ? EVertexLayoutType::MESH_INDEX_FULL : EVertexLayoutType::MESH_INDEX_HALF;
				Stride = (PartType == EVertexLayoutType::MESH_INDEX_FULL) ? 4u : 2u;
			}
			ElementNum = InElementNum;
		}
		EIndexData(const EIndexData& Other)
			: Indices(nullptr)
		{
			PartType	= Other.PartType;
			Stride		= Other.Stride;
			ElementNum	= Other.ElementNum;
			if (Other.Indices && Other.Stride > 0u && Other.ElementNum > 0u)
			{
				Indices = new UINT[ElementNum * sizeof(UINT)];
				for (UINT i = 0u, n = ElementNum; i < n; i++)
				{
					Indices[i] = Other.Indices[i];
				}
			}
		}
		EIndexData& operator=(const EIndexData& Other)
		{
			PartType	= Other.PartType;
			Stride		= Other.Stride;
			ElementNum	= Other.ElementNum;
			if (Indices)
			{
				delete[]Indices;
				Indices = nullptr;
			}
			if (Other.Indices && Other.Stride > 0u && Other.ElementNum > 0u)
			{
				Indices = new UINT[ElementNum * sizeof(UINT)];
				for (UINT i = 0u, n = ElementNum; i < n; i++)
				{
					Indices[i] = Other.Indices[i];
				}
			}
			return (*this);
		}
		void Release()
		{
			if (Indices)
			{
				delete[]Indices;
				Indices = nullptr;
			}
			PartType	= 0u;
			ElementNum	= 0u;
			Stride		= 0u;
		}

		UINT*	Indices;
	};
	struct EVertexData : public EVertexDescriptor
	{
		EVertexData() noexcept : EVertexDescriptor(), Datas(nullptr) {}
		EVertexData(EVertexLayoutType InType, UINT InStride = 0u, UINT InElementNum = 0u) noexcept
#if _EDITOR_ONLY
			: EVertexDescriptor()
#else
			: EVertexDescriptor(InType, InStride, InElementNum)
#endif
			, Datas(nullptr)
		{
#if _EDITOR_ONLY
			if ((InType == EVertexLayoutType::MESH_INDEX_FULL) || (InType == EVertexLayoutType::MESH_INDEX_HALF) || (InType == EVertexLayoutType::MESH_SKIN))
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Check vertex data error(part type is invalid)."));
			}
			else if ((InStride % 4u) != 0u)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Check vertex data error(stride is not time of 4 bytes)."));
			}
			else
			{
				PartType = InType;
				Stride = InStride;
			}
			ElementNum = InElementNum;
#endif
		}
		EVertexData(const EVertexData& Other)
			: Datas(nullptr)
		{
			PartType	= Other.PartType;
			Stride		= Other.Stride;
			ElementNum	= Other.ElementNum;
			if (Other.Datas && Other.Stride > 0u && Other.ElementNum > 0u)
			{
				const UINT StrideIn32Bits = Stride / sizeof(FLOAT);
				Datas = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
					{
						Datas[i * StrideIn32Bits + StrideIndex] = Other.Datas[i * StrideIn32Bits + StrideIndex];
					}
				}
			}
		}
		EVertexData& operator=(const EVertexData& Other)
		{
			PartType	= Other.PartType;
			Stride		= Other.Stride;
			ElementNum	= Other.ElementNum;
			if (Datas)
			{
				delete[]Datas;
				Datas = nullptr;
			}
			if (Other.Datas && Other.Stride > 0u && Other.ElementNum > 0u)
			{
				const UINT StrideIn32Bits = Stride / sizeof(FLOAT);
				Datas = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
					{
						Datas[i * StrideIn32Bits + StrideIndex] = Other.Datas[i * StrideIn32Bits + StrideIndex];
					}
				}
			}
			return (*this);
		}
		void Release()
		{
			if (Datas)
			{
				delete[]Datas;
				Datas = nullptr;
			}
			PartType	= 0u;
			ElementNum	= 0u;
			Stride		= 0u;
		}

		FLOAT*	Datas;
	};
	struct ESkinData : public EVertexDescriptor
	{
		ESkinData() noexcept : EVertexDescriptor(), Indices(nullptr), Weights(nullptr) {}
		ESkinData(UINT InStride, UINT InElementNum = 0u) noexcept
#if _EDITOR_ONLY
			: EVertexDescriptor(EVertexLayoutType::MESH_SKIN)
#else
			: EVertexDescriptor(EVertexLayoutType::MESH_SKIN, InStride, InElementNum)
#endif
			, Indices(nullptr), Weights(nullptr)
		{
#if _EDITOR_ONLY
			if ((InStride % 4u) != 0u)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Check skin data error(stride is not time of 4 bytes)."));
			}
			else
			{
				Stride = InStride;
			}
			ElementNum = InElementNum;
#endif
		}
		ESkinData(const ESkinData& Other)
			: Indices(nullptr), Weights(nullptr)
		{
			PartType	= Other.PartType;
			Stride		= Other.Stride;
			ElementNum	= Other.ElementNum;
			if (Other.Indices && Other.Weights && Other.Stride > 0u && Other.ElementNum > 0u)
			{
				const UINT StrideIn32Bits = Stride / sizeof(FLOAT);
				Indices = new UINT[ElementNum * StrideIn32Bits];
				Weights = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
					{
						Indices[i * StrideIn32Bits + StrideIndex] = Other.Indices[i * StrideIn32Bits + StrideIndex];
						Weights[i * StrideIn32Bits + StrideIndex] = Other.Weights[i * StrideIn32Bits + StrideIndex];
					}
				}
			}
		}
		ESkinData& operator=(const ESkinData& Other)
		{
			PartType	= Other.PartType;
			Stride		= Other.Stride;
			ElementNum	= Other.ElementNum;
			if (Indices)
			{
				delete[]Indices;
				Indices = nullptr;
			}
			if (Weights)
			{
				delete[]Weights;
				Weights = nullptr;
			}
			if (Other.Indices && Other.Weights && Other.Stride > 0u && Other.ElementNum > 0u)
			{
				const UINT StrideIn32Bits = Stride / sizeof(FLOAT);
				Indices = new UINT[ElementNum * StrideIn32Bits];
				Weights = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
					{
						Indices[i * StrideIn32Bits + StrideIndex] = Other.Indices[i * StrideIn32Bits + StrideIndex];
						Weights[i * StrideIn32Bits + StrideIndex] = Other.Weights[i * StrideIn32Bits + StrideIndex];
					}
				}
			}
			return (*this);
		}
		void Release()
		{
			if (Indices)
			{
				delete[]Indices;
				Indices = nullptr;
			}
			if (Weights)
			{
				delete[]Weights;
				Weights = nullptr;
			}
			PartType	= 0u;
			ElementNum	= 0u;
			Stride		= 0u;
		}

		UINT*	Indices;
		FLOAT*	Weights;
	};
	struct ESubmeshData
	{
		ESubmeshData() noexcept : StartVertex(0u), VertexNum(0u), StartIndex(0u), IndexNum(0u) {}
		ESubmeshData(const ESubmeshData& Other) noexcept : StartVertex(Other.StartVertex), VertexNum(Other.VertexNum), StartIndex(Other.StartIndex), IndexNum(Other.IndexNum) {}
		constexpr ESubmeshData(UINT InStartVertex, UINT InVertexNum, UINT InStartIndex, UINT InIndexNum) noexcept : StartVertex(InStartVertex), VertexNum(InVertexNum), StartIndex(InStartIndex), IndexNum(InIndexNum) {}
		ESubmeshData& operator=(const ESubmeshData& Other)
		{
			StartVertex	= Other.StartVertex;
			VertexNum	= Other.VertexNum;
			StartIndex	= Other.StartIndex;
			IndexNum	= Other.IndexNum;
			return (*this);
		}

		UINT	StartVertex;
		UINT	VertexNum;
		UINT	StartIndex;
		UINT	IndexNum;
	};

	class EMesh : public EObjectBase
	{

		EClass(EMesh, EObjectBase)

	public:
		typedef	EIndexData				EIndexPart;
		typedef	TArray<EVertexData>		EVertexPart;
		typedef	TArray<ESubmeshData>	ESubmeshPart;
	public:
		const static UINT MeshVertexLayoutPartMaxNum = 4;
	public:
		EMesh();
		EMesh(const EString& InMeshName);
		EMesh(const EMesh& Other);
		virtual ~EMesh();
		EMesh& operator=(const EMesh& Other);
		virtual void Release();
	public:
		BOOL				CheckVertexLayoutPartExist(EVertexLayoutType InLayoutType, UINT InPartIndex)const;
		const EString&		GetMeshName()const;
		const EBoundAABB&	GetBoundAABB()const;
		UINT32				GetVertexLayout()const;
		const EIndexPart&	GetIndices()const;
		const EVertexPart&	GetVertices()const;
		const ESubmeshPart&	GetSubmeshes()const;
	public:
		void	SetMeshName(const EString& InMeshName);
		void	SetBoxToBoundAABB(const Vector3& InOrigin, const Vector3& InExtent);
		void	SetBoundAABB(const Vector3& InMin, const Vector3& InMax);
		BOOL	AddIndexElement(EIndexData* InIndexData);
		BOOL	RemoveIndexElement();
		BOOL	GetIndexElement(const EIndexData*& OutIndexData)const;
		BOOL	AddVertexElement(EVertexData* InVertexData, UINT* OutLayoutIndex = nullptr);
		BOOL	RemoveVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex);
		BOOL	GetVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex, const EVertexData*& OutVertexData)const;
		BOOL	AddSubmesh(const ESubmeshData* InSubmeshData, UINT* OutSubmeshIndex = nullptr);
		BOOL	RemoveSubmesh(UINT InSubmeshIndex);
		BOOL	GetSubmesh(UINT InSubmeshIndex, const ESubmeshData*& OutSubmeshData)const;
	protected:
		void	CopyBaseDataFromOtherInternal(const EMesh& Other);
		void	SetVertexLayoutPartExistInternal(EVertexLayoutType InLayoutType, UINT InPartIndex, BOOL InIsExist, BOOL* OutIsAlreadyExist = nullptr);
	protected:
		EString					MeshName;
		EBoundAABB				BoundAABB;
		UINT32					VertexLayout;
		EIndexPart				Indices;
		EVertexPart				Vertices;
		ESubmeshPart			Submeshes;
	};

	class EStaticMesh : public EMesh
	{

		EClass(EStaticMesh, EMesh)

	public:
		EStaticMesh();
		EStaticMesh(const EStaticMesh& Other);
		EStaticMesh(const EString& InMeshName);
		virtual ~EStaticMesh();
		EStaticMesh& operator=(const EStaticMesh& Other);
		virtual void Release()override;
	};

	class ESkinnedMesh : public EMesh
	{

		EClass(ESkinnedMesh, EMesh)

	public:
		using EBindPoseValue	= TMap<EString, Matrix4x4>;
		using EBindPoseIndex	= TMap<EString, USHORT>;
		using ESkinPart			= TArray<ESkinData>;
	public:
		ESkinnedMesh();
		ESkinnedMesh(const ESkinnedMesh& Other);
		ESkinnedMesh(const EString& InMeshName);
		virtual ~ESkinnedMesh();
		ESkinnedMesh& operator=(const ESkinnedMesh& Other);
		virtual void Release()override;
	public:
		BOOL					AddBindPose(const EString& InBoneName, const Matrix4x4& InBindPose);
		BOOL					RemoveBindPose(const EString& InBoneName);
		void					ClearBindPose();
		void					GenerateBindPoseIndex();
		const EBindPoseValue&	GetBindPoseValue()const;
		const EBindPoseIndex&	GetBindPoseIndex()const;
		USHORT					GetEffectBoneNum()const;
		BOOL					AddSkinElement(ESkinData* InSkinData, UINT* OutLayoutIndex = nullptr);
		BOOL					RemoveSkinElement(UINT InLayoutIndex);
		BOOL					GetSkinElement(UINT InLayoutIndex, const ESkinData*& OutSkinData)const;
	protected:
		EBindPoseValue	BindPoseValue;
		EBindPoseIndex	BindPoseIndex;
		USHORT			EffectBoneNum;
		ESkinPart		Skins;
	};

	class EMeshRenderResource : public EObjectBase
	{

		EClass(EMeshRenderResource, EObjectBase)

	public:
		EMeshRenderResource(EMesh* InMesh);
		virtual ~EMeshRenderResource();
	public:
		void Release();
	protected:
		EMesh*	Mesh;
		TArray<RDeviceD3D11::RBufferResource> RenderResources;
	public:
		EMeshRenderResource() = delete;

		CLASS_REMOVE_COPY_BODY(EMeshRenderResource)

	};

	template<EMeshType _MeshType, typename TMeshResourceType, typename TMeshRenderResourceType>
	class TMeshBaseAsset : public TRenderBaseAsset<TMeshResourceType, TMeshRenderResourceType>
	{
	public:
		typedef TRenderBaseAsset<TMeshResourceType, TMeshRenderResourceType> Super;

		EClass(TMeshBaseAsset, Super)

	public:
		TMeshBaseAsset(
			const EString& InMeshPath
#ifdef _EDITOR_ONLY
			, const EString& InDebugName
#endif
		) : TRenderBaseAsset<TMeshResourceType, TMeshRenderResourceType>(
#ifdef _EDITOR_ONLY
			InDebugName
#endif
		), MeshPath(InMeshPath), MeshType(_MeshType)
		{
		}
		virtual ~TMeshBaseAsset()
		{
		}
	public:
		const EString&	GetMeshPath()const { return MeshPath; }
		EMeshType		GetMeshType()const { return MeshType; }
	protected:
		EString		MeshPath;
		EMeshType	MeshType;

	public:
		TMeshBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TMeshBaseAsset)

	};

	class EStaticMeshAsset : public TMeshBaseAsset<EMeshType::MESH_TYPE_STATIC, EStaticMesh, EMeshRenderResource>
	{
	public:
		typedef TMeshBaseAsset<EMeshType::MESH_TYPE_STATIC, EStaticMesh, EMeshRenderResource> Super;

		EClass(EStaticMeshAsset, Super)

	public:
		EStaticMeshAsset(
			const EString& InMeshPath
#ifdef _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EStaticMeshAsset();
	public:
		virtual BOOL InitResource()override;
	public:
		EStaticMeshAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EStaticMeshAsset)

	};

};