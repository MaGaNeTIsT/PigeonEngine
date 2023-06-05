#include "MeshAsset.h"

namespace PigeonEngine
{

	UINT GetMeshVertexLayoutTypeStartBitIndex(EMeshVertexLayoutType InType)
	{
		switch (InType)
		{
		case EMeshVertexLayoutType::MESH_INDEX_FULL:
			return 0u;
		case EMeshVertexLayoutType::MESH_INDEX_HALF:
			return 1u;
		case EMeshVertexLayoutType::MESH_VERTEX:
			return 2u;
		case EMeshVertexLayoutType::MESH_TEXTURECOORD:
			return 6u;
		case EMeshVertexLayoutType::MESH_NORMAL:
			return 10u;
		case EMeshVertexLayoutType::MESH_TANGENT:
			return 14u;
		case EMeshVertexLayoutType::MESH_COLOR:
			return 18u;
		case EMeshVertexLayoutType::MESH_BITANGENT:
			return 22u;
		case EMeshVertexLayoutType::MESH_SKIN:
			return 26u;
		}
	}

	EMesh::EMesh(const EString& InMeshName)
		: MeshName(InMeshName)
		, VertexLayout(0u)
	{
#ifdef _EDITOR_ONLY
		DebugName = ENGINE_DEFAULT_NAME;
#endif
	}
	BOOL EMesh::CheckVertexLayoutPartExist(EMeshVertexLayoutType InLayoutType, UINT InPartIndex)const
	{
		if (InLayoutType == EMeshVertexLayoutType::MESH_INDEX_FULL)
		{
			return ((VertexLayout & EMeshVertexLayoutType::MESH_INDEX_FULL) > 0);
		}
		else if (InLayoutType == EMeshVertexLayoutType::MESH_INDEX_HALF)
		{
			return ((VertexLayout & EMeshVertexLayoutType::MESH_INDEX_HALF) > 0);
		}
		else
		{
			if (InPartIndex > 3)
			{
				return FALSE;
			}
			return ((VertexLayout & (InLayoutType << InPartIndex)) > 0);
		}
	}
	void EMesh::SetVertexLayoutPartExistInternal(EMeshVertexLayoutType InLayoutType, UINT InPartIndex, BOOL InIsExist, BOOL& OutIsAlreadyExist)
	{
		if (InLayoutType == EMeshVertexLayoutType::MESH_INDEX_FULL)
		{
			OutIsAlreadyExist = (VertexLayout & EMeshVertexLayoutType::MESH_INDEX_FULL) > 0;
			VertexLayout = InIsExist ? (VertexLayout | EMeshVertexLayoutType::MESH_INDEX_FULL) : (VertexLayout & (~(EMeshVertexLayoutType::MESH_INDEX_FULL)));
		}
		else if (InLayoutType == EMeshVertexLayoutType::MESH_INDEX_HALF)
		{
			OutIsAlreadyExist = (VertexLayout & EMeshVertexLayoutType::MESH_INDEX_HALF) > 0;
			VertexLayout = InIsExist ? (VertexLayout | EMeshVertexLayoutType::MESH_INDEX_HALF) : (VertexLayout & (~(EMeshVertexLayoutType::MESH_INDEX_HALF)));
		}
		else
		{
			if (InPartIndex > 3)
			{
				OutIsAlreadyExist = FALSE;
				return;
			}
			OutIsAlreadyExist = (VertexLayout & (InLayoutType << InPartIndex)) > 0;
			VertexLayout = InIsExist ? (VertexLayout | (InLayoutType << InPartIndex)) : (VertexLayout & (~(InLayoutType << InPartIndex)));
		}
	}

};