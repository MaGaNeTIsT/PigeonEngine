#include "SkeletalMeshSceneProxy.h"

#include <ShaderAsset/ShaderInstance.h>
#include <Shader/ShaderAssetBinding.h>
#include <MeshAsset/MeshAsset.h>
#include <RHI/IRCommandList.h>
#include <RenderProxy/PrimitiveSceneProxyFactory.h>

namespace PigeonEngine
{

	// The bones are the vertex factory's binding surface, and this proxy is the only publisher
	// of these two names (the material's vertex stage never writes them).
	static const FShaderBindingClaim g_SkeletalBindingClaims[] =
	{
		{ EName(PE_TEXT("BoneMatrices")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("BoneInverseTransposeMatrices")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(g_SkeletalBindingClaims, EName(SHADER_BINDING_SCOPE_SKELETAL));

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
		, SkinnedLocalBound(Vector3::Zero(), Vector3::Zero(), FALSE)
	{
		SetPrimitiveCategoryMask(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_SKELETAL);
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(const PSkeletalMeshComponent* InComponent)
		: MeshAsset(nullptr), SkeletonAsset(nullptr)
		, SkinnedLocalBound(Vector3::Zero(), Vector3::Zero(), FALSE)
	{
		SetPrimitiveCategoryMask(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_SKELETAL);
		// The component is not stored: everything this proxy needs arrives through the snapshot,
		// so no game thread object is reachable from here. The argument stays so the create path
		// keeps its meaning while the scene finishes moving onto the factory.
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create skeletal mesh scene proxy failed"), (!!InComponent));
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(const RSkeletalMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), MeshAsset(Other.MeshAsset), SkeletonAsset(Other.SkeletonAsset)
		, SkinnedLocalBound(Other.SkinnedLocalBound)
	{
		SetPrimitiveCategoryMask(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_SKELETAL);
	}
	RSkeletalMeshSceneProxy::~RSkeletalMeshSceneProxy()
	{
	}
	BOOL32 RSkeletalMeshSceneProxy::IsRenderValid() const
	{
		//TODO
		return TRUE;
	}
	BOOL32 RSkeletalMeshSceneProxy::GetWorldBound(Vector3& OutMin, Vector3& OutMax) const
	{
		if (SkinnedLocalBound.IsValid == FALSE)
		{
			return FALSE;
		}
		const Matrix4x4& LocalToWorld = GetLocalToWorldMatrix();
		Vector3 WorldMin(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
		Vector3 WorldMax(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			const Vector3 LocalCorner(
				(((CornerIndex & 1u) != 0u) ? SkinnedLocalBound.AABBMax.x : SkinnedLocalBound.AABBMin.x),
				(((CornerIndex & 2u) != 0u) ? SkinnedLocalBound.AABBMax.y : SkinnedLocalBound.AABBMin.y),
				(((CornerIndex & 4u) != 0u) ? SkinnedLocalBound.AABBMax.z : SkinnedLocalBound.AABBMin.z));
			const Vector3 WorldCorner = Matrix4x4TransformPosition(LocalToWorld, LocalCorner);
			WorldMin = MinVector3(WorldMin, WorldCorner);
			WorldMax = MaxVector3(WorldMax, WorldCorner);
		}
		OutMin = WorldMin;
		OutMax = WorldMax;
		return TRUE;
	}
	void RSkeletalMeshSceneProxy::UpdateSkinnedLocalBoundFromBones(const ESkinnedMesh& InMesh)
	{
		// The authored bound is the base: a bone whose matrix has not been reported yet keeps the
		// mesh at least inside it, and the accumulation below can only grow the result.
		const EBoundAABB BaseBound = InMesh.GetAdjustedBoundAABB();
		SkinnedLocalBound = BaseBound;
		if (BaseBound.IsValid == FALSE)
		{
			return;
		}
		for (UINT32 BoneIndex = 0u, BoneNum = BoneValues.Num<UINT32>(); BoneIndex < BoneNum; BoneIndex++)
		{
			const Matrix4x4& BoneMatrix = BoneValues[BoneIndex];
			for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
			{
				const Vector3 LocalCorner(
					(((CornerIndex & 1u) != 0u) ? BaseBound.AABBMax.x : BaseBound.AABBMin.x),
					(((CornerIndex & 2u) != 0u) ? BaseBound.AABBMax.y : BaseBound.AABBMin.y),
					(((CornerIndex & 4u) != 0u) ? BaseBound.AABBMax.z : BaseBound.AABBMin.z));
				const Vector3 SkinnedCorner = Matrix4x4TransformPosition(BoneMatrix, LocalCorner);
				SkinnedLocalBound += EBoundAABB(SkinnedCorner, SkinnedCorner, TRUE);
			}
		}
	}
	void RSkeletalMeshSceneProxy::ApplyAssets(const FPrimitiveRenderAssetReport& InAssets)
	{
		MeshAsset = static_cast<const ESkinnedMeshAsset*>(InAssets.GetMeshAsset());
		SkeletonAsset = static_cast<const ESkeletonAsset*>(InAssets.GetSkeletonAsset());
		// A different mesh invalidates the bound the previous one accumulated.
		SkinnedLocalBound = EBoundAABB(Vector3::Zero(), Vector3::Zero(), FALSE);

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
						// A bone the first snapshot does not map yet must not transform a corner
						// with an all zero matrix; the bind pose is the conservative placeholder.
						for (UINT32 BoneIndex = 0u, BoneCount = BoneValues.Num<UINT32>(); BoneIndex < BoneCount; BoneIndex++)
						{
							BoneValues[BoneIndex] = Matrix4x4::Identity();
						}
					}
					// Until the first skeleton update the authored bound is the bound. It keeps the
					// primitive cullable and is the shipping answer; the measured variant below is
					// opt in because it costs a transform per bone.
					SkinnedLocalBound = SkinnedMesh->GetAdjustedBoundAABB();
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

	const EMesh* RSkeletalMeshSceneProxy::GetMeshColdData() const
	{
		return (MeshAsset ? MeshAsset->GetStoragedResource() : nullptr);
	}
	const EMeshRenderResource* RSkeletalMeshSceneProxy::GetMeshRenderResource() const
	{
		return (MeshAsset ? MeshAsset->GetRenderResource() : nullptr);
	}
	const RVertexBufferResource* RSkeletalMeshSceneProxy::GetSkinVertexStream(const BOOL32 InIsIndexStream) const
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
	BOOL32 RSkeletalMeshSceneProxy::HasSkinVertexStreams() const
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
	BOOL32 RSkeletalMeshSceneProxy::BindExtraResources(IRCommandList* InCommandList, const FPrimitivePassData& InData) const
	{
		if ((!InCommandList) || (!InData.Program))
		{
			return FALSE;
		}
		const EShaderStageEntry* VertexEntry = ResolveShaderStageEntry(*InData.Program, EShaderStageType::SHADER_STAGE_VERTEX);
		if (!VertexEntry)
		{
			return FALSE;
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

		BindShaderResources(InCommandList, Parameters, VertexEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_VERTEX
			, EName(SHADER_BINDING_SCOPE_SKELETAL));
		// Even a refused publish may have written part of the table, so the caller has to treat
		// the material bindings as stale either way.
		return TRUE;
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
				// Off by default: the authored BoundScale / BoundExtend pair is the shipping
				// bound, and this per bone accumulation is a validation aid rather than a cost
				// every animated character pays.
				if PE_CONSTEXPR (EEngineSettings::ENGINE_SKELETAL_BOUND_FROM_BONES != FALSE)
				{
					UpdateSkinnedLocalBoundFromBones(*SkinnedMesh);
				}
				else
				{
					// The authored pair is O(1) to apply and reading it here keeps the proxy in
					// step with an asset whose adjustment changed after it was installed.
					SkinnedLocalBound = SkinnedMesh->GetAdjustedBoundAABB();
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
