#pragma once

#include <CoreMinimal.h>
#include <Base/RegisterBase.h>
#include <EngineCommon.h>
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
		BOOL32					IsUseShortIndex()const;
		UINT32					GetIndexCount()const;
		const EBoundAABB&		GetBound()const;
		const RBufferResource&	GetPrimitiveVertexBuffer()const;
		const RBufferResource&	GetPrimitiveIndexBuffer()const;
	private:
		BOOL32				UseShortIndex;
		UINT32				IndexCount;
		EBoundAABB			Bound;
		RBufferResource		VertexBuffer;
		RBufferResource		IndexBuffer;
	public:
		RDebugWireframePrimitive();
		RDebugWireframePrimitive(const RDebugWireframePrimitive& Other);
		~RDebugWireframePrimitive();
		RDebugWireframePrimitive& operator=(const RDebugWireframePrimitive& Other);
	};

	enum RDebugWireframeType : UINT32
	{
		DEBUG_WIREFRAME_UNKNOWN				= 0,
		DEBUG_WIREFRAME_ENGINE_SINGLELINE,
		DEBUG_WIREFRAME_ENGINE_PLANE,
		DEBUG_WIREFRAME_ENGINE_CIRCLE,
		DEBUG_WIREFRAME_ENGINE_CUBOID,
		DEBUG_WIREFRAME_ENGINE_SPHERE,
		DEBUG_WIREFRAME_ENGINE_CONE,
		DEBUG_WIREFRAME_ENGINE_CYLINDER,
		DEBUG_WIREFRAME_ENGINE_COUNT,
		DEBUG_WIREFRAME_CUSTOM
	};

	class RDebugWireframePrimitiveManager final : public EManagerBase
	{
	public:
		PE_CONSTEXPR_STATIC UINT32 PrimitiveDrawableMaxNum = 256u;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		void			RenderPrimitives_RenderThread(const class RViewProxy* InView);
	public:
		void			DrawSingleLine(const Vector3& InPos0, const Vector3& InPos1, const Color4& InDebugColor = Color4::Black());
		void			DrawPlane(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const Color4& InDebugColor = Color4::Black());
		void			DrawCircle(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InRadius, const Color4& InDebugColor = Color4::Black());
		void			DrawCuboid(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const FLOAT InLengthZ, const Color4& InDebugColor = Color4::Black());
		void			DrawSphere(const Vector3& InLocation, const FLOAT InRadius, const Color4& InDebugColor = Color4::Black(), const Quaternion& InRotation = Quaternion::Identity());
		void			DrawCone(const Vector3& InBottomCenterLocation, const FLOAT InTopLocation, const FLOAT InRadius, const Color4& InDebugColor = Color4::Black());
		void			DrawCylinder(const Vector3& InBottomCenterLocation, const FLOAT InTopLocation, const FLOAT InRadius, const Color4& InDebugColor = Color4::Black());
		void			DrawCustom(const EString& InCustomName, const Matrix4x4& InLocalToWorld, const Color4& InDebugColor = Color4::Black());
	public:
		void			CreatePrimitive(RDebugWireframeType InType);
		void			CreateCustomPrimitive(const EString& InCustomName, const Vector3* InPointDatas, const UINT32 InPointNum);
	private:
		class RCommand final
		{
		public:
			template<typename _TFunctionType>
			void EnqueueCommand(_TFunctionType InFunction)
			{
				RawCommands.AddRegisterFunction<_TFunctionType>(InFunction);
			}
			void DoCommands()
			{
				RawCommands.DoRegister();
			}
			void EmptyQueue()
			{
				RawCommands.ClearRegister();
			}
		private:
			ERegisterBase<EVoidFunctionType>	RawCommands;
		public:
			RCommand() {}
			RCommand(const RCommand&) = delete;
			~RCommand() {}
			RCommand& operator=(const RCommand&) = delete;
		};
		void CreatePrimitiveInternal(RDebugWireframeType InType, const EString* InCustomName = nullptr, const UINT32* InVectorNum = nullptr, const Vector3* InVectorDatas = nullptr);
	private:
		RDebugWireframePrimitive				DebugWireframePrimitives[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		TMap<EString, RDebugWireframePrimitive>	DebugWireframeCustomPrimitives;
	private:
		RCommand							RequireCreateCustomCommands;
		RCommand							RequireCommands;
		RStructuredBuffer					PrimitiveTransformsBuffer[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		RStructuredBuffer					PrimitiveColorsBuffer[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		TMap<EString, RStructuredBuffer>	CustomPrimitiveTransformsBuffer;
		TMap<EString, RStructuredBuffer>	CustomPrimitiveColorsBuffer;
		TArray<Matrix4x4>					PrimitiveTransforms[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		TArray<Color4>						PrimitiveColors[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		TMap<EString, TArray<Matrix4x4>>	CustomPrimitiveTransforms;
		TMap<EString, TArray<Color4>>		CustomPrimitiveColors;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(RDebugWireframePrimitiveManager)

	};
#endif

};