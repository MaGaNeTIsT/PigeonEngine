#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{
	// 32 bit number                    31 30| 29    26| 25    22| 21    18| 17    14| 13    10| 9     6 | 5     2 | 1 0
	// Vertex layout by bit (32 bits) :  0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0
	// Vertex layout layer :           NotUse|   Skin  |Bitangent|  Color  | Tangent |  Normal | TexCoord|  Vertex | Index
	enum EMeshVertexLayoutType : UINT32
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
	extern UINT GetMeshVertexLayoutTypeStartBitIndex(EMeshVertexLayoutType InType);

	typedef	TArray<UINT32>		EMeshFullIndexPart;
	typedef	TArray<UINT16>		EMeshHalfIndexPart;
	typedef	TArray<Vector4>		EMeshVertexPart;
	typedef	TArray<Vector4>		EMeshNormalPart;
	typedef	TArray<Vector4>		EMeshTangentPart;
	typedef	TArray<Vector4>		EMeshBitangentPart;
	typedef	TArray<Color4>		EMeshColorPart;
	struct EMeshTextureCoordData
	{
		EString			Name;
		TArray<Vector2>	TextureCoords;
	};
	typedef	EMeshTextureCoordData EMeshTextureCoordPart;
	struct EMeshSkinData
	{
		UINT			EffectBoneNum;
		TArray<Vector4>	Indices;
		TArray<Vector4>	Weights;
	};
	typedef	EMeshSkinData EMeshSkinPart;
	struct EMeshSubmeshData
	{
		UINT StartVertex;
		UINT CountVertices;
		UINT StartIndex;
		UINT CountIndices;
	};
	typedef	TArray<EMeshSubmeshData> EMeshSubmeshPart;

	class EMesh : public EObjectBase
	{
	public:
		const static UINT MeshVertexLayoutPartMaxNum = 4;
	public:
		EMesh(const EString& InMeshName);
		BOOL CheckVertexLayoutPartExist(EMeshVertexLayoutType InLayoutType, UINT InPartIndex)const;
	protected:
		void SetVertexLayoutPartExistInternal(EMeshVertexLayoutType InLayoutType, UINT InPartIndex, BOOL InIsExist, BOOL& OutIsAlreadyExist);
	protected:
		EString					MeshName;
		UINT32					VertexLayout;
		EBoundAABB				BoundAABB;
		EMeshSubmeshPart		SubmeshPart;
		EMeshFullIndexPart		FullIndexPart;
		EMeshHalfIndexPart		HalfIndexPart;
		EMeshVertexPart			VertexPart;
		EMeshNormalPart			NormalPart[4];
		EMeshTangentPart		TangentPart[4];
		EMeshBitangentPart		BitangentPart[4];
		EMeshColorPart			ColorPart[4];
		EMeshTextureCoordPart	TextureCoordPart[4];
		EMeshSkinPart			SkinPart;
	public:
		EMesh() = delete;

		CLASS_REMOVE_COPY_BODY(EMesh)
	};

};