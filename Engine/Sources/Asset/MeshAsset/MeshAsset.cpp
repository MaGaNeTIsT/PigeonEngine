#include "MeshAsset.h"

namespace PigeonEngine
{

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
	}

	EMesh::EMesh(const EString& InMeshName)
		: MeshName(InMeshName)
		, VertexLayout(0u)
	{
#ifdef _EDITOR_ONLY
		DebugName = InMeshName;
#endif
	}
	EMesh::~EMesh()
	{
		Indices.Release();
		for (UINT Index = 0u, Length = Vertices.Length(); Index < Length; Index++)
		{
			Vertices[Index].Release();
		}
	}
	void EMesh::Release()
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
		if (InLayoutType == EVertexLayoutType::MESH_INDEX_FULL)
		{
			return ((VertexLayout & EVertexLayoutType::MESH_INDEX_FULL) > 0);
		}
		else if (InLayoutType == EVertexLayoutType::MESH_INDEX_HALF)
		{
			return ((VertexLayout & EVertexLayoutType::MESH_INDEX_HALF) > 0);
		}
		else
		{
			if (InPartIndex >= EMesh::MeshVertexLayoutPartMaxNum)
			{
				return FALSE;
			}
			return ((VertexLayout & (InLayoutType << InPartIndex)) > 0);
		}
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
	void EMesh::SetBoundAABB(const Vector3& InOrigin, const Vector3& InExtent)
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
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh add index descriptor check failed."));
			return FALSE;
		}
		if (Indices.PartType != 0u)
		{
			EVertexLayoutType OldPartType = static_cast<EVertexLayoutType>(Indices.PartType);
			if (((OldPartType == EVertexLayoutType::MESH_INDEX_HALF) && (Indices.Stride != 2u)) ||
				((OldPartType == EVertexLayoutType::MESH_INDEX_FULL) && (Indices.Stride != 4u)))
			{
				PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh [old] index check failed. Part type is not match with stride."));
				return FALSE;
			}
			Indices.Release();
			SetVertexLayoutPartExistInternal(OldPartType, 0u, FALSE);
		}
		EVertexLayoutType NewPartType = static_cast<EVertexLayoutType>(InIndexData->PartType);
		if (((NewPartType == EVertexLayoutType::MESH_INDEX_HALF) && (InIndexData->Stride != 2u)) ||
			((NewPartType == EVertexLayoutType::MESH_INDEX_FULL) && (InIndexData->Stride != 4u)))
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh [new] index check failed. Part type is not match with stride."));
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
			if (((OldPartType == EVertexLayoutType::MESH_INDEX_HALF) && (Indices.Stride != 2u)) ||
				((OldPartType == EVertexLayoutType::MESH_INDEX_FULL) && (Indices.Stride != 4u)))
			{
				PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Removed index check failed. Part type is not match with stride."));
			}
#endif
			Indices.Release();
			SetVertexLayoutPartExistInternal(OldPartType, 0u, FALSE);
			return TRUE;
		}
#ifdef _DEBUG_MODE
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to removing indices of mesh but do not exist."));
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
			if (((OldPartType == EVertexLayoutType::MESH_INDEX_HALF) && (Indices.Stride != 2u)) ||
				((OldPartType == EVertexLayoutType::MESH_INDEX_FULL) && (Indices.Stride != 4u)))
			{
				PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Get index check failed. Part type is not match with stride."));
			}
#endif
			OutIndexData = (const EIndexData*)(&Indices);
			return TRUE;
		}
