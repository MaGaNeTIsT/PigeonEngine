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
		: IndexCount(0u)
	{
	}
	RDebugWireframePrimitive::RDebugWireframePrimitive(const RDebugWireframePrimitive& Other)
		: IndexCount(Other.IndexCount), VertexBuffer(Other.VertexBuffer)
	{
	}
	RDebugWireframePrimitive::~RDebugWireframePrimitive()
	{
	}
	RDebugWireframePrimitive& RDebugWireframePrimitive::operator=(const RDebugWireframePrimitive& Other)
	{
		IndexCount		= Other.IndexCount;
		VertexBuffer	= Other.VertexBuffer;
		return (*this);
	}
	BOOL32 RDebugWireframePrimitive::InitPrimitive(const Vector3* InDatas, const UINT32& InNum)
	{
		if (VertexBuffer.IsRenderResourceValid())
		{
			Check((IndexCount > 0u), (ENGINE_RENDER_CORE_ERROR));
			return TRUE;
		}

		IndexCount = InNum;

		RSubresourceDataDesc SubresourceDataDesc;
		SubresourceDataDesc.pSysMem = InDatas;

		BOOL32 Result = RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(VertexBuffer.Buffer,
			RBufferDesc((sizeof(Vector3) * InNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
			(&SubresourceDataDesc));
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
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SPHERE:
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