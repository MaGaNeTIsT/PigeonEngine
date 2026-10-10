#include "SkeletalMeshSceneProxy.h"

#include <ShaderAsset/ShaderInstance.h>
#include <Shader/ShaderAssetBinding.h>
#include <MeshAsset/MeshAsset.h>
#include <RHI/IRCommandList.h>
#include <RenderProxy/PrimitiveSceneProxyFactory.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSkeletalMeshSceneProxy, RMeshSceneProxy>();
		// The render thread builds this proxy itself, inside the frame's add command, so the
		// component never hands a game thread object across the boundary. The name is the one
		// PSkeletalMeshComponent reports through GetRenderProxyTypeName.
		RPrimitiveSceneProxyFactory::Register(EName(PE_TEXT("SkeletalMeshPrimitive"))
			, []() -> RPrimitiveSceneProxy* { return new RSkeletalMeshSceneProxy(); });
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy()
		: MeshAsset(nullptr), SkeletonAsset(nullptr)
	{
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(const PSkeletalMeshComponent* InComponent)
		: MeshAsset(nullptr), SkeletonAsset(nullptr)
	{
		// The component is not stored: everything this proxy needs arrives through the snapshot,
		// so no game thread object is reachable from here. The argument stays so the create path
		// keeps its meaning while the scene finishes moving onto the factory.
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create skeletal mesh scene proxy failed"), (!!InComponent));
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(const RSkeletalMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), MeshAsset(Other.MeshAsset), SkeletonAsset(Other.SkeletonAsset)
	{
	}
	RSkeletalMeshSceneProxy::~RSkeletalMeshSceneProxy()
	{
	}
	BOOL32 RSkeletalMeshSceneProxy::IsRenderValid()const
	{
		//TODO
		return TRUE;
	}
	void RSkeletalMeshSceneProxy::ApplyAssets(const FPrimitiveRenderAssetReport& InAssets)
	{
		MeshAsset = static_cast<const ESkinnedMeshAsset*>(InAssets.GetMeshAsset());
		SkeletonAsset = static_cast<const ESkeletonAsset*>(InAssets.GetSkeletonAsset());

		// The bone buffers are sized from the mesh's bind pose count, which is an asset level
		// fact, so it is settled here rather than per frame.
		if (MeshAsset)
		{
			const ESkinnedMesh* SkinnedMesh = MeshAsset->GetStoragedResource();
			if (SkinnedMesh)
			{
				const INT32 BoneNum = SkinnedMesh->GetBindPoseValue().Num();
				if (BoneNum > 0)
				{
					SkeletonRenderResource.SetBoneNum(static_cast<UINT32>(BoneNum));
					if (BoneNum != BoneValues.Num<INT32>())
					{
						BoneValues.SetNum(BoneNum);
					}
				}
			}
		}

		// The base half stores the report itself and rebuilds the pass slots, which is also what
		// resolves the program the skin variant compiles to.
		RMeshSceneProxy::ApplyAssets(InAssets);
	}

	// ---------------------------------------------------------------------------
	// The shared mesh draw path's view of this primitive
	// ---------------------------------------------------------------------------

	const EMesh* RSkeletalMeshSceneProxy::GetMeshColdData()const
	{
		return (MeshAsset ? MeshAsset->GetStoragedResource() : nullptr);
	}
	const EMeshRenderResource* RSkeletalMeshSceneProxy::GetMeshRenderResource()const
	{
		return (MeshAsset ? MeshAsset->GetRenderResource() : nullptr);
	}
	const RVertexBufferResource* RSkeletalMeshSceneProxy::GetSkinVertexStream(const BOOL32 InIsIndexStream)const
	{
		const ESkinnedMeshRenderResource* MeshRenderResource = static_cast<const ESkinnedMeshRenderResource*>(GetMeshRenderResource());
		if (!MeshRenderResource)
		{
			return nullptr;
		}
		// The pair is stored as (blend index, blend weight), which is the order the importer
		// writes it in and the order the shader declares the two streams in.
		const TArray<RVertexBufferResource>& SkinRenderResources = MeshRenderResource->GetSkinRenderResource();
		const UINT32 StreamIndex = (InIsIndexStream ? 0u : 1u);
		if (StreamIndex >= SkinRenderResources.Num<UINT32>())
		{
			return nullptr;
		}
		return &(SkinRenderResources[StreamIndex]);
	}
	BOOL32 RSkeletalMeshSceneProxy::HasSkinVertexStreams()const
	{
		const ESkinnedMeshRenderResource* MeshRenderResource = static_cast<const ESkinnedMeshRenderResource*>(GetMeshRenderResource());
		if (!MeshRenderResource)
		{
			return FALSE;
		}
		// The data side decides: a mesh whose render resource carries the pair is bound with the
		// pair, and its index stream is UINT16 x 4 while the shader declares uint4, which is
		// exactly what the pipeline state has to be told.
		return ((MeshRenderResource->GetSkinRenderResource().Num<UINT32>() >= 2u) ? TRUE : FALSE);
	}
	void RSkeletalMeshSceneProxy::BindExtraResources(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const
	{
		if ((!InCommandList) || (!InSlot.Program))
		{
			return;
		}
		const EShaderStageEntry* VertexEntry = ResolveShaderStageEntry(*InSlot.Program, EShaderStageType::SHADER_STAGE_VERTEX);
		if (!VertexEntry)
		{
			return;
		}

		// The bones are named exactly as the shader source declares them and bound through the
		// vertex stage's own plan, so the register they land on is whatever the compiler assigned
		// - a literal pair would land on whatever else the program binds there. A program compiled
		// without the skin streams has no such binding and binds nothing.
		EShaderParameterSet& Parameters = const_cast<EShaderParameterSet&>(SkeletonParameters);
		const RStructuredBuffer& MatrixBuffer = SkeletonRenderResource.GetRenderResource(
			ESkeletonRenderResource::SKELETON_RENDER_RESOURCE_MATRIX);
		const RStructuredBuffer& InverseTransposeBuffer = SkeletonRenderResource.GetRenderResource(
			ESkeletonRenderResource::SKELETON_RENDER_RESOURCE_INVERSE_TRANSPOSE_MATRIX);
		Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("BoneMatrices")), &MatrixBuffer);
		Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("BoneInverseTransposeMatrices")), &InverseTransposeBuffer);

		BindShaderParameterResources(InCommandList, Parameters, VertexEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_VERTEX);
	}
	void RSkeletalMeshSceneProxy::ApplyRuntimePayload(const FPrimitiveRenderSnapshot& InSnapshot)
	{
		// Where the bones are this frame. Neither an asset nor a shader value, so it arrives with
		// the snapshot and lands in this proxy's own bone buffers.
		UpdateSkeletonRenderResource(InSnapshot.BoneToRootMatrices);
	}

	// ---------------------------------------------------------------------------
	// Bones
	// ---------------------------------------------------------------------------

	void RSkeletalMeshSceneProxy::UpdateSkeletonRenderResource(const TArray<Matrix4x4>& InBoneToRootMatrices)
	{
		// The upload runs on every back end: the buffers it fills are bound through the shader's
		// own plan, which both back ends read the same way.
		if (!!SkeletonAsset)
		{
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeleton render resource is not valid."), (SkeletonRenderResource.IsRenderResourceValid()));
			if (SkeletonRenderResource.IsRenderResourceValid())
			{
				Check((SkeletonAsset->IsResourceValid()), (ENGINE_RENDER_CORE_ERROR));
				const ESkeleton* Skeleton = SkeletonAsset->GetStoragedResource();
				Check((Skeleton->GetBoneCount() > 0u), (ENGINE_RENDER_CORE_ERROR));

				Check((!!MeshAsset), (ENGINE_RENDER_CORE_ERROR));
				Check((MeshAsset->GetStoragedResource()), (ENGINE_RENDER_CORE_ERROR));
				const ESkinnedMesh* SkinnedMesh = MeshAsset->GetStoragedResource();
				Check((!!SkinnedMesh), (ENGINE_RENDER_CORE_ERROR));
				const ESkinnedMesh::EBindPoseValue& BindPoses = SkinnedMesh->GetBindPoseValue();
				const ESkinnedMesh::EBindPoseIndex& BindPoseMappings = SkinnedMesh->GetBindPoseIndex();

				Check((BindPoses.Num() <= InBoneToRootMatrices.Num()), (ENGINE_RENDER_CORE_ERROR));
				Check(((BindPoses.Num() > 0) && (BindPoses.Num() == BindPoseMappings.Num())), (ENGINE_RENDER_CORE_ERROR));
				Check((BindPoses.Num() == BoneValues.Num()), (ENGINE_RENDER_CORE_ERROR));

#if _EDITOR_ONLY
				BOOL32 CheckBoneUpdate = FALSE;
#endif
				for (auto It = BindPoseMappings.Begin(); It != BindPoseMappings.End(); It++)
				{
					const Matrix4x4* BindPosePtr = BindPoses.FindValueAsPtr(It->first);
					const USHORT BoneIndex = Skeleton->GetBoneIndex(It->first);
#if _EDITOR_ONLY
					if ((!!BindPosePtr) && (BoneIndex < InBoneToRootMatrices.Num<USHORT>()))
#endif
					{
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
						BoneValues[It->second] = (InBoneToRootMatrices[BoneIndex]) * (*BindPosePtr);
#else
						BoneValues[It->second] = (InBoneToRootMatrices[BoneIndex].Transpose()) * (*BindPosePtr);
#endif
#if _EDITOR_ONLY
						CheckBoneUpdate = TRUE;
#endif
					}
#if _EDITOR_ONLY
					else
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Skeleton render resource is not valid."));
					}
#endif
				}
#if _EDITOR_ONLY
				if (CheckBoneUpdate)
#endif
				{
					SkeletonRenderResource.UpdateRenderResource(BoneValues);
				}
#if _EDITOR_ONLY
				else
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Updating skeleton render resource failed."));
				}
#endif
			}
		}
	}

} // namespace PigeonEngine