#ifdef _DEBUG_MODE
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to getting indices of mesh but do not exist."));
#endif
		return FALSE;
	}
	BOOL EMesh::AddVertexElement(EVertexData* InVertexData, UINT* OutLayoutIndex)
	{
		if ((!InVertexData) || (InVertexData->PartType == 0u) || (InVertexData->ElementNum < 3u) ||
			(!(InVertexData->Datas)) || ((InVertexData->Stride % 4u) != 0u))
		{
#ifdef _DEBUG_MODE
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh add vertex descriptor check failed."));
			if ((InVertexData->Stride % 4u) != 0u)
			{
				PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh add vertex check failed(stride is not 4 bytes alignment)."));
			}
#endif
			return FALSE;
		}
		const EVertexLayoutType InLayoutType = static_cast<EVertexLayoutType>(InVertexData->PartType);
		if ((InLayoutType == EVertexLayoutType::MESH_INDEX_FULL) ||
			(InLayoutType == EVertexLayoutType::MESH_INDEX_HALF) ||
			(InLayoutType == EVertexLayoutType::MESH_SKIN))
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh add vertex layout type check failed(wrong type)."));
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
				PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh add vertex failed(same type layout is full)."));
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
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh add vertex failed(fill in layout index is already exist)."));
		return FALSE;
	}
	BOOL EMesh::RemoveVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex)
	{
		if ((InLayoutType == EVertexLayoutType::MESH_INDEX_FULL) ||
			(InLayoutType == EVertexLayoutType::MESH_INDEX_HALF) ||
			(InLayoutType == EVertexLayoutType::MESH_SKIN))
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh remove vertex layout type check failed(wrong type)."));
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
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to remove not exist vertex."));
#endif
		return FALSE;
	}
	BOOL EMesh::GetVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex, const EVertexData*& OutVertexData)const
	{
		if ((InLayoutType == EVertexLayoutType::MESH_INDEX_FULL) ||
			(InLayoutType == EVertexLayoutType::MESH_INDEX_HALF) ||
			(InLayoutType == EVertexLayoutType::MESH_SKIN))
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Mesh get vertex layout type check failed(wrong type)."));
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
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to get not exist vertex."));
#endif
		return FALSE;
	}
	BOOL EMesh::AddSubmesh(const ESubmeshData* InSubmeshData, UINT* OutSubmeshIndex)
	{
		if ((!InSubmeshData) || (InSubmeshData->VertexNum < 3u) || (InSubmeshData->IndexNum < 3u))
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to add invalid submesh."));
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
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to remove not exist submesh."));
			return FALSE;
		}
		Submeshes.RemoveAt(InSubmeshIndex);
		return TRUE;
	}
	BOOL EMesh::GetSubmesh(UINT InSubmeshIndex, const ESubmeshData*& OutSubmeshData)const
	{
		if (UINT TempSubmeshNum = Submeshes.Length(); (TempSubmeshNum == 0u) || (InSubmeshIndex >= Submeshes.Length()))
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to get not exist submesh."));
			return FALSE;
		}
		OutSubmeshData = (const ESubmeshData*)(&(Submeshes[InSubmeshIndex]));
		return TRUE;
	}
	void EMesh::SetVertexLayoutPartExistInternal(EVertexLayoutType InLayoutType, UINT InPartIndex, BOOL InIsExist, BOOL* OutIsAlreadyExist)
	{
		if (InLayoutType == EVertexLayoutType::MESH_INDEX_FULL)
		{
			if (OutIsAlreadyExist)
			{
				(*OutIsAlreadyExist) = (VertexLayout & EVertexLayoutType::MESH_INDEX_FULL) > 0;
			}
			VertexLayout = InIsExist ? (VertexLayout | EVertexLayoutType::MESH_INDEX_FULL) : (VertexLayout & (~(EVertexLayoutType::MESH_INDEX_FULL)));
		}
		else if (InLayoutType == EVertexLayoutType::MESH_INDEX_HALF)
		{
			if (OutIsAlreadyExist)
			{
				(*OutIsAlreadyExist) = (VertexLayout & EVertexLayoutType::MESH_INDEX_HALF) > 0;
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
				(*OutIsAlreadyExist) = (VertexLayout & (InLayoutType << InPartIndex)) > 0;
			}
			VertexLayout = InIsExist ? (VertexLayout | (InLayoutType << InPartIndex)) : (VertexLayout & (~(InLayoutType << InPartIndex)));
		}
	}

	EStaticMesh::EStaticMesh(const EString& InMeshName) : EMesh(InMeshName)
	{
	}
	EStaticMesh::~EStaticMesh()
	{
	}
	void EStaticMesh::Release()
	{
		EMesh::Release();
	}

	ESkinnedMesh::ESkinnedMesh(const EString& InMeshName) : EMesh(InMeshName)
	{
	}
	ESkinnedMesh::~ESkinnedMesh()
	{
		for (UINT Index = 0u, Length = Skins.Length(); Index < Length; Index++)
		{
			Skins[Index].Release();
		}
	}
	void ESkinnedMesh::Release()
	{
		for (UINT Index = 0u, Length = Skins.Length(); Index < Length; Index++)
		{
			Skins[Index].Release();
		}
		EMesh::Release();
	}
	BOOL ESkinnedMesh::AddSkinElement(ESkinData* InSkinData, UINT* OutLayoutIndex)
	{
		if ((!InSkinData) || (InSkinData->ElementNum < 3u) || (InSkinData->PartType != EVertexLayoutType::MESH_SKIN) || ((InSkinData->Stride % 4u) != 0u) || (!(InSkinData->Indices)) || (!(InSkinData->Weights)))
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to add invalid skin data."));
			return FALSE;
		}
		if (Skins.Length() >= EMesh::MeshVertexLayoutPartMaxNum)
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Skin mesh can not contains skin datas over EMesh::MeshVertexLayoutPartMaxNum."));
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
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Skin mesh can not contains skin datas over EMesh::MeshVertexLayoutPartMaxNum."));
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
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to get not exist skin data."));
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
		PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to get not exist skin data."));
#endif
		return FALSE;
	}

};