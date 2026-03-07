#include "MeshAsset.h"
#include <RenderDevice/DeviceD3D11.h>
#include <SkeletonAsset/SkeletonAsset.h>
#if _EDITOR_ONLY
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#endif

namespace PigeonEngine
{

	template <typename _TAssetResourceType, typename _TAssetRenderResourceType, EMeshType _MeshType, typename _MeshAssetType>
	static void RegisterMeshClassTypes()
	{
		RegisterRenderBaseAssetClassTypes<_TAssetResourceType, _TAssetRenderResourceType>();
		RegisterClassType<TMeshBaseAsset<_MeshType, _TAssetResourceType, _TAssetRenderResourceType>, TRenderBaseAsset<_TAssetResourceType, _TAssetRenderResourceType>>();
		RegisterClassType<_MeshAssetType, TMeshBaseAsset<_MeshType, _TAssetResourceType, _TAssetRenderResourceType>>();
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<EMesh, EObjectBase, EResourceInterface>();
		RegisterClassType<EStaticMesh, EMesh>();
		RegisterClassType<ESkinnedMesh, EMesh>();
		RegisterClassType<EMeshRenderResource, EObjectBase, RRenderResourceInterface>();
		RegisterClassType<EStaticMeshRenderResource, EMeshRenderResource>();
		RegisterClassType<ESkinnedMeshRenderResource, EMeshRenderResource>();

		RegisterMeshClassTypes<EStaticMesh, EStaticMeshRenderResource, EMeshType::MESH_TYPE_STATIC, EStaticMeshAsset>();
		RegisterMeshClassTypes<ESkinnedMesh, ESkinnedMeshRenderResource, EMeshType::MESH_TYPE_SKIN, ESkinnedMeshAsset>();

		RegisterClassType<EMeshAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	UINT32 GetMeshVertexLayoutTypeStartBitIndex(EVertexLayoutType InType)
	{
		switch (InType)
		{
		case EVertexLayoutType::MESH_INDEX_FULL:
			return 0u;
		case EVertexLayoutType::MESH_INDEX_HALF:
			return 1u;
		case EVertexLayoutType::MESH_VERTEX:
			return 2u;
		case EVertexLayoutType::MESH_TEXTURECOORD:
			return 6u;
		case EVertexLayoutType::MESH_NORMAL:
			return 10u;
		case EVertexLayoutType::MESH_TANGENT:
			return 14u;
		case EVertexLayoutType::MESH_COLOR:
			return 18u;
		case EVertexLayoutType::MESH_BITANGENT:
			return 22u;
		case EVertexLayoutType::MESH_SKIN:
			return 26u;
		}
		//TODO 32 bit index can not get 32 in fact.
		PE_FAILED((ENGINE_ASSET_ERROR), ("MeshVertexLayoutType 32 bit index can not get 32 in fact."));
		return 32u;
	}
	EVertexLayoutType TranslateSemanticBaseTypeToVertexBaseLayout(RShaderSemanticType InBaseType)
	{
		switch (InBaseType)
		{
		case RShaderSemanticType::SHADER_SEMANTIC_POSITION:
			return (EVertexLayoutType::MESH_VERTEX);
		case RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD:
			return (EVertexLayoutType::MESH_TEXTURECOORD);
		case RShaderSemanticType::SHADER_SEMANTIC_NORMAL:
			return (EVertexLayoutType::MESH_NORMAL);
		case RShaderSemanticType::SHADER_SEMANTIC_TANGENT:
			return (EVertexLayoutType::MESH_TANGENT);
		case RShaderSemanticType::SHADER_SEMANTIC_COLOR:
			return (EVertexLayoutType::MESH_COLOR);
		case RShaderSemanticType::SHADER_SEMANTIC_BINORMAL:
			return (EVertexLayoutType::MESH_BITANGENT);
		case RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES:
			return (EVertexLayoutType::MESH_SKIN);
		case RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT:
			return (EVertexLayoutType::MESH_SKIN);
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Do not support this shader semantic type for mesh layout."));
		return (EVertexLayoutType::MESH_INDEX_FULL);	//TODO Not support type is fix output index type for now.
	}
	UINT32 TranslateVertexBaseLayoutToSemanticBaseType(EVertexLayoutType InBaseType)
	{
		switch (InBaseType)
		{
		case EVertexLayoutType::MESH_INDEX_FULL:
			return (RShaderSemanticType::SHADER_SEMANTIC_NONE);
		case EVertexLayoutType::MESH_INDEX_HALF:
			return (RShaderSemanticType::SHADER_SEMANTIC_NONE);
		case EVertexLayoutType::MESH_VERTEX:
			return (RShaderSemanticType::SHADER_SEMANTIC_POSITION);
		case EVertexLayoutType::MESH_TEXTURECOORD:
			return (RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD);
		case EVertexLayoutType::MESH_NORMAL:
			return (RShaderSemanticType::SHADER_SEMANTIC_NORMAL);
		case EVertexLayoutType::MESH_TANGENT:
			return (RShaderSemanticType::SHADER_SEMANTIC_TANGENT);
		case EVertexLayoutType::MESH_COLOR:
			return (RShaderSemanticType::SHADER_SEMANTIC_COLOR);
		case EVertexLayoutType::MESH_BITANGENT:
			return (RShaderSemanticType::SHADER_SEMANTIC_BINORMAL);
		case EVertexLayoutType::MESH_SKIN:
			return ((RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES) | (RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT));	//TODO Skin is mapping two type for now.
		}
		return (RShaderSemanticType::SHADER_SEMANTIC_NONE);
	}
	EVertexLayoutType TranslateVertexPartTypeToVertexBaseLayout(UINT32 InVertexPartType)
	{
		if ((InVertexPartType & 0x3u) != 0u)
		{
			if ((InVertexPartType & 0x1u) != 0u)
			{
				return (EVertexLayoutType::MESH_INDEX_FULL);
			}
			else if ((InVertexPartType & 0x2u) != 0u)
			{
				return (EVertexLayoutType::MESH_INDEX_HALF);
			}
		}
		else if ((InVertexPartType & (0xfu << 2u)) != 0u)
		{
			return (EVertexLayoutType::MESH_VERTEX);
		}
		else if ((InVertexPartType & (0xfu << 6u)) != 0u)
		{
			return (EVertexLayoutType::MESH_TEXTURECOORD);
		}
		else if ((InVertexPartType & (0xfu << 10u)) != 0u)
		{
			return (EVertexLayoutType::MESH_NORMAL);
		}
		else if ((InVertexPartType & (0xfu << 14u)) != 0u)
		{
			return (EVertexLayoutType::MESH_TANGENT);
		}
		else if ((InVertexPartType & (0xfu << 18u)) != 0u)
		{
			return (EVertexLayoutType::MESH_COLOR);
		}
		else if ((InVertexPartType & (0xfu << 22u)) != 0u)
		{
			return (EVertexLayoutType::MESH_BITANGENT);
		}
		else if ((InVertexPartType & (0xfu << 26u)) != 0u)
		{
			return (EVertexLayoutType::MESH_SKIN);
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Do not support this vertex part type for mesh layout."));
		return (EVertexLayoutType::MESH_INDEX_FULL);	//TODO Not support type is fix output index type for now.
	}
	UINT32 TranslateVertexPartTypeToVertexBaseIndex(UINT32 InVertexPartType)
	{
		if ((InVertexPartType & 0x3u) != 0u)
		{
			if ((InVertexPartType & 0x1u) != 0u)
			{
				return (EVertexLayoutType::MESH_INDEX_FULL);
			}
			else if ((InVertexPartType & 0x2u) != 0u)
			{
				return (EVertexLayoutType::MESH_INDEX_HALF);
			}
		}
		else if ((InVertexPartType & (0xfu << 2u)) != 0u)
		{
			return ((InVertexPartType >> 2u) & 0xfu);
		}
		else if ((InVertexPartType & (0xfu << 6u)) != 0u)
		{
			return ((InVertexPartType >> 6u) & 0xfu);
		}
		else if ((InVertexPartType & (0xfu << 10u)) != 0u)
		{
			return ((InVertexPartType >> 10u) & 0xfu);
		}
		else if ((InVertexPartType & (0xfu << 14u)) != 0u)
		{
			return ((InVertexPartType >> 14u) & 0xfu);
		}
		else if ((InVertexPartType & (0xfu << 18u)) != 0u)
		{
			return ((InVertexPartType >> 18u) & 0xfu);
		}
		else if ((InVertexPartType & (0xfu << 22u)) != 0u)
		{
			return ((InVertexPartType >> 22u) & 0xfu);
		}
		else if ((InVertexPartType & (0xfu << 26u)) != 0u)
		{
			return ((InVertexPartType >> 26u) & 0xfu);
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Do not support this vertex part type for mesh layout."));
		return (EVertexLayoutType::MESH_INDEX_FULL);	//TODO Not support type is fix output index type for now.
	}
	EVertexLayoutType TranslateVertexResourceTypeToVertexLayoutType(const UINT8 InVertexResourcesType)
	{
		PE_CHECK((ENGINE_ASSET_ERROR), ("Translate vertex resource type to vertex layout type failed."), ((InVertexResourcesType >= EVertexResourceType::VERTEX_RESOURCE_TYPE_POSITION) && (InVertexResourcesType < EVertexResourceType::VERTEX_RESOURCE_TYPE_COUNT)));
		const EVertexLayoutType TempVertexLayoutLists[] =
		{
			EVertexLayoutType::MESH_VERTEX,
			EVertexLayoutType::MESH_NORMAL,
			EVertexLayoutType::MESH_TEXTURECOORD,
			EVertexLayoutType::MESH_TANGENT,
			EVertexLayoutType::MESH_COLOR,
			EVertexLayoutType::MESH_BITANGENT
		};
		return TempVertexLayoutLists[InVertexResourcesType];
	}
	EVertexResourceType TranslateVertexLayoutTypeToVertexResourceType(EVertexLayoutType InVertexLayoutType)
	{
		PE_CHECK((ENGINE_ASSET_ERROR), ("Translate vertex resource type to vertex layout type failed."), ((InVertexLayoutType > EVertexLayoutType::MESH_INDEX_HALF) && (InVertexLayoutType < (0x1u << 30u))));
		const UINT32 TempVertexLayoutType = InVertexLayoutType;
		if ((TempVertexLayoutType & 0x3u) != 0u)
		{
			if ((TempVertexLayoutType & 0x1u) != 0u)
			{
				// Not support
				return (EVertexResourceType::VERTEX_RESOURCE_TYPE_COUNT);
			}
			else if ((TempVertexLayoutType & 0x2u) != 0u)
			{
				// Not support
				return (EVertexResourceType::VERTEX_RESOURCE_TYPE_COUNT);
			}
		}
		else if ((TempVertexLayoutType & (0xfu << 2u)) != 0u)
		{
			return (EVertexResourceType::VERTEX_RESOURCE_TYPE_POSITION);
		}
		else if ((TempVertexLayoutType & (0xfu << 6u)) != 0u)
		{
			return (EVertexResourceType::VERTEX_RESOURCE_TYPE_UV);
		}
		else if ((TempVertexLayoutType & (0xfu << 10u)) != 0u)
		{
			return (EVertexResourceType::VERTEX_RESOURCE_TYPE_NORMAL);
		}
		else if ((TempVertexLayoutType & (0xfu << 14u)) != 0u)
		{
			return (EVertexResourceType::VERTEX_RESOURCE_TYPE_TANGENT);
		}
		else if ((TempVertexLayoutType & (0xfu << 18u)) != 0u)
		{
			return (EVertexResourceType::VERTEX_RESOURCE_TYPE_COLOR);
		}
		else if ((TempVertexLayoutType & (0xfu << 22u)) != 0u)
		{
			return (EVertexResourceType::VERTEX_RESOURCE_TYPE_BINORMAL);
		}
		else if ((TempVertexLayoutType & (0xfu << 26u)) != 0u)
		{
			// Not support
			return (EVertexResourceType::VERTEX_RESOURCE_TYPE_COUNT);
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Do not support this vertex part type for vertex resource."));
		return (EVertexResourceType::VERTEX_RESOURCE_TYPE_COUNT);	// Not support type is fix output index type for now.
	}

	EMesh::EMesh()
		: MeshName(EEngineSettings::ENGINE_DEFAULT_NAME)
		, VertexLayout(0u)
	{
#if _EDITOR_ONLY
		DebugName = EEngineSettings::ENGINE_DEFAULT_NAME;
#endif
	}
	EMesh::EMesh(const EString& InMeshName)
		: MeshName(InMeshName)
		, VertexLayout(0u)
	{
#if _EDITOR_ONLY
		DebugName = InMeshName;
#endif
	}
	EMesh::EMesh(const EMesh& Other)
		: MeshName(Other.MeshName)
		, BoundAABB(Other.BoundAABB)
		, VertexLayout(Other.VertexLayout)
		, Indices(Other.Indices)
	{
		for (INT32 i = 0, n = Other.Vertices.Num(); i < n; i++)
		{
			const EVertexData& OtherVertex = Other.Vertices[i];
			Vertices.Add(EVertexData());
			Vertices[Vertices.Num() - 1] = OtherVertex;
		}
		for (INT32 i = 0, n = Other.Submeshes.Num(); i < n; i++)
		{
			const ESubmeshData& OtherSubmesh = Other.Submeshes[i];
			Submeshes.Add(ESubmeshData());
			Submeshes[Submeshes.Num() - 1] = OtherSubmesh;
		}
	}
	EMesh::~EMesh()
	{
		Indices.Release();
		for (INT32 Index = 0, Length = Vertices.Num(); Index < Length; Index++)
		{
			Vertices[Index].Release();
		}
	}
	EMesh& EMesh::operator=(const EMesh& Other)
	{
		CopyBaseDataFromOtherInternal(Other);
		return (*this);
	}
	BOOL32 EMesh::IsResourceValid()const
	{
		if (Vertices.Num() > 0)
		{
			BOOL32 Result = FALSE;
			for (INT32 i = 0, n = Vertices.Num(); i < n; i++)
			{
				const EVertexData& TempVertexData = Vertices[i];
				Result = Result || ((TempVertexData.ElementNum > 3u) && (TempVertexData.Stride > 0u) && (!!(TempVertexData.Datas)));
			}
			return (Result && ((Indices.ElementNum > 0u) && (Indices.Stride > 0u) && (!!(Indices.Indices))));
		}
		return FALSE;
	}
	BOOL32 EMesh::InitResource()
	{
		// Mesh resource must init by mesh manager.
		return TRUE;
	}
	void EMesh::ReleaseResource()
	{
		Indices.Release();
		for (INT32 Index = 0, Length = Vertices.Num(); Index < Length; Index++)
		{
			Vertices[Index].Release();
		}
		Vertices.Empty();
		Submeshes.Empty();
		VertexLayout = 0u;
	}
	BOOL32 EMesh::CheckVertexLayoutPartExist(UINT32 InVertexLayoutType, UINT32 InPartIndex)const
	{
		const EVertexLayoutType TempLayout = TranslateVertexPartTypeToVertexBaseLayout(InVertexLayoutType);
		if ((TempLayout & EVertexLayoutType::MESH_INDEX_FULL) != 0u)
		{
			return ((VertexLayout & EVertexLayoutType::MESH_INDEX_FULL) != 0u);
		}
		else if ((TempLayout & EVertexLayoutType::MESH_INDEX_HALF) != 0u)
		{
			return ((VertexLayout & EVertexLayoutType::MESH_INDEX_HALF) != 0u);
		}
		else
		{
			if (InPartIndex >= EMesh::MeshVertexLayoutPartMaxNum)
			{
				return FALSE;
			}
			return ((VertexLayout & (TempLayout << InPartIndex)) != 0u);
		}
	}
	const EString& EMesh::GetMeshName()const
	{
		return MeshName;
	}
	const EBoundAABB& EMesh::GetBoundAABB()const
	{
		return BoundAABB;
	}
	UINT32 EMesh::GetVertexLayout()const
	{
		return VertexLayout;
	}
	const EMesh::EIndexPart& EMesh::GetIndexPart()const
	{
		return Indices;
	}
	const EMesh::EVertexPart& EMesh::GetVertexPart()const
	{
		return Vertices;
	}
	const EMesh::ESubmeshPart& EMesh::GetSubmeshPart()const
	{
		return Submeshes;
	}
	void EMesh::SetMeshName(const EString& InMeshName)
	{
		MeshName = InMeshName;
	}
	void EMesh::SetBoxToBoundAABB(const Vector3& InOrigin, const Vector3& InExtent)
	{
		BoundAABB.IsValid = TRUE;
		BoundAABB.AABBMin = InOrigin - InExtent;
		BoundAABB.AABBMax = InOrigin + InExtent;
	}
	void EMesh::SetBoundAABB(const Vector3& InMin, const Vector3& InMax)
	{
		BoundAABB.IsValid = TRUE;
		BoundAABB.AABBMin = InMin;
		BoundAABB.AABBMax = InMax;
	}
	BOOL32 EMesh::AddIndexElement(EIndexData* InIndexData)
	{
		if ((!InIndexData) || (InIndexData->PartType == 0u) || (InIndexData->ElementNum < 3u) ||
			(!(InIndexData->Indices)) || ((InIndexData->Stride != 2u) && (InIndexData->Stride != 4u)))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add index descriptor check failed."));
			return FALSE;
		}
		if (Indices.PartType != 0u)
		{
			EVertexLayoutType OldPartType = static_cast<EVertexLayoutType>(Indices.PartType);
			if ((((OldPartType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) && (Indices.Stride != 2u)) ||
				(((OldPartType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) && (Indices.Stride != 4u)))
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh [old] index check failed. Part type is not match with stride."));
				return FALSE;
			}
			Indices.Release();
			SetVertexLayoutPartExistInternal(OldPartType, 0u, FALSE);
		}
		EVertexLayoutType NewPartType = static_cast<EVertexLayoutType>(InIndexData->PartType);
		if ((((NewPartType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) && (InIndexData->Stride != 2u)) ||
			(((NewPartType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) && (InIndexData->Stride != 4u)))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh [new] index check failed. Part type is not match with stride."));
			return FALSE;
		}
		Indices.PartType	= NewPartType;
		Indices.ElementNum	= InIndexData->ElementNum;
		Indices.Stride		= InIndexData->Stride;
		Indices.Indices		= InIndexData->Indices;
		SetVertexLayoutPartExistInternal(NewPartType, 0u, TRUE);
		return TRUE;
	}
	BOOL32 EMesh::RemoveIndexElement()
	{
		if (Indices.PartType != 0u)
		{
			EVertexLayoutType OldPartType = static_cast<EVertexLayoutType>(Indices.PartType);
#if _DEBUG_MODE
			if ((((OldPartType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) && (Indices.Stride != 2u)) ||
				(((OldPartType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) && (Indices.Stride != 4u)))
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Removed index check failed. Part type is not match with stride."));
			}
#endif
			Indices.Release();
			SetVertexLayoutPartExistInternal(OldPartType, 0u, FALSE);
			return TRUE;
		}
#if _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing indices of mesh but do not exist."));
#endif
		return FALSE;
	}
	BOOL32 EMesh::GetIndexElement(const EIndexData*& OutIndexData)const
	{
		if ((Indices.PartType != 0u)
#if _EDITOR_ONLY
			&& (Indices.ElementNum > 2u) && (!!(Indices.Indices))
#endif
			)
		{
#if _EDITOR_ONLY
			EVertexLayoutType OldPartType = static_cast<EVertexLayoutType>(Indices.PartType);
			if ((((OldPartType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) && (Indices.Stride != 2u)) ||
				(((OldPartType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) && (Indices.Stride != 4u)))
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Get index check failed. Part type is not match with stride."));
			}
#endif
			OutIndexData = (const EIndexData*)(&Indices);
			return TRUE;
		}
#if _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to getting indices of mesh but do not exist."));
#endif
		return FALSE;
	}
	BOOL32 EMesh::AddVertexElement(EVertexData* InVertexData, UINT32* OutLayoutIndex)
	{
		if ((!InVertexData) || (InVertexData->PartType == 0u) || (InVertexData->ElementNum < 3u) ||
			(!(InVertexData->Datas)) || ((InVertexData->Stride % 4u) != 0u))
		{
#if _DEBUG_MODE
			PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add vertex descriptor check failed."));
			if (InVertexData && (InVertexData->Stride % 4u) != 0u)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add vertex check failed(stride is not 4 bytes alignment)."));
			}
#endif
			return FALSE;
		}
		const EVertexLayoutType InLayoutType = TranslateVertexPartTypeToVertexBaseLayout(static_cast<UINT32>(InVertexData->PartType));
		if (((InLayoutType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) ||
			((InLayoutType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 0u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 1u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 2u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 3u)) != 0u))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add vertex layout type check failed(wrong type)."));
			return FALSE;
		}
		UINT32 FindIndex = 0u;
		if (Vertices.Num() > 0)
		{
			FindIndex = EMesh::MeshVertexLayoutPartMaxNum;
			for (UINT32 Index = 0u; Index < EMesh::MeshVertexLayoutPartMaxNum; Index++)
			{
				if (!CheckVertexLayoutPartExist(InLayoutType, Index))
				{
					FindIndex = Index;
					break;
				}
			}
			if (FindIndex == EMesh::MeshVertexLayoutPartMaxNum)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add vertex failed(same type layout is full)."));
				return FALSE;
			}
		}
		if (!CheckVertexLayoutPartExist(InLayoutType, FindIndex))
		{
			SetVertexLayoutPartExistInternal(InLayoutType, FindIndex, TRUE);
			const UINT32 NewElementIndex = Vertices.Num<UINT32>();
			Vertices.Add(EVertexData());
			Vertices[NewElementIndex].PartType		= (InLayoutType << FindIndex);
			Vertices[NewElementIndex].ElementNum	= InVertexData->ElementNum;
			Vertices[NewElementIndex].Stride		= InVertexData->Stride;
			Vertices[NewElementIndex].Datas			= InVertexData->Datas;
			if (OutLayoutIndex)
			{
				*OutLayoutIndex = FindIndex;
			}
			return TRUE;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add vertex failed(fill in layout index is already exist)."));
		return FALSE;
	}
	BOOL32 EMesh::RemoveVertexElement(UINT32 InVertexLayoutType, UINT32 InLayoutIndex)
	{
		const EVertexLayoutType InLayoutType = TranslateVertexPartTypeToVertexBaseLayout(InVertexLayoutType);
		if (((InLayoutType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) ||
			((InLayoutType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 0u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 1u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 2u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 3u)) != 0u))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh remove vertex layout type check failed(wrong type)."));
			return FALSE;
		}
		if ((Vertices.Num() > 0) && CheckVertexLayoutPartExist(InLayoutType, InLayoutIndex))
		{
			auto MoveVertexData = [](EVertexData& InA, EVertexData& InB)->void
			{
				EVertexData TempData;
				TempData.PartType = InA.PartType;
				TempData.ElementNum = InA.ElementNum;
				TempData.Stride = InA.Stride;
				TempData.Datas = InA.Datas;
				InA.PartType = InB.PartType;
				InA.ElementNum = InB.ElementNum;
				InA.Stride = InB.Stride;
				InA.Datas = InB.Datas;
				InB.PartType = TempData.PartType;
				InB.ElementNum = TempData.ElementNum;
				InB.Stride = TempData.Stride;
				InB.Datas = TempData.Datas;
			};
			const UINT32 DeleteLayoutType = InLayoutType << InLayoutIndex;
			for (INT32 Index = 0, Length = Vertices.Num(); Index < Length; Index++)
			{
				if (Vertices[Index].PartType == DeleteLayoutType)
				{
					Vertices[Index].Release();
					if (Index != (Length - 1))
					{
						MoveVertexData(Vertices[Index], Vertices[Length - 1u]);
					}
					Vertices.Pop();
					SetVertexLayoutPartExistInternal(InLayoutType, InLayoutIndex, FALSE);
					return TRUE;
				}
			}
		}
#if _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to remove not exist vertex."));
#endif
		return FALSE;
	}
	BOOL32 EMesh::GetVertexElement(UINT32 InVertexLayoutType, UINT32 InLayoutIndex, const EVertexData*& OutVertexData)const
	{
		const EVertexLayoutType InLayoutType = TranslateVertexPartTypeToVertexBaseLayout(InVertexLayoutType);
		if (((InLayoutType & EVertexLayoutType::MESH_INDEX_FULL) != 0u) ||
			((InLayoutType & EVertexLayoutType::MESH_INDEX_HALF) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 0u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 1u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 2u)) != 0u) ||
			((InLayoutType & (EVertexLayoutType::MESH_SKIN << 3u)) != 0u))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh get vertex layout type check failed(wrong type)."));
			return FALSE;
		}
		if ((Vertices.Num() > 0) && CheckVertexLayoutPartExist(InLayoutType, InLayoutIndex))
		{
			const UINT32 DeleteLayoutType = InLayoutType << InLayoutIndex;
			for (INT32 Index = 0, Length = Vertices.Num(); Index < Length; Index++)
			{
				if (Vertices[Index].PartType == DeleteLayoutType)
				{
					OutVertexData = (const EVertexData*)(&(Vertices[Index]));
					return TRUE;
				}
			}
		}
#if (false && _DEBUG_MODE)
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist vertex."));
#endif
		return FALSE;
	}
	BOOL32 EMesh::AddSubmesh(const ESubmeshData* InSubmeshData, UINT32* OutSubmeshIndex)
	{
		if ((!InSubmeshData) || (InSubmeshData->VertexNum < 3u) || (InSubmeshData->IndexNum < 3u))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to add invalid submesh."));
			return FALSE;
		}
		const UINT32 NewIndex = Submeshes.Num<UINT32>();
		Submeshes.Add(ESubmeshData());
		Submeshes[NewIndex].StartVertex	= InSubmeshData->StartVertex;
		Submeshes[NewIndex].VertexNum	= InSubmeshData->VertexNum;
		Submeshes[NewIndex].StartIndex	= InSubmeshData->StartIndex;
		Submeshes[NewIndex].IndexNum	= InSubmeshData->IndexNum;
		if (OutSubmeshIndex)
		{
			*OutSubmeshIndex = NewIndex;
		}
		return TRUE;
	}
	BOOL32 EMesh::RemoveSubmesh(UINT32 InSubmeshIndex)
	{
		if (UINT32 TempSubmeshNum = Submeshes.Num<UINT32>(); (TempSubmeshNum == 0u) || (InSubmeshIndex >= Submeshes.Num<UINT32>()))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to remove not exist submesh."));
			return FALSE;
		}
		Submeshes.RemoveAt(InSubmeshIndex);
		return TRUE;
	}
	BOOL32 EMesh::GetSubmesh(UINT32 InSubmeshIndex, const ESubmeshData*& OutSubmeshData)const
	{
		if (UINT32 TempSubmeshNum = Submeshes.Num<UINT32>(); (TempSubmeshNum == 0u) || (InSubmeshIndex >= Submeshes.Num<UINT32>()))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist submesh."));
			return FALSE;
		}
		OutSubmeshData = (const ESubmeshData*)(&(Submeshes[InSubmeshIndex]));
		return TRUE;
	}
	void EMesh::UnsafeReleaseIndexPart()
	{
		Indices.Release();
	}
	void EMesh::UnsafeReleaseVertexPart()
	{
		for (INT32 i = 0, n = Vertices.Num(); i < n; i++)
		{
			Vertices[i].Release();
		}
		Vertices.Empty();
	}
	void EMesh::CopyBaseDataFromOtherInternal(const EMesh& Other)
	{
		MeshName		= Other.MeshName;
		BoundAABB		= Other.BoundAABB;
		VertexLayout	= Other.VertexLayout;

		Indices.Release();
		Indices = Other.Indices;

		if (Vertices.Num() > 0)
		{
			for (INT32 i = 0, n = Vertices.Num(); i < n; i++)
			{
				Vertices[i].Release();
			}
			Vertices.Empty();
		}
		for (INT32 i = 0, n = Other.Vertices.Num(); i < n; i++)
		{
			const EVertexData& OtherVertex = Other.Vertices[i];
			Vertices.Add(EVertexData());
			Vertices[Vertices.Num() - 1] = OtherVertex;
		}

		if (Submeshes.Num() > 0)
		{
			Submeshes.Empty();
		}
		for (INT32 i = 0, n = Other.Submeshes.Num(); i < n; i++)
		{
			const ESubmeshData& OtherSubmesh = Other.Submeshes[i];
			Submeshes.Add(ESubmeshData());
			Submeshes[Submeshes.Num() - 1] = OtherSubmesh;
		}
	}
	void EMesh::SetVertexLayoutPartExistInternal(UINT32 InVertexLayoutType, UINT32 InPartIndex, BOOL32 InIsExist, BOOL32* OutIsAlreadyExist)
	{
		const EVertexLayoutType InLayoutType = TranslateVertexPartTypeToVertexBaseLayout(InVertexLayoutType);
		if ((InLayoutType & EVertexLayoutType::MESH_INDEX_FULL) != 0u)
		{
			if (OutIsAlreadyExist)
			{
				(*OutIsAlreadyExist) = (VertexLayout & EVertexLayoutType::MESH_INDEX_FULL) != 0u;
			}
			VertexLayout = InIsExist ? (VertexLayout | EVertexLayoutType::MESH_INDEX_FULL) : (VertexLayout & (~(EVertexLayoutType::MESH_INDEX_FULL)));
		}
		else if ((InLayoutType & EVertexLayoutType::MESH_INDEX_HALF) != 0u)
		{
			if (OutIsAlreadyExist)
			{
				(*OutIsAlreadyExist) = (VertexLayout & EVertexLayoutType::MESH_INDEX_HALF) != 0u;
			}
			VertexLayout = InIsExist ? (VertexLayout | EVertexLayoutType::MESH_INDEX_HALF) : (VertexLayout & (~(EVertexLayoutType::MESH_INDEX_HALF)));
		}
		else
		{
			if (InPartIndex >= EMesh::MeshVertexLayoutPartMaxNum)
			{
				if (OutIsAlreadyExist)
				{
					(*OutIsAlreadyExist) = FALSE;
				}
				return;
			}
			if (OutIsAlreadyExist)
			{
				(*OutIsAlreadyExist) = (VertexLayout & (InLayoutType << InPartIndex)) != 0u;
			}
			VertexLayout = InIsExist ? (VertexLayout | (InLayoutType << InPartIndex)) : (VertexLayout & (~(InLayoutType << InPartIndex)));
		}
	}

	EStaticMesh::EStaticMesh()
		: EMesh(EEngineSettings::ENGINE_DEFAULT_NAME)
	{
	}
	EStaticMesh::EStaticMesh(const EStaticMesh& Other)
		: EMesh(Other)
	{
	}
	EStaticMesh::EStaticMesh(const EString& InMeshName)
		: EMesh(InMeshName)
	{
	}
	EStaticMesh::~EStaticMesh()
	{
	}
	BOOL32 EStaticMesh::IsResourceValid()const
	{
		return (EMesh::IsResourceValid());
	}
	BOOL32 EStaticMesh::InitResource()
	{
		// Mesh resource must init by mesh manager.
		return (EMesh::InitResource());
	}
	void EStaticMesh::ReleaseResource()
	{
		EMesh::ReleaseResource();
	}
	EStaticMesh& EStaticMesh::operator=(const EStaticMesh& Other)
	{
		CopyBaseDataFromOtherInternal(Other);
		return (*this);
	}

	ESkinnedMesh::ESkinnedMesh()
		: EMesh(EEngineSettings::ENGINE_DEFAULT_NAME)
		, EffectBoneNum(0u)
	{
	}
	ESkinnedMesh::ESkinnedMesh(const ESkinnedMesh& Other)
		: EMesh(Other)
		, EffectBoneNum(Other.EffectBoneNum)
	{
		for (auto It = Other.BindPoseValue.Begin(); It != Other.BindPoseValue.End(); It++)
		{
			BindPoseValue.Add(It->first, It->second);
		}
		for (auto It = Other.BindPoseIndex.Begin(); It != Other.BindPoseIndex.End(); It++)
		{
			BindPoseIndex.Add(It->first, It->second);
		}
		for (INT32 i = 0, n = Other.Skins.Num(); i < n; i++)
		{
			const ESkinData& OtherSkin = Other.Skins[i];
			Skins.Add(ESkinData());
			Skins[Skins.Num() - 1] = OtherSkin;
		}
	}
	ESkinnedMesh::ESkinnedMesh(const EString& InMeshName)
		: EMesh(InMeshName), EffectBoneNum(0u)
	{
	}
	ESkinnedMesh::~ESkinnedMesh()
	{
		for (INT32 Index = 0u, Length = Skins.Num(); Index < Length; Index++)
		{
			Skins[Index].Release();
		}
	}
	BOOL32 ESkinnedMesh::IsResourceValid()const
	{
		if (Skins.Num() > 0)
		{
			BOOL32 Result = FALSE;
			for (INT32 Index = 0u, Length = Skins.Num(); Index < Length; Index++)
			{
				Result = Result ||
					(
						(Skins[Index].ElementNum > 3u) &&
						(Skins[Index].Stride > 0u) &&
						(BindPoseIndex.Num() > 0) &&
						(BindPoseIndex.Num<USHORT>() <= RCommonSettings::RENDER_MESH_BONE_NUM_MAX) &&
						(BindPoseIndex.Num() == BindPoseValue.Num()) &&
						(EffectBoneNum > 0u) &&
						(EffectBoneNum <= EEngineSettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM)
						);
			}
			return (Result && (EMesh::IsResourceValid()));
		}
		return FALSE;
	}
	BOOL32 ESkinnedMesh::InitResource()
	{
		// Mesh resource must init by mesh manager.
		return (EMesh::InitResource());
	}
	void ESkinnedMesh::ReleaseResource()
	{
		for (INT32 Index = 0, Length = Skins.Num(); Index < Length; Index++)
		{
			Skins[Index].Release();
		}
		EMesh::ReleaseResource();
	}
	ESkinnedMesh& ESkinnedMesh::operator=(const ESkinnedMesh& Other)
	{
		CopyBaseDataFromOtherInternal(Other);

		if (BindPoseValue.Num() > 0)
		{
			BindPoseValue.Empty();
		}
		for (auto It = Other.BindPoseValue.Begin(); It != Other.BindPoseValue.End(); It++)
		{
			BindPoseValue.Add(It->first, It->second);
		}

		if (BindPoseIndex.Num() > 0)
		{
			BindPoseIndex.Empty();
		}
		for (auto It = Other.BindPoseIndex.Begin(); It != Other.BindPoseIndex.End(); It++)
		{
			BindPoseIndex.Add(It->first, It->second);
		}

		EffectBoneNum = Other.EffectBoneNum;

		if (Skins.Num() > 0)
		{
			for (INT32 i = 0, n = Skins.Num(); i < n; i++)
			{
				Skins[i].Release();
			}
			Skins.Empty();
		}
		for (INT32 i = 0u, n = Other.Skins.Num(); i < n; i++)
		{
			const ESkinData& OtherSkin = Other.Skins[i];
			Skins.Add(ESkinData());
			Skins[Skins.Num() - 1] = OtherSkin;
		}

		return (*this);
	}
	BOOL32 ESkinnedMesh::AddBindPose(const EString& InBoneName, const Matrix4x4& InBindPose)
	{
		BOOL32 Result = FALSE;
		if (!(BindPoseValue.ContainsKey(InBoneName)))
		{
			BindPoseValue.Add(InBoneName, InBindPose);
			Result = TRUE;
		}
		return Result;
	}
	BOOL32 ESkinnedMesh::RemoveBindPose(const EString& InBoneName)
	{
		BOOL32 Result = FALSE;
		if (BindPoseValue.ContainsKey(InBoneName))
		{
			BindPoseValue.Remove(InBoneName);
			Result = TRUE;
		}
		return Result;
	}
	void ESkinnedMesh::ClearBindPose()
	{
		if (BindPoseValue.Num() > 0)
		{
			BindPoseValue.Empty();
		}
		if (BindPoseIndex.Num() > 0)
		{
			BindPoseIndex.Empty();
		}
	}
	void ESkinnedMesh::GenerateBindPoseIndex()
	{
		if (BindPoseIndex.Num() > 0)
		{
			BindPoseIndex.Empty();
		}
		EffectBoneNum = 0u;
		if (BindPoseValue.Num() <= 0)
		{
			return;
		}
		USHORT TempIndex = 0u;
		for (auto It = BindPoseValue.Begin(); It != BindPoseValue.End(); It++)
		{
			if (!(BindPoseIndex.ContainsKey(It->first)))
			{
				BindPoseIndex.Add(It->first, TempIndex);
				TempIndex += 1u;
			}
		}
	}
	const ESkinnedMesh::EBindPoseValue& ESkinnedMesh::GetBindPoseValue()const
	{
		return BindPoseValue;
	}
	const ESkinnedMesh::EBindPoseIndex& ESkinnedMesh::GetBindPoseIndex()const
	{
		return BindPoseIndex;
	}
	void ESkinnedMesh::SetEffectBoneNum(const USHORT InEffectBoneNum)
	{
		Check(((InEffectBoneNum <= static_cast<USHORT>(EEngineSettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM)) && (InEffectBoneNum > 0u)), (ENGINE_ASSET_ERROR));
		EffectBoneNum = InEffectBoneNum;
	}
	USHORT ESkinnedMesh::GetEffectBoneNum()const
	{
		return EffectBoneNum;
	}
	BOOL32 ESkinnedMesh::AddSkinElement(ESkinData* InSkinData, UINT32* OutLayoutIndex)
	{
		if ((!InSkinData) || (InSkinData->ElementNum < 3u) || (InSkinData->PartType != EVertexLayoutType::MESH_SKIN) || ((InSkinData->Stride % 4u) != 0u) || (!(InSkinData->Indices)) || (!(InSkinData->Weights)))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to add invalid skin data."));
			return FALSE;
		}
		if (Skins.Num<UINT32>() >= EMesh::MeshVertexLayoutPartMaxNum)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Skin mesh can not contains skin datas over EMesh::MeshVertexLayoutPartMaxNum."));
			return FALSE;
		}
		for (UINT32 FindIndex = 0u; FindIndex < EMesh::MeshVertexLayoutPartMaxNum; FindIndex++)
		{
			if (!CheckVertexLayoutPartExist(EVertexLayoutType::MESH_SKIN, FindIndex))
			{
				SetVertexLayoutPartExistInternal(EVertexLayoutType::MESH_SKIN, FindIndex, TRUE);
				const UINT32 NewIndex = Skins.Num<UINT32>();
				Skins.Add(ESkinData());
				Skins[NewIndex].PartType = InSkinData->PartType << FindIndex;
				Skins[NewIndex].ElementNum = InSkinData->ElementNum;
				Skins[NewIndex].Stride = InSkinData->Stride;
				Skins[NewIndex].Indices = InSkinData->Indices;
				Skins[NewIndex].Weights = InSkinData->Weights;
				if (OutLayoutIndex)
				{
					*OutLayoutIndex = FindIndex;
				}
				return TRUE;
			}
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Skin mesh can not contains skin datas over EMesh::MeshVertexLayoutPartMaxNum."));
		return FALSE;
	}
	BOOL32 ESkinnedMesh::RemoveSkinElement(UINT32 InLayoutIndex)
	{
		if ((Skins.Num() > 0) && CheckVertexLayoutPartExist(EVertexLayoutType::MESH_SKIN, InLayoutIndex))
		{
			auto MoveSkinData = [](ESkinData& InA, ESkinData& InB)->void
			{
				ESkinData TempData;
				TempData.PartType	= InA.PartType;
				TempData.ElementNum	= InA.ElementNum;
				TempData.Stride		= InA.Stride;
				TempData.Indices	= InA.Indices;
				TempData.Weights	= InA.Weights;
				InA.PartType		= InB.PartType;
				InA.ElementNum		= InB.ElementNum;
				InA.Stride			= InB.Stride;
				InA.Indices			= InB.Indices;
				InA.Weights			= InB.Weights;
				InB.PartType		= TempData.PartType;
				InB.ElementNum		= TempData.ElementNum;
				InB.Stride			= TempData.Stride;
				InB.Indices			= TempData.Indices;
				InB.Weights			= TempData.Weights;
			};
			const UINT32 DeleteLayoutType = EVertexLayoutType::MESH_SKIN << InLayoutIndex;
			for (INT32 Index = 0, Length = Skins.Num(); Index < Length; Index++)
			{
				if (Skins[Index].PartType == DeleteLayoutType)
				{
					Skins[Index].Release();
					if (Index != (Length - 1))
					{
						MoveSkinData(Skins[Index], Skins[Length - 1]);
					}
					Skins.Pop();
					SetVertexLayoutPartExistInternal(EVertexLayoutType::MESH_SKIN, InLayoutIndex, FALSE);
					return TRUE;
				}
			}
		}
#if _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist skin data."));
#endif
		return FALSE;
	}
	BOOL32 ESkinnedMesh::GetSkinElement(UINT32 InLayoutIndex, const ESkinData*& OutSkinData)const
	{
		if ((Skins.Num() > 0) && CheckVertexLayoutPartExist(EVertexLayoutType::MESH_SKIN, InLayoutIndex))
		{
			const UINT32 DeleteLayoutType = EVertexLayoutType::MESH_SKIN << InLayoutIndex;
			for (INT32 Index = 0u, Length = Skins.Num(); Index < Length; Index++)
			{
				if (Skins[Index].PartType == DeleteLayoutType)
				{
					OutSkinData = (const ESkinData*)(&(Skins[Index]));
					return TRUE;
				}
			}
		}
#if _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist skin data."));
#endif
		return FALSE;
	}
	const ESkinnedMesh::ESkinPart& ESkinnedMesh::GetSkinPart()const
	{
		return Skins;
	}
	void ESkinnedMesh::UnsafeReleaseSkinPart()
	{
		for (INT32 i = 0, n = Skins.Num(); i < n; i++)
		{
			Skins[i].Release();
		}
		Skins.Empty();
	}

	EMeshRenderResource::EMeshRenderResource()
	{
	}
	EMeshRenderResource::EMeshRenderResource(const EMeshRenderResource& Other)
	{
		CopyRenderResourcesInternal(&Other);
	}
	EMeshRenderResource::~EMeshRenderResource()
	{
		ReleaseRenderResource();
	}
	BOOL32 EMeshRenderResource::IsRenderResourceValid()const
	{
		if ((IndexRenderResource.IsRenderResourceValid()) && (VertexRenderResources.Num() > 0))
		{
			BOOL32 Result = TRUE;
			for (auto It = VertexRenderResources.Begin(); It != VertexRenderResources.End(); It++)
			{
				const TArray<RVertexBufferResource*>& Buffers = It->second;
				if (const UINT32 BufferNum = Buffers.Num<UINT32>(); BufferNum > 0u)
				{
					for (UINT32 i = 0u; i < BufferNum; i++)
					{
						if (Buffers[i])
						{
							Result = Result && (Buffers[i]->IsRenderResourceValid());
						}
					}
				}
			}
			return Result;
		}
		return FALSE;
	}
	void EMeshRenderResource::ReleaseRenderResource()
	{
		IndexRenderResource.ReleaseRenderResource();
		if (VertexRenderResources.Num() > 0)
		{
			for (auto It = VertexRenderResources.Begin(); It != VertexRenderResources.End(); It++)
			{
				TArray<RVertexBufferResource*>& Buffers = It->second;
				if (const UINT32 BufferNum = Buffers.Num<UINT32>(); BufferNum > 0u)
				{
					for (UINT32 i = 0u; i < BufferNum; i++)
					{
						if (Buffers[i])
						{
							Buffers[i]->ReleaseRenderResource();
							delete (Buffers[i]);
							Buffers[i] = nullptr;
						}
					}
					Buffers.Empty();
				}
			}
			VertexRenderResources.Empty();
		}
	}
	const RIndexBufferResource* EMeshRenderResource::GetIndexRenderResource()const
	{
		return (&IndexRenderResource);
	}
	const TArray<RVertexBufferResource*>* EMeshRenderResource::GetVertexRenderResource(const UINT8 InType)const
	{
		return (VertexRenderResources.FindValueAsPtr(InType));
	}
	BOOL32 EMeshRenderResource::CreateIndexRenderResourceInternal(const EMesh* InMesh)
	{
		if (const EIndexData* OutIndexData = nullptr; InMesh->GetIndexElement(OutIndexData))
		{
			if (IndexRenderResource.IsRenderResourceValid())
			{
				IndexRenderResource.ReleaseRenderResource();
			}
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			UINT32 IndexType = static_cast<UINT32>(OutIndexData->PartType);
			if ((IndexType & EVertexLayoutType::MESH_INDEX_FULL) != 0u)
			{
				IndexRenderResource.UseShort = FALSE;
				IndexRenderResource.IndexCount = OutIndexData->ElementNum;
				RSubresourceDataDesc SubresourceDataDesc;
				SubresourceDataDesc.pSysMem = OutIndexData->Indices;
				if (!(RenderDevice->CreateBuffer(
					IndexRenderResource,
					RBufferDesc((4u * OutIndexData->ElementNum), RBindFlagType::BIND_INDEX_BUFFER, 4u),
					(&SubresourceDataDesc))))
				{
#if _EDITOR_ONLY
					EString ErrorInfo("Create mesh [name = ");
					ErrorInfo += InMesh->GetMeshName();
					ErrorInfo += "] index buffer [num = ";
					ErrorInfo += ToString(OutIndexData->ElementNum);
					ErrorInfo += "] [stride = 32bits] failed.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
					return FALSE;
				}
			}
			else if ((IndexType & EVertexLayoutType::MESH_INDEX_HALF) != 0u)
			{
				UINT16* TempIndices = new UINT16[OutIndexData->ElementNum];
				for (UINT32 i = 0u, n = OutIndexData->ElementNum; i < n; i++)
				{
					PE_CHECK((ENGINE_ASSET_ERROR), ("Mesh index type check failed, half index element num must lower than 65535u."), ((OutIndexData->Indices[i]) < 0xffffu));
					TempIndices[i] = static_cast<UINT16>(OutIndexData->Indices[i]);
				}
				IndexRenderResource.UseShort = TRUE;
				IndexRenderResource.IndexCount = OutIndexData->ElementNum;
				RSubresourceDataDesc SubresourceDataDesc;
				SubresourceDataDesc.pSysMem = TempIndices;
				if (!(RenderDevice->CreateBuffer(
					IndexRenderResource,
					RBufferDesc((2u * OutIndexData->ElementNum), RBindFlagType::BIND_INDEX_BUFFER, 4u),
					(&SubresourceDataDesc))))
				{
#if _EDITOR_ONLY
					EString ErrorInfo("Create mesh [name = ");
					ErrorInfo += InMesh->GetMeshName();
					ErrorInfo += "] index buffer [num = ";
					ErrorInfo += ToString(OutIndexData->ElementNum);
					ErrorInfo += "] [stride = 16bits] failed.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
					delete[]TempIndices;
					return FALSE;
				}
				delete[]TempIndices;
			}
			else
			{
				return FALSE;
			}
		}
		return TRUE;
	}
	BOOL32 EMeshRenderResource::CreateVertexRenderResourceInternal(const EMesh* InMesh)
	{
		const EMesh::EVertexPart& MeshVertices = InMesh->GetVertexPart();
		if (MeshVertices.Num() <= 0)
		{
#if _EDITOR_ONLY
			EString ErrorInfo("Create vertex render resource but mesh [name = ");
			ErrorInfo += InMesh->GetMeshName();
			ErrorInfo += "] does not contain vertex datas.";
			PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
			return FALSE;
		}
		if (VertexRenderResources.Num() > 0)
		{
			for (auto It = VertexRenderResources.Begin(); It != VertexRenderResources.End(); It++)
			{
				TArray<RVertexBufferResource*>& Buffers = It->second;
				if (const UINT32 BufferNum = Buffers.Num<UINT32>(); BufferNum > 0u)
				{
					for (UINT32 i = 0u; i < BufferNum; i++)
					{
						if (Buffers[i])
						{
							Buffers[i]->ReleaseRenderResource();
							delete (Buffers[i]);
							Buffers[i] = nullptr;
						}
					}
					Buffers.Empty();
				}
			}
			VertexRenderResources.Empty();
		}
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		BOOL32 FullyCreated = TRUE;

		for (UINT8 VertexLayoutIndex = 0u, VertexLayoutNum = EVertexResourceType::VERTEX_RESOURCE_TYPE_COUNT; VertexLayoutIndex < VertexLayoutNum; VertexLayoutIndex++)
		{
			EVertexLayoutType TempVertexLayout = TranslateVertexResourceTypeToVertexLayoutType(VertexLayoutIndex);
			if (!(VertexRenderResources.ContainsKey(VertexLayoutIndex)))
			{
				VertexRenderResources.Add(VertexLayoutIndex, TArray<RVertexBufferResource*>());
			}
			TArray<RVertexBufferResource*>& TempVertexPartArray = VertexRenderResources[VertexLayoutIndex];
			for (UINT32 VertexPartIndex = 0u; VertexPartIndex < 4u; VertexPartIndex++)
			{
				TempVertexPartArray.Add(nullptr);
				if (const EVertexData* TempVertexData = nullptr; InMesh->GetVertexElement(TempVertexLayout, VertexPartIndex, TempVertexData))
				{
					TempVertexPartArray[VertexPartIndex] = new RVertexBufferResource();
					TempVertexPartArray[VertexPartIndex]->Stride = TempVertexData->Stride;

					RSubresourceDataDesc SubresourceDataDesc;
					SubresourceDataDesc.pSysMem = TempVertexData->Datas;

					if (!(RenderDevice->CreateBuffer(
						*(TempVertexPartArray[VertexPartIndex]),
						RBufferDesc((TempVertexData->Stride * TempVertexData->ElementNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
						(&SubresourceDataDesc))))
					{
#if _EDITOR_ONLY
						EString ErrorInfo("Create mesh [name = ");
						ErrorInfo += InMesh->GetMeshName();
						ErrorInfo += "] vertex buffer [num = ";
						ErrorInfo += ToString(TempVertexData->ElementNum);
						ErrorInfo += "] [stride = ";
						ErrorInfo += ToString(TempVertexData->Stride);
						ErrorInfo += "] failed.";
						PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
						FullyCreated = FALSE;
					}
				}
			}
		}
		return FullyCreated;
	}
	void EMeshRenderResource::CopyRenderResourcesInternal(const EMeshRenderResource* Other)
	{
		if ((!Other) || (!(Other->IsRenderResourceValid())))
		{
			return;
		}
		IndexRenderResource = Other->IndexRenderResource;
		if (Other->VertexRenderResources.Num() > 0)
		{
			for (auto It = Other->VertexRenderResources.Begin(); It != Other->VertexRenderResources.End(); It++)
			{
				const UINT8 Key = It->first;
				const TArray<RVertexBufferResource*>& Buffers = It->second;
				VertexRenderResources.Add(Key, Buffers);
			}
		}
	}

	EStaticMeshRenderResource::EStaticMeshRenderResource()
		: StaticMesh(nullptr)
	{
	}
	EStaticMeshRenderResource::EStaticMeshRenderResource(EStaticMesh* InMesh)
		: StaticMesh(InMesh)
	{
	}
	EStaticMeshRenderResource::EStaticMeshRenderResource(const EStaticMeshRenderResource& Other)
		: EMeshRenderResource(Other), StaticMesh(Other.StaticMesh)
	{
	}
	EStaticMeshRenderResource::~EStaticMeshRenderResource()
	{
		ReleaseRenderResource();
	}
	EStaticMeshRenderResource& EStaticMeshRenderResource::operator=(const EStaticMeshRenderResource& Other)
	{
		ReleaseRenderResource();
		CopyRenderResourcesInternal(&Other);
		return (*this);
	}
	BOOL32 EStaticMeshRenderResource::IsRenderResourceValid()const
	{
		return ((!!StaticMesh) && (EMeshRenderResource::IsRenderResourceValid()));
	}
	BOOL32 EStaticMeshRenderResource::InitRenderResource()
	{
		if (EMeshRenderResource::IsRenderResourceValid())
		{
			EMeshRenderResource::ReleaseRenderResource();
		}
		if ((!!StaticMesh) && (StaticMesh->IsResourceValid()))
		{
			if (!(EMeshRenderResource::CreateIndexRenderResourceInternal(StaticMesh)))
			{
				return FALSE;
			}
			if (!(EMeshRenderResource::CreateVertexRenderResourceInternal(StaticMesh)))
			{
				return FALSE;
			}
			return TRUE;
		}
		return FALSE;
	}
	void EStaticMeshRenderResource::ReleaseRenderResource()
	{
		StaticMesh = nullptr;
		EMeshRenderResource::ReleaseRenderResource();
	}

	ESkinnedMeshRenderResource::ESkinnedMeshRenderResource()
		: SkinnedMesh(nullptr)
	{
	}
	ESkinnedMeshRenderResource::ESkinnedMeshRenderResource(const ESkeleton* InSkeleton, ESkinnedMesh* InMesh)
		: SkinnedMesh(InMesh)
	{
	}
	ESkinnedMeshRenderResource::ESkinnedMeshRenderResource(const ESkinnedMeshRenderResource& Other)
		: EMeshRenderResource(Other), SkinnedMesh(Other.SkinnedMesh)
	{
		CopySkinRenderResourcesInternal(&Other);
	}
	ESkinnedMeshRenderResource::~ESkinnedMeshRenderResource()
	{
		ReleaseRenderResource();
	}
	ESkinnedMeshRenderResource& ESkinnedMeshRenderResource::operator=(const ESkinnedMeshRenderResource& Other)
	{
		ReleaseRenderResource();
		CopyRenderResourcesInternal(&Other);
		SkinnedMesh		= Other.SkinnedMesh;
		CopySkinRenderResourcesInternal(&Other);
		return (*this);
	}
	BOOL32 ESkinnedMeshRenderResource::IsRenderResourceValid()const
	{
		if (SkinRenderResources.Num() > 0)
		{
			BOOL32 Result = TRUE;
			for (INT32 i = 0, n = SkinRenderResources.Num(); i < n; i++)
			{
				Result = Result && SkinRenderResources[i].IsRenderResourceValid();
			}
			return (Result && (!!SkinnedMesh) && (EMeshRenderResource::IsRenderResourceValid()));
		}
		return FALSE;
	}
	BOOL32 ESkinnedMeshRenderResource::InitRenderResource()
	{
		if (EMeshRenderResource::IsRenderResourceValid())
		{
			EMeshRenderResource::ReleaseRenderResource();
		}
		if ((!!SkinnedMesh) && (SkinnedMesh->IsResourceValid()))
		{
			if (!(EMeshRenderResource::CreateIndexRenderResourceInternal(SkinnedMesh)))
			{
				return FALSE;
			}
			if (!(EMeshRenderResource::CreateVertexRenderResourceInternal(SkinnedMesh)))
			{
				return FALSE;
			}
			if (!(CreateSkinRenderResourceInternal()))
			{
				return FALSE;
			}
			return TRUE;
		}
		return FALSE;
	}
	void ESkinnedMeshRenderResource::ReleaseRenderResource()
	{
		SkinnedMesh	= nullptr;
		for (INT32 i = 0, n = SkinRenderResources.Num(); i < n; i++)
		{
			SkinRenderResources[i].ReleaseRenderResource();
		}
		EMeshRenderResource::ReleaseRenderResource();
	}
	const TArray<RVertexBufferResource>& ESkinnedMeshRenderResource::GetSkinRenderResource()const
	{
		return SkinRenderResources;
	}
	BOOL32 ESkinnedMeshRenderResource::CreateSkinRenderResourceInternal()
	{
#if _EDITOR_ONLY
		const INT32 MeshSkinnedBoneNum = ((!!SkinnedMesh) && SkinnedMesh->IsResourceValid()) ? (SkinnedMesh->GetBindPoseIndex().Num()) : 0;
#endif
		if ((!SkinnedMesh) || (!(SkinnedMesh->IsResourceValid())))
		{
#if _EDITOR_ONLY
			if (!SkinnedMesh)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Create skin render resource but skinned mesh is null."));
			}
			else if (MeshSkinnedBoneNum <= 0)
			{
				EString ErrorInfo("Create skin render resource but skinned mesh [name = ");
				ErrorInfo += SkinnedMesh->GetMeshName();
				ErrorInfo += "] does not contain bone datas.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
			if (((USHORT)MeshSkinnedBoneNum) > RCommonSettings::RENDER_MESH_BONE_NUM_MAX)
			{
				EString ErrorInfo("Create skin render resource but skinned mesh [name = ");
				ErrorInfo += SkinnedMesh->GetMeshName();
				ErrorInfo += "]'s bone num [";
				ErrorInfo += ToString(MeshSkinnedBoneNum);
				ErrorInfo += "] is greater than engine's bone max num [";
				ErrorInfo += ToString(RCommonSettings::RENDER_MESH_BONE_NUM_MAX);
				ErrorInfo += "].";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return FALSE;
		}
		if (const UINT32 SkinRenderResourceNum = SkinRenderResources.Num<UINT32>(); SkinRenderResourceNum > 0u)
		{
			for (UINT32 SkinRenderResourceIndex = 0u; SkinRenderResourceIndex < SkinRenderResourceNum; SkinRenderResourceIndex++)
			{
				SkinRenderResources[SkinRenderResourceIndex].ReleaseRenderResource();
			}
			SkinRenderResources.Empty();
		}
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		BOOL32 FullyCreated = TRUE;
		const ESkinnedMesh::ESkinPart& Skins = SkinnedMesh->GetSkinPart();
		for (INT32 SkinIndex = 0u, SkinNum = Skins.Num(); SkinIndex < SkinNum; SkinIndex++)
		{
			const ESkinData& SkinData = Skins[SkinIndex];
			const UINT32 EffectBoneNum = SkinData.Stride / sizeof(FLOAT);
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check effect bone num failed when create skin render resource."), (EffectBoneNum == SkinnedMesh->GetEffectBoneNum()));
			{
				const UINT32 IndexNum = EffectBoneNum * SkinData.ElementNum;
				UINT16* TempSkinIndexMem = new UINT16[IndexNum];
				for (UINT32 i = 0u, n = SkinData.ElementNum; i < n; i++)
				{
					for (UINT32 BoneIndex = 0u; BoneIndex < EffectBoneNum; BoneIndex++)
					{
						if ((i * EffectBoneNum + BoneIndex) < IndexNum)
						{
							PE_CHECK((ENGINE_ASSET_ERROR), ("Check skin bone index is lower than 65535u failed when create skin render resource."), ((SkinData.Indices[i * EffectBoneNum + BoneIndex]) <= 0xffffu));
							TempSkinIndexMem[i * EffectBoneNum + BoneIndex] = static_cast<UINT16>(SkinData.Indices[i * EffectBoneNum + BoneIndex]);
						}
					}
				}
				const UINT32 IndexStride = EffectBoneNum * sizeof(UINT16);	//We assum the number of bone can not greater than 65535u.
				RSubresourceDataDesc SubresourceDataDesc;
				SubresourceDataDesc.pSysMem = TempSkinIndexMem;
				RVertexBufferResource TempSkinIndexBufferResource;
				if (!(RenderDevice->CreateBuffer(
					TempSkinIndexBufferResource,
					RBufferDesc(IndexStride * SkinData.ElementNum, RBindFlagType::BIND_VERTEX_BUFFER, 2u),
					(&SubresourceDataDesc))))
				{
#if _EDITOR_ONLY
					EString ErrorInfo("Create skin resource skinned mesh [name = ");
					ErrorInfo += SkinnedMesh->GetMeshName();
					ErrorInfo += "] skin index buffer [num = ";
					ErrorInfo += ToString(SkinData.ElementNum);
					ErrorInfo += "] failed.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
					FullyCreated = FALSE;
				}
				delete[]TempSkinIndexMem;
				TempSkinIndexBufferResource.Stride = IndexStride;
				SkinRenderResources.Add(TempSkinIndexBufferResource);
			}

			{
				const UINT32 WeightNum = EffectBoneNum * SkinData.ElementNum;
				FLOAT* TempSkinWeightMem = new FLOAT[WeightNum];
				for (UINT32 i = 0u, n = SkinData.ElementNum; i < n; i++)
				{
					for (UINT32 BoneIndex = 0u; BoneIndex < EffectBoneNum; BoneIndex++)
					{
						if ((i * EffectBoneNum + BoneIndex) < WeightNum)
						{
							TempSkinWeightMem[i * EffectBoneNum + BoneIndex] = SkinData.Weights[i * EffectBoneNum + BoneIndex];
						}
					}
				}
				const UINT32 WeightStride = EffectBoneNum * sizeof(FLOAT);
				RSubresourceDataDesc SubresourceDataDesc;
				SubresourceDataDesc.pSysMem = TempSkinWeightMem;
				RVertexBufferResource TempSkinWeightBufferResource;
				if (!(RenderDevice->CreateBuffer(
					TempSkinWeightBufferResource,
					RBufferDesc(WeightStride * SkinData.ElementNum, RBindFlagType::BIND_VERTEX_BUFFER, 4u),
					(&SubresourceDataDesc))))
				{
#if _EDITOR_ONLY
					EString ErrorInfo("Create skin resource skinned mesh [name = ");
					ErrorInfo += SkinnedMesh->GetMeshName();
					ErrorInfo += "] skin weight buffer [num = ";
					ErrorInfo += ToString(SkinData.ElementNum);
					ErrorInfo += "] failed.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
#endif
					FullyCreated = FALSE;
				}
				delete[]TempSkinWeightMem;
				TempSkinWeightBufferResource.Stride = WeightStride;
				SkinRenderResources.Add(TempSkinWeightBufferResource);
			}
		}
		return FullyCreated;
	}
	void ESkinnedMeshRenderResource::CopySkinRenderResourcesInternal(const ESkinnedMeshRenderResource* Other)
	{
		if ((!Other) || (!(Other->IsRenderResourceValid())))
		{
			return;
		}
		if (Other->SkinRenderResources.Num() > 0)
		{
			for (INT32 i = 0, n = Other->SkinRenderResources.Num(); i < n; i++)
			{
				SkinRenderResources.Add(Other->SkinRenderResources[i]);
			}
		}
	}

	EStaticMeshAsset::EStaticMeshAsset(const EString& InMeshPath, const EString& InMeshName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	)
		: TMeshBaseAsset<EMeshType::MESH_TYPE_STATIC, EStaticMesh, EStaticMeshRenderResource>(InMeshPath, InMeshName
#if _EDITOR_ONLY
			, InDebugName
#endif
		)
	{
	}
	EStaticMeshAsset::~EStaticMeshAsset()
	{
	}
	BOOL32 EStaticMeshAsset::InitResource()
	{
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Static mesh name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EStaticMesh* InResource)->EStaticMeshRenderResource*
			{
				return (this->CreateMeshResource(InResource));
			}, TRUE, [](EStaticMesh* InResource)->void
			{
				InResource->UnsafeReleaseIndexPart();
				InResource->UnsafeReleaseVertexPart();
			}))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	EStaticMeshRenderResource* EStaticMeshAsset::CreateMeshResource(EStaticMesh* InResource)
	{
		EStaticMeshRenderResource* Result = nullptr;
		if ((!InResource) || (!(InResource->IsResourceValid())))
		{
			return Result;
		}
		Result = new EStaticMeshRenderResource(InResource);
		if (!(Result->InitRenderResource()))
		{
			Result->ReleaseRenderResource();
			delete Result;
			Result = nullptr;
		}
		return Result;
	}

	ESkinnedMeshAsset::ESkinnedMeshAsset(const EString& InMeshPath, const EString& InMeshName
#if _EDITOR_ONLY
		, const EString& InDebugName
#endif
	)
		: TMeshBaseAsset<EMeshType::MESH_TYPE_SKIN, ESkinnedMesh, ESkinnedMeshRenderResource>(InMeshPath, InMeshName
#if _EDITOR_ONLY
			, InDebugName
#endif
		), Skeleton(nullptr)
	{
	}
	ESkinnedMeshAsset::~ESkinnedMeshAsset()
	{
	}
	BOOL32 ESkinnedMeshAsset::InitResource()
	{
		if (IsInitialized())
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Skinned mesh name=[") + GetAssetName() + "] path = [" + GetAssetPath() + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](ESkinnedMesh* InResource)->ESkinnedMeshRenderResource*
			{
				return (this->CreateMeshResource(InResource));
			}, TRUE, [](ESkinnedMesh* InResource)->void
			{
				InResource->UnsafeReleaseIndexPart();
				InResource->UnsafeReleaseVertexPart();
				InResource->UnsafeReleaseSkinPart();
			}))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	void ESkinnedMeshAsset::SetSkeleton(const ESkeleton* InSkeleton)const
	{
		if (!!InSkeleton)
		{
			Skeleton = InSkeleton;
		}
	}
	const ESkeleton* ESkinnedMeshAsset::GetSkeleton()const
	{
		return Skeleton;
	}
	ESkinnedMeshRenderResource* ESkinnedMeshAsset::CreateMeshResource(ESkinnedMesh* InResource)
	{
		ESkinnedMeshRenderResource* Result = nullptr;
		if ((!InResource) || (!(InResource->IsResourceValid())))
		{
			return Result;
		}
		Result = new ESkinnedMeshRenderResource(Skeleton, InResource);
		if (!(Result->InitRenderResource()))
		{
			Result->ReleaseRenderResource();
			delete Result;
			Result = nullptr;
		}
		return Result;
	}

	EMeshAssetManager::EMeshAssetManager()
	{
#if _EDITOR_ONLY
		DebugName = ENGINE_MESH_ASSET_MANAGER_NAME;
#endif
	}
	EMeshAssetManager::~EMeshAssetManager()
	{
	}
	void EMeshAssetManager::Initialize()
	{
	}
	void EMeshAssetManager::ShutDown()
	{
		ClearStaticMeshes();
		ClearSkinnedMeshes();
	}
#if _EDITOR_ONLY
	BOOL32 EMeshAssetManager::ImportStaticMesh(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath, const BOOL32 IsCombineSubmeshes)
	{
		EString TempSaveFullPathName(InSavePath);
		TempSaveFullPathName = TempSaveFullPathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempSaveFullPathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for mesh importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString ImportPathName; EString ImportFileType;
		if (!(SplitByLastSign('.', InImportFullPathName, ImportPathName, ImportFileType)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for mesh importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		//TODO Check import type(like fbx, obj, ab...).
		if (ImportPathName.Length() <= 3u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for mesh importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		CAssimpManager* AssimpManager = CAssimpManager::GetManagerSingleton();
		TArray<EStaticMesh> AssimpMeshes;
		if ((AssimpManager->ReadStaticMeshFile(InImportFullPathName, AssimpMeshes, IsCombineSubmeshes)) != (CAssimpManager::CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Assimp importer can not load mesh file from path (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		UINT32 SavedMeshCount = 0u;
		for (INT32 i = 0, n = AssimpMeshes.Num(); i < n; i++)
		{
			EStaticMesh& AssimpMesh = AssimpMeshes[i];
			EString OutputName(InAssetName);
			if (i > 0)
			{
				OutputName += "_";
				OutputName += ToString(i);
			}
			if (!(SaveMeshResource<EStaticMesh>(InSavePath, OutputName, (&(AssimpMesh)))))
			{
				continue;
			}
			SavedMeshCount++;
		}
		return (SavedMeshCount > 0u);
	}
	BOOL32 EMeshAssetManager::ImportSkinnedMesh(const EString& InAssetName, const EString& InImportFullPathName, const EString& InSavePath, const BOOL32 IsCombineSubmeshes)
	{
		EString TempSaveFullPathName(InSavePath);
		TempSaveFullPathName = TempSaveFullPathName + InAssetName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if ((InImportFullPathName.Length() < 3u) || (TempSaveFullPathName.Length() < 10u))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for mesh importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EString ImportPathName; EString ImportFileType;
		if (!(SplitByLastSign('.', InImportFullPathName, ImportPathName, ImportFileType)))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for mesh importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		//TODO Check import type(like fbx, obj, ab...).
		if (ImportPathName.Length() <= 3u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Error file path for mesh importer (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		CAssimpManager* AssimpManager = CAssimpManager::GetManagerSingleton();
		TArray<ESkinnedMesh> AssimpMeshes;
		if ((AssimpManager->ReadSkinnedMeshFile(InImportFullPathName, AssimpMeshes, IsCombineSubmeshes)) != (CAssimpManager::CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Assimp importer can not load mesh file from path (import file path : ");
				ErrorData = ErrorData + InImportFullPathName + ", save assset path : " + TempSaveFullPathName + ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		UINT32 SavedMeshCount = 0u;
		for (INT32 i = 0, n = AssimpMeshes.Num(); i < n; i++)
		{
			ESkinnedMesh& AssimpMesh = AssimpMeshes[i];
			EString OutputName(InAssetName);
			if (i > 0)
			{
				OutputName += "_";
				OutputName += ToString(i);
			}
			if (!(SaveMeshResource<ESkinnedMesh>(InSavePath, OutputName, (&(AssimpMesh)))))
			{
				continue;
			}
			SavedMeshCount++;
		}
		return (SavedMeshCount > 0u);
	}
#endif
	BOOL32 EMeshAssetManager::LoadStaticMeshAsset(const EString& InLoadPath, const EString& InLoadName, const EStaticMeshAsset*& OutStaticMeshAsset)
	{
		EString TempLoadFullPathName(InLoadPath);
		TempLoadFullPathName = TempLoadFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		EStaticMeshAsset* ResultMeshAsset = StaticMeshManager.Find(TempLoadFullPathName);
		if (ResultMeshAsset)
		{
			OutStaticMeshAsset = ResultMeshAsset;
			return TRUE;
		}
		ResultMeshAsset = LoadMeshAsset<EStaticMeshAsset, EStaticMesh>(InLoadPath, InLoadName);
		if (!ResultMeshAsset)
		{
			return FALSE;
		}
		if (!ResultMeshAsset->InitResource())
		{
			delete ResultMeshAsset;
			return FALSE;
		}
		if (StaticMeshManager.Add(TempLoadFullPathName, ResultMeshAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Static mesh asset path = [") + TempLoadFullPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultMeshAsset;
			return FALSE;
		}
		OutStaticMeshAsset = ResultMeshAsset;
		return TRUE;
	}
	BOOL32 EMeshAssetManager::LoadSkinnedMeshAsset(const EString& InLoadPath, const EString& InLoadName, const ESkinnedMeshAsset*& OutSkinnedMeshAsset)
	{
		EString TempLoadFullPathName(InLoadPath);
		TempLoadFullPathName = TempLoadFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		ESkinnedMeshAsset* ResultMeshAsset = SkinnedMeshManager.Find(TempLoadFullPathName);
		if (ResultMeshAsset)
		{
			OutSkinnedMeshAsset = ResultMeshAsset;
			return TRUE;
		}
		ResultMeshAsset = LoadMeshAsset<ESkinnedMeshAsset, ESkinnedMesh>(InLoadPath, InLoadName);
		if (!ResultMeshAsset)
		{
			return FALSE;
		}
		if (!ResultMeshAsset->InitResource())
		{
			delete ResultMeshAsset;
			return FALSE;
		}
		if (SkinnedMeshManager.Add(TempLoadFullPathName, ResultMeshAsset, TRUE) == 0u)
		{
#if _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Skinned mesh asset path = [") + TempLoadFullPathName + "] add into manager list failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
			}
#endif
			delete ResultMeshAsset;
			return FALSE;
		}
		OutSkinnedMeshAsset = ResultMeshAsset;
		return TRUE;
	}
	void EMeshAssetManager::ClearStaticMeshes()
	{
		StaticMeshManager.Clear();
	}
	void EMeshAssetManager::ClearSkinnedMeshes()
	{
		SkinnedMeshManager.Clear();
	}
	template<typename _TMeshAssetType, typename _TMeshResourceType>
	_TMeshAssetType* EMeshAssetManager::LoadMeshAsset(const EString& InLoadPath, const EString& InLoadName)
	{
		EString TempLoadFullPathName(InLoadPath);
		TempLoadFullPathName = TempLoadFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		_TMeshAssetType* NewMeshAsset = new _TMeshAssetType(InLoadPath, InLoadName
#if _EDITOR_ONLY
			, InLoadName
#endif
		);
		EMeshType MeshType = NewMeshAsset->GetMeshType();
		switch (MeshType)
		{
		case EMeshType::MESH_TYPE_STATIC:
			if ((!(IsClass(GetClassHashCode<_TMeshAssetType>(), GetClassHashCode<EStaticMeshAsset>()))) ||
				(!(IsClass(GetClassHashCode<_TMeshResourceType>(), GetClassHashCode<EStaticMesh>()))))
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Load mesh asset failed (load file path : ");
					ErrorData += TempLoadFullPathName;
					ErrorData += ") this asset is not match mesh asset type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				delete NewMeshAsset;
				NewMeshAsset = nullptr;
				return NewMeshAsset;
			}
			break;
		case EMeshType::MESH_TYPE_SKIN:
			if ((!(IsClass(GetClassHashCode<_TMeshAssetType>(), GetClassHashCode<ESkinnedMeshAsset>()))) ||
				(!(IsClass(GetClassHashCode<_TMeshResourceType>(), GetClassHashCode<ESkinnedMesh>()))))
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Load mesh asset failed (load file path : ");
					ErrorData += TempLoadFullPathName;
					ErrorData += ") this asset is not match mesh asset type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				delete NewMeshAsset;
				NewMeshAsset = nullptr;
				return NewMeshAsset;
			}
			break;
		default:
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Load mesh asset failed (load file path : ");
					ErrorData += TempLoadFullPathName;
					ErrorData += ") this asset is not mesh type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				delete NewMeshAsset;
				NewMeshAsset = nullptr;
				return NewMeshAsset;
			}
			break;
		}
		_TMeshResourceType* LoadedMeshResource = LoadMeshResource<_TMeshResourceType>(InLoadPath, InLoadName);
		if (!LoadedMeshResource)
		{
			delete NewMeshAsset;
			NewMeshAsset = nullptr;
			return NewMeshAsset;
		}
		if (!(NewMeshAsset->StorageResourceInternal(
			[LoadedMeshResource]()->_TMeshResourceType*
			{
				return LoadedMeshResource;
			})))
		{
			//TODO
			delete NewMeshAsset;
			NewMeshAsset = nullptr;
			return NewMeshAsset;
		}
		return NewMeshAsset;
	}
	template<typename _TMeshAssetType, typename _TMeshResourceType>
	BOOL32 EMeshAssetManager::SaveMeshAsset(const EString& InSavePath, const EString& InSaveName, const _TMeshAssetType* InMeshAsset)
	{
		EString TempSaveFullPathName = InSavePath + InSaveName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (!InMeshAsset)
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save mesh asset failed (save file path : ");
				ErrorData += TempSaveFullPathName;
				ErrorData += ") this asset is null.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		EMeshType MeshType = InMeshAsset->GetMeshType();
		const _TMeshResourceType* SavedMeshResource = InMeshAsset->GetStoragedResource();
		if ((!SavedMeshResource) || (!(SavedMeshResource->IsResourceValid())))
		{
#if _EDITOR_ONLY
			{
				EString ErrorData("Save mesh asset failed (save file path : ");
				ErrorData += TempSaveFullPathName;
				ErrorData += ", mesh asset name : ";
				ErrorData += InMeshAsset->GetAssetName();
				ErrorData += ") this asset is not contain valid mesh resource.";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return FALSE;
		}
		switch (MeshType)
		{
		case EMeshType::MESH_TYPE_STATIC:
			if ((!(IsClass(GetClassHashCode<_TMeshAssetType>(), GetClassHashCode<EStaticMeshAsset>()))) ||
				(!(IsClass(GetClassHashCode<_TMeshResourceType>(), GetClassHashCode<EStaticMesh>()))) ||
				(!(SavedMeshResource->IsTypeOf<EStaticMesh>())))
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Save mesh asset failed (save file path : ");
					ErrorData += TempSaveFullPathName;
					ErrorData += ", mesh asset name : ";
					ErrorData += InMeshAsset->GetAssetName();
					ErrorData += ") this asset is not match mesh asset type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				return FALSE;
			}
			break;
		case EMeshType::MESH_TYPE_SKIN:
			if ((!(IsClass(GetClassHashCode<_TMeshAssetType>(), GetClassHashCode<ESkinnedMeshAsset>()))) ||
				(!(IsClass(GetClassHashCode<_TMeshResourceType>(), GetClassHashCode<ESkinnedMesh>()))) ||
				(!(SavedMeshResource->IsTypeOf<ESkinnedMesh>())))
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Save mesh asset failed (save file path : ");
					ErrorData += TempSaveFullPathName;
					ErrorData += ", mesh asset name : ";
					ErrorData += InMeshAsset->GetAssetName();
					ErrorData += ") this asset is not match mesh asset type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				return FALSE;
			}
			break;
		default:
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Save mesh asset failed (save file path : ");
					ErrorData += TempSaveFullPathName;
					ErrorData += ", mesh asset name : ";
					ErrorData += InMeshAsset->GetAssetName();
					ErrorData += ") this asset is not mesh type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				return FALSE;
			}
			break;
		}
		return (SaveMeshResource<_TMeshResourceType>(InSavePath, InSaveName, SavedMeshResource));
	}
	template<typename _TMeshResourceType>
	_TMeshResourceType* EMeshAssetManager::LoadMeshResource(const EString& InLoadPath, const EString& InLoadName)
	{
		EString TempLoadFullPathName = (InLoadPath);
		TempLoadFullPathName = TempLoadFullPathName + InLoadName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		_TMeshResourceType* LoadedMeshResource = nullptr;
		void* ReadFileMem = nullptr; ULONG ReadFileSize = 0u;
		if (!EFileHelper::ReadFileAsBinary(TempLoadFullPathName, ReadFileMem, ReadFileSize))
		{
			if (ReadFileMem)
			{
				delete[]ReadFileMem;
			}
#if _EDITOR_ONLY
			{
				EString ErrorData("Load mesh asset failed (load file path : ");
				ErrorData += TempLoadFullPathName;
				ErrorData += ").";
				PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
			}
#endif
			return LoadedMeshResource;
		}

		const void* RstMemPtr = ReadFileMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(ReadFileSize);

#define LOAD_ASSET_MEMORY(__MemType, __MemSize, __OutputValue) \
		__MemType __OutputValue;\
		{\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load mesh asset [rest memory size] failed."), (RstMemSize >= (__MemSize)));\
			const __MemType* TempPtr = (const __MemType*)RstMemPtr;\
			(__OutputValue) = TempPtr[0];\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (__MemSize);\
		}\

#define LOAD_ASSET_STRING_MEMORY(__LengthMax, __OutputEString) \
		EString __OutputEString;\
		{\
			const UINT32 StrStride = sizeof(CHAR) * (__LengthMax);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load mesh asset [rest memory size] failed."), (RstMemSize >= StrStride));\
			const CHAR* TempPtr = (const CHAR*)RstMemPtr;\
			CHAR StrValues[(__LengthMax)];\
			::memcpy_s(StrValues, StrStride, TempPtr, StrStride);\
			(__OutputEString) = StrValues;\
			RstMemPtr = &(TempPtr[(__LengthMax)]);\
			RstMemSize -= StrStride;\
		}\

#define LOAD_ASSET_PTR_MEMORY(__ElementStride, __ElementNum, __PtrType, __Ptr) \
		__PtrType* __Ptr = nullptr;\
		{\
			const UINT32 MemSize = (__ElementStride) * (__ElementNum);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check load mesh asset [rest memory size] failed."), (RstMemSize >= MemSize));\
			void* NewPtr = new BYTE[MemSize];\
			const BYTE* TempPtr = (const BYTE*)RstMemPtr;\
			::memcpy_s(NewPtr, MemSize, TempPtr, MemSize);\
			(__Ptr) = (__PtrType*)NewPtr;\
			RstMemPtr = &(TempPtr[MemSize]);\
			RstMemSize -= MemSize;\
		}\

		EMeshType MeshType = EMeshType::MESH_TYPE_COUNT;
		{
			EAssetType AssetType = EAssetType::ASSET_TYPE_UNKNOWN;
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempAssetType);
				AssetType = static_cast<EAssetType>(TempAssetType);
			}
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempMeshType);
				MeshType = static_cast<EMeshType>(TempMeshType);
			}
			if (AssetType != EAssetType::ASSET_TYPE_MESH)
			{
#if _EDITOR_ONLY
				{
					EString ErrorData("Load mesh asset failed (load file path : ");
					ErrorData += TempLoadFullPathName;
					ErrorData += ") this asset is not mesh type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				delete[]ReadFileMem;
				return LoadedMeshResource;
			}
			switch (MeshType)
			{
			case EMeshType::MESH_TYPE_STATIC:
				if (!(IsClass(GetClassHashCode<_TMeshResourceType>(), GetClassHashCode<EStaticMesh>())))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Load mesh asset failed, type check failed. This asset is static mesh asset which is not wanted."));
					delete[]ReadFileMem;
					return LoadedMeshResource;
				}
				break;
			case EMeshType::MESH_TYPE_SKIN:
				if (!(IsClass(GetClassHashCode<_TMeshResourceType>(), GetClassHashCode<ESkinnedMesh>())))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Load mesh asset failed, type check failed. This asset is skinned mesh asset which is not wanted."));
					delete[]ReadFileMem;
					return LoadedMeshResource;
				}
				break;
			default:
#if _EDITOR_ONLY
				{
					EString ErrorData("Load mesh asset failed (load file path : ");
					ErrorData += TempLoadFullPathName;
					ErrorData += ") this asset is not mesh type.";
					PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorData));
				}
#endif
				delete[]ReadFileMem;
				return LoadedMeshResource;
				break;
			}
		}

		{
			LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_MESH_NAME_LENGTH_MAX, TempMeshName);
			LoadedMeshResource = new _TMeshResourceType(TempMeshName);
		}
		{
			EBoundAABB& MeshBoundAABB = LoadedMeshResource->BoundAABB;

			LOAD_ASSET_MEMORY(EBoundAABB, sizeof(EBoundAABB), TempBoundAABB);
			MeshBoundAABB = TempBoundAABB;
		}
		{
			UINT32& MeshVertexLayout = LoadedMeshResource->VertexLayout;

			LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempVertexLayout);
			MeshVertexLayout = TempVertexLayout;
		}
		{
			EMesh::ESubmeshPart& MeshSubmeshPart = LoadedMeshResource->Submeshes;

			LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempSubmeshNum);
			if (TempSubmeshNum > 0u)
			{
				MeshSubmeshPart.SetNum(static_cast<UINT32>(TempSubmeshNum));
				for (UINT32 i = 0u; i < TempSubmeshNum; i++)
				{
					ESubmeshData& MeshSubmeshData = MeshSubmeshPart[i];

					LOAD_ASSET_MEMORY(ESubmeshData, sizeof(ESubmeshData), TempSubmeshData);

					MeshSubmeshData = TempSubmeshData;
				}
			}
		}
		{
			EMesh::EIndexPart& MeshIndexPart = LoadedMeshResource->Indices;

			LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempIndexElementNum);
			LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempIndexStride);
			LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempIndexPartType);
			LOAD_ASSET_PTR_MEMORY(sizeof(UINT32), TempIndexElementNum, UINT32, TempIndexDatas);

			MeshIndexPart.ElementNum	= static_cast<UINT32>(TempIndexElementNum);
			MeshIndexPart.Stride		= static_cast<UINT32>(TempIndexStride);
			MeshIndexPart.PartType		= static_cast<UINT32>(TempIndexPartType);
			MeshIndexPart.Indices		= TempIndexDatas;
		}
		{
			EMesh::EVertexPart& MeshVertexPart = LoadedMeshResource->Vertices;

			LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempVertexPartNum);
			if (TempVertexPartNum > 0u)
			{
				MeshVertexPart.SetNum(static_cast<UINT32>(TempVertexPartNum));
				for (UINT32 i = 0u; i < TempVertexPartNum; i++)
				{
					EVertexData& MeshVertexData = MeshVertexPart[i];

					LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempVertexElementNum);
					LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempVertexStride);
					LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempVertexPartType);
					LOAD_ASSET_PTR_MEMORY(TempVertexStride, TempVertexElementNum, FLOAT, TempVertexDatas);

					MeshVertexData.ElementNum	= static_cast<UINT32>(TempVertexElementNum);
					MeshVertexData.Stride		= static_cast<UINT32>(TempVertexStride);
					MeshVertexData.PartType		= static_cast<UINT32>(TempVertexPartType);
					MeshVertexData.Datas		= TempVertexDatas;
				}
			}
		}
		if (MeshType == EMeshType::MESH_TYPE_SKIN)
		{
			ESkinnedMesh* TempMeshPtr = LoadedMeshResource->AsType<ESkinnedMesh>();
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check skinned mesh is null."), (!!TempMeshPtr));
#if _EDITOR_ONLY
			if (TempMeshPtr)
#endif
			{
				ESkinnedMesh::EBindPoseValue& MeshBindPoseValue = TempMeshPtr->BindPoseValue;
				ESkinnedMesh::EBindPoseIndex& MeshBindPoseIndex = TempMeshPtr->BindPoseIndex;

				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempBindPoseNum);
				for (UINT32 i = 0u; i < TempBindPoseNum; i++)
				{
					LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX, TempBindPoseName);
					LOAD_ASSET_MEMORY(Matrix4x4, sizeof(Matrix4x4), TempBindPoseValue);
					MeshBindPoseValue.Add(TempBindPoseName, TempBindPoseValue);
				}
				for (UINT32 i = 0u; i < TempBindPoseNum; i++)
				{
					LOAD_ASSET_STRING_MEMORY(EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX, TempBindPoseName);
					LOAD_ASSET_MEMORY(USHORT, sizeof(USHORT), TempBindPoseIndex);
					MeshBindPoseIndex.Add(TempBindPoseName, TempBindPoseIndex);
				}
			}
			{
				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempEffectBoneNum);
				TempMeshPtr->EffectBoneNum = static_cast<USHORT>(TempEffectBoneNum);

				ESkinnedMesh::ESkinPart& MeshSkinPart = TempMeshPtr->Skins;

				LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempSkinPartNum);
				if (TempSkinPartNum > 0u)
				{
					MeshSkinPart.SetNum(static_cast<UINT32>(TempSkinPartNum));
					for (UINT32 i = 0u; i < TempSkinPartNum; i++)
					{
						ESkinData& MeshSkinData = MeshSkinPart[i];

						LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempSkinElementNum);
						LOAD_ASSET_MEMORY(UINT32, sizeof(UINT32), TempSkinPartType);
						LOAD_ASSET_PTR_MEMORY(sizeof(UINT32)* TempEffectBoneNum, TempSkinElementNum, UINT32, TempSkinIndices);
						LOAD_ASSET_PTR_MEMORY(sizeof(FLOAT)* TempEffectBoneNum, TempSkinElementNum, FLOAT, TempSkinWeights);

						MeshSkinData.ElementNum	= static_cast<UINT32>(TempSkinElementNum);
						MeshSkinData.Stride		= static_cast<UINT32>(TempEffectBoneNum * sizeof(FLOAT));
						MeshSkinData.PartType	= static_cast<UINT32>(TempSkinPartType);
						MeshSkinData.Indices	= TempSkinIndices;
						MeshSkinData.Weights	= TempSkinWeights;
					}
				}
			}
		}

		PE_CHECK((ENGINE_ASSET_ERROR), ("Check read mesh resource rest memory already failed."), (RstMemSize == 0));

#undef LOAD_ASSET_MEMORY
#undef LOAD_ASSET_STRING_MEMORY
#undef LOAD_ASSET_PTR_MEMORY

		delete[]ReadFileMem;
		if (!(LoadedMeshResource->IsResourceValid()))
		{
			delete LoadedMeshResource;
			LoadedMeshResource = nullptr;
		}
		return LoadedMeshResource;
	}
	template<typename _TMeshResourceType>
	BOOL32 EMeshAssetManager::SaveMeshResource(const EString& InSavePath, const EString& InSaveName, const _TMeshResourceType* InMeshResource)
	{
		if ((!InMeshResource) || (!(InMeshResource->IsResourceValid())))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check saved mesh resource error, mesh resource is null."));
			return FALSE;
		}

		auto CalculateMeshVertexBytes = [](const _TMeshResourceType* InMesh)->ULONG
		{
			ULONG Result = sizeof(UINT32) + sizeof(UINT32);	// EAssetType + EMeshType
			{
				Result += sizeof(CHAR) * EEngineSettings::ENGINE_MESH_NAME_LENGTH_MAX;	// Mesh name (path)
				Result += sizeof(EBoundAABB);		// AABB
				Result += sizeof(UINT32);			// Vertex layout
				Result += sizeof(UINT32);			// Submesh element num
				Result += sizeof(ESubmeshData) * InMesh->GetSubmeshPart().Num<UINT32>();	// Submesh datas
			}
			{
				Result += sizeof(UINT32);	// Indices element num
				Result += sizeof(UINT32);	// Indices stride
				Result += sizeof(UINT32);	// Indices part type
				Result += sizeof(UINT32) * InMesh->GetIndexPart().ElementNum;	// Indices datas
			}
			{
				const EMesh::EVertexPart& VertexPart = InMesh->GetVertexPart();
				Result += sizeof(UINT32);	// Vertex part element num
				for (INT32 VertexIndex = 0, VertexNum = VertexPart.Num(); VertexIndex < VertexNum; VertexIndex++)
				{
					const EVertexData& VertexData = VertexPart[VertexIndex];
					Result += sizeof(UINT32);	// Vertices element num
					Result += sizeof(UINT32);	// Vertices stride
					Result += sizeof(UINT32);	// Vertices part type
					Result += VertexData.Stride * VertexData.ElementNum;	// Vertices datas
				}
			}
			if (InMesh->IsTypeOf<ESkinnedMesh>())
			{
				const ESkinnedMesh* TempMeshPtr = InMesh->AsType<ESkinnedMesh>();
				PE_CHECK((ENGINE_ASSET_ERROR), ("Check skinned mesh is null."), (!!TempMeshPtr));
#if _EDITOR_ONLY
				if (TempMeshPtr)
#endif
				{
					PE_CHECK((ENGINE_ASSET_ERROR), ("Check skinned mesh bind pose num error."), ((TempMeshPtr->GetBindPoseValue().Num()) == (TempMeshPtr->GetBindPoseIndex().Num())));
					Result += sizeof(UINT32);	// Bind pose num
					Result += (sizeof(CHAR) * EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX + sizeof(Matrix4x4)) * TempMeshPtr->GetBindPoseValue().Num<UINT32>();	// Bind pose value datas
					Result += (sizeof(CHAR) * EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX + sizeof(USHORT)) * TempMeshPtr->GetBindPoseIndex().Num<UINT32>();		// Bind pose Index datas
					Result += sizeof(UINT32);	// Effect bone num
					const ESkinnedMesh::ESkinPart& SkinPart = TempMeshPtr->GetSkinPart();
					Result += sizeof(UINT32);	// Skin part num
					for (INT32 SkinIndex = 0, SkinNum = SkinPart.Num(); SkinIndex < SkinNum; SkinIndex++)
					{
						const ESkinData& SkinData = SkinPart[SkinIndex];
						const UINT32 EffectBoneNum = SkinData.Stride / sizeof(FLOAT);
						PE_CHECK((ENGINE_ASSET_ERROR), ("Check skinned mesh effect bone num error."), (EffectBoneNum == (TempMeshPtr->GetEffectBoneNum())));
						Result += sizeof(UINT32);	// Skin element num
						Result += sizeof(UINT32);	// Skin part type
						Result += sizeof(UINT32) * EffectBoneNum * SkinData.ElementNum;	// Skin index datas
						Result += sizeof(FLOAT) * EffectBoneNum * SkinData.ElementNum;	// Skin weight datas
					}
				}
			}
			return Result;
		};

		EMeshType MeshType = EMeshType::MESH_TYPE_COUNT;
		if (InMeshResource->IsTypeOf<EStaticMesh>())
		{
			MeshType = EMeshType::MESH_TYPE_STATIC;
		}
		else if (InMeshResource->IsTypeOf<ESkinnedMesh>())
		{
			MeshType = EMeshType::MESH_TYPE_SKIN;
		}
		if (MeshType >= EMeshType::MESH_TYPE_COUNT)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check saved mesh resource error, mesh resource type is weird."));
			return FALSE;
		}

		const ULONG OutputMemSize = CalculateMeshVertexBytes(InMeshResource);
		BYTE* OutputMem = new BYTE[OutputMemSize];

		void* RstMemPtr = OutputMem;
		LONGLONG RstMemSize = static_cast<LONGLONG>(OutputMemSize);

#define SAVE_ASSET_MEMORY(__Type, __Value) \
		{\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save mesh asset [rest memory size] failed."), (RstMemSize >= (sizeof(__Type))));\
			__Type* TempPtr = (__Type*)RstMemPtr;\
			TempPtr[0] = (__Value);\
			RstMemPtr = &(TempPtr[1]);\
			RstMemSize -= (sizeof(__Type));\
		}\

#define SAVE_ASSET_STRING_MEMORY(__EString, __LengthMax) \
		{\
			const UINT32 NameLengthMax = sizeof(CHAR) * (__LengthMax);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save mesh asset [rest memory size] failed."), (RstMemSize >= NameLengthMax));\
			const EString& SavedName = (__EString);\
			const UINT32 NameLength = EMath::Clamp(static_cast<UINT32>(sizeof(CHAR) * (SavedName.Length() + 1u)), 0u, NameLengthMax);\
			CHAR* TempPtr = (CHAR*)RstMemPtr;\
			::memcpy_s(TempPtr, NameLengthMax, (*SavedName), NameLength);\
			RstMemPtr = &(TempPtr[(__LengthMax)]);\
			RstMemSize -= NameLengthMax;\
		}\

#define SAVE_ASSET_PTR_MEMORY(__ElementStride, __ElementNum, __Ptr) \
		{\
			const UINT32 MemSize = (__ElementStride) * (__ElementNum);\
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check save mesh asset [rest memory size] failed."), (RstMemSize >= MemSize));\
			BYTE* TempPtr = (BYTE*)RstMemPtr;\
			::memcpy_s(TempPtr, MemSize, (__Ptr), MemSize);\
			RstMemPtr = &(TempPtr[MemSize]);\
			RstMemSize -= MemSize;\
		}\

		SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(EAssetType::ASSET_TYPE_MESH));
		SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(MeshType));
		SAVE_ASSET_STRING_MEMORY(InMeshResource->GetMeshName(), EEngineSettings::ENGINE_MESH_NAME_LENGTH_MAX);
		SAVE_ASSET_MEMORY(EBoundAABB, InMeshResource->GetBoundAABB());
		SAVE_ASSET_MEMORY(UINT32, InMeshResource->GetVertexLayout());
		{
			const EMesh::ESubmeshPart& SubmeshPart = InMeshResource->GetSubmeshPart();

			SAVE_ASSET_MEMORY(UINT32, (SubmeshPart.Num<UINT32>()));

			for (INT32 i = 0, n = SubmeshPart.Num(); i < n; i++)
			{
				const ESubmeshData& SubmeshData = SubmeshPart[i];

				SAVE_ASSET_MEMORY(ESubmeshData, SubmeshData);
			}
		};
		{
			const EMesh::EIndexPart& IndexPart = InMeshResource->GetIndexPart();

			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(IndexPart.ElementNum));
			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(IndexPart.Stride));
			SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(IndexPart.PartType));
			SAVE_ASSET_PTR_MEMORY(sizeof(UINT32), IndexPart.ElementNum, IndexPart.Indices);
		};
		{
			const EMesh::EVertexPart& VertexPart = InMeshResource->GetVertexPart();

			SAVE_ASSET_MEMORY(UINT32, (VertexPart.Num<UINT32>()));

			for (INT32 i = 0, n = VertexPart.Num(); i < n; i++)
			{
				const EVertexData& VertexData = VertexPart[i];

				SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(VertexData.ElementNum));
				SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(VertexData.Stride));
				SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(VertexData.PartType));
				SAVE_ASSET_PTR_MEMORY(VertexData.Stride, VertexData.ElementNum, VertexData.Datas);
			}
		};
		if (InMeshResource->IsTypeOf<ESkinnedMesh>())
		{
			const ESkinnedMesh* TempMeshPtr = InMeshResource->AsType<ESkinnedMesh>();
			PE_CHECK((ENGINE_ASSET_ERROR), ("Check skinned mesh is null."), (!!TempMeshPtr));
#if _EDITOR_ONLY
			if (TempMeshPtr)
#endif
			{
				const ESkinnedMesh::EBindPoseValue& BindPoseValues = TempMeshPtr->GetBindPoseValue();
				const ESkinnedMesh::EBindPoseIndex& BindPoseIndices = TempMeshPtr->GetBindPoseIndex();
				PE_CHECK((ENGINE_ASSET_ERROR), ("Check save skinned mesh asset bind pose num error."), (BindPoseValues.Num() == BindPoseIndices.Num()));
				const INT32 BindPoseNum = BindPoseValues.Num();

				SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(BindPoseNum));

				for (auto ItValue = BindPoseValues.Begin(); ItValue != BindPoseValues.End(); ItValue++)
				{
					SAVE_ASSET_STRING_MEMORY(ItValue->first, EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX);
					SAVE_ASSET_MEMORY(Matrix4x4, (ItValue->second));
				}
				for (auto ItIndices = BindPoseIndices.Begin(); ItIndices != BindPoseIndices.End(); ItIndices++)
				{
					SAVE_ASSET_STRING_MEMORY(ItIndices->first, EEngineSettings::ENGINE_BONE_NAME_LENGTH_MAX);
					SAVE_ASSET_MEMORY(USHORT, (ItIndices->second));
				}
			};
			{
				const UINT32 EffectBoneNum = static_cast<UINT32>(TempMeshPtr->GetEffectBoneNum());

				SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(EffectBoneNum));

				const ESkinnedMesh::ESkinPart& SkinPart = TempMeshPtr->GetSkinPart();

				SAVE_ASSET_MEMORY(UINT32, (SkinPart.Num<UINT32>()));

				for (INT32 i = 0, n = SkinPart.Num(); i < n; i++)
				{
					const ESkinData& SkinData = SkinPart[i];
					PE_CHECK((ENGINE_ASSET_ERROR), ("Check skinned mesh effect bone num error."), ((SkinData.Stride / sizeof(FLOAT)) == EffectBoneNum));

					SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(SkinData.ElementNum));
					SAVE_ASSET_MEMORY(UINT32, static_cast<UINT32>(SkinData.PartType));
					SAVE_ASSET_PTR_MEMORY((EffectBoneNum * sizeof(UINT32)), SkinData.ElementNum, SkinData.Indices);
					SAVE_ASSET_PTR_MEMORY((EffectBoneNum * sizeof(FLOAT)), SkinData.ElementNum, SkinData.Weights);
				}
			};
		}

		PE_CHECK((ENGINE_ASSET_ERROR), ("Check write mesh resource rest memory already failed."), (RstMemSize == 0));

#undef SAVE_ASSET_MEMORY
#undef SAVE_ASSET_STRING_MEMORY
#undef SAVE_ASSET_PTR_MEMORY

		EString TempSaveFullPathName(InSavePath);
		TempSaveFullPathName = TempSaveFullPathName + InSaveName + EEngineSettings::ENGINE_ASSET_NAME_TYPE;
		if (EFileHelper::SaveBytesToFile(TempSaveFullPathName, OutputMem, OutputMemSize))
		{
			delete[]OutputMem;
			return TRUE;
		}
		delete[]OutputMem;
		return FALSE;
	}

	void TryLoadStaticMesh(const EString& InLoadPath, const EString& InLoadName, const EStaticMeshAsset*& OutAsset, const EString* InImportPath, const EString* InImportName, const EString* InImportFileType, const BOOL32 IsCombineSubmeshes)
	{
		EMeshAssetManager* MeshAssetManager = EMeshAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output static mesh asset pointer is initialized failed."), (!OutAsset));
		if (MeshAssetManager->LoadStaticMeshAsset(InLoadPath, InLoadName, OutAsset))
		{
			return;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load static mesh asset failed."));
#if _EDITOR_ONLY
		if ((!!InImportPath) && (!!InImportName) && (!!InImportFileType))
		{
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + (".") + (*InImportFileType);
			BOOL32 Result = MeshAssetManager->ImportStaticMesh(
				InLoadName,
				TempImportFullPath,
				InLoadPath,
				IsCombineSubmeshes
			);
			if (Result)
			{
				if (!(MeshAssetManager->LoadStaticMeshAsset(InLoadPath, InLoadName, OutAsset)))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try load static mesh asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import static mesh asset failed."));
			}
		}
#endif
	}
	void TryLoadSkinnedMesh(const EString& InLoadPath, const EString& InLoadName, const ESkinnedMeshAsset*& OutAsset, const EString* InImportPath, const EString* InImportName, const EString* InImportFileType, const BOOL32 IsCombineSubmeshes)
	{
		EMeshAssetManager* MeshAssetManager = EMeshAssetManager::GetManagerSingleton();
		PE_CHECK((ENGINE_ASSET_ERROR), ("Check output skinned mesh asset pointer is initialized failed."), (!OutAsset));
		if (MeshAssetManager->LoadSkinnedMeshAsset(InLoadPath, InLoadName, OutAsset))
		{
			return;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Load skinned mesh asset failed."));
#if _EDITOR_ONLY
		if ((!!InImportPath) && (!!InImportName) && (!!InImportFileType))
		{
			EString TempImportFullPath(*InImportPath);
			TempImportFullPath = TempImportFullPath + (*InImportName) + (".") + (*InImportFileType);
			BOOL32 Result = MeshAssetManager->ImportSkinnedMesh(
				InLoadName,
				TempImportFullPath,
				InLoadPath,
				IsCombineSubmeshes
			);
			if (Result)
			{
				if (!(MeshAssetManager->LoadSkinnedMeshAsset(InLoadPath, InLoadName, OutAsset)))
				{
					PE_FAILED((ENGINE_ASSET_ERROR), ("Try load skinned mesh asset failed."));
				}
			}
			else
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Try import skinned mesh asset failed."));
			}
		}
#endif
	}

};