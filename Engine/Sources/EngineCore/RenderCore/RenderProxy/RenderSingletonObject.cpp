#include "RenderSingletonObject.h"
#include <RenderResource.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/IRCommandList.h>
#include <Pipeline/RPipelineStateCache.h>
#include <Renderer/ScenePass.h>
#include <Renderer/SceneRenderer.h>
#include <Main/MainManager.h>
// The full screen triangle's input slot is read from the vertex stage's reflection, so this
// header is part of the code every configuration compiles and not of the editor-only block
// below.
#include <Shader/ShaderAssetBinding.h>
#if _EDITOR_ONLY
#include <RenderConfig/RenderConfig.h>
#include <Shader/ShaderParameter.h>
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include "../../Editor/EditorLogManager.h"
#include <RenderProxy/ViewProxy.h>
#endif

namespace PigeonEngine
{
	static RBufferResource _GFullScreenTriangleVertexBuffer;
	static RBufferResource _GFullScreenTriangleIndexBuffer;

	static void CreateFullScreenTriangleVertexBuffer()
	{
		if (!(_GFullScreenTriangleVertexBuffer.IsRenderResourceValid()))
		{
			// The four corners of the quad in UV space, origin top left: (0,1) (1,1) (0,0) (1,0).
			// The index buffer below decides which way the triangles wind.
			FLOAT FullScreenTriangleVertex[] =
			{
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f
			};
			RRHISubresourceData SubData(reinterpret_cast<const UINT8*>(FullScreenTriangleVertex), sizeof(FLOAT) * 2u * 4u, 0u, 0u);
			const BOOL8 Result = _GFullScreenTriangleVertexBuffer.InitFromRHI(
				EMainManager::GetManagerSingleton()->GetRenderDevice(),
				RBufferDesc((sizeof(FLOAT) * 2u * 4u), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
				&SubData);
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create full screen triangle vertex buffer failed."));
			}
#else
			(void)Result;
#endif
		}
	}
	static void CreateFullScreenTriangleIndexBuffer()
	{
		if (!(_GFullScreenTriangleIndexBuffer.IsRenderResourceValid()))
		{
			// Clockwise on the render target, which is what a front face is on a back-end with
			// FrontCounterClockwise = FALSE (both of them are). The quad's clip space mapping
			// (UV * 2 - 1 with y negated, see FullScreenVF.hlsl) turns the UV order into that
			// screen space order, so the winding is decided here: winding the other way makes
			// every pass that culls back faces drop the whole full screen draw, which is a pass
			// that renders nothing with no error anywhere.
			USHORT FullScreenTriangleIndex[] =
			{
				0, 2, 1,
				2, 3, 1
			};
			RRHISubresourceData SubData(reinterpret_cast<const UINT8*>(FullScreenTriangleIndex), 2u * 6u, 0u, 0u);
			const BOOL8 Result = _GFullScreenTriangleIndexBuffer.InitFromRHI(
				EMainManager::GetManagerSingleton()->GetRenderDevice(),
				RBufferDesc((2u * 6u), RBindFlagType::BIND_INDEX_BUFFER, 2u),
				&SubData);
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create full screen triangle index buffer failed."));
			}
#else
			(void)Result;
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
	void RFullScreenTriangle::BindPrimitiveBuffers(IRCommandList* InCommandList, const EShaderProgram& InProgram)const
	{
		// Phase 4.3.e fix: previously this hard-gated on D3D11 backend and
		// returned early under D3D12 - so FinalOutputPass's DrawIndexed had no
		// vertex / index buffer bound and only the cleared back-buffer color
		// was visible. Route both backends through the IRCommandList API so
		// the dual-mode buffer wrapper does the right thing on either path.
		if (!InCommandList)
		{
			return;
		}
#if _DEBUG_MODE
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check full screen triangle vertex buffer is invalid.")), (_GFullScreenTriangleVertexBuffer.IsRenderResourceValid()));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check full screen triangle index buffer is invalid.")), (_GFullScreenTriangleIndexBuffer.IsRenderResourceValid()));
#endif
		// Where the stream goes is the shader's answer, read from the reflection the pipeline's
		// input layout is built from: the factory declares one input and its slot, so binding
		// anywhere else leaves that input unbound - and this factory reconstructs its clip space
		// position from that input, so the draw would produce nothing at all rather than
		// something wrong. A program with no declared input gets slot 0, which is where a stream
		// nobody reads may as well sit.
		UINT32 InputSlot = 0u;
		const RShaderStageResource* VertexStage = ResolveShaderStageResource(InProgram, EShaderStageType::SHADER_STAGE_VERTEX);
		if (VertexStage)
		{
			const TArray<RInputLayoutDesc>& InputLayouts = VertexStage->GetInputLayouts();
			if (InputLayouts.Num<UINT32>() > 0u)
			{
				InputSlot = InputLayouts[0].InputSlot;
			}
		}

		IRRHIBuffer* VB = _GFullScreenTriangleVertexBuffer.GetResolvedRHIBuffer();
		IRRHIBuffer* IB = _GFullScreenTriangleIndexBuffer.GetResolvedRHIBuffer();
		// The triangle's own layout: four float2 coordinates. It is this class's data, so its
		// stride is stated here rather than derived from the input element.
		const UINT32 Stride = sizeof(FLOAT) * 2u;
		const UINT32 Offset = 0u;
		InCommandList->SetVertexBuffers(InputSlot, 1u, &VB, &Stride, &Offset);
		InCommandList->SetIndexBuffer(IB, ERHIIndexFormat::RHI_INDEX_FORMAT_UINT16, 0u);
	}

#if _EDITOR_ONLY
	// The engine debug wireframe program. Loaded once per process and shared: the manager
	// keeps only the resolved program, which is what the debug pass builds its pipeline state
	// from.
	const EShaderProgram* _GDebugWireframePrimitiveProgram = nullptr;
	// The instance the program came from. The overlay's colour is a material parameter, so the
	// pass has to be able to reach the instance's parameter buffers - the program alone cannot
	// answer for values.
	const EShaderInstance* _GDebugWireframePrimitiveInstance = nullptr;

	static const EShaderInstance* GetDebugWireframeProgramInstance()
	{
		// The debug wireframe pass states its own source: the DebugWireframe program,
		// its Unlit material and a mesh shaped vertex axis. The wireframe stream carries
		// positions only, so the factory's required UV0 has no buffer behind it; the draw
		// skips that stream, which leaves the shader reading a slot nothing is bound to.
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("M_EngineDebugWireframe"));
		Request.MaterialName = EString(PE_TEXT("M_EngineDebugWireframe"));
		Request.ProgramName = EName(PE_TEXT("DebugWireframe"));
		Request.VertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
		Request.Profile.StreamProfile = (SHADER_VERTEX_ATTRIBUTE_POSITION | SHADER_VERTEX_ATTRIBUTE_UV0);
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_GRAPHICS;

		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(Request);
	}

	RDebugWireframePrimitive::RDebugWireframePrimitive()
		: UseShortIndex(FALSE), IndexCount(0u)
	{
		Bound.IsValid = TRUE;
	}
	RDebugWireframePrimitive::RDebugWireframePrimitive(const RDebugWireframePrimitive& Other)
		: UseShortIndex(Other.UseShortIndex), IndexCount(Other.IndexCount), Bound(Other.Bound), VertexBuffer(Other.VertexBuffer), IndexBuffer(Other.IndexBuffer)
	{
		Bound.IsValid = TRUE;
	}
	RDebugWireframePrimitive::~RDebugWireframePrimitive()
	{
	}
	RDebugWireframePrimitive& RDebugWireframePrimitive::operator=(const RDebugWireframePrimitive& Other)
	{
		UseShortIndex	= Other.UseShortIndex;
		IndexCount		= Other.IndexCount;
		Bound			= Other.Bound;
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

		RRHISubresourceData VertexSubData(reinterpret_cast<const UINT8*>(InVertexDatas), sizeof(Vector3) * InVertexNum, 0u, 0u);
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();

		BOOL32 Result = VertexBuffer.InitFromRHI(RenderDevice,
			RBufferDesc((sizeof(Vector3) * InVertexNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
			&VertexSubData);

		const void* IndexBytes = UseShortIndex ? ((const void*)UshortIndices) : ((const void*)UintIndices);
		RRHISubresourceData IndexSubData(static_cast<const UINT8*>(IndexBytes), (UseShortIndex ? 2u : 4u) * IndexCount, 0u, 0u);

		Result = Result && IndexBuffer.InitFromRHI(RenderDevice,
			RBufferDesc(((UseShortIndex ? 2u : 4u) * IndexCount), RBindFlagType::BIND_INDEX_BUFFER, (UseShortIndex ? 2u : 4u)),
			&IndexSubData);

		if (!!UintIndices)
		{
			delete[]UintIndices;
		}
		if (!!UshortIndices)
		{
			delete[]UshortIndices;
		}

		{
			Bound.AABBMin = Vector3(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
			Bound.AABBMax = Vector3(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
			for (UINT32 i = 0u; i < InVertexNum; i++)
			{
				Bound.AABBMin = MinVector3(InVertexDatas[i], Bound.AABBMin);
				Bound.AABBMax = MaxVector3(InVertexDatas[i], Bound.AABBMax);
			}
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

		RRHISubresourceData VertexSubData(reinterpret_cast<const UINT8*>(InVertexDatas), sizeof(Vector3) * InVertexNum, 0u, 0u);
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();

		BOOL32 Result = VertexBuffer.InitFromRHI(RenderDevice,
			RBufferDesc((sizeof(Vector3) * InVertexNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
			&VertexSubData);

		const void* IndexBytes = UseShortIndex ? ((const void*)UshortIndices) : ((const void*)InIndexDatas);
		RRHISubresourceData IndexSubData(static_cast<const UINT8*>(IndexBytes), (UseShortIndex ? 2u : 4u) * IndexCount, 0u, 0u);

		Result = Result && IndexBuffer.InitFromRHI(RenderDevice,
			RBufferDesc(((UseShortIndex ? 2u : 4u) * IndexCount), RBindFlagType::BIND_INDEX_BUFFER, (UseShortIndex ? 2u : 4u)),
			&IndexSubData);

		if (!!UshortIndices)
		{
			delete[]UshortIndices;
		}

		{
			Bound.AABBMin = Vector3(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
			Bound.AABBMax = Vector3(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
			for (UINT32 i = 0u; i < InVertexNum; i++)
			{
				Bound.AABBMin = MinVector3(InVertexDatas[i], Bound.AABBMin);
				Bound.AABBMax = MaxVector3(InVertexDatas[i], Bound.AABBMax);
			}
		}

		return Result;
	}
	BOOL32 RDebugWireframePrimitive::IsUseShortIndex()const
	{
		return UseShortIndex;
	}
	UINT32 RDebugWireframePrimitive::GetIndexCount()const
	{
		return IndexCount;
	}
	const EBoundAABB& RDebugWireframePrimitive::GetBound()const
	{
		return Bound;
	}
	const RBufferResource& RDebugWireframePrimitive::GetPrimitiveVertexBuffer()const
	{
		return VertexBuffer;
	}
	const RBufferResource& RDebugWireframePrimitive::GetPrimitiveIndexBuffer()const
	{
		return IndexBuffer;
	}

	RDebugWireframePrimitiveManager::RDebugWireframePrimitiveManager()
	{
	}
	RDebugWireframePrimitiveManager::~RDebugWireframePrimitiveManager()
	{
	}
	void RDebugWireframePrimitiveManager::Initialize()
	{
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SINGLELINE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_PLANE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CIRCLE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CUBOID);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SPHERE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CONE);

		{
			// Phase 4.3.0.4: the debug wireframe shader is sourced from the engine
			// shader asset M_EngineDebugWireframe. The instance is loaded once per
			// process; VS, PS and the input layout all come from that one program.
			if (!_GDebugWireframePrimitiveProgram)
			{
				const EShaderInstance* DebugWireframeInstance = GetDebugWireframeProgramInstance();
				if (DebugWireframeInstance)
				{
					_GDebugWireframePrimitiveProgram = DebugWireframeInstance->GetProgram();
					// The instance is kept as well, not only its program: the pass binds the
					// material's parameter block (the overlay's WireframeColor) through it, and
					// that block belongs to the instance.
					_GDebugWireframePrimitiveInstance = DebugWireframeInstance;
				}
			}
		}
	}
	void RDebugWireframePrimitiveManager::OnShaderAssetReplaced()
	{
		if (_GDebugWireframePrimitiveInstance)
		{
			// The instance is the stable handle: the process wide cache owns it and re-pointed it to
			// the new asset before this ran, so re-reading its program is what makes the overlay
			// current. The pipeline state needs nothing here - it is derived from the pipeline state
			// cache on every frame, and that cache dropped the states built for this shader.
			_GDebugWireframePrimitiveProgram = _GDebugWireframePrimitiveInstance->GetProgram();
		}
		// The draw block had its layout read from the previous program's reflection, so it is
		// dropped and rebuilt from the current one by its next user.
		DrawParameterBlock.ReleaseRenderResources();
	}
	void RDebugWireframePrimitiveManager::ShutDown()
	{
	}
	void RDebugWireframePrimitiveManager::SwapCommandSlots()
	{
		const UINT32 NewBack = FrontSlotIndex;
		FrontSlotIndex       = BackSlotIndex;
		BackSlotIndex        = NewBack;
	}
	void RDebugWireframePrimitiveManager::InitNewFrame()
	{
		{
			for (UINT32 i = 0u, n = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u; i < n; i++)
			{
				PrimitiveTransforms[i].Empty();
				PrimitiveColors[i].Empty();
			}
			for (auto It = CustomPrimitiveTransforms.Begin(); It != CustomPrimitiveTransforms.End(); It++)
			{
				It->second.Empty();
			}
			for (auto It = CustomPrimitiveColors.Begin(); It != CustomPrimitiveColors.End(); It++)
			{
				It->second.Empty();
			}
		}

		{
			RequireCreateCustomCommands.DoCommands();
			RequireCreateCustomCommands.EmptyQueue();
			RequireCommands[FrontSlotIndex].DoCommands();
			RequireCommands[FrontSlotIndex].EmptyQueue();

#if _DEBUG_MODE
			for (UINT32 i = 0u, n = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u; i < n; i++)
			{
				Check(PrimitiveTransforms[i].Num() == PrimitiveColors[i].Num());
			}
			Check(CustomPrimitiveTransforms.Num() == CustomPrimitiveColors.Num());
#endif
		}
	}
	// The instance data of the debug wireframe pass: one structured buffer of transforms and
	// one of colors per primitive shape. Both are read only, which is what the element stride
	// plus the structured misc flag describes on either back-end.
	static void CreateDebugStructuredBuffer(RStructuredBuffer& OutBuffer, const UINT32 InStride, const UINT32 InElementNum)
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}
		OutBuffer.InitFromRHI(RenderDevice,
			RBufferDesc(static_cast<UINT64>(InStride) * InElementNum
				, RBindFlagType::BIND_SHADER_RESOURCE
				, InStride
				, RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED));
	}
	// The state the debug pass draws with. The pass borrows the forward shape (one target, no
	// GBuffer) and states the line topology itself, because the renderer draws it as a list of
	// line primitives rather than as a triangle list.
	static IRRHIPipelineState* ResolveDebugWireframePipelineState()
	{
		if (!_GDebugWireframePrimitiveProgram)
		{
			return nullptr;
		}
		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
		if (!Renderer)
		{
			return nullptr;
		}
		RGraphicsPipelineStateDesc Desc;
		FRenderPassRegistry::FillPipelineStateDesc(
			FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_FORWARD), _GDebugWireframePrimitiveProgram, Desc);
		Desc.Topology = RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST;
		return Renderer->GetPipelineStateCache().GetOrCreateGraphics(Desc);
	}
	// Binds one debug primitive with the instance data of this frame and draws it. The vertex
	// stage reads its two instance buffers and the geometry is a position only stream, so its
	// stride is the one Vector3 gives. The two buffers land at the registers the vertex stage's
	// plan reports rather than at a number written here: the program declares them by name, and
	// the register the compiler assigned them is what the plan carries - a component list that
	// grows a binding in front of them moves them without this draw knowing, and a draw that
	// wrote down t0 and t1 would then feed the wrong buffer into a lookup that indexes it.
	static void DrawDebugPrimitive(IRCommandList* InCommandList
		, const RDebugWireframePrimitive& InPrimitive
		, RStructuredBuffer& InTransformsBuffer, RStructuredBuffer& InColorsBuffer
		, const UINT32 InInstanceNum
		, const EShaderBindingPlan& InVertexPlan)
	{
		IRRHIBuffer* VertexBuffer = const_cast<RBufferResource&>(InPrimitive.GetPrimitiveVertexBuffer()).GetResolvedRHIBuffer();
		IRRHIBuffer* IndexBuffer = const_cast<RBufferResource&>(InPrimitive.GetPrimitiveIndexBuffer()).GetResolvedRHIBuffer();
		if ((!VertexBuffer) || (!IndexBuffer))
		{
			return;
		}

		// The same route a material's textures take: a value keyed by the resource name, bound
		// through the plan of the stage that declared it.
		EShaderParameterSet InstanceParameters;
		InstanceParameters.SetStructuredBuffer(EName(), EName(PE_TEXT("DebugPrimitiveTransforms")), &InTransformsBuffer);
		InstanceParameters.SetStructuredBuffer(EName(), EName(PE_TEXT("DebugPrimitiveColors")), &InColorsBuffer);
		BindShaderParameterResources(InCommandList, InstanceParameters, InVertexPlan, ERHIShaderStage::RHI_SHADER_STAGE_VERTEX);

		const UINT32 Stride = sizeof(FLOAT) * 3u;
		const UINT32 Offset = 0u;
		InCommandList->SetVertexBuffers(0u, 1u, &VertexBuffer, &Stride, &Offset);
		InCommandList->SetIndexBuffer(IndexBuffer
			, InPrimitive.IsUseShortIndex() ? ERHIIndexFormat::RHI_INDEX_FORMAT_UINT16 : ERHIIndexFormat::RHI_INDEX_FORMAT_UINT32
			, 0u);
		InCommandList->DrawIndexed(InPrimitive.GetIndexCount(), InInstanceNum, 0u, 0, 0u);
	}

	void RDebugWireframePrimitiveManager::RenderPrimitives_RenderThread(const RViewProxy* InView)
	{
		(void)InView;
		IRCommandList* CmdList = EMainManager::GetManagerSingleton()->GetActiveCommandList();
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!CmdList) || (!RenderDevice))
		{
			return;
		}
		IRRHIPipelineState* PipelineState = ResolveDebugWireframePipelineState();
		if (!PipelineState)
		{
			// The overlay is a whole pass of its own, so nothing draws when its state is missing and
			// the only symptom is the absence. It stays quiet once the program is assembled, which
			// is why this line means something if it is still there after the load.
			ReportSkippedDraw(EString(PE_TEXT("the debug wireframe overlay")), EString(PE_TEXT("its pipeline state could not be built for program '"))
				+ ((_GDebugWireframePrimitiveProgram) ? _GDebugWireframePrimitiveProgram->GetShaderName().ToString() : EString(PE_TEXT("<none>")))
				+ EString(PE_TEXT("'")));
			return;
		}

		// The vertex stage's plan, resolved once for the frame: it is what the two instance
		// buffers are bound through, so a draw never names a register.
		const EShaderStageEntry* VertexEntry = ResolveShaderStageEntry(*_GDebugWireframePrimitiveProgram, EShaderStageType::SHADER_STAGE_VERTEX);
		if (!VertexEntry)
		{
			ReportSkippedDraw(EString(PE_TEXT("the debug wireframe overlay")), EString(PE_TEXT("its program has no vertex stage to bind the primitives through")));
			return;
		}
		const EShaderBindingPlan& VertexPlan = VertexEntry->BindingPlan;

		{
			constexpr UINT32 TransformSize = sizeof(Matrix4x4) * PrimitiveDrawableMaxNum;
			constexpr UINT32 ColorSize = sizeof(Color4) * PrimitiveDrawableMaxNum;

			// The pipeline state goes first: it brings the root signature, and switching a root
			// signature voids every root binding made before it.
			CmdList->SetPipelineState(PipelineState);

			// Then the blocks the pass's program reads, because the state above voided whatever an
			// earlier pass had left bound: the renderer's shared blocks (the wireframe vertex stage
			// transforms through ViewParams.ViewProjectionMatrix), and this pass's own draw block.
			// A draw whose roots are left unset reads zeros, which for a transform means every
			// vertex collapses into one point - the overlay silently disappears instead of drawing
			// something wrong.
			RSceneRenderer* PassRenderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
			if (PassRenderer)
			{
				PassRenderer->BindEngineParameters(CmdList, *_GDebugWireframePrimitiveProgram);
			}
			if (!DrawParameterBlock.IsInitialized())
			{
				if (!DrawParameterBlock.EnsureInitialized(RenderDevice, *_GDebugWireframePrimitiveProgram))
				{
					// The program cannot describe the block, so the state that is already set
					// cannot be drawn into correctly. Skipping the overlay is the honest answer,
					// and saying so is what keeps the skip from being an unexplained absence.
					ReportSkippedDraw(EString(PE_TEXT("the debug wireframe overlay")), EString(PE_TEXT("the program cannot describe its per draw block")));
					return;
				}
				FLOAT Identity[16] = {};
				FlattenShaderMatrix(Matrix4x4::Identity(), Identity);
				EShaderParameterSet& Parameters = DrawParameterBlock.GetParameters();
				const EName& BlockName = DrawParameterBlock.GetBlockName();
				Parameters.SetFloatArray(BlockName, EName(SHADER_FIELD_WORLD_MATRIX), Identity, 16u);
				Parameters.SetFloatArray(BlockName, EName(SHADER_FIELD_WORLD_INV_MATRIX), Identity, 16u);
				Parameters.SetFloatArray(BlockName, EName(SHADER_FIELD_WORLD_INV_TRANSPOSE_MATRIX), Identity, 16u);
			}
			DrawParameterBlock.Upload(RenderDevice);
			DrawParameterBlock.Bind(CmdList, *_GDebugWireframePrimitiveProgram);

			// The overlay's own block: the material's WireframeColor. The same three steps a scene
			// material goes through - evaluate the cpu side, upload the mirrors that moved, bind
			// through the program's plan - so a material parameter behaves here exactly as it does
			// in a mesh draw, and the pass never carries a register of its own for it.
			if (_GDebugWireframePrimitiveInstance)
			{
				_GDebugWireframePrimitiveInstance->Prepare();
				RShaderParameterBuffers* MaterialBuffers = ResolveShaderParameterBuffers(*_GDebugWireframePrimitiveInstance);
				if (MaterialBuffers)
				{
					MaterialBuffers->UploadFromParameterSet(RenderDevice, _GDebugWireframePrimitiveInstance->GetParameters());
					BindShaderParameterBuffers(CmdList, *MaterialBuffers, *_GDebugWireframePrimitiveProgram
						, EShaderStageType::SHADER_STAGE_PIXEL, FALSE);
				}
			}

			for (UINT32 i = 0u, n = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u; i < n; i++)
			{
				if (!(PrimitiveTransformsBuffer[i].IsRenderResourceValid()))
				{
					CreateDebugStructuredBuffer(PrimitiveTransformsBuffer[i], sizeof(Matrix4x4), PrimitiveDrawableMaxNum);
				}
				if (!(PrimitiveColorsBuffer[i].IsRenderResourceValid()))
				{
					CreateDebugStructuredBuffer(PrimitiveColorsBuffer[i], sizeof(Color4), PrimitiveDrawableMaxNum);
				}

				const TArray<Matrix4x4>& TempTransforms = PrimitiveTransforms[i];
				const TArray<Color4>& TempColors = PrimitiveColors[i];
				Check(TempTransforms.Num() == TempColors.Num());

				if (const UINT32 InstanceNum = TempTransforms.Num<UINT32>(); InstanceNum > 0u)
				{
					Matrix4x4* TempDataTransforms = new Matrix4x4[PrimitiveDrawableMaxNum];
					Color4* TempDataColor4s = new Color4[PrimitiveDrawableMaxNum];

					for (UINT32 Index = 0u; (Index < InstanceNum) && (Index < PrimitiveDrawableMaxNum); Index++)
					{
						TempDataTransforms[Index] = TempTransforms[Index];
						TempDataColor4s[Index] = TempColors[Index];
					}

					PrimitiveTransformsBuffer[i].UploadDataViaRHI(RenderDevice, TempDataTransforms, TransformSize, 0u);
					PrimitiveColorsBuffer[i].UploadDataViaRHI(RenderDevice, TempDataColor4s, ColorSize, 0u);

					delete[]TempDataTransforms;
					delete[]TempDataColor4s;

					DrawDebugPrimitive(CmdList, DebugWireframePrimitives[i]
						, PrimitiveTransformsBuffer[i], PrimitiveColorsBuffer[i], InstanceNum, VertexPlan);
				}
			}

			for (auto It = CustomPrimitiveTransforms.Begin(); It != CustomPrimitiveTransforms.End(); It++)
			{
				const EString& Name = (It->first);

				const TArray<Matrix4x4>& CustomTransforms = (It->second);
				TArray<Color4>* CustomColors = CustomPrimitiveColors.FindValueAsPtr(Name);
				Check(((!!CustomColors) && (CustomColors->Num() == CustomTransforms.Num())));

				if (!(CustomPrimitiveTransformsBuffer.ContainsKey(Name)))
				{
					CustomPrimitiveTransformsBuffer.Add(Name, RStructuredBuffer());
				}
				if (!(CustomPrimitiveColorsBuffer.ContainsKey(Name)))
				{
					CustomPrimitiveColorsBuffer.Add(Name, RStructuredBuffer());
				}
				RStructuredBuffer* TransformsBuffer = CustomPrimitiveTransformsBuffer.FindValueAsPtr(Name);
				RStructuredBuffer* ColorsBuffer = CustomPrimitiveColorsBuffer.FindValueAsPtr(Name);
				if ((!TransformsBuffer) || (!ColorsBuffer))
				{
					continue;
				}
				if (!(TransformsBuffer->IsRenderResourceValid()))
				{
					CreateDebugStructuredBuffer((*TransformsBuffer), sizeof(Matrix4x4), PrimitiveDrawableMaxNum);
				}
				if (!(ColorsBuffer->IsRenderResourceValid()))
				{
					CreateDebugStructuredBuffer((*ColorsBuffer), sizeof(Color4), PrimitiveDrawableMaxNum);
				}

				if (const UINT32 InstanceNum = CustomTransforms.Num<UINT32>(); InstanceNum > 0u)
				{
					Matrix4x4* TempDataTransforms = new Matrix4x4[PrimitiveDrawableMaxNum];
					Color4* TempDataColor4s = new Color4[PrimitiveDrawableMaxNum];

					for (UINT32 Index = 0u; (Index < InstanceNum) && (Index < PrimitiveDrawableMaxNum); Index++)
					{
						TempDataTransforms[Index] = CustomTransforms[Index];
						TempDataColor4s[Index] = (*CustomColors)[Index];
					}

					TransformsBuffer->UploadDataViaRHI(RenderDevice, TempDataTransforms, TransformSize, 0u);
					ColorsBuffer->UploadDataViaRHI(RenderDevice, TempDataColor4s, ColorSize, 0u);

					delete[]TempDataTransforms;
					delete[]TempDataColor4s;

					RDebugWireframePrimitive* Primitive = DebugWireframeCustomPrimitives.FindValueAsPtr(Name);
					if (Primitive)
					{
						DrawDebugPrimitive(CmdList, (*Primitive), (*TransformsBuffer), (*ColorsBuffer), InstanceNum, VertexPlan);
					}
				}
			}
		}
	}
	void RDebugWireframePrimitiveManager::DrawSingleLine(const Vector3& InPos0, const Vector3& InPos1, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SINGLELINE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		Vector3 Direction = InPos1 - InPos0;
		FLOAT DirectionLength = Direction.Length();
		if (DirectionLength <= 1e-3f)
		{
			Direction = Vector3::XVector();
			DirectionLength = 1e-3f;
		}
		else
		{
			Direction = Direction / DirectionLength;
		}

		RequireCommands[BackSlotIndex].EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InPos0, MakeQuaternion(Vector3::YVector(), Direction), Vector3(1.f, DirectionLength, 1.f)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawPlane(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_PLANE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands[BackSlotIndex].EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InWidthX, InHeightY, 1.f)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCircle(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InRadius, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CIRCLE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands[BackSlotIndex].EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InRadius, InRadius, 1.f)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCuboid(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const FLOAT InLengthZ, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CUBOID - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands[BackSlotIndex].EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InWidthX, InHeightY, InLengthZ)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawSphere(const Vector3& InLocation, const FLOAT InRadius, const Color4& InDebugColor, const Quaternion& InRotation)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SPHERE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands[BackSlotIndex].EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InRadius, InRadius, InRadius)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCone(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InRadius, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CONE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		Vector3 Direction = InTopLocation - InBottomCenterLocation;
		FLOAT DirectionLength = Direction.Length();
		if (DirectionLength <= 1e-3f)
		{
			Direction = Vector3::XVector();
			DirectionLength = 1e-3f;
		}
		else
		{
			Direction = Direction / DirectionLength;
		}

		RequireCommands[BackSlotIndex].EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InBottomCenterLocation, MakeQuaternion(Vector3::YVector(), Direction), Vector3(InRadius, DirectionLength, InRadius)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCylinder(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InBottomRadius, const FLOAT InTopRadius, const Color4& InDebugColor)
	{
		Vector3 Direction = InTopLocation - InBottomCenterLocation;
		FLOAT DirectionLength = Direction.Length();
		if (DirectionLength <= 1e-3f)
		{
			Direction = Vector3::XVector();
			DirectionLength = 1e-3f;
		}
		else
		{
			Direction = Direction / DirectionLength;
		}
		const Quaternion Rotation = MakeQuaternion(Vector3::YVector(), Direction) * MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.f));
		DrawCircle(InBottomCenterLocation, Rotation, InBottomRadius, InDebugColor);
		DrawCircle(InTopLocation, Rotation, InTopRadius, InDebugColor);
		DrawSingleLine(InBottomCenterLocation, InTopLocation, InDebugColor);
	}
	void RDebugWireframePrimitiveManager::DrawCapsule(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InBottomRadius, const FLOAT InTopRadius, const Color4& InDebugColor)
	{
		DrawSphere(InBottomCenterLocation, InBottomRadius, InDebugColor);
		DrawSphere(InTopLocation, InTopRadius, InDebugColor);
		DrawSingleLine(InBottomCenterLocation, InTopLocation, InDebugColor);
	}
	void RDebugWireframePrimitiveManager::DrawCustom(const EString& InCustomName, const Matrix4x4& InLocalToWorld, const Color4& InDebugColor)
	{
		TMap<EString, RDebugWireframePrimitive>& TargetPrimitives = DebugWireframeCustomPrimitives;
		TMap<EString, TArray<Matrix4x4>>& TargetTransforms = CustomPrimitiveTransforms;
		TMap<EString, TArray<Color4>>& TargetColors = CustomPrimitiveColors;

		RequireCommands[BackSlotIndex].EnqueueCommand(
			[&TargetPrimitives, &TargetTransforms, &TargetColors,
			CustomName = InCustomName,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(InLocalToWorld)]()->void
			{
				if (TargetPrimitives.ContainsKey(CustomName))
				{
					if (!(TargetTransforms.ContainsKey(CustomName)))
					{
						TargetTransforms.Add(CustomName, TArray<Matrix4x4>());
					}
					if (!(TargetColors.ContainsKey(CustomName)))
					{
						TargetColors.Add(CustomName, TArray<Color4>());
					}
					TArray<Matrix4x4>* TransformContainer = TargetTransforms.FindValueAsPtr(CustomName);
					TArray<Color4>* ColorContainer = TargetColors.FindValueAsPtr(CustomName);
					TransformContainer->Add(LocalToWorld);
					ColorContainer->Add(DebugColor);
				}
			}
		);
	}
	void RDebugWireframePrimitiveManager::CreatePrimitive(RDebugWireframeType InType)
	{
		CreatePrimitiveInternal(InType);
	}
	void RDebugWireframePrimitiveManager::CreateCustomPrimitive(const EString& InCustomName, const Vector3* InPointDatas, const UINT32 InPointNum)
	{
		RDebugWireframePrimitiveManager* Manager = this;
		Vector3* PointDatas = nullptr;
		if (InPointNum > 0u)
		{
			PointDatas = new Vector3[InPointNum];
			const UINT32 MemSize = sizeof(Vector3) * InPointNum;
			::memset(PointDatas, 0, MemSize);
			::memcpy_s(PointDatas, MemSize, InPointDatas, MemSize);
		}

		RequireCreateCustomCommands.EnqueueCommand(
			[Manager, PointDatas,
			CustomName = InCustomName,
			PointNum = InPointNum
			]()->void
			{
				Manager->CreatePrimitiveInternal(RDebugWireframeType::DEBUG_WIREFRAME_CUSTOM, &CustomName, &PointNum, PointDatas);
				if (!!PointDatas)
				{
					delete[]PointDatas;
				}
			}
		);
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
				Vector3(0.f, 0.0f, 0.f),
				Vector3(0.f, 1.0f, 0.f)
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
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CUBOID:
		{
			const Vector3 CubePoints[] =
			{
				Vector3(-0.5f, 0.5f, 0.5f),
				Vector3(0.5f, 0.5f, 0.5f),
				Vector3(0.5f, 0.5f, -0.5f),
				Vector3(-0.5f, 0.5f, -0.5f),

				Vector3(-0.5f, -0.5f, 0.5f),
				Vector3(0.5f, -0.5f, 0.5f),
				Vector3(0.5f, -0.5f, -0.5f),
				Vector3(-0.5f, -0.5f, -0.5f)
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
				7u, 4u
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
		{
			Matrix4x4 RotX(MakeMatrix4x4(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.f))));
			const Vector3 ConePoints[] =
			{
				Matrix4x4TransformPosition(RotX, Vector3(1.f, 0.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.92388f, 0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.707107f, 0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.382683f, 0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-4.37114e-08f, 1.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.382683f, 0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.707107f, 0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.92388f, 0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-1.f, -8.74228e-08f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.92388f, -0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.707107f, -0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.382684f, -0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(1.19249e-08f, -1.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.382684f, -0.923879f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.707107f, -0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.92388f, -0.3826830f, 0.f)),

				Vector3(0.f, 0.f, 0.f),
				Vector3(0.f, 1.f, 0.f)
			};
			const UINT32 ConeLines[] =
			{
				0u, 1u,
				1u, 2u,
				2u, 3u,
				3u, 4u,
				4u, 5u,
				5u, 6u,
				6u, 7u,
				7u, 8u,
				8u, 9u,
				9u, 10,
				10u, 11u,
				11u, 12u,
				12u, 13u,
				13u, 14u,
				14u, 15u,
				15u, 0u,

				16u, 17u
			};
			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(ConePoints, PE_ARRAYSIZE(ConePoints), ConeLines, PE_ARRAYSIZE(ConeLines));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_CUSTOM:
		{
			if ((!!InCustomName) && (!!InVectorNum) && (!!InVectorDatas) && ((*InCustomName).Length() > 1u) && (*InVectorNum > 0u))
			{
				const EString& CustomName = (*InCustomName);
				if (!(DebugWireframeCustomPrimitives.ContainsKey(CustomName)))
				{
					DebugWireframeCustomPrimitives.Add(CustomName, RDebugWireframePrimitive());
					RDebugWireframePrimitive* New = DebugWireframeCustomPrimitives.FindValueAsPtr(CustomName);
					Result = New->InitPrimitive(InVectorDatas, (*InVectorNum));
				}
			}
		}
		break;
		default:
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Error type for debug wireframe primitive."));
		}
		break;
		}

		{
			EString Info("Init debug wireframe primitive.");
			Info = Info + "Type : " + ToString(static_cast<UINT32>(InType));
			if (InCustomName)
			{
				Info = Info + " Custom name : " + (*InCustomName);
			}
			if (Result)
			{
				Info = Info + " success.";
				PE_LOG_LOG(Info);
			}
			else
			{
				Info = Info + " failed.";
				PE_LOG_WARN(Info);
			}
		}
	}
#endif

};
