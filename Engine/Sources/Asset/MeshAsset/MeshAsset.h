#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <RenderCommon.h>
#include <Base/DataStructure/Container/Array.h>

namespace PigeonEngine
{
	// 32 bit number                    31 30| 29    26| 25    22| 21    16| 17    12| 13    10| 9     6 | 5     2 | 1 0
	// Vertex layout by bit (32 bits) :  0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0
	// Vertex layout layer :           NotUse|   Bone  |Bitangent|  Color  | Tangent |  Normal |    UV   |  Vertex | Index
	enum EMeshVertexLayoutType : UINT32
	{
		MESH_INDEX_FULL	= (1 << 0),
		MESH_INDEX_HALF	= (1 << 1),
		MESH_VERTEX		= (1 << 2),
		MESH_UV			= (1 << 6),
		MESH_NORMAL		= (1 << 10),
		MESH_TANGENT	= (1 << 12),
		MESH_COLOR		= (1 << 16),
		MESH_BITANGENT	= (1 << 22),
		MESH_BONE		= (1 << 26)
	};

	typedef	TArray<UINT32>		EMeshFullIndexPart;
	typedef	TArray<UINT16>		EMeshHalfIndexPart;
	typedef	TArray<Vector4>		EMeshVertexPart;
	typedef	TArray<Vector2>		EMeshUVPart;
	typedef	TArray<Vector4>		EMeshNormalPart;
	typedef	TArray<Vector4>		EMeshTangentPart;
	typedef	TArray<Vector4>		EMeshBitangentPart;
	typedef	TArray<Color4>		EMeshColorPart;
	struct EMeshBoneData
	{
		Vector4	Indices;
		Vector4	Weights;
	};
	typedef	EMeshBoneData		EMeshBonePart;

	class EMesh : public EObjectBase
	{

		mVertices mNormals mTangents mBitangents mColors mTextureCoords indices mBones mAABB mTextureCoordsNames mName
	};

};