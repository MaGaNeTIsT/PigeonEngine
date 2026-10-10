#pragma once

#include <CoreMinimal.h>
#include <Base/RegisterBase.h>
#include <EngineCommon.h>
#include <RenderResource.h>
#include <Shader/ShaderEngineParameters.h>

namespace PigeonEngine
{

	class IRCommandList;

	class EShaderProgram;

	class RFullScreenTriangle final
	{
	public:
		UINT32	GetIndexCount()const;
		// Binds the triangle's vertex stream and index buffer for a draw of one program. Which
		// slot the stream lands in comes from that program's vertex stage reflection, never from
		// a number written here: the fullscreen factory declares where its one input sits, and a
		// full screen pass that binds anywhere else reads an unbound attribute, which for a
		// factory that builds its position out of that attribute means an invisible draw.
		void	BindPrimitiveBuffers(IRCommandList* InCommandList, const EShaderProgram& InProgram)const;
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
		void			InitNewFrame();
		void			SwapCommandSlots();
		void			RenderPrimitives_RenderThread(const class RViewProxy* InView);
		// Render thread: the debug wireframe shader was replaced. The instance is the stable handle
		// (the process wide cache owns it and re-pointed it), so only the program it resolves and
		// the draw block that was laid out from the previous one move.
		void			OnShaderAssetReplaced();
	public:
		void			DrawSingleLine(const Vector3& InPos0, const Vector3& InPos1, const Color4& InDebugColor = Color4::Black());
		void			DrawPlane(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const Color4& InDebugColor = Color4::Black());
		void			DrawCircle(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InRadius, const Color4& InDebugColor = Color4::Black());
		void			DrawCuboid(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const FLOAT InLengthZ, const Color4& InDebugColor = Color4::Black());
		void			DrawSphere(const Vector3& InLocation, const FLOAT InRadius, const Color4& InDebugColor = Color4::Black(), const Quaternion& InRotation = Quaternion::Identity());
		void			DrawCone(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InRadius, const Color4& InDebugColor = Color4::Black());
		void			DrawCylinder(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InBottomRadius, const FLOAT InTopRadius, const Color4& InDebugColor = Color4::Black());
		void			DrawCapsule(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InBottomRadius, const FLOAT InTopRadius, const Color4& InDebugColor = Color4::Black());
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
		RCommand							RequireCommands[2];
		UINT32								BackSlotIndex  = 0u;
		UINT32								FrontSlotIndex = 1u;
		RStructuredBuffer					PrimitiveTransformsBuffer[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		RStructuredBuffer					PrimitiveColorsBuffer[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		TMap<EString, RStructuredBuffer>	CustomPrimitiveTransformsBuffer;
		TMap<EString, RStructuredBuffer>	CustomPrimitiveColorsBuffer;
		TArray<Matrix4x4>					PrimitiveTransforms[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		TArray<Color4>						PrimitiveColors[RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u];
		TMap<EString, TArray<Matrix4x4>>	CustomPrimitiveTransforms;
		TMap<EString, TArray<Color4>>		CustomPrimitiveColors;
		// The pass's own draw block. The wireframe program declares it, and this pass is its
		// publisher - the same relationship a view has with its view block. The geometry is drawn
		// from the shape meshes below with the per instance transforms carried in the vertex
		// stage's buffers, so the transform published here is the identity.
		RShaderParameterBlock				DrawParameterBlock{ EName(SHADER_BLOCK_DRAW_PARAMS) };

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(RDebugWireframePrimitiveManager)

	};
#endif

};
