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

};