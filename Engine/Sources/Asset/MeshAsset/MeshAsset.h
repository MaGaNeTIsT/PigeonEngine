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

	struct EVertexDescriptor
	{
		EVertexDescriptor() noexcept : PartType(0u), ElementNum(0u), Stride(0u) {}
		constexpr EVertexDescriptor() noexcept : PartType(0u), ElementNum(0u), Stride(0u) {}

		UINT	PartType;
		UINT	ElementNum;
		UINT	Stride;
	};
	struct EIndexData : public EVertexDescriptor
	{
		EIndexData() noexcept : EVertexDescriptor(), Indices(nullptr) {}
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
		constexpr ESubmeshData() noexcept : StartVertex(0u), VertexNum(0u), StartIndex(0u), IndexNum(0u) {}

		UINT	StartVertex;
		UINT	VertexNum;
		UINT	StartIndex;
		UINT	IndexNum;
	};

	class EMesh : public EObjectBase
	{
	public:
		typedef	EIndexData				EIndexPart;
		typedef	TArray<EVertexData>		EVertexPart;
		typedef	TArray<ESubmeshData>	ESubmeshPart;
	public:
		const static UINT MeshVertexLayoutPartMaxNum = 4;
	public:
		EMesh(const EString& InMeshName);
	public:
		BOOL CheckVertexLayoutPartExist(EVertexLayoutType InLayoutType, UINT InPartIndex)const;
		const EBoundAABB& GetBoundAABB()const;
		UINT32 GetVertexLayout()const;
		const EIndexPart& GetIndices()const;
		const EVertexPart& GetVertices()const;
		const ESubmeshPart& GetSubmeshes()const;
	public:
		void SetBoundAABB(const Vector3& InOrigin, const Vector3& InExtent);
		void SetBoundAABB(const Vector3& InMin, const Vector3& InMax);
		BOOL AddIndexElement(EIndexData* InIndexData);
		BOOL RemoveIndexElement();
		BOOL GetIndexElement(const EIndexData*& OutIndexData)const;
		BOOL AddVertexElement(EVertexData* InVertexData, UINT& OutLayoutIndex);
		BOOL RemoveVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex);
		BOOL GetVertexElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex, const EVertexData*& OutVertexData)const;
		BOOL AddSubmesh(const ESubmeshData* InSubmeshData, UINT& OutSubmeshIndex);
		BOOL RemoveSubmesh(UINT InSubmeshIndex);
		BOOL GetSubmesh(UINT InSubmeshIndex, const ESubmeshData*& OutSubmeshData)const;
	protected:
		void SetVertexLayoutPartExistInternal(EVertexLayoutType InLayoutType, UINT InPartIndex, BOOL InIsExist, BOOL* OutIsAlreadyExist = nullptr);
	protected:
		EString					MeshName;
		EBoundAABB				BoundAABB;
		UINT32					VertexLayout;
		EIndexPart				Indices;
		EVertexPart				Vertices;
		ESubmeshPart			Submeshes;
	public:
		EMesh() = delete;

		CLASS_REMOVE_COPY_BODY(EMesh)
	};

	class EStaticMesh : public EMesh
	{
	public:
		EStaticMesh(const EString& InMeshName);

	public:
		EStaticMesh() = delete;

		CLASS_REMOVE_COPY_BODY(EStaticMesh)
	};

	class ESkinnedMesh : public EMesh
	{
	public:
		typedef	TArray<ESkinData>		ESkinPart;
	public:
		ESkinnedMesh(const EString& InMeshName);
	public:
		BOOL AddSkinElement(ESkinData* InSkinData, UINT& OutLayoutIndex);
		BOOL RemoveSkinElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex);
		BOOL GetSkinElement(EVertexLayoutType InLayoutType, UINT InLayoutIndex, const ESkinData*& OutSkinData)const;
	protected:
		ESkinPart	Skins;
	public:
		ESkinnedMesh() = delete;

		CLASS_REMOVE_COPY_BODY(ESkinnedMesh)
	};

};