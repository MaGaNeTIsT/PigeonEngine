#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <RenderResource.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	enum EMeshType : UINT8
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
	enum EVertexResourceType : UINT8
	{
		VERTEX_RESOURCE_TYPE_POSITION = 0,
		VERTEX_RESOURCE_TYPE_NORMAL,
		VERTEX_RESOURCE_TYPE_UV,
		VERTEX_RESOURCE_TYPE_TANGENT,
		VERTEX_RESOURCE_TYPE_COLOR,
		VERTEX_RESOURCE_TYPE_BINORMAL,
		VERTEX_RESOURCE_TYPE_COUNT
	};
	extern UINT32 GetMeshVertexLayoutTypeStartBitIndex(EVertexLayoutType InType);
	extern EVertexLayoutType TranslateSemanticBaseTypeToVertexBaseLayout(RShaderSemanticType InBaseType);
	extern UINT32 TranslateVertexBaseLayoutToSemanticBaseType(EVertexLayoutType InBaseType);
	extern EVertexLayoutType TranslateVertexPartTypeToVertexBaseLayout(UINT32 InVertexPartType);
	extern UINT32 TranslateVertexPartTypeToVertexBaseIndex(UINT32 InVertexPartType);
	extern EVertexLayoutType TranslateVertexResourceTypeToVertexLayoutType(const UINT8 InVertexResourceType);
	extern EVertexResourceType TranslateVertexLayoutTypeToVertexResourceType(EVertexLayoutType InVertexLayoutType);

	struct EVertexDescriptor
	{
		EVertexDescriptor() noexcept : PartType(0u), ElementNum(0u), Stride(0u) {}
		EVertexDescriptor(const EVertexDescriptor& Other) noexcept : PartType(Other.PartType), ElementNum(Other.ElementNum), Stride(Other.Stride) {}
		constexpr EVertexDescriptor(EVertexLayoutType InType, UINT32 InStride = 0u, UINT32 InElementNum = 0u) noexcept : PartType(InType), ElementNum(InElementNum), Stride(InStride) {}

		UINT32	PartType;
		UINT32	ElementNum;
		UINT32	Stride;
	};
	struct EIndexData : public EVertexDescriptor
	{
		EIndexData() noexcept : EVertexDescriptor(), Indices(nullptr) {}
		EIndexData(UINT32 InStride, UINT32 InElementNum = 0u) noexcept
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
				Indices = new UINT32[ElementNum * sizeof(UINT32)];
				for (UINT32 i = 0u, n = ElementNum; i < n; i++)
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
				Indices = new UINT32[ElementNum * sizeof(UINT32)];
				for (UINT32 i = 0u, n = ElementNum; i < n; i++)
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

		UINT32*	Indices;
	};
	struct EVertexData : public EVertexDescriptor
	{
		EVertexData() noexcept : EVertexDescriptor(), Datas(nullptr) {}
		EVertexData(EVertexLayoutType InType, UINT32 InStride = 0u, UINT32 InElementNum = 0u) noexcept
#if _EDITOR_ONLY
			: EVertexDescriptor()
#else
			: EVertexDescriptor(InType, InStride, InElementNum)
#endif
			, Datas(nullptr)
		{
#if _EDITOR_ONLY
			if (((InType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) ||
				((InType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) ||
				((InType & (EVertexLayoutType::MESH_SKIN << 0)) != 0u) ||
				((InType & (EVertexLayoutType::MESH_SKIN << 1)) != 0u) ||
				((InType & (EVertexLayoutType::MESH_SKIN << 2)) != 0u) ||
				((InType & (EVertexLayoutType::MESH_SKIN << 3)) != 0u))
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
				const UINT32 StrideIn32Bits = Stride / sizeof(FLOAT);
				Datas = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT32 i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT32 StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
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
				const UINT32 StrideIn32Bits = Stride / sizeof(FLOAT);
				Datas = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT32 i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT32 StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
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
		ESkinData(UINT32 InStride, UINT32 InElementNum = 0u) noexcept
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
				const UINT32 StrideIn32Bits = Stride / sizeof(FLOAT);
				Indices = new UINT32[ElementNum * StrideIn32Bits];
				Weights = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT32 i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT32 StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
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
				const UINT32 StrideIn32Bits = Stride / sizeof(FLOAT);
				Indices = new UINT32[ElementNum * StrideIn32Bits];
				Weights = new FLOAT[ElementNum * StrideIn32Bits];
				for (UINT32 i = 0u, n = ElementNum; i < n; i++)
				{
					for (UINT32 StrideIndex = 0u; StrideIndex < StrideIn32Bits; StrideIndex++)
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

		UINT32*	Indices;
		FLOAT*	Weights;
	};
	struct ESubmeshData
	{
		ESubmeshData() noexcept : StartVertex(0u), VertexNum(0u), StartIndex(0u), IndexNum(0u) {}
		ESubmeshData(const ESubmeshData& Other) noexcept : StartVertex(Other.StartVertex), VertexNum(Other.VertexNum), StartIndex(Other.StartIndex), IndexNum(Other.IndexNum) {}
		constexpr ESubmeshData(UINT32 InStartVertex, UINT32 InVertexNum, UINT32 InStartIndex, UINT32 InIndexNum) noexcept : StartVertex(InStartVertex), VertexNum(InVertexNum), StartIndex(InStartIndex), IndexNum(InIndexNum) {}
		ESubmeshData& operator=(const ESubmeshData& Other)
		{
			StartVertex	= Other.StartVertex;
			VertexNum	= Other.VertexNum;
			StartIndex	= Other.StartIndex;
			IndexNum	= Other.IndexNum;
			return (*this);
		}

		UINT32	StartVertex;
		UINT32	VertexNum;
		UINT32	StartIndex;
		UINT32	IndexNum;
	};

	class EMesh : public EObjectBase, public EResourceInterface
	{
	public:
		typedef	EIndexData				EIndexPart;
		typedef	TArray<EVertexData>		EVertexPart;
		typedef	TArray<ESubmeshData>	ESubmeshPart;
	public:
		PE_CONSTEXPR_STATIC UINT32 MeshVertexLayoutPartMaxNum = 4u;
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		EMesh& operator=(const EMesh& Other);
	public:
		BOOL32				CheckVertexLayoutPartExist(UINT32 InVertexLayoutType, UINT32 InPartIndex)const;
		const EString&		GetMeshName()const;
		const EBoundAABB&	GetBoundAABB()const;
		UINT32				GetVertexLayout()const;
		const EIndexPart&	GetIndexPart()const;
		const EVertexPart&	GetVertexPart()const;
		const ESubmeshPart&	GetSubmeshPart()const;
	public:
		void	SetMeshName(const EString& InMeshName);
		void	SetBoxToBoundAABB(const Vector3& InOrigin, const Vector3& InExtent);
		void	SetBoundAABB(const Vector3& InMin, const Vector3& InMax);
		BOOL32	AddIndexElement(EIndexData* InIndexData);
		BOOL32	RemoveIndexElement();
		BOOL32	GetIndexElement(const EIndexData*& OutIndexData)const;
		BOOL32	AddVertexElement(EVertexData* InVertexData, UINT32* OutLayoutIndex = nullptr);
		BOOL32	RemoveVertexElement(UINT32 InVertexLayoutType, UINT32 InLayoutIndex);
		BOOL32	GetVertexElement(UINT32 InVertexLayoutType, UINT32 InLayoutIndex, const EVertexData*& OutVertexData)const;
		BOOL32	AddSubmesh(const ESubmeshData* InSubmeshData, UINT32* OutSubmeshIndex = nullptr);
		BOOL32	RemoveSubmesh(UINT32 InSubmeshIndex);
		BOOL32	GetSubmesh(UINT32 InSubmeshIndex, const ESubmeshData*& OutSubmeshData)const;
	protected:
		void	CopyBaseDataFromOtherInternal(const EMesh& Other);
		void	SetVertexLayoutPartExistInternal(UINT32 InVertexLayoutType, UINT32 InPartIndex, BOOL32 InIsExist, BOOL32* OutIsAlreadyExist = nullptr);
	protected:
		EString					MeshName;
		EBoundAABB				BoundAABB;
		UINT32					VertexLayout;
		EIndexPart				Indices;
		EVertexPart				Vertices;
		ESubmeshPart			Submeshes;
	public:
		EMesh();
		EMesh(const EString& InMeshName);
		EMesh(const EMesh& Other);
		virtual ~EMesh();
	private:
		friend class EMeshAssetManager;
	};

	class EStaticMesh : public EMesh
	{
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		EStaticMesh& operator=(const EStaticMesh& Other);
	public:
		EStaticMesh();
		EStaticMesh(const EStaticMesh& Other);
		EStaticMesh(const EString& InMeshName);
		virtual ~EStaticMesh();
	private:
		friend class EMeshAssetManager;
	};

	class ESkinnedMesh : public EMesh
	{
	public:
		using EBindPoseValue	= TMap<EString, Matrix4x4>;
		using EBindPoseIndex	= TMap<EString, USHORT>;
		using ESkinPart			= TArray<ESkinData>;
	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;
	public:
		ESkinnedMesh& operator=(const ESkinnedMesh& Other);
	public:
		BOOL32					AddBindPose(const EString& InBoneName, const Matrix4x4& InBindPose);
		BOOL32					RemoveBindPose(const EString& InBoneName);
		void					ClearBindPose();
		void					GenerateBindPoseIndex();
		const EBindPoseValue&	GetBindPoseValue()const;
		const EBindPoseIndex&	GetBindPoseIndex()const;
		USHORT					GetEffectBoneNum()const;
		BOOL32					AddSkinElement(ESkinData* InSkinData, UINT32* OutLayoutIndex = nullptr);
		BOOL32					RemoveSkinElement(UINT32 InLayoutIndex);
		BOOL32					GetSkinElement(UINT32 InLayoutIndex, const ESkinData*& OutSkinData)const;
		const ESkinPart&		GetSkinPart()const;
	protected:
		EBindPoseValue	BindPoseValue;
		EBindPoseIndex	BindPoseIndex;
		USHORT			EffectBoneNum;
		ESkinPart		Skins;
	public:
		ESkinnedMesh();
		ESkinnedMesh(const ESkinnedMesh& Other);
		ESkinnedMesh(const EString& InMeshName);
		virtual ~ESkinnedMesh();
	private:
		friend class EMeshAssetManager;
	};

	class EMeshRenderResource : public EObjectBase, public RRenderResourceInterface
	{
	public:
		using TVertexResourceType = TMap<UINT8, TArray<RVertexBufferResource*>>;
	public:
		virtual BOOL32	IsRenderResourceValid()const override;
		virtual void	ReleaseRenderResource()override;
		const RIndexBufferResource*				GetIndexRenderResource()const;
		const TArray<RVertexBufferResource*>*	GetVertexRenderResource(const UINT8 InType)const;
	public:
		EMeshRenderResource();
		EMeshRenderResource(const EMeshRenderResource& Other);
		virtual ~EMeshRenderResource();
	protected:
		BOOL32	CreateIndexRenderResourceInternal(const EMesh* InMesh);
		BOOL32	CreateVertexRenderResourceInternal(const EMesh* InMesh);
		void	CopyRenderResourcesInternal(const EMeshRenderResource* Other);
	protected:
		RIndexBufferResource	IndexRenderResource;
		TVertexResourceType		VertexRenderResources;
	};

	class EStaticMeshRenderResource : public EMeshRenderResource
	{
	public:
		virtual BOOL32	IsRenderResourceValid()const override;
		virtual BOOL32	InitRenderResource()override;
		virtual void	ReleaseRenderResource()override;
	public:
		EStaticMeshRenderResource& operator=(const EStaticMeshRenderResource& Other);
	public:
		EStaticMeshRenderResource();
		EStaticMeshRenderResource(EStaticMesh* InMesh);
		EStaticMeshRenderResource(const EStaticMeshRenderResource& Other);
		virtual ~EStaticMeshRenderResource();
	protected:
		EStaticMesh* StaticMesh;
	};

	class ESkinnedMeshRenderResource : public EMeshRenderResource
	{
	public:
		virtual BOOL32	IsRenderResourceValid()const override;
		virtual BOOL32	InitRenderResource()override;
		virtual void	ReleaseRenderResource()override;
	public:
		ESkinnedMeshRenderResource& operator=(const ESkinnedMeshRenderResource& Other);
	public:
		ESkinnedMeshRenderResource();
		ESkinnedMeshRenderResource(const class ESkeleton* InSkeleton, ESkinnedMesh* InMesh);
		ESkinnedMeshRenderResource(const ESkinnedMeshRenderResource& Other);
		virtual ~ESkinnedMeshRenderResource();
	protected:
		BOOL32	CreateMeshSkeletonRenderResourceInternal();
		BOOL32	CreateSkinRenderResourceInternal();
		void	CopySkinRenderResourcesInternal(const ESkinnedMeshRenderResource* Other);
	protected:
		const class ESkeleton*	Skeleton;
		RStructuredBuffer		MeshSkeletonRenderResource;
		ESkinnedMesh*			SkinnedMesh;
		TArray<RBufferResource>	SkinRenderResources;
	};

	template<EMeshType _MeshType, typename TMeshResourceType, typename TMeshRenderResourceType>
	class TMeshBaseAsset : public TRenderBaseAsset<TMeshResourceType, TMeshRenderResourceType>
	{
	public:
		TMeshBaseAsset(const EString& InMeshPath, const EString& InMeshName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		) : TRenderBaseAsset<TMeshResourceType, TMeshRenderResourceType>(InMeshPath, InMeshName
#if _EDITOR_ONLY
			, InDebugName
#endif
		), MeshType(_MeshType)
		{
		}
		virtual ~TMeshBaseAsset()
		{
		}
	public:
		EMeshType	GetMeshType()const { return MeshType; }
	protected:
		EMeshType	MeshType;
	public:
		TMeshBaseAsset() = delete;

		CLASS_REMOVE_COPY_BODY(TMeshBaseAsset)

	};

	class EStaticMeshAsset : public TMeshBaseAsset<EMeshType::MESH_TYPE_STATIC, EStaticMesh, EStaticMeshRenderResource>
	{
	public:
		EStaticMeshAsset(const EString& InMeshPath, const EString& InMeshName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EStaticMeshAsset();
	public:
		virtual BOOL32	InitResource()override;
	protected:
		EStaticMeshRenderResource*	CreateMeshResource(EStaticMesh* InResource);
	public:
		EStaticMeshAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EStaticMeshAsset)

	};

	class ESkinnedMeshAsset : public TMeshBaseAsset<EMeshType::MESH_TYPE_SKIN, ESkinnedMesh, ESkinnedMeshRenderResource>
	{
	public:
		ESkinnedMeshAsset(const EString& InMeshPath, const EString& InMeshName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ESkinnedMeshAsset();
	public:
		virtual BOOL32	InitResource()override;
	public:
		void	SetSkeleton(const class ESkeleton* InSkeleton);
		const class ESkeleton*	GetSkeleton()const;
	protected:
		ESkinnedMeshRenderResource*	CreateMeshResource(ESkinnedMesh* InResource);
	protected:
		const class ESkeleton* Skeleton;
	public:
		ESkinnedMeshAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ESkinnedMeshAsset)

	};

	class EMeshAssetManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, EStaticMeshAsset>	EStaticMeshAssetManager;
		typedef TAssetManager<EString, ESkinnedMeshAsset>	ESkinnedMeshAssetManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
#if _EDITOR_ONLY
		BOOL32	ImportStaticMesh(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
		BOOL32	ImportSkinnedMesh(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath);
#endif
		BOOL32	LoadStaticMeshAsset(const EString& InLoadPath, const EString& InLoadName, const EStaticMeshAsset*& OutStaticMeshAsset);
		BOOL32	LoadSkinnedMeshAsset(const EString& InLoadPath, const EString& InLoadName, const ESkinnedMeshAsset*& OutSkinnedMeshAsset);
	private:
		void	ClearStaticMeshes();
		void	ClearSkinnedMeshes();
	private:
		template<typename _TMeshAssetType, typename _TMeshResourceType>
		_TMeshAssetType* LoadMeshAsset(const EString& InLoadPath, const EString& InLoadName);
		template<typename _TMeshAssetType, typename _TMeshResourceType>
		BOOL32 SaveMeshAsset(const EString& InSavePath, const EString& InSaveName, const _TMeshAssetType* InMeshAsset);
		template<typename _TMeshResourceType>
		_TMeshResourceType* LoadMeshResource(const EString& InLoadPath, const EString& InLoadName);
		template<typename _TMeshResourceType>
		BOOL32 SaveMeshResource(const EString& InSavePath, const EString& InSaveName, const _TMeshResourceType* InMeshResource);
	private:
		EStaticMeshAssetManager		StaticMeshManager;
		ESkinnedMeshAssetManager	SkinnedMeshManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EMeshAssetManager)

	};

	extern void TryLoadStaticMesh(const EString& InLoadPath, const EString& InLoadName,
		const EStaticMeshAsset*& OutAsset,
		const EString* InImportPath = nullptr, const EString* InImportName = nullptr, const EString* InImportFileType = nullptr);
	extern void TryLoadSkinnedMesh(const EString& InLoadPath, const EString& InLoadName,
		const ESkinnedMeshAsset*& OutAsset,
		const EString* InImportPath = nullptr, const EString* InImportName = nullptr, const EString* InImportFileType = nullptr);

};