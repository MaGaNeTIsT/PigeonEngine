#include "MeshAsset.h"
#include <RenderDevice/DeviceD3D11.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	template <typename _TAssetResourceType, typename _TAssetRenderResourceType, EMeshType _MeshType, typename _MeshAssetType>
	static void RegisterMeshClassTypes()
	{
		RegisterClassType<TBaseAsset<_TAssetResourceType>, EObjectBase>();
		RegisterClassType<TRenderBaseAsset<_TAssetResourceType, _TAssetRenderResourceType>, TBaseAsset<_TAssetResourceType>>();
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
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	UINT GetMeshVertexLayoutTypeStartBitIndex(EVertexLayoutType InType)
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
			return EVertexLayoutType::MESH_VERTEX;
		case RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD:
			return EVertexLayoutType::MESH_TEXTURECOORD;
		case RShaderSemanticType::SHADER_SEMANTIC_NORMAL:
			return EVertexLayoutType::MESH_NORMAL;
		case RShaderSemanticType::SHADER_SEMANTIC_TANGENT:
			return EVertexLayoutType::MESH_TANGENT;
		case RShaderSemanticType::SHADER_SEMANTIC_COLOR:
			return EVertexLayoutType::MESH_COLOR;
		case RShaderSemanticType::SHADER_SEMANTIC_BINORMAL:
			return EVertexLayoutType::MESH_BITANGENT;
		case RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES:
			return EVertexLayoutType::MESH_SKIN;
		case RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT:
			return EVertexLayoutType::MESH_SKIN;
		}
		PE_FAILED((ENGINE_ASSET_ERROR), ("Do not support this shader semantic type for mesh layout."));
		return EVertexLayoutType::MESH_VERTEX;	//TODO Not support type is fix output vertex type for now.
	}
	UINT TranslateVertexBaseLayoutToSemanticBaseType(EVertexLayoutType InBaseType)
	{
		switch (InBaseType)
		{
		case EVertexLayoutType::MESH_INDEX_FULL:
			return RShaderSemanticType::SHADER_SEMANTIC_NONE;
		case EVertexLayoutType::MESH_INDEX_HALF:
			return RShaderSemanticType::SHADER_SEMANTIC_NONE;
		case EVertexLayoutType::MESH_VERTEX:
			return RShaderSemanticType::SHADER_SEMANTIC_POSITION;
		case EVertexLayoutType::MESH_TEXTURECOORD:
			return RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD;
		case EVertexLayoutType::MESH_NORMAL:
			return RShaderSemanticType::SHADER_SEMANTIC_NORMAL;
		case EVertexLayoutType::MESH_TANGENT:
			return RShaderSemanticType::SHADER_SEMANTIC_TANGENT;
		case EVertexLayoutType::MESH_COLOR:
			return RShaderSemanticType::SHADER_SEMANTIC_COLOR;
		case EVertexLayoutType::MESH_BITANGENT:
			return RShaderSemanticType::SHADER_SEMANTIC_BINORMAL;
		case EVertexLayoutType::MESH_SKIN:
			return RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES | RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT;	//TODO Skin is mapping two type for now.
		}
		return RShaderSemanticType::SHADER_SEMANTIC_NONE;
	}

	EMesh::EMesh()
		: MeshName(ENGINE_DEFAULT_NAME)
		, VertexLayout(0u)
	{
#ifdef _EDITOR_ONLY
		DebugName = ENGINE_DEFAULT_NAME;
#endif
	}
	EMesh::EMesh(const EString& InMeshName)
		: MeshName(InMeshName)
		, VertexLayout(0u)
	{
#ifdef _EDITOR_ONLY
		DebugName = InMeshName;
#endif
	}
	EMesh::EMesh(const EMesh& Other)
		: MeshName(Other.MeshName)
		, BoundAABB(Other.BoundAABB)
		, VertexLayout(Other.VertexLayout)
		, Indices(Other.Indices)
	{
		for (UINT i = 0u, n = Other.Vertices.Length(); i < n; i++)
		{
			const EVertexData& OtherVertex = Other.Vertices[i];
			Vertices.Add(EVertexData());
			Vertices[Vertices.Length() - 1u] = OtherVertex;
		}
		for (UINT i = 0u, n = Other.Submeshes.Length(); i < n; i++)
		{
			const ESubmeshData& OtherSubmesh = Other.Submeshes[i];
			Submeshes.Add(ESubmeshData());
			Submeshes[Submeshes.Length() - 1u] = OtherSubmesh;
		}
	}
	EMesh::~EMesh()
	{
		Indices.Release();
		for (UINT Index = 0u, Length = Vertices.Length(); Index < Length; Index++)
		{
			Vertices[Index].Release();
		}
	}
	EMesh& EMesh::operator=(const EMesh& Other)
	{
		CopyBaseDataFromOtherInternal(Other);
		return (*this);
	}
	BOOL EMesh::IsResourceValid()const
	{
		if (Vertices.Length() > 0u)
		{
			BOOL Result = FALSE;
			for (UINT i = 0u, n = Vertices.Length(); i < n; i++)
			{
				const EVertexData& TempVertexData = Vertices[i];
				Result = Result || ((TempVertexData.ElementNum > 3u) && (TempVertexData.Stride > 0u) && (!!(TempVertexData.Datas)));
			}
			return (Result && ((Indices.ElementNum > 0u) && (Indices.Stride > 0u) && (!!(Indices.Indices))));
		}
		return FALSE;
	}
	BOOL EMesh::InitResource()
	{
		// Mesh resource must init by mesh manager.
		return TRUE;
	}
	void EMesh::ReleaseResource()
	{
		Indices.Release();
		for (UINT Index = 0u, Length = Vertices.Length(); Index < Length; Index++)
		{
			Vertices[Index].Release();
		}
		Vertices.Clear();
		Submeshes.Clear();
		VertexLayout = 0u;
	}
	BOOL EMesh::CheckVertexLayoutPartExist(EVertexLayoutType InLayoutType, UINT InPartIndex)const
	{
		if ((InLayoutType & EVertexLayoutType::MESH_INDEX_FULL) != 0u)
		{
			return ((VertexLayout & EVertexLayoutType::MESH_INDEX_FULL) != 0u);
		}
		else if ((InLayoutType & EVertexLayoutType::MESH_INDEX_HALF) != 0u)
		{
			return ((VertexLayout & EVertexLayoutType::MESH_INDEX_HALF) != 0u);
		}
		else
		{
			if (InPartIndex >= EMesh::MeshVertexLayoutPartMaxNum)
			{
				return FALSE;
			}
			return ((VertexLayout & (InLayoutType << InPartIndex)) != 0u);
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
	const EMesh::EIndexPart& EMesh::GetIndices()const
	{
		return Indices;
	}
	const EMesh::EVertexPart& EMesh::GetVertices()const
	{
		return Vertices;
	}
	const EMesh::ESubmeshPart& EMesh::GetSubmeshes()const
	{
		return Submeshes;
	}
	void EMesh::SetMeshName(const EString& InMeshName)
	{
		MeshName = InMeshName;
	}
	void EMesh::SetBoxToBoundAABB(const Vector3& InOrigin, const Vector3& InExtent)
	{
		BoundAABB.AABBMin = InOrigin - InExtent;
		BoundAABB.AABBMax = InOrigin + InExtent;
	}
	void EMesh::SetBoundAABB(const Vector3& InMin, const Vector3& InMax)
	{
		BoundAABB.AABBMin = InMin;
		BoundAABB.AABBMax = InMax;
	}
	BOOL EMesh::AddIndexElement(EIndexData* InIndexData)
	{
		if ((!InIndexData) || (InIndexData->PartType == 0u) || (InIndexData->ElementNum < 3u) ||
			(!(InIndexData->Indices)) || ((InIndexData->Stride != 2u) || (InIndexData->Stride != 4u)))
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
	BOOL EMesh::RemoveIndexElement()
	{
		if (Indices.PartType != 0u)
		{
			EVertexLayoutType OldPartType = static_cast<EVertexLayoutType>(Indices.PartType);
#ifdef _DEBUG_MODE
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
#ifdef _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing indices of mesh but do not exist."));
#endif
		return FALSE;
	}
	BOOL EMesh::GetIndexElement(const EIndexData*& OutIndexData)const
	{
		if ((Indices.PartType != 0u)
#ifdef _EDITOR_ONLY
			&& (Indices.ElementNum > 2u) && (!!(Indices.Indices))
#endif
			)
		{
#ifdef _EDITOR_ONLY
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
#ifdef _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to getting indices of mesh but do not exist."));
#endif
		return FALSE;
	}
	BOOL EMesh::AddVertexElement(EVertexData* InVertexData, UINT* OutLayoutIndex)
	{
		if ((!InVertexData) || (InVertexData->PartType == 0u) || (InVertexData->ElementNum < 3u) ||
			(!(InVertexData->Datas)) || ((InVertexData->Stride % 4u) != 0u))
		{
#ifdef _DEBUG_MODE
			PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add vertex descriptor check failed."));
			if (InVertexData && (InVertexData->Stride % 4u) != 0u)
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Mesh add vertex check failed(stride is not 4 bytes alignment)."));
			}
#endif
			return FALSE;
		}
		const EVertexLayoutType InLayoutType = static_cast<EVertexLayoutType>(InVertexData->PartType);
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
		UINT FindIndex = 0u;
		if (Vertices.Length() > 0u)
		{
			FindIndex = EMesh::MeshVertexLayoutPartMaxNum;
			for (UINT Index = 0u; Index < EMesh::MeshVertexLayoutPartMaxNum; Index++)
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
			const UINT NewElementIndex = Vertices.Length();
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
	BOOL EMesh::RemoveVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex)
	{
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
		if ((Vertices.Length() > 0u) && CheckVertexLayoutPartExist(InLayoutType, InLayoutIndex))
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
			const UINT DeleteLayoutType = InLayoutType << InLayoutIndex;
			for (UINT Index = 0u, Length = Vertices.Length(); Index < Length; Index++)
			{
				if (Vertices[Index].PartType == DeleteLayoutType)
				{
					Vertices[Index].Release();
					if (Index != (Length - 1u))
					{
						MoveVertexData(Vertices[Index], Vertices[Length - 1u]);
					}
					Vertices.Pop();
					SetVertexLayoutPartExistInternal(InLayoutType, InLayoutIndex, FALSE);
					return TRUE;
				}
			}
		}
#ifdef _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to remove not exist vertex."));
#endif
		return FALSE;
	}
	BOOL EMesh::GetVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex, const EVertexData*& OutVertexData)const
	{
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
		if ((Vertices.Length() > 0u) && CheckVertexLayoutPartExist(InLayoutType, InLayoutIndex))
		{
			const UINT DeleteLayoutType = InLayoutType << InLayoutIndex;
			for (UINT Index = 0u, Length = Vertices.Length(); Index < Length; Index++)
			{
				if (Vertices[Index].PartType == DeleteLayoutType)
				{
					OutVertexData = (const EVertexData*)(&(Vertices[Index]));
					return TRUE;
				}
			}
		}
#ifdef _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist vertex."));
#endif
		return FALSE;
	}
	BOOL EMesh::AddSubmesh(const ESubmeshData* InSubmeshData, UINT* OutSubmeshIndex)
	{
		if ((!InSubmeshData) || (InSubmeshData->VertexNum < 3u) || (InSubmeshData->IndexNum < 3u))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to add invalid submesh."));
			return FALSE;
		}
		const UINT NewIndex = Submeshes.Length();
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
	BOOL EMesh::RemoveSubmesh(UINT InSubmeshIndex)
	{
		if (UINT TempSubmeshNum = Submeshes.Length(); (TempSubmeshNum == 0u) || (InSubmeshIndex >= Submeshes.Length()))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to remove not exist submesh."));
			return FALSE;
		}
		Submeshes.RemoveAt(InSubmeshIndex);
		return TRUE;
	}
	BOOL EMesh::GetSubmesh(UINT InSubmeshIndex, const ESubmeshData*& OutSubmeshData)const
	{
		if (UINT TempSubmeshNum = Submeshes.Length(); (TempSubmeshNum == 0u) || (InSubmeshIndex >= Submeshes.Length()))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist submesh."));
			return FALSE;
		}
		OutSubmeshData = (const ESubmeshData*)(&(Submeshes[InSubmeshIndex]));
		return TRUE;
	}
	void EMesh::CopyBaseDataFromOtherInternal(const EMesh& Other)
	{
		MeshName		= Other.MeshName;
		BoundAABB		= Other.BoundAABB;
		VertexLayout	= Other.VertexLayout;

		Indices.Release();
		Indices = Other.Indices;

		if (Vertices.Length() > 0u)
		{
			for (UINT i = 0u, n = Vertices.Length(); i < n; i++)
			{
				Vertices[i].Release();
			}
			Vertices.Clear();
		}
		for (UINT i = 0u, n = Other.Vertices.Length(); i < n; i++)
		{
			const EVertexData& OtherVertex = Other.Vertices[i];
			Vertices.Add(EVertexData());
			Vertices[Vertices.Length() - 1u] = OtherVertex;
		}

		if (Submeshes.Length() > 0u)
		{
			Submeshes.Clear();
		}
		for (UINT i = 0u, n = Other.Submeshes.Length(); i < n; i++)
		{
			const ESubmeshData& OtherSubmesh = Other.Submeshes[i];
			Submeshes.Add(ESubmeshData());
			Submeshes[Submeshes.Length() - 1u] = OtherSubmesh;
		}
	}
	void EMesh::SetVertexLayoutPartExistInternal(EVertexLayoutType InLayoutType, UINT InPartIndex, BOOL InIsExist, BOOL* OutIsAlreadyExist)
	{
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
		: EMesh(ENGINE_DEFAULT_NAME)
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
	BOOL EStaticMesh::IsResourceValid()const
	{
		return (EMesh::IsResourceValid());
	}
	BOOL EStaticMesh::InitResource()
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
		: EMesh(ENGINE_DEFAULT_NAME)
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
		for (UINT i = 0u, n = Other.Skins.Length(); i < n; i++)
		{
			const ESkinData& OtherSkin = Other.Skins[i];
			Skins.Add(ESkinData());
			Skins[Skins.Length() - 1u] = OtherSkin;
		}
	}
	ESkinnedMesh::ESkinnedMesh(const EString& InMeshName)
		: EMesh(InMeshName), EffectBoneNum(0u)
	{
	}
	ESkinnedMesh::~ESkinnedMesh()
	{
		for (UINT Index = 0u, Length = Skins.Length(); Index < Length; Index++)
		{
			Skins[Index].Release();
		}
	}
	BOOL ESkinnedMesh::IsResourceValid()const
	{
		if (Skins.Length() > 0u)
		{
			BOOL Result = FALSE;
			for (UINT Index = 0u, Length = Skins.Length(); Index < Length; Index++)
			{
				Result = Result || ((Skins[Index].ElementNum > 3u) && (Skins[Index].Stride > 0u));
			}
			return (Result && (EMesh::IsResourceValid()));
		}
		return FALSE;
	}
	BOOL ESkinnedMesh::InitResource()
	{
		// Mesh resource must init by mesh manager.
		return (EMesh::InitResource());
	}
	void ESkinnedMesh::ReleaseResource()
	{
		for (UINT Index = 0u, Length = Skins.Length(); Index < Length; Index++)
		{
			Skins[Index].Release();
		}
		EMesh::ReleaseResource();
	}
	ESkinnedMesh& ESkinnedMesh::operator=(const ESkinnedMesh& Other)
	{
		CopyBaseDataFromOtherInternal(Other);

		if (BindPoseValue.Length() > 0u)
		{
			BindPoseValue.Clear();
		}
		for (auto It = Other.BindPoseValue.Begin(); It != Other.BindPoseValue.End(); It++)
		{
			BindPoseValue.Add(It->first, It->second);
		}

		if (BindPoseIndex.Length() > 0u)
		{
			BindPoseIndex.Clear();
		}
		for (auto It = Other.BindPoseIndex.Begin(); It != Other.BindPoseIndex.End(); It++)
		{
			BindPoseIndex.Add(It->first, It->second);
		}

		EffectBoneNum = Other.EffectBoneNum;

		if (Skins.Length() > 0u)
		{
			for (UINT i = 0u, n = Skins.Length(); i < n; i++)
			{
				Skins[i].Release();
			}
			Skins.Clear();
		}
		for (UINT i = 0u, n = Other.Skins.Length(); i < n; i++)
		{
			const ESkinData& OtherSkin = Other.Skins[i];
			Skins.Add(ESkinData());
			Skins[Skins.Length() - 1u] = OtherSkin;
		}

		return (*this);
	}
	BOOL ESkinnedMesh::AddBindPose(const EString& InBoneName, const Matrix4x4& InBindPose)
	{
		BOOL Result = FALSE;
		if (!(BindPoseValue.ContainsKey(InBoneName)))
		{
			BindPoseValue.Add(InBoneName, InBindPose);
			Result = TRUE;
		}
		return Result;
	}
	BOOL ESkinnedMesh::RemoveBindPose(const EString& InBoneName)
	{
		BOOL Result = FALSE;
		if (BindPoseValue.ContainsKey(InBoneName))
		{
			BindPoseValue.Remove(InBoneName);
			Result = TRUE;
		}
		return Result;
	}
	void ESkinnedMesh::ClearBindPose()
	{
		if (BindPoseValue.Length() > 0u)
		{
			BindPoseValue.Clear();
		}
		if (BindPoseIndex.Length() > 0u)
		{
			BindPoseIndex.Clear();
		}
	}
	void ESkinnedMesh::GenerateBindPoseIndex()
	{
		if (BindPoseIndex.Length() > 0u)
		{
			BindPoseIndex.Clear();
		}
		EffectBoneNum = 0u;
		if (BindPoseValue.Length() == 0u)
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
		EffectBoneNum = TempIndex;
	}
	const ESkinnedMesh::EBindPoseValue& ESkinnedMesh::GetBindPoseValue()const
	{
		return BindPoseValue;
	}
	const ESkinnedMesh::EBindPoseIndex& ESkinnedMesh::GetBindPoseIndex()const
	{
		return BindPoseIndex;
	}
	USHORT ESkinnedMesh::GetEffectBoneNum()const
	{
		return EffectBoneNum;
	}
	BOOL ESkinnedMesh::AddSkinElement(ESkinData* InSkinData, UINT* OutLayoutIndex)
	{
		if ((!InSkinData) || (InSkinData->ElementNum < 3u) || (InSkinData->PartType != EVertexLayoutType::MESH_SKIN) || ((InSkinData->Stride % 4u) != 0u) || (!(InSkinData->Indices)) || (!(InSkinData->Weights)))
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to add invalid skin data."));
			return FALSE;
		}
		if (Skins.Length() >= EMesh::MeshVertexLayoutPartMaxNum)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Skin mesh can not contains skin datas over EMesh::MeshVertexLayoutPartMaxNum."));
			return FALSE;
		}
		for (UINT FindIndex = 0u; FindIndex < EMesh::MeshVertexLayoutPartMaxNum; FindIndex++)
		{
			if (!CheckVertexLayoutPartExist(EVertexLayoutType::MESH_SKIN, FindIndex))
			{
				SetVertexLayoutPartExistInternal(EVertexLayoutType::MESH_SKIN, FindIndex, TRUE);
				const UINT NewIndex = Skins.Length();
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
	BOOL ESkinnedMesh::RemoveSkinElement(UINT InLayoutIndex)
	{
		if ((Skins.Length() > 0u) && CheckVertexLayoutPartExist(EVertexLayoutType::MESH_SKIN, InLayoutIndex))
		{
			auto MoveSkinData = [](ESkinData& InA, ESkinData& InB)->void
			{
				ESkinData TempData;
				TempData.PartType = InA.PartType;
				TempData.ElementNum = InA.ElementNum;
				TempData.Stride = InA.Stride;
				TempData.Indices = InA.Indices;
				TempData.Weights = InA.Weights;
				InA.PartType = InB.PartType;
				InA.ElementNum = InB.ElementNum;
				InA.Stride = InB.Stride;
				InA.Indices = InB.Indices;
				InA.Weights = InB.Weights;
				InB.PartType = TempData.PartType;
				InB.ElementNum = TempData.ElementNum;
				InB.Stride = TempData.Stride;
				InB.Indices = TempData.Indices;
				InB.Weights = TempData.Weights;
			};
			const UINT DeleteLayoutType = EVertexLayoutType::MESH_SKIN << InLayoutIndex;
			for (UINT Index = 0u, Length = Skins.Length(); Index < Length; Index++)
			{
				if (Skins[Index].PartType == DeleteLayoutType)
				{
					Skins[Index].Release();
					if (Index != (Length - 1u))
					{
						MoveSkinData(Skins[Index], Skins[Length - 1u]);
					}
					Skins.Pop();
					SetVertexLayoutPartExistInternal(EVertexLayoutType::MESH_SKIN, InLayoutIndex, FALSE);
					return TRUE;
				}
			}
		}
#ifdef _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist skin data."));
#endif
		return FALSE;
	}
	BOOL ESkinnedMesh::GetSkinElement(UINT InLayoutIndex, const ESkinData*& OutSkinData)const
	{
		if ((Skins.Length() > 0u) && CheckVertexLayoutPartExist(EVertexLayoutType::MESH_SKIN, InLayoutIndex))
		{
			const UINT DeleteLayoutType = EVertexLayoutType::MESH_SKIN << InLayoutIndex;
			for (UINT Index = 0u, Length = Skins.Length(); Index < Length; Index++)
			{
				if (Skins[Index].PartType == DeleteLayoutType)
				{
					OutSkinData = (const ESkinData*)(&(Skins[Index]));
					return TRUE;
				}
			}
		}
#ifdef _DEBUG_MODE
		PE_FAILED((ENGINE_ASSET_ERROR), ("Try to get not exist skin data."));
#endif
		return FALSE;
	}
	const ESkinnedMesh::ESkinPart& ESkinnedMesh::GetSkins()const
	{
		return Skins;
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
	BOOL EMeshRenderResource::IsRenderResourceValid()const
	{
		if ((IndexRenderResource.IsRenderResourceValid()) && (VertexRenderResources.Length() > 0u))
		{
			BOOL Result = TRUE;
			for (UINT Index = 0u, Length = VertexRenderResources.Length(); Index < Length; Index++)
			{
				Result = Result && (VertexRenderResources[Index].IsRenderResourceValid());
			}
			return Result;
		}
		return FALSE;
	}
	void EMeshRenderResource::ReleaseRenderResource()
	{
		IndexRenderResource.ReleaseRenderResource();
		if (VertexRenderResources.Length() > 0u)
		{
			for (UINT Index = 0u, Length = VertexRenderResources.Length(); Index < Length; Index++)
			{
				VertexRenderResources[Index].ReleaseRenderResource();
			}
			VertexRenderResources.Clear();
		}
	}
	BOOL EMeshRenderResource::CreateIndexRenderResourceInternal(const EMesh* InMesh)
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		if (const EIndexData* OutIndexData = nullptr; InMesh->GetIndexElement(OutIndexData))
		{
			if (IndexRenderResource.IsRenderResourceValid())
			{
				IndexRenderResource.ReleaseRenderResource();
			}
			UINT32 IndexType = static_cast<UINT32>(OutIndexData->PartType);
			if ((IndexType & EVertexLayoutType::MESH_INDEX_FULL) != 0u)
			{
				RSubresourceDataDesc SubresourceDataDesc;
				SubresourceDataDesc.pSysMem = OutIndexData->Indices;
				if (!(RenderDevice->CreateBuffer(
					(IndexRenderResource.Buffer),
					RBufferDesc((4u * OutIndexData->ElementNum), RBindFlagType::BIND_INDEX_BUFFER, 4u),
					(&SubresourceDataDesc))))
				{
#if _EDITOR_ONLY
					EString ErrorInfo("Create mesh [name = ");
					ErrorInfo += InMesh->GetDebugName();
					ErrorInfo += "] index buffer [num = ";
					ErrorInfo += ToString(OutIndexData->ElementNum);
					ErrorInfo += "] [stride = 32bits] failed.";
					PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
#endif
					return FALSE;
				}
			}
			else if ((IndexType & EVertexLayoutType::MESH_INDEX_HALF) != 0u)
			{
				UINT16* TempIndices = new UINT16[OutIndexData->ElementNum];
				for (UINT i = 0u, n = OutIndexData->ElementNum; i < n; i++)
				{
					Check((ENGINE_ASSET_ERROR), ("Mesh index type check failed, half index element num must lower than 65535u."), ((OutIndexData->Indices[i]) < 0xffffu));
					TempIndices[i] = static_cast<UINT16>(OutIndexData->Indices[i]);
				}
				RSubresourceDataDesc SubresourceDataDesc;
				SubresourceDataDesc.pSysMem = TempIndices;
				if (!(RenderDevice->CreateBuffer(
					(IndexRenderResource.Buffer),
					RBufferDesc((4u * OutIndexData->ElementNum), RBindFlagType::BIND_INDEX_BUFFER, 4u),
					(&SubresourceDataDesc))))
				{
#if _EDITOR_ONLY
					EString ErrorInfo("Create mesh [name = ");
					ErrorInfo += InMesh->GetDebugName();
					ErrorInfo += "] index buffer [num = ";
					ErrorInfo += ToString(OutIndexData->ElementNum);
					ErrorInfo += "] [stride = 16bits] failed.";
					PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
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
	BOOL EMeshRenderResource::CreateVertexRenderResourceInternal(const EMesh* InMesh)
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		const EMesh::EVertexPart& MeshVertices = InMesh->GetVertices();
		if (MeshVertices.Length() == 0u)
		{
#if _EDITOR_ONLY
			EString ErrorInfo("Create vertex render resource but mesh [name = ");
			ErrorInfo += InMesh->GetDebugName();
			ErrorInfo += "] does not contain vertex datas";
			PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
#endif
			return FALSE;
		}
		if (VertexRenderResources.Length() > 0u)
		{
			for (UINT i = 0u, n = VertexRenderResources.Length(); i < n; i++)
			{
				VertexRenderResources[i].ReleaseRenderResource();
			}
			VertexRenderResources.Clear();
		}
		BOOL FullyCreated = TRUE;
		for (UINT MeshVertexIndex = 0u, MeshVertexNum = MeshVertices.Length(); MeshVertexIndex < MeshVertexNum; MeshVertexIndex++)
		{
			const EVertexData& MeshVertexData = MeshVertices[MeshVertexIndex];
			RSubresourceDataDesc SubresourceDataDesc;
			SubresourceDataDesc.pSysMem = MeshVertexData.Datas;
			RBufferResource NewVertexRenderResource;
			if (!(RenderDevice->CreateBuffer(
				(NewVertexRenderResource.Buffer),
				RBufferDesc((MeshVertexData.Stride * MeshVertexData.ElementNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
				(&SubresourceDataDesc))))
			{
#if _EDITOR_ONLY
				EString ErrorInfo("Create mesh [name = ");
				ErrorInfo += InMesh->GetDebugName();
				ErrorInfo += "] vertex buffer [num = ";
				ErrorInfo += ToString(MeshVertexData.ElementNum);
				ErrorInfo += "] [stride = ";
				ErrorInfo += ToString(MeshVertexData.Stride);
				ErrorInfo += "] failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
#endif
				FullyCreated = FALSE;
			}
			VertexRenderResources.Add(NewVertexRenderResource);
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
		if (Other->VertexRenderResources.Length() > 0u)
		{
			for (UINT i = 0u, n = Other->VertexRenderResources.Length(); i < n; i++)
			{
				VertexRenderResources.Add(Other->VertexRenderResources[i]);
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
	BOOL EStaticMeshRenderResource::IsRenderResourceValid()const
	{
		return ((!!StaticMesh) && (EMeshRenderResource::IsRenderResourceValid()));
	}
	BOOL EStaticMeshRenderResource::InitRenderResource()
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
		: Skeleton(nullptr), SkinnedMesh(nullptr)
	{
	}
	ESkinnedMeshRenderResource::ESkinnedMeshRenderResource(const ESkeleton* InSkeleton, ESkinnedMesh* InMesh)
		: Skeleton(InSkeleton), SkinnedMesh(InMesh)
	{
	}
	ESkinnedMeshRenderResource::ESkinnedMeshRenderResource(const ESkinnedMeshRenderResource& Other)
		: EMeshRenderResource(Other), Skeleton(Other.Skeleton), SkinnedMesh(Other.SkinnedMesh)
	{

	}
	ESkinnedMeshRenderResource::~ESkinnedMeshRenderResource()
	{
		ReleaseRenderResource();
	}
	ESkinnedMeshRenderResource& ESkinnedMeshRenderResource::operator=(const ESkinnedMeshRenderResource& Other)
	{
		ReleaseRenderResource();
		CopyRenderResourcesInternal(&Other);
		Skeleton		= Other.Skeleton;
		SkinnedMesh		= Other.SkinnedMesh;
		return (*this);
	}
	BOOL ESkinnedMeshRenderResource::IsRenderResourceValid()const
	{
		if (SkinRenderResources.Length() > 0u)
		{
			BOOL Result = TRUE;
			for (UINT i = 0u, n = SkinRenderResources.Length(); i < n; i++)
			{
				Result = Result && SkinRenderResources[i].IsRenderResourceValid();
			}
			return (Result && (!!Skeleton) && (!!SkinnedMesh) && (SkeletonRenderResource.IsRenderResourceValid()) && (EMeshRenderResource::IsRenderResourceValid()));
		}
		return FALSE;
	}
	BOOL ESkinnedMeshRenderResource::InitRenderResource()
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

			return TRUE;
		}
		return FALSE;
	}
	void ESkinnedMeshRenderResource::ReleaseRenderResource()
	{
		Skeleton = nullptr;
		SkinnedMesh = nullptr;
		SkeletonRenderResource.ReleaseRenderResource();
		EMeshRenderResource::ReleaseRenderResource();
	}
	BOOL ESkinnedMeshRenderResource::CreateSkeletonRenderResourceInternal()
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		const EMesh::EVertexPart& MeshVertices = SkinnedMesh->GetSkinElement();
		if (MeshVertices.Length() == 0u)
		{
#if _EDITOR_ONLY
			EString ErrorInfo("Create vertex render resource but mesh [name = ");
			ErrorInfo += InMesh->GetDebugName();
			ErrorInfo += "] does not contain vertex datas";
			PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
#endif
			return FALSE;
		}
		if (VertexRenderResources.Length() > 0u)
		{
			for (UINT i = 0u, n = VertexRenderResources.Length(); i < n; i++)
			{
				VertexRenderResources[i].ReleaseRenderResource();
			}
			VertexRenderResources.Clear();
		}
		BOOL FullyCreated = TRUE;
		for (UINT MeshVertexIndex = 0u, MeshVertexNum = MeshVertices.Length(); MeshVertexIndex < MeshVertexNum; MeshVertexIndex++)
		{
			const EVertexData& MeshVertexData = MeshVertices[MeshVertexIndex];
			RSubresourceDataDesc SubresourceDataDesc;
			SubresourceDataDesc.pSysMem = MeshVertexData.Datas;
			RBufferResource NewVertexRenderResource;
			if (!(RenderDevice->CreateBuffer(
				(NewVertexRenderResource.Buffer),
				RBufferDesc((MeshVertexData.Stride * MeshVertexData.ElementNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
				(&SubresourceDataDesc))))
			{
#if _EDITOR_ONLY
				EString ErrorInfo("Create mesh [name = ");
				ErrorInfo += InMesh->GetDebugName();
				ErrorInfo += "] vertex buffer [num = ";
				ErrorInfo += ToString(MeshVertexData.ElementNum);
				ErrorInfo += "] [stride = ";
				ErrorInfo += ToString(MeshVertexData.Stride);
				ErrorInfo += "] failed.";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
#endif
				FullyCreated = FALSE;
			}
			VertexRenderResources.Add(NewVertexRenderResource);
		}
		return FullyCreated;
	}
	void ESkinnedMeshRenderResource::CopySkinRenderResourcesInternal(const ESkinnedMeshRenderResource* Other)
	{
		if ((!Other) || (!(Other->IsRenderResourceValid())))
		{
			return;
		}
		if (Other->SkinRenderResources.Length() > 0u)
		{
			for (UINT i = 0u, n = Other->SkinRenderResources.Length(); i < n; i++)
			{
				SkinRenderResources.Add(Other->SkinRenderResources[i]);
			}
		}
	}

	EStaticMeshAsset::EStaticMeshAsset(
		const EString& InMeshPath
#ifdef _EDITOR_ONLY
		, const EString& InDebugName
#endif
	)
		: TMeshBaseAsset<EMeshType::MESH_TYPE_STATIC, EStaticMesh, EMeshRenderResource>(InMeshPath
#ifdef _EDITOR_ONLY
			, InDebugName
#endif
		)
	{
	}
	EStaticMeshAsset::~EStaticMeshAsset()
	{
	}
	BOOL EStaticMeshAsset::InitResource()
	{
		if (IsInitialized())
		{
#ifdef _EDITOR_ONLY
			{
				EString ErrorInfo = EString("Static mesh name=[") + DebugName + "] path = [" + MeshPath + "] has been Initialized.";
				PE_FAILED((ENGINE_ASSET_ERROR), (ErrorInfo));
			}
#endif
			return TRUE;
		}
		if (CreateRenderResourceInternal(
			[this](EStaticMesh* InResource)->EMeshRenderResource*
			{
				return (this->CreateMeshResource(InResource));
			},
			FALSE))
		{
			bIsInitialized = TRUE;
			return TRUE;
		}
		return FALSE;
	}
	EStaticMeshRenderResource* EStaticMeshAsset::CreateMeshResource(EStaticMesh* InResource)
	{

		return nullptr;
	}

};