#include "MeshSceneProxy.h"

#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <RHI/IRCommandList.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RMeshSceneProxy, RPrimitiveSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EVertexResourceType TranslateInputLayoutToVertexResource(const RInputLayoutDesc& InLayout, UINT32& OutSlot)
	{
		RShaderSemanticType ShaderSemantic = GetShaderSemanticBaseType(InLayout.SemanticName);
		OutSlot = GetShaderSemanticTypeSlot(InLayout.SemanticName);
		EVertexLayoutType VertexLayout = TranslateSemanticBaseTypeToVertexBaseLayout(ShaderSemantic);
		return (TranslateVertexLayoutTypeToVertexResourceType(VertexLayout));
	}

// ---------------------------------------------------------------------------
// The engine fallback material
// ---------------------------------------------------------------------------

const EShaderInstance* GetFallbackMeshShader(const EMesh* InMesh)
{
	if (!InMesh)
	{
		return nullptr;
	}

	// The axis comes from the mesh layout, so a static and a skinned mesh resolve to the
	// same asset with different variants rather than to two factories.
	EShaderVertexFactoryType VertexFactory = EShaderVertexFactoryType::ENGINE_MESH;
	EShaderVertexProfile Profile;
	if (!EShaderJobBuilder::ResolveVertexAxis(InMesh, VertexFactory, Profile))
	{
		return nullptr;
	}

	// What a mesh with no material of its own is drawn with. A mesh whose material failed to
	// assemble does not arrive here: its material was resolved to the error material on the game
	// thread, so the slot it reports already holds that instance and the draw is marked without
	// this function being involved. What is left here is the quiet case - nothing was assigned -
	// so the fallback is the quiet material, and M_EngineError stays reserved for faults.
	EShaderAssemblyRequest Request;
	Request.ShaderName = EName(PE_TEXT("M_EngineFallback"));
	Request.MaterialName = EString(PE_TEXT("M_EngineFallback"));
	Request.ProgramName = EName(PE_TEXT("BasePass"));
	Request.VertexFactory = VertexFactory;
	Request.Profile = Profile;

	// The cache hands out one instance per resolved request, so every mesh whose layout
	// resolves to the same axis shares the same fallback material.
	//
	// RequireInstance rather than FindOrCreate, and that choice is the point of the API split:
	// this runs inside a draw, and on a cold start the fallback is usually not assembled yet.
	// The request is recorded for the load thread and this primitive is simply not drawn this
	// frame; blocking the render thread on a file read or a shader compile - which is what the
	// old call did - is exactly what the render thread may never do.
	return REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(Request);
}


	// ---------------------------------------------------------------------------
	// RMeshSceneProxy
	// ---------------------------------------------------------------------------

	RMeshSceneProxy::RMeshSceneProxy()
	{
	}
	RMeshSceneProxy::RMeshSceneProxy(const RMeshSceneProxy& Other)
		: RPrimitiveSceneProxy(Other)
	{
	}
	RMeshSceneProxy::~RMeshSceneProxy()
	{
	}

	const EShaderInstance* RMeshSceneProxy::ResolveFallbackInstance(const UINT32 InSlotIndex)const
	{
		(void)InSlotIndex;
		return GetFallbackMeshShader(GetMeshColdData());
	}
	const RInputLayoutDesc* RMeshSceneProxy::GetPipelineStateInputLayouts(UINT32& OutCount)const
	{
		// The skin index stream is UINT16 x 4 bytes in the vertex data while the shader
		// declares uint4, so the back-end has to be told the real format. Without this it
		// reads 16 bytes out of an 8 byte stride: every element would mix this vertex with
		// the next one, and the last vertex could read past the end of the stream.
		//
		// The weight stream needs no declaration: 32 bit float already matches the float4 the
		// shader declares. Only the elements whose real layout differs belong here, because a
		// declared element the shader has no input for makes the pipeline refuse to build.
		static const RInputLayoutDesc SkinInputLayouts[] =
		{
			RInputLayoutDesc(SHADER_SEMANTIC_BLENDINDICES0, 2u, 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT)
		};

		if (!HasSkinVertexStreams())
		{
			OutCount = 0u;
			return nullptr;
		}
		OutCount = PE_ARRAYSIZE(SkinInputLayouts);
		return SkinInputLayouts;
	}
	UINT32 RMeshSceneProxy::GetDrawGroupNum()const
	{
		const EMesh* Mesh = GetMeshColdData();
		if (!Mesh)
		{
			return 0u;
		}
		const UINT32 SubmeshNum = Mesh->GetSubmeshPart().Num<UINT32>();
		// A mesh with no submesh table is one implicit submesh covering the whole index
		// buffer, which is the single submesh semantics the importers follow.
		return ((SubmeshNum > 0u) ? SubmeshNum : 1u);
	}
	BOOL32 RMeshSceneProxy::GetDrawGroup(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex)const
	{
		const EMesh* Mesh = GetMeshColdData();
		const EMeshRenderResource* MeshRenderResource = GetMeshRenderResource();
		if ((!Mesh) || (!MeshRenderResource))
		{
			return FALSE;
		}
		const RIndexBufferResource* IndexRenderResource = MeshRenderResource->GetIndexRenderResource();
		if (!IndexRenderResource)
		{
			return FALSE;
		}

		const EMesh::ESubmeshPart& Submeshes = Mesh->GetSubmeshPart();
		if (Submeshes.Num<UINT32>() == 0u)
		{
			if (InGroupIndex != 0u)
			{
				return FALSE;
			}
			OutSlotIndex	= 0u;
			OutIndexCount	= IndexRenderResource->IndexCount;
			OutStartIndex	= 0u;
			return TRUE;
		}
		if (InGroupIndex >= Submeshes.Num<UINT32>())
		{
			return FALSE;
		}

		const ESubmeshData& Submesh = Submeshes[InGroupIndex];
		// A submesh past the last material slot reuses slot 0, which is the engine's fallback
		// rule for a mesh whose submesh count exceeds its authored materials.
		const UINT32 SlotNum = Assets.GetMaterialSlotNum();
		OutSlotIndex	= ((InGroupIndex < SlotNum) ? InGroupIndex : 0u);
		OutIndexCount	= Submesh.IndexNum;
		OutStartIndex	= Submesh.StartIndex;
		return TRUE;
	}
	BOOL32 RMeshSceneProxy::BindGeometry(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const
	{
		if ((!InCommandList) || (!InSlot.Program))
		{
			return FALSE;
		}
		const RShaderStageResource*			VertexStage			= ResolveShaderStageResource(*InSlot.Program, EShaderStageType::SHADER_STAGE_VERTEX);
		const EMeshRenderResource*			MeshRenderResource	= GetMeshRenderResource();
		if ((!VertexStage) || (!MeshRenderResource))
		{
			if (!VertexStage)
			{
				// The program's vertex stage has no render resource. On a cold start this is what a
				// stage asset that was replaced under its own name leaves behind: this program still
				// points at the object whose device shader was retired, and the primitive simply
				// stops being drawn. Naming the stage and the program is what makes that readable.
				//
				// The name is read from the entry and never through the entry's stage asset pointer:
				// this branch is reached exactly when that asset was replaced under its own name, and
				// by then the object behind the pointer has been released. Reading its name is what
				// made this report abort instead of print - the freed string reports a garbage length
				// and the allocation its copy asks for fails.
				const EShaderStageEntry* VertexEntry = ResolveShaderStageEntry(*InSlot.Program, EShaderStageType::SHADER_STAGE_VERTEX);
				const EString StageAssetName = ((VertexEntry && (VertexEntry->StageAssetName.Length() > 0u))
					? VertexEntry->StageAssetName : EString(PE_TEXT("unknown")));
				ReportSkippedDraw(InSlot.PassName, EString(PE_TEXT("the vertex stage '")) + StageAssetName
					+ EString(PE_TEXT("' of program '")) + InSlot.Program->GetShaderName().ToString()
					+ EString(PE_TEXT("' has no render resource")));
			}
			else if (GetMeshColdData())
			{
				// The mesh asset is there and its render resource is not, which is not the "asset
				// still loading" state: a mesh with no cold data returns above without a report.
				ReportSkippedDraw(InSlot.PassName, EString(PE_TEXT("its mesh has no render resource")));
			}
			return FALSE;
		}
		const RIndexBufferResource*			IndexRenderResource	= MeshRenderResource->GetIndexRenderResource();
		if ((!IndexRenderResource) || (!IndexRenderResource->IsRenderResourceValid()))
		{
			ReportSkippedDraw(InSlot.PassName, EString(PE_TEXT("its mesh has no usable index buffer")));
			return FALSE;
		}
		{
			const ERHIIndexFormat IndexFormat = IndexRenderResource->UseShort
				? ERHIIndexFormat::RHI_INDEX_FORMAT_UINT16
				: ERHIIndexFormat::RHI_INDEX_FORMAT_UINT32;
			InCommandList->SetIndexBuffer(
				const_cast<RIndexBufferResource*>(IndexRenderResource)->GetResolvedRHIBuffer(),
				IndexFormat, 0u);
		}

		// The shader's input layout decides which streams to bind and at which slot. Binding
		// by the array index instead would shift every stream that follows a hole into the
		// wrong slot, and the optional skin streams are exactly such a hole.
		const TArray<RInputLayoutDesc>& Layouts = VertexStage->GetInputLayouts();
		for (UINT32 LayoutIndex = 0u, LayoutNum = Layouts.Num<UINT32>(); LayoutIndex < LayoutNum; LayoutIndex++)
		{
			const RInputLayoutDesc& Layout = Layouts[LayoutIndex];

			const BOOL32 IsSkinIndex = ((Layout.SemanticName >= RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES)
				&& (Layout.SemanticName <= RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES15));
			const BOOL32 IsSkinWeight = ((Layout.SemanticName >= RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT)
				&& (Layout.SemanticName <= RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT15));

			const RVertexBufferResource* VertexRenderResource = nullptr;
			if (IsSkinIndex || IsSkinWeight)
			{
				VertexRenderResource = GetSkinVertexStream(IsSkinIndex);
			}
			else
			{
				UINT32									ShaderSemanticSlot		= 0u;
				const EVertexResourceType				VertexResource			= TranslateInputLayoutToVertexResource(Layout, ShaderSemanticSlot);
				const TArray<RVertexBufferResource*>*	VertexRenderResources	= MeshRenderResource->GetVertexRenderResource(VertexResource);
				if ((VertexRenderResources) && (ShaderSemanticSlot < VertexRenderResources->Num<UINT32>()))
				{
					VertexRenderResource = (*VertexRenderResources)[ShaderSemanticSlot];
				}
			}
			if ((!VertexRenderResource) || (!VertexRenderResource->IsRenderResourceValid()))
			{
				// An optional stream the mesh does not carry is skipped rather than filled: the
				// layout slot stays unbound and the shader was compiled for exactly that.
				continue;
			}
			IRRHIBuffer* VertexBuffer = const_cast<RVertexBufferResource*>(VertexRenderResource)->GetResolvedRHIBuffer();
			const UINT32 Stride = VertexRenderResource->Stride;
			const UINT32 Offset = 0u;
			InCommandList->SetVertexBuffers(Layout.InputSlot, 1u, &VertexBuffer, &Stride, &Offset);
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Hooks: "no mesh" by default, so an unmigrated mesh proxy draws nothing.
	// ---------------------------------------------------------------------------

	const EMesh* RMeshSceneProxy::GetMeshColdData()const
	{
		return nullptr;
	}
	const EMeshRenderResource* RMeshSceneProxy::GetMeshRenderResource()const
	{
		return nullptr;
	}
	const RVertexBufferResource* RMeshSceneProxy::GetSkinVertexStream(const BOOL32 InIsIndexStream)const
	{
		(void)InIsIndexStream;
		return nullptr;
	}
	BOOL32 RMeshSceneProxy::HasSkinVertexStreams()const
	{
		return FALSE;
	}

} // namespace PigeonEngine
