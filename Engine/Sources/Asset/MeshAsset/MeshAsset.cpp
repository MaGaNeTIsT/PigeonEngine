#include "MeshAsset.h"

namespace PigeonEngine
{

	EMesh::EMesh(const EString& InMeshName)
		: MeshName(InMeshName)
	{
#ifdef _EDITOR_ONLY
		DebugName = ENGINE_DEFAULT_NAME;
#endif
	}
	BOOL EMesh::CheckVertexLayoutPartExist(EMeshVertexLayoutType InLayoutType, UINT InPartIndex)const
	{

	}
	BOOL EMesh::SetVertexLayoutPartIndexInternal(EMeshVertexLayoutType InLayoutType, UINT InPartIndex)
	{

	}

};