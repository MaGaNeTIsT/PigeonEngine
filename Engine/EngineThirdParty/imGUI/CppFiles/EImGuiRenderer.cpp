#include "../Headers/EImGuiRenderer.h"

#if _EDITOR_ONLY

#include <imgui.h>
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/ShaderBindingPlan.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <Shader/ShaderEngineParameters.h>
#include <Shader/ShaderParameter.h>
#include <Shader/ShaderParameterBuffers.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/IRCommandList.h>
#include <Main/MainManager.h>
#include <RenderResourceLifetime.h>
#include <Config/EngineConfig.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	// ----------------------------------------------------------------------
	// A matrix reaches a constant buffer as it is: the shader side matrix is the
	// transpose of the cpu matrix, which is what the library's mul(Matrix, Vector)
	// expects for a row vector cpu matrix, so flattening row major is the whole
	// conversion. The helper lives in RenderCore/Shader/ShaderEngineParameters.h and
	// this is a thin forwarder so the ImGui path and the engine blocks cannot drift.
	// ----------------------------------------------------------------------
	static void FlattenUploadMatrix(const Matrix4x4& InMatrix, FLOAT* OutScalars)
	{
		FlattenShaderMatrix(InMatrix, OutScalars);
	}

	EImGuiRenderer::EImGuiRenderer()
	{
	}
	EImGuiRenderer::~EImGuiRenderer()
	{
	}

	void EImGuiRenderer::ReleaseImGuiMaterialInstance()
	{
		// Nothing is released: the instance belongs to the process wide instance cache, which keeps it
		// alive across a reload and re-points it to the rebuilt asset. This renderer only drops its
		// hold on it.
		ImGuiMaterialInstance = nullptr;
	}

	// ----------------------------------------------------------------------
	// Initialize: load the shader instance -> build the PSO. Font atlas creation
	// is deferred to first Render() (RendererHasTextures).
	// ----------------------------------------------------------------------
	BOOL8 EImGuiRenderer::Initialize()
	{
		if (bInitialized) { return TRUE; }

		if (!LoadImGuiMaterial())
		{
			// The overlay is the editor's only output, so a renderer that never comes up is
			// reported rather than left absent: this runs every frame until it succeeds, and the
			// report is deduplicated per reason.
			ReportSkippedDraw(EString(PE_TEXT("the ImGui overlay")), EString(PE_TEXT("its renderer could not load the ImGui material")));
			return FALSE;
		}
		if (!BuildPipelineState())
		{
			ReportSkippedDraw(EString(PE_TEXT("the ImGui overlay")), EString(PE_TEXT("its renderer could not build the pipeline state")));
			ReleaseImGuiMaterialInstance();
			return FALSE;
		}

		bInitialized = TRUE;
		return TRUE;
	}

	void EImGuiRenderer::ShutDown()
	{
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (Device)
		{
			ReleaseDynamicBuffers();
		}
		/*
		* What this deliberately does not release: the font atlas and the bindless index it is
		* registered under. The atlas is session scoped - ImGui builds it from its own IO once, and
		* every reload reuses it - while this call runs on each shader replacement, where the only
		* thing that went stale is what was derived from the program: the pipeline state, the two
		* blocks and the slot the atlas is bound at, all released or re-derived below.
		*
		* Releasing the texture here would destroy a device object that ImGui's already recorded draw
		* commands still name, and a backend cannot repair that mid frame: ImDrawCmd::GetTexID reads
		* the texture data's id live at replay time, so an id cleared between recording and replay
		* trips ImGui's own assertion (imgui.h) and binds nothing in a release build. The atlas goes
		* with the device instead, through ReleaseFontTexture at engine shutdown.
		*/
		// The pass's blocks own gpu buffers and mirrors sized from a program of this session, so
		// they go back with it: a later Initialize() rebuilds both from the program it finds then.
		ViewBlock.ReleaseRenderResources();
		DrawBlock.ReleaseRenderResources();
		// The pipeline state belongs to the cache, which releases it through the device
		// (deferred until the GPU is done with it) and lets the parameter layouts it
		// generated go with it. The device is handed over first so the release reaches the
		// device that created it even if the manager has already dropped its reference.
		Pipelines.SetDevice(Device);
		Pipelines.InvalidateAll();
		PipelineState = nullptr;
		// The instance is the cache's, so only this renderer's hold on it goes; what was derived from
		// its program was released above.
		ReleaseImGuiMaterialInstance();
		bInitialized          = FALSE;
		FontTextureSlot       = 0u;
		LastBoundTextureIndex = 0xFFFFFFFFu;
	}

	void EImGuiRenderer::ReleaseFontTexture()
	{
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (Device)
		{
			if (FontBindlessSRVIndex != RHI_INVALID_BINDLESS_INDEX)
			{
				Device->ReleaseBindlessIndex(FontBindlessSRVIndex);
				FontBindlessSRVIndex = RHI_INVALID_BINDLESS_INDEX;
			}
			if (FontTextureResource.Texture)
			{
				Device->DestroyResource(FontTextureResource.Texture);
				FontTextureResource = RRHITexture2D();
			}
		}
		/*
		* ImGui keeps its own view of the atlas, and a texture it believes is OK is never uploaded
		* again - so destroying the device object behind its id has to be said out loud, which is what
		* the official backends do in the same place. The id goes first: it is the one the draw list
		* ImGui already recorded names the texture by, the index behind it was released above, and the
		* slot that index names may hold anything by the next frame. SetStatus then asks for the
		* texture again (imgui.h turns Destroyed into WantCreate while the atlas still holds its
		* pixels), which this renderer answers with a fresh create and register in UpdateTexture.
		*/
		if (!ImGui::GetCurrentContext())
		{
			return;
		}
		ImTextureData* FontTextureData = ImGui::GetIO().Fonts->TexData;
		if (!FontTextureData)
		{
			return;
		}
		FontTextureData->SetTexID(ImTextureID_Invalid);
		FontTextureData->SetStatus(ImTextureStatus_Destroyed);
	}

	void EImGuiRenderer::ReleaseDynamicBuffers()
	{
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!Device) { return; }
		if (DynamicVB.Buffer)
		{
			Device->DestroyResource(DynamicVB.Buffer);
			DynamicVB = RRHIVertexBuffer();
		}
		if (DynamicIB.Buffer)
		{
			Device->DestroyResource(DynamicIB.Buffer);
			DynamicIB = RRHIIndexBuffer();
		}
		VBVertexCapacity = 0u;
		IBIndexCapacity  = 0u;
	}

	// ----------------------------------------------------------------------
	// Load the M_EngineImGui shader. A request states the material, the program and
	// the vertex axis, and the process wide instance cache assembles it into the
	// instance this renderer draws with - asked for, never created here, which is
	// also what lets a shader reload re-point it instead of stranding a private copy.
	//
	// The UI's factory is the one under Assets/ShaderSources/ImGui/, whose input
	// stream is a single interleaved ImDrawVert. The renderer owns what the shader
	// sources cannot state: the colour's normalized eight bit format in the
	// pipeline's input layout, and the per frame values of the instance whose
	// parameter set it writes.
	// ----------------------------------------------------------------------
	BOOL8 EImGuiRenderer::LoadImGuiMaterial()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName		= EName(PE_TEXT("M_EngineImGui"));
		Request.MaterialName	= EString(PE_TEXT("M_EngineImGui"));
		Request.ProgramName		= EName(PE_TEXT("ImGui"));
		Request.VertexFactory	= EShaderVertexFactoryType::IMGUI;
		// The UI stream carries exactly these three: a two component pixel position,
		// one texture coordinate and the packed vertex colour.
		Request.Profile.StreamProfile	= (SHADER_VERTEX_ATTRIBUTE_POSITION
			| SHADER_VERTEX_ATTRIBUTE_UV0 | SHADER_VERTEX_ATTRIBUTE_COLOR);
		Request.StageMask		= ENGINE_SHADER_STAGE_MASK_GRAPHICS;

		// Asked for, not resolved: the process wide instance cache is the owner, which is what makes a
		// shader reload reach this renderer at all - the cache re-points the instance this draw uses to
		// the rebuilt asset. It is also the only form that may run here: resolving would load, and this
		// renderer initializes inside a frame.
		//
		// Null means the assembly is still pending. The renderer is loaded lazily and retried every
		// frame until it succeeds, so a miss leaves it uninitialized until the instance exists.
		ImGuiMaterialInstance = REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(Request);
		if (!ImGuiMaterialInstance)
		{
			return FALSE;
		}
		const EShaderProgram* Program = ImGuiMaterialInstance->GetProgram();
		if (!Program)
		{
			// Not "the instance is missing" but "its program did not resolve": drop the hold and let the
			// next attempt ask again. The instance is the cache's, so there is nothing to hand back.
			ReleaseImGuiMaterialInstance();
			return FALSE;
		}

		// The pass's two blocks are latched from this program: it is the one whose binding plan
		// the draw binds through, so it is the one that sizes them. A program that declares
		// neither is not a UI program, and the pass cannot draw without them.
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		const BOOL32 bBlocksReady = ((Device != nullptr)
			&& (ViewBlock.EnsureInitialized(Device, *Program) != FALSE)
			&& (DrawBlock.EnsureInitialized(Device, *Program) != FALSE)) ? TRUE : FALSE;
		if (bBlocksReady == FALSE)
		{
			PE_LOG_ERROR(EString(PE_TEXT("ImGui pass blocks could not be built from the program's reflection.")));
			// Both go back, not just the one that failed: the next attempt may resolve a
			// different program, and a block latched from this one would keep its layout.
			ViewBlock.ReleaseRenderResources();
			DrawBlock.ReleaseRenderResources();
			ReleaseImGuiMaterialInstance();
			return FALSE;
		}

		// Where the font atlas lands in the pixel stage's parameter space, read from
		// the binding plan rather than assumed: a material's textures are packed from
		// register zero, so a slot written down here would bind a different register
		// the moment the program's component list changed.
		FontTextureSlot = 0u;
		const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*Program, EShaderStageType::SHADER_STAGE_PIXEL);
		if (PixelEntry)
		{
			const EShaderResourceBinding* FontBinding = PixelEntry->BindingPlan.FindResourceBinding(EName(PE_TEXT("FontTexture")));
			if (FontBinding)
			{
				FontTextureSlot = FontBinding->RegisterSlot;
			}
		}

		// The draw transform is the identity and stays the identity, so it is written
		// once and uploaded once; the view projection is written per frame, because the
		// window can be resized.
		SetIdentityDrawParams();
		DrawBlock.Upload(Device);

		// The instance's own parameter buffers have no GPU storage until something asks
		// for them: Prepare evaluates the reflection defaults and the values written
		// so far and uploads the result.
		ImGuiMaterialInstance->Prepare();
		return TRUE;
	}

	// ----------------------------------------------------------------------
	// The UI's factory places the UI through the engine's world matrix, so the draw
	// transform is published as the identity. Leaving those fields at their
	// reflection default of zero would collapse every UI vertex into one point.
	// ----------------------------------------------------------------------
	void EImGuiRenderer::SetIdentityDrawParams()
	{
		if (!DrawBlock.IsInitialized())
		{
			return;
		}
		FLOAT Identity[16] = {};
		FlattenUploadMatrix(Matrix4x4::Identity(), Identity);
		EShaderParameterSet& Parameters = DrawBlock.GetParameters();
		// The block is the pass's own, so the names come from it and from the engine's field
		// table: nothing here spells a block or a field name out a second time.
		const EName& BlockName = DrawBlock.GetBlockName();
		if ((!Parameters.SetFloatArray(BlockName, EName(SHADER_FIELD_WORLD_MATRIX), Identity, 16u))
			|| (!Parameters.SetFloatArray(BlockName, EName(SHADER_FIELD_WORLD_INV_TRANSPOSE_MATRIX), Identity, 16u)))
		{
			PE_LOG_WARN(EString(PE_TEXT("ImGui draw block rejected the identity draw transform.")));
		}
	}

	// ----------------------------------------------------------------------
	// PSO: imgui-tuned blend (SRC_ALPHA/INV_SRC_ALPHA), depth disabled,
	// scissor enabled, cull NONE. RT format = backbuffer (R8G8B8A8_UNORM).
	//
	// The desc names the program and the one vertex element whose stored bytes differ
	// from what the shader reads. The cache pairs the two, generates the program's
	// parameter layout and owns the state it creates.
	// ----------------------------------------------------------------------
	BOOL8 EImGuiRenderer::BuildPipelineState()
	{
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!Device) || (!ImGuiMaterialInstance))
		{
			return FALSE;
		}

		const EShaderProgram* Program = ImGuiMaterialInstance->GetProgram();
		if (!Program)
		{
			return FALSE;
		}
		Pipelines.SetDevice(Device);

		// One interleaved ImDrawVert, and its colour is the only element whose data layout
		// differs from the shader's declaration: four normalized bytes (R8G8B8A8_UNORM,
		// converted into the float4 the shader reads) where the shader assumes four floats.
		// Naming that element is enough -- every element this array does not mention keeps
		// the shader's own -- and no offset is given, so the element appends where the
		// packed colour actually sits, at byte 16 of the 20 byte stride.
		static const RInputLayoutDesc ImGuiDeclaredElements[] =
		{
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_COLOR
				, 1u, 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM, 0u),
		};

		RGraphicsPipelineStateDesc Desc;
		Desc.Program			= Program;
		Desc.InputLayouts		= ImGuiDeclaredElements;
		Desc.InputLayoutCount	= PE_ARRAYSIZE(ImGuiDeclaredElements);
		// ImGui: no backface culling (UI quads may be CW or CCW depending on
		// scissor-flip), wireframe disabled, scissor honored per draw.
		Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_SOLID);
		// Premultiplied-alpha-friendly straight-alpha blend, matches the
		// reference imgui_impl_dx11/dx12 backend defaults.
		Desc.Blend					= RBlendState(
			RBlendOptionType::BLEND_SRC_ALPHA, RBlendOptionType::BLEND_INV_SRC_ALPHA, RBlendOperationType::BLEND_OP_ADD,
			RBlendOptionType::BLEND_ONE,       RBlendOptionType::BLEND_INV_SRC_ALPHA, RBlendOperationType::BLEND_OP_ADD,
			RColorWriteMaskType::COLOR_WRITE_MASK_ALL, TRUE);
		// UI doesn't participate in depth - disable test+write, leave stencil off.
		Desc.Depth					= RDepthState(RComparisonFunctionType::COMPARISON_ALWAYS, RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO, FALSE);
		Desc.Stencil				= RStencilState(
			0xffu, 0xffu,
			RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
			RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
			FALSE);
		Desc.Topology				= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		Desc.RenderTargetCount		= 1u;
		Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R8G8B8A8_UNORM;	// backbuffer
		Desc.DepthBits				= 0u;									// no depth attachment in ImGui pass
		Desc.HasStencil				= FALSE;

		// Null when a declared element cannot feed the shader input it names, or when the
		// back-end refused the create. The cache names the element it could not honor in
		// the log, so a failure here is a pass that does not draw rather than a silent
		// wrong picture.
		PipelineState = Pipelines.GetOrCreateGraphics(Desc);
		return ((PipelineState != nullptr) ? TRUE : FALSE);
	}

	// ----------------------------------------------------------------------
	// Process a single ImTextureData create/destroy/update request from the
	// RendererHasTextures pipeline. Called once per texture at the top of Render.
	// WantUpdates (partial update) is not yet supported; ImGui falls back to
	// full rebuild when we leave the status untouched.
	// ----------------------------------------------------------------------
	void EImGuiRenderer::UpdateTexture(::ImTextureData* Tex)
	{
		if (!Tex) { return; }

		if (Tex->Status == ImTextureStatus_WantCreate)
		{
			IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
			if (!Device) { return; }

				if (Tex->Pixels && Tex->Width > 0 && Tex->Height > 0)
				{
					const UINT32 RowPitch   = static_cast<UINT32>(Tex->Width) * 4u;
					const UINT32 SlicePitch = RowPitch * static_cast<UINT32>(Tex->Height);
					RRHISubresourceData SubData(static_cast<const UINT8*>(Tex->Pixels), SlicePitch, RowPitch, SlicePitch);
					RTextureDesc Desc(
						static_cast<UINT32>(Tex->Width), static_cast<UINT32>(Tex->Height),
						RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_R8G8B8A8_UNORM);
					RRHITexture2D NewTex;
					if (Device->CreateTexture2DResource(Desc, Color4(0.f, 0.f, 0.f, 0.f), SubData, NewTex) && NewTex.Texture)
					{
						UINT32 NewIdx = Device->RegisterBindlessSRV(NewTex.Texture);
						Tex->SetTexID(static_cast<ImTextureID>(NewIdx + 1u));
						if (Tex == ImGui::GetIO().Fonts->TexData)
						{
							if (FontTextureResource.Texture)
								Device->DestroyResource(FontTextureResource.Texture);
							// The index the replaced texture was registered under goes back as well: an
							// index whose slot still names a destroyed texture is a leak of the slot and a
							// live looking pointer to a dead object at the same time, and a texture id that
							// still names it would resolve to exactly that.
							if (FontBindlessSRVIndex != RHI_INVALID_BINDLESS_INDEX)
							{
								Device->ReleaseBindlessIndex(FontBindlessSRVIndex);
							}
							FontTextureResource  = NewTex;
							FontBindlessSRVIndex = NewIdx;
						}
						Tex->SetStatus(ImTextureStatus_OK);
					}
				}
			else if (Tex == ImGui::GetIO().Fonts->TexData && FontBindlessSRVIndex != RHI_INVALID_BINDLESS_INDEX)
			{
				Tex->SetTexID(static_cast<ImTextureID>(FontBindlessSRVIndex + 1u));
				Tex->SetStatus(ImTextureStatus_OK);
			}
		}
		else if (Tex->Status == ImTextureStatus_WantDestroy)
		{
			Tex->SetStatus(ImTextureStatus_Destroyed);
		}
		else if (Tex->Status == ImTextureStatus_WantUpdates)
		{
			static UINT32 WantUpdatesCount = 0u;
			if ((++WantUpdatesCount & 0xFFu) == 0u)
			{
				PE_LOG_WARN(EString("ImGui partial update (WantUpdates) not yet supported; ImGui falling back to full rebuild. Count: ") + EString::FromInt(WantUpdatesCount));
			}
		}
	}

	// ----------------------------------------------------------------------
	// Grow VB/IB if the next frame needs more vertices/indices than the
	// current capacity. Old buffers go through Device->DestroyResource so
	// the deferred-delete queue holds them until their fence retires.
	// ----------------------------------------------------------------------
	BOOL8 EImGuiRenderer::EnsureBuffers(UINT32 InVertexNum, UINT32 InIndexNum)
	{
		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!Device) { return FALSE; }

		// Both dynamic buffers start empty and are filled per frame through
		// UploadBuffer, so the factories are handed an empty initial payload.
		const TArray<UINT8, UINT64> EmptyInitialData;
		if (InVertexNum > VBVertexCapacity)
		{
			if (DynamicVB.Buffer)
			{
				Device->DestroyResource(DynamicVB.Buffer);
				DynamicVB = RRHIVertexBuffer();
			}
			const UINT32 NewCap = InVertexNum + 5000u;
			if (!Device->CreateVertexBufferResource(EmptyInitialData, NewCap, static_cast<UINT32>(sizeof(ImDrawVert)), DynamicVB))
			{
				return FALSE;
			}
			VBVertexCapacity = NewCap;
		}
		if (InIndexNum > IBIndexCapacity)
		{
			if (DynamicIB.Buffer)
			{
				Device->DestroyResource(DynamicIB.Buffer);
				DynamicIB = RRHIIndexBuffer();
			}
			const UINT32 NewCap = InIndexNum + 10000u;
			const ERHIIndexFormat IdxFmt = (sizeof(ImDrawIdx) == 2u)
				? ERHIIndexFormat::RHI_INDEX_FORMAT_UINT16
				: ERHIIndexFormat::RHI_INDEX_FORMAT_UINT32;
			if (!Device->CreateIndexBufferResource(EmptyInitialData, NewCap, IdxFmt, DynamicIB))
			{
				return FALSE;
			}
			IBIndexCapacity = NewCap;
		}
		return TRUE;
	}

	// ----------------------------------------------------------------------
	// Build the orthographic projection from ImDrawData and publish it as the pass's
	// view projection, which is where the engine keeps the world to clip step for
	// every factory. ImGui's coordinate system is Y-down with origin at DisplayPos,
	// so the projection collapses to:
	//
	//   x' = (x - DisplayPos.x) * 2/Width - 1
	//   y' = 1 - (y - DisplayPos.y) * 2/Height
	//
	// which gives the row-major matrix below. The engine's upload convention and the
	// library's clip transform then yield clip space directly.
	//
	// The value is keyed by (block name, field name): ViewParams is the block the
	// system layer declares and ViewProjectionMatrix the field the factory reads, and
	// the pass's own ViewBlock carries both. The rest of the block keeps its reflection
	// defaults, which the UI's material does not read.
	// ----------------------------------------------------------------------
	void EImGuiRenderer::UpdateViewParams(ImDrawData* InDrawData)
	{
		if ((!ViewBlock.IsInitialized()) || (!InDrawData))
		{
			return;
		}
		const FLOAT L = InDrawData->DisplayPos.x;
		const FLOAT R = InDrawData->DisplayPos.x + InDrawData->DisplaySize.x;
		const FLOAT T = InDrawData->DisplayPos.y;
		const FLOAT B = InDrawData->DisplayPos.y + InDrawData->DisplaySize.y;
		const Matrix4x4 Proj(
			2.f / (R - L),    0.f,              0.f, 0.f,
			0.f,              2.f / (T - B),    0.f, 0.f,
			0.f,              0.f,              0.5f, 0.f,
			(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.f);
		FLOAT UploadValue[16] = {};
		FlattenUploadMatrix(Proj, UploadValue);

		EShaderParameterSet& Parameters = ViewBlock.GetParameters();
		if (!Parameters.SetFloatArray(ViewBlock.GetBlockName(), EName(SHADER_FIELD_VIEW_PROJECTION_MATRIX), UploadValue, 16u))
		{
			PE_LOG_WARN(EString(PE_TEXT("ImGui view block rejected the view projection matrix.")));
		}
	}

	// ----------------------------------------------------------------------
	// Per-frame draw replay. Caller has already opened the render pass.
	// ----------------------------------------------------------------------
	void EImGuiRenderer::Render(IRCommandList* InCmd, ImDrawData* InDrawData)
	{
		if (!bInitialized || !InCmd || !InDrawData) { return; }
		if (InDrawData->TotalVtxCount <= 0 || InDrawData->TotalIdxCount <= 0) { return; }

		IRRHIDevice* Device = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!Device) || (!PipelineState) || (!ImGuiMaterialInstance))
		{
			// The overlay is the editor's only output, so not drawing it is the most visible
			// absence there is. It is normally one or two frames at start up, before the pass's
			// state and material are built - which is why the report is deduplicated rather than
			// printed per frame.
			ReportSkippedDraw(EString(PE_TEXT("the ImGui overlay")), EString(PE_TEXT("its pipeline state or its material instance is not ready")));
			return;
		}
		const EShaderProgram* Program = ImGuiMaterialInstance->GetProgram();
		if (!Program)
		{
			// The instance is held, so this is not "not built yet": its asset has no usable program,
			// and the UI would stay missing for as long as that lasts.
			ReportSkippedDraw(EString(PE_TEXT("the ImGui overlay")), EString(PE_TEXT("its material instance resolved to no usable program")));
			return;
		}

		// ImGui 1.92+ RendererHasTextures: process pending texture create/destroy
		// requests before issuing any draw calls.
		if (InDrawData->Textures)
		{
			for (ImTextureData* Tex : *InDrawData->Textures)
				UpdateTexture(Tex);
		}

		// Grow / allocate VB/IB on demand. Failure here means we can't draw
		// this frame; silently bail (UI will reappear next frame if alloc
		// succeeds).
		if (!EnsureBuffers(
			static_cast<UINT32>(InDrawData->TotalVtxCount),
			static_cast<UINT32>(InDrawData->TotalIdxCount)))
		{
			return;
		}

		// Upload merged VB/IB into a temporary CPU buffer, then push it to the
		// GPU buffer in one call. ImDrawList::VtxBuffer is contiguous so we
		// can memcpy each list into the running offset.
		{
			const SIZE_T VBSizeBytes = static_cast<SIZE_T>(InDrawData->TotalVtxCount) * sizeof(ImDrawVert);
			const SIZE_T IBSizeBytes = static_cast<SIZE_T>(InDrawData->TotalIdxCount) * sizeof(ImDrawIdx);

			// Stage in heap memory then UploadBuffer in one shot per buffer.
			// Heap-alloc per-frame is fine - total size is small (KBs) and
			// the alternative (Map/Discard on the engine buffer wrapper)
			// doesn't have a unified RHI API yet.
			BYTE* VBStaging = new BYTE[VBSizeBytes];
			BYTE* IBStaging = new BYTE[IBSizeBytes];
			SIZE_T VBOffset = 0u;
			SIZE_T IBOffset = 0u;
			for (INT32 i = 0; i < InDrawData->CmdListsCount; i++)
			{
				const ImDrawList* List = InDrawData->CmdLists[i];
				const SIZE_T VBBytes = static_cast<SIZE_T>(List->VtxBuffer.Size) * sizeof(ImDrawVert);
				const SIZE_T IBBytes = static_cast<SIZE_T>(List->IdxBuffer.Size) * sizeof(ImDrawIdx);
				::memcpy(VBStaging + VBOffset, List->VtxBuffer.Data, VBBytes);
				::memcpy(IBStaging + IBOffset, List->IdxBuffer.Data, IBBytes);
				VBOffset += VBBytes;
				IBOffset += IBBytes;
			}
			Device->UploadBuffer(DynamicVB.Buffer, VBStaging, VBSizeBytes, 0u);
			Device->UploadBuffer(DynamicIB.Buffer, IBStaging, IBSizeBytes, 0u);
			delete[] VBStaging;
			delete[] IBStaging;
		}

		// View projection - written into the pass's view block, then re-evaluated and pushed into
		// its gpu buffers. The draw transform was published once at load time and never changes,
		// so its upload is a no-op from the second frame on. The instance's own values are the
		// material's, and a material of this pass may declare blocks of its own.
		UpdateViewParams(InDrawData);
		ViewBlock.Upload(Device);
		DrawBlock.Upload(Device);
		ImGuiMaterialInstance->Prepare();

		// Pipeline + viewport (covering the full draw area). The pipeline state goes first: it
		// carries the program's own root signature, and switching a signature voids every root
		// binding made before it, so a material block bound ahead of it would be lost.
		InCmd->SetPipelineState(PipelineState);
		const EViewport VP(0.f, 0.f
			, InDrawData->DisplaySize.x * InDrawData->FramebufferScale.x
			, InDrawData->DisplaySize.y * InDrawData->FramebufferScale.y
			, 0.f, 1.f);
		InCmd->SetViewports(&VP, 1u);

		// The material's parameter blocks land where the binding plan of each stage
		// reports; the plan is built from the same reflection the buffers were sized
		// from, so no root index is computed here. The buffers are the instance's own
		// render side objects: the instance only carries the reference, so the first
		// frame after a load can legitimately have none yet.
		RShaderParameterBuffers* ParameterBuffers = ResolveShaderParameterBuffers(*ImGuiMaterialInstance);
		if (ParameterBuffers)
		{
			ParameterBuffers->UploadFromParameterSet(EMainManager::GetManagerSingleton()->GetRenderDevice()
				, ImGuiMaterialInstance->GetParameters());
			BindShaderParameterBuffers(InCmd, *ParameterBuffers
				, *Program, EShaderStageType::SHADER_STAGE_VERTEX, FALSE);
			BindShaderParameterBuffers(InCmd, *ParameterBuffers
				, *Program, EShaderStageType::SHADER_STAGE_PIXEL, FALSE);
		}

		// The pass's own two blocks: the UI's view projection and the identity draw transform.
		// Each bind writes the roots of the block it publishes and nothing else, so the order
		// against the material bind above is a statement of intent rather than a requirement.
		ViewBlock.Bind(InCmd, *Program);
		DrawBlock.Bind(InCmd, *Program);

		// VB/IB binding once for the whole frame - every ImDrawCmd indexes
		// into the same merged buffers using IdxOffset / VtxOffset.
		IRRHIBuffer* VB = DynamicVB.Buffer;
		const UINT32 Stride = static_cast<UINT32>(sizeof(ImDrawVert));
		const UINT32 Offset = 0u;
		InCmd->SetVertexBuffers(0u, 1u, &VB, &Stride, &Offset);
		InCmd->SetIndexBuffer(DynamicIB.Buffer, DynamicIB.Format, 0u);

		// Force the first ImDrawCmd's texture-change branch to fire, so the first
		// SRV bind happens unconditionally instead of depending on the id the first
		// command happens to carry.
		LastBoundTextureIndex = 0xFFFFFFFFu;

		// Replay the merged draw list. clip_off / clip_scale are needed
		// because ImGui may run in a sub-rect of the framebuffer (multi-viewport).
		const ImVec2 ClipOff   = InDrawData->DisplayPos;
		const ImVec2 ClipScale = InDrawData->FramebufferScale;
		INT32 GlobalIdxOffset = 0;
		INT32 GlobalVtxOffset = 0;
		for (INT32 ListIdx = 0; ListIdx < InDrawData->CmdListsCount; ListIdx++)
		{
			const ImDrawList* List = InDrawData->CmdLists[ListIdx];
			for (INT32 CmdIdx = 0; CmdIdx < List->CmdBuffer.Size; CmdIdx++)
			{
				const ImDrawCmd& Cmd = List->CmdBuffer[CmdIdx];
				if (Cmd.UserCallback != nullptr)
				{
					Cmd.UserCallback(List, &Cmd);
					continue;
				}

				// Scissor in framebuffer pixels.
				const FLOAT ClipMinX = (Cmd.ClipRect.x - ClipOff.x) * ClipScale.x;
				const FLOAT ClipMinY = (Cmd.ClipRect.y - ClipOff.y) * ClipScale.y;
				const FLOAT ClipMaxX = (Cmd.ClipRect.z - ClipOff.x) * ClipScale.x;
				const FLOAT ClipMaxY = (Cmd.ClipRect.w - ClipOff.y) * ClipScale.y;
				if (ClipMaxX <= ClipMinX || ClipMaxY <= ClipMinY) { continue; }
				const ERect Scissor(
					ClipMinX, ClipMinY,
					ClipMaxX, ClipMaxY);
				InCmd->SetScissorRects(&Scissor, 1u);

				// Texture switch. ImTextureID stores (RealBindlessIdx + 1), and the
				// texture is bound at the register the pixel stage's binding plan reports
				// for FontTexture: one call that each back end maps to its own mechanism
				// (a descriptor table slot on D3D12, a plain SRV slot on D3D11), so
				// nothing here has to know which one is active.
				const UINT32 TexIdxImGui = static_cast<UINT32>(Cmd.GetTexID());
				if (TexIdxImGui != LastBoundTextureIndex)
				{
					LastBoundTextureIndex = TexIdxImGui;
					// ImTextureID_Invalid (0) is what a texture that is gone reports, and it names no
					// slot: resolving it would look up index 0, which is a real slot on the back ends
					// that number their bindless region from zero. The previous binding stays, which is
					// the atlas in every frame ImGui records by itself.
					if (TexIdxImGui > 0u)
					{
						const UINT32 TexIdx = TexIdxImGui - 1u;
						IRRHIResource* Resource = Device->GetBindlessResource(TexIdx);
						IRRHITexture*  Texture  = static_cast<IRRHITexture*>(Resource);
						if (Texture)
						{
							InCmd->SetShaderResourceView(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, FontTextureSlot, Texture);
						}
						else
						{
							// A live id that names no resource: the texture behind it was released or
							// replaced while this frame's draw list still named the old index. The draw
							// is not skipped - only this bind - and the line names the id, because what
							// has to be found is who kept it after its texture went away.
							ReportSkippedDraw(EString(PE_TEXT("the ImGui overlay"))
								, EString(PE_TEXT("texture id ")) + EString::FromInt(TexIdxImGui) + EString(PE_TEXT(" names no live texture")));
						}
					}
				}

				InCmd->DrawIndexed(
					Cmd.ElemCount, 1u,
					static_cast<UINT32>(Cmd.IdxOffset) + static_cast<UINT32>(GlobalIdxOffset),
					static_cast<INT32>(Cmd.VtxOffset)  + GlobalVtxOffset,
					0u);
			}
			GlobalIdxOffset += List->IdxBuffer.Size;
			GlobalVtxOffset += List->VtxBuffer.Size;
		}
	}

};

#endif	// _EDITOR_ONLY
