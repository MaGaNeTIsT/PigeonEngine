#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include <RenderResource.h>

namespace PigeonEngine
{

	class RFullScreenTriangle final
	{
	public:
		UINT32	GetIndexCount()const;
		void	BindPrimitiveBuffers();
	public:
		RFullScreenTriangle();
		RFullScreenTriangle(const RFullScreenTriangle& Other);
		~RFullScreenTriangle();
		RFullScreenTriangle& operator=(const RFullScreenTriangle& Other);
	};

#if _EDITOR_ONLY
	class RDebugWireframePrimitive
	{
	public:
		BOOL32					InitPrimitive(const Vector3* InVertexDatas, const UINT32 InVertexNum);
		BOOL32					InitPrimitive(const Vector3* InVertexDatas, const UINT32 InVertexNum, const UINT32* InIndexDatas, const UINT32 InIndexNum);
		UINT32					GetIndexCount()const;
		const RBufferResource&	GetPrimitiveBuffer()const;
	private:
		BOOL32				UseShortIndex;
		UINT32				IndexCount;
		RBufferResource		VertexBuffer;
		RBufferResource		IndexBuffer;
	public:
		RDebugWireframePrimitive();
		RDebugWireframePrimitive(const RDebugWireframePrimitive& Other);
		~RDebugWireframePrimitive();
		RDebugWireframePrimitive& operator=(const RDebugWireframePrimitive& Other);
	};

	class RDebugWireframePrimitiveManager : public EManagerBase
	{
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		void			CreateCustomPrimitive(RDebugWireframeType InType);
	private:
		void			CreatePrimitiveInternal(RDebugWireframeType InType, const EString* InCustomName = nullptr, const UINT32* InVectorNum = nullptr, const Vector3* InVectorDatas = nullptr);
	private:
		RDebugWireframePrimitive DebugWireframePrimitives[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(RDebugWireframePrimitiveManager)

	};
#endif

};