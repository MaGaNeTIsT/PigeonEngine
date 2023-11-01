#include "RenderSingletonObject.h"
#include <RenderResource.h>
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{
	static RBufferResource _GFullScreenTriangleVertexBuffer;
	static RBufferResource _GFullScreenTriangleIndexBuffer;

	static void CreateFullScreenTriangleVertexBuffer()
	{
		if (!(_GFullScreenTriangleVertexBuffer.IsRenderResourceValid()))
		{
			FLOAT FullScreenTriangleVertex[] =
			{
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f
			};
			RSubresourceDataDesc SubresourceDataDesc;
			SubresourceDataDesc.pSysMem = FullScreenTriangleVertex;
#if _EDITOR_ONLY
			BOOL32 Result =
#endif
				RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(_GFullScreenTriangleVertexBuffer.Buffer,
					RBufferDesc((sizeof(FLOAT) * 2u * 4u), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
					(&SubresourceDataDesc));
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create full screen triangle vertex buffer failed."));
			}
#endif
		}
	}
	static void CreateFullScreenTriangleIndexBuffer()
	{
		if (!(_GFullScreenTriangleIndexBuffer.IsRenderResourceValid()))
		{
			USHORT FullScreenTriangleIndex[] =
			{
				0, 1, 2,
				2, 1, 3
			};
			RSubresourceDataDesc SubresourceDataDesc;
			SubresourceDataDesc.pSysMem = FullScreenTriangleIndex;
#if _EDITOR_ONLY
			BOOL32 Result =
#endif
				RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(_GFullScreenTriangleIndexBuffer.Buffer,
					RBufferDesc((2u * 6u), RBindFlagType::BIND_INDEX_BUFFER, 2u),
					(&SubresourceDataDesc));
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create full screen triangle index buffer failed."));
			}
#endif
		}
	}

	RFullScreenTriangle::RFullScreenTriangle()
	{
		CreateFullScreenTriangleVertexBuffer();
		CreateFullScreenTriangleIndexBuffer();
	}
	RFullScreenTriangle::RFullScreenTriangle(const RFullScreenTriangle& Other)
	{
		CreateFullScreenTriangleVertexBuffer();
		CreateFullScreenTriangleIndexBuffer();
	}
	RFullScreenTriangle::~RFullScreenTriangle()
	{

	}
	RFullScreenTriangle& RFullScreenTriangle::operator=(const RFullScreenTriangle& Other)
	{
		CreateFullScreenTriangleVertexBuffer();
		CreateFullScreenTriangleIndexBuffer();
		return (*this);
	}
	UINT32 RFullScreenTriangle::GetIndexCount()const
	{
		return 6u;	//TODO
	}
	void RFullScreenTriangle::BindPrimitiveBuffers()
	{
#if _DEBUG_MODE
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check full screen triangle vertex buffer is invalid."), (_GFullScreenTriangleVertexBuffer.IsRenderResourceValid()));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check full screen triangle index buffer is invalid."), (_GFullScreenTriangleIndexBuffer.IsRenderResourceValid()));
#endif
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RenderDevice->SetIndexBuffer(_GFullScreenTriangleIndexBuffer.Buffer, 0u, RFormatType::FORMAT_R16_UINT);
		RenderDevice->SetVertexBuffer(_GFullScreenTriangleVertexBuffer.Buffer, (sizeof(FLOAT) * 2u), 0u, 0u);
	}

#if _EDITOR_ONLY
	RDebugWireframePrimitive::RDebugWireframePrimitive()
		: UseShortIndex(FALSE), IndexCount(0u)
	{
	}
	RDebugWireframePrimitive::RDebugWireframePrimitive(const RDebugWireframePrimitive& Other)
		: UseShortIndex(Other.UseShortIndex), IndexCount(Other.IndexCount), VertexBuffer(Other.VertexBuffer), IndexBuffer(Other.IndexBuffer)
	{
	}
	RDebugWireframePrimitive::~RDebugWireframePrimitive()
	{
	}
	RDebugWireframePrimitive& RDebugWireframePrimitive::operator=(const RDebugWireframePrimitive& Other)
	{
		UseShortIndex	= Other.UseShortIndex;
		IndexCount		= Other.IndexCount;
		VertexBuffer	= Other.VertexBuffer;
		IndexBuffer		= Other.IndexBuffer;
		return (*this);
	}
	BOOL32 RDebugWireframePrimitive::InitPrimitive(const Vector3* InVertexDatas, const UINT32 InVertexNum)
	{
		if (InVertexNum == 0u)
		{
			return FALSE;
		}

		const UINT32 LineCount = InVertexNum - 1u;

		if (VertexBuffer.IsRenderResourceValid() && IndexBuffer.IsRenderResourceValid())
		{
			if (IndexCount == (LineCount * 2u))
			{
				return TRUE;
			}
			VertexBuffer.ReleaseRenderResource();
			IndexBuffer.ReleaseRenderResource();
		}

		UseShortIndex = (InVertexNum <= 65536u) ? TRUE : FALSE;
		IndexCount = LineCount * 2u;

		UINT32* UintIndices = nullptr;
		USHORT* UshortIndices = nullptr;
		if (UseShortIndex)
		{
			UshortIndices = new USHORT[IndexCount];
			for (USHORT i = 0u, n = static_cast<USHORT>(LineCount); i < n; i++)
			{
				if ((static_cast<UINT32>(i * 2u + 1u)) < IndexCount)
				{
					UshortIndices[i * 2u + 0u] = i;
					UshortIndices[i * 2u + 1u] = i + 1u;
				}
			}
		}
		else
		{
			UintIndices = new UINT32[IndexCount];
			for (UINT32 i = 0u; i < LineCount; i++)
			{
				if ((i * 2u + 1u) < IndexCount)
				{
					UintIndices[i * 2u + 0u] = i;
					UintIndices[i * 2u + 1u] = i + 1u;
				}
			}
		}

		RSubresourceDataDesc SubresourceDataDesc;
		SubresourceDataDesc.pSysMem = InVertexDatas;

		BOOL32 Result = RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(VertexBuffer.Buffer,
			RBufferDesc((sizeof(Vector3) * InVertexNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
			(&SubresourceDataDesc));

		SubresourceDataDesc.pSysMem = UseShortIndex ? ((const void*)UshortIndices) : ((const void*)UintIndices);

		Result = Result && RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(IndexBuffer.Buffer,
			RBufferDesc(((UseShortIndex ? 2u : 4u) * IndexCount), RBindFlagType::BIND_INDEX_BUFFER, (UseShortIndex ? 2u : 4u)),
			(&SubresourceDataDesc));

		if (!!UintIndices)
		{
			delete[]UintIndices;
		}
		if (!!UshortIndices)
		{
			delete[]UshortIndices;
		}

		return Result;
	}
	BOOL32 RDebugWireframePrimitive::InitPrimitive(const Vector3* InVertexDatas, const UINT32 InVertexNum, const UINT32* InIndexDatas, const UINT32 InIndexNum)
	{
		if ((InVertexNum == 0u) || (InIndexNum == 0u) || ((InIndexNum & 0x1u) != 0u))
		{
			return FALSE;
		}

		if (VertexBuffer.IsRenderResourceValid() && IndexBuffer.IsRenderResourceValid())
		{
			if (IndexCount == InIndexNum)
			{
				return TRUE;
			}
			VertexBuffer.ReleaseRenderResource();
			IndexBuffer.ReleaseRenderResource();
		}

		UseShortIndex = (InVertexNum <= 65536u) ? TRUE : FALSE;
		IndexCount = InIndexNum;

		USHORT* UshortIndices = nullptr;
		if (UseShortIndex)
		{
			UshortIndices = new USHORT[IndexCount];
			for (UINT32 i = 0u; i < InIndexNum; i++)
			{
				UshortIndices[i] = static_cast<USHORT>(InIndexDatas[i]);
			}
		}

		RSubresourceDataDesc SubresourceDataDesc;
		SubresourceDataDesc.pSysMem = InVertexDatas;

		BOOL32 Result = RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(VertexBuffer.Buffer,
			RBufferDesc((sizeof(Vector3) * InVertexNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
			(&SubresourceDataDesc));

		SubresourceDataDesc.pSysMem = UseShortIndex ? ((const void*)UshortIndices) : ((const void*)InIndexDatas);

		Result = Result && RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(IndexBuffer.Buffer,
			RBufferDesc(((UseShortIndex ? 2u : 4u) * IndexCount), RBindFlagType::BIND_INDEX_BUFFER, (UseShortIndex ? 2u : 4u)),
			(&SubresourceDataDesc));

		if (!!UshortIndices)
		{
			delete[]UshortIndices;
		}

		return Result;
	}
	UINT32 RDebugWireframePrimitive::GetIndexCount()const
	{
		return IndexCount;
	}
	const RBufferResource& RDebugWireframePrimitive::GetPrimitiveBuffer()const
	{
		return VertexBuffer;
	}

	RDebugWireframePrimitiveManager::RDebugWireframePrimitiveManager()
	{
	}
	RDebugWireframePrimitiveManager::~RDebugWireframePrimitiveManager()
	{
	}
	void RDebugWireframePrimitiveManager::Initialize()
	{

	}
	void RDebugWireframePrimitiveManager::ShutDown()
	{

	}
	void RDebugWireframePrimitiveManager::CreateCustomPrimitive(RDebugWireframeType InType)
	{

	}
	void RDebugWireframePrimitiveManager::CreatePrimitiveInternal(RDebugWireframeType InType, const EString* InCustomName, const UINT32* InVectorNum, const Vector3* InVectorDatas)
	{
		const UINT32 TargetIndex = InType - 1u;
		BOOL32 Result = FALSE;
		switch (InType)
		{
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SINGLELINE:
			{
				const Vector3 PointListSingleLine[] =
				{
					Vector3(0.f, 0.5f, 0.f),
					Vector3(0.f, -0.5f, 0.f)
				};
				Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(PointListSingleLine, PE_ARRAYSIZE(PointListSingleLine));
			}
			break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_PLANE:
			{
				const Vector3 PointListPlane[] =
				{
					Vector3(-0.5f, 0.5f, 0.f),
					Vector3(0.5f, 0.5f, 0.f),
					Vector3(0.5f, -0.5f, 0.f),
					Vector3(-0.5f, -0.5f, 0.f),
					Vector3(-0.5f, 0.5f, 0.f)
				};
				Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(PointListPlane, PE_ARRAYSIZE(PointListPlane));
			}
			break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CIRCLE:
			{
				const Vector3 PointListCircle[] =
				{
					Vector3(1.f, 0.f, 0.f),
					Vector3(0.92388f, 0.382683f, 0.f),
					Vector3(0.707107f, 0.707107f, 0.f),
					Vector3(0.382683f, 0.92388f, 0.f),
					Vector3(-4.37114e-08f, 1.f, 0.f),
					Vector3(-0.382683f, 0.92388f, 0.f),
					Vector3(-0.707107f, 0.707107f, 0.f),
					Vector3(-0.92388f, 0.382683f, 0.f),
					Vector3(-1.f, -8.74228e-08f, 0.f),
					Vector3(-0.92388f, -0.382683f, 0.f),
					Vector3(-0.707107f, -0.707107f, 0.f),
					Vector3(-0.382684f, -0.92388f, 0.f),
					Vector3(1.19249e-08f, -1.f, 0.f),
					Vector3(0.382684f, -0.923879f, 0.f),
					Vector3(0.707107f, -0.707107f, 0.f),
					Vector3(0.92388f, -0.3826830f, 0.f),
					Vector3(1.f, 0.f, 0.f)
				};
				Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(PointListCircle, PE_ARRAYSIZE(PointListCircle));
			}
			break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CUBE:
			{
				const Vector3 CubePoints[] =
				{
					Vector3(-0.5f, 0.5f, 0.5f),
					Vector3(0.5f, 0.5f, 0.5f),
					Vector3(-0.5f, 0.5f, -0.5f),
					Vector3(0.5f, 0.5f, -0.5f),

					Vector3(-0.5f, -0.5f, 0.5f),
					Vector3(0.5f, -0.5f, 0.5f),
					Vector3(-0.5f, -0.5f, -0.5f),
					Vector3(0.5f, -0.5f, -0.5f)
				};
				const UINT32 CubeLines[] =
				{
					0u, 1u,
					1u, 2u,
					2u, 3u,
					3u, 0u,

					0u, 4u,
					1u, 5u,
					2u, 6u,
					3u, 7u,

					4u, 5u,
					5u, 6u,
					6u, 7u,
					7u, 0u
				};
				Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(CubePoints, PE_ARRAYSIZE(CubePoints), CubeLines, PE_ARRAYSIZE(CubeLines));
			}
			break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SPHERE:
			{
				Matrix4x4 RotY(MakeMatrix4x4(MakeQuaternion(Vector3::YVector(), EMath::DegreesToRadians(90.f))));
				Matrix4x4 RotX(MakeMatrix4x4(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.f))));

				const Vector3 SpherePoints[] =
				{
					Vector3(1.f, 0.f, 0.f),
					Vector3(0.92388f, 0.382683f, 0.f),
					Vector3(0.707107f, 0.707107f, 0.f),
					Vector3(0.382683f, 0.92388f, 0.f),
					Vector3(-4.37114e-08f, 1.f, 0.f),
					Vector3(-0.382683f, 0.92388f, 0.f),
					Vector3(-0.707107f, 0.707107f, 0.f),
					Vector3(-0.92388f, 0.382683f, 0.f),
					Vector3(-1.f, -8.74228e-08f, 0.f),
					Vector3(-0.92388f, -0.382683f, 0.f),
					Vector3(-0.707107f, -0.707107f, 0.f),
					Vector3(-0.382684f, -0.92388f, 0.f),
					Vector3(1.19249e-08f, -1.f, 0.f),
					Vector3(0.382684f, -0.923879f, 0.f),
					Vector3(0.707107f, -0.707107f, 0.f),
					Vector3(0.92388f, -0.3826830f, 0.f),
					Matrix4x4TransformPosition(RotY, SpherePoints[0]),
					Matrix4x4TransformPosition(RotY, SpherePoints[1]),
					Matrix4x4TransformPosition(RotY, SpherePoints[2]),
					Matrix4x4TransformPosition(RotY, SpherePoints[3]),
					Matrix4x4TransformPosition(RotY, SpherePoints[4]),
					Matrix4x4TransformPosition(RotY, SpherePoints[5]),
					Matrix4x4TransformPosition(RotY, SpherePoints[6]),
					Matrix4x4TransformPosition(RotY, SpherePoints[7]),
					Matrix4x4TransformPosition(RotY, SpherePoints[8]),
					Matrix4x4TransformPosition(RotY, SpherePoints[9]),
					Matrix4x4TransformPosition(RotY, SpherePoints[10]),
					Matrix4x4TransformPosition(RotY, SpherePoints[11]),
					Matrix4x4TransformPosition(RotY, SpherePoints[12]),
					Matrix4x4TransformPosition(RotY, SpherePoints[13]),
					Matrix4x4TransformPosition(RotY, SpherePoints[14]),
					Matrix4x4TransformPosition(RotY, SpherePoints[15]),
					Matrix4x4TransformPosition(RotX, SpherePoints[0]),
					Matrix4x4TransformPosition(RotX, SpherePoints[1]),
					Matrix4x4TransformPosition(RotX, SpherePoints[2]),
					Matrix4x4TransformPosition(RotX, SpherePoints[3]),
					Matrix4x4TransformPosition(RotX, SpherePoints[4]),
					Matrix4x4TransformPosition(RotX, SpherePoints[5]),
					Matrix4x4TransformPosition(RotX, SpherePoints[6]),
					Matrix4x4TransformPosition(RotX, SpherePoints[7]),
					Matrix4x4TransformPosition(RotX, SpherePoints[8]),
					Matrix4x4TransformPosition(RotX, SpherePoints[9]),
					Matrix4x4TransformPosition(RotX, SpherePoints[10]),
					Matrix4x4TransformPosition(RotX, SpherePoints[11]),
					Matrix4x4TransformPosition(RotX, SpherePoints[12]),
					Matrix4x4TransformPosition(RotX, SpherePoints[13]),
					Matrix4x4TransformPosition(RotX, SpherePoints[14]),
					Matrix4x4TransformPosition(RotX, SpherePoints[15]),
				};

#define GENERATE_SPHERE_LINES(__Index) \
	0u + (16u * __Index), 1u + (16u * __Index),\
	1u + (16u * __Index), 2u + (16u * __Index),\
	2u + (16u * __Index), 3u + (16u * __Index),\
	3u + (16u * __Index), 4u + (16u * __Index),\
	4u + (16u * __Index), 5u + (16u * __Index),\
	5u + (16u * __Index), 6u + (16u * __Index),\
	6u + (16u * __Index), 7u + (16u * __Index),\
	7u + (16u * __Index), 8u + (16u * __Index),\
	8u + (16u * __Index), 9u + (16u * __Index),\
	9u + (16u * __Index), 10u + (16u * __Index),\
	10u + (16u * __Index), 11u + (16u * __Index),\
	11u + (16u * __Index), 12u + (16u * __Index),\
	12u + (16u * __Index), 13u + (16u * __Index),\
	13u + (16u * __Index), 14u + (16u * __Index),\
	14u + (16u * __Index), 15u + (16u * __Index),\
	15u + (16u * __Index), 0u + (16u * __Index)\

				const UINT32 SphereLines[] =
				{
					GENERATE_SPHERE_LINES(0u),
					GENERATE_SPHERE_LINES(1u),
					GENERATE_SPHERE_LINES(2u)
				};

#undef GENERATE_SPHERE_LINES

				Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(SpherePoints, PE_ARRAYSIZE(SpherePoints), SphereLines, PE_ARRAYSIZE(SphereLines));
			}
			break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CONE:
			break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CYLINDER:
			break;
		case RDebugWireframeType::DEBUG_WIREFRAME_CUSTOM:
			break;
		default:
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Error type for debug wireframe primitive."));
			break;
		}
		Check((Result), (ENGINE_RENDER_CORE_ERROR));
	}
#endif

};