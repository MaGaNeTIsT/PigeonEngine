#include "SkeletalMeshSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <MeshAsset/MeshAsset.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RSkeletalMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RSkeletalMeshMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddPrimitiveMaterialParameter();
		AddMeshMaterialParameter();
		EndSetupParameter();
		CreateBuffer();
	}

	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(PSkeletalMeshComponent* InComponent)
		: VertexShader(nullptr), PixelShader(nullptr), MeshAsset(nullptr), SkeletonAsset(nullptr), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create skeletal mesh scene proxy failed"), (!!Component));
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy()
		: VertexShader(nullptr), PixelShader(nullptr), MeshAsset(nullptr), SkeletonAsset(nullptr), Component(nullptr)
	{
	}
	RSkeletalMeshSceneProxy::RSkeletalMeshSceneProxy(const RSkeletalMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), VertexShader(Other.VertexShader), PixelShader(Other.PixelShader),
		MeshAsset(Other.MeshAsset), SkeletonAsset(Other.SkeletonAsset), Component(Other.Component)
	{
	}
	RSkeletalMeshSceneProxy::~RSkeletalMeshSceneProxy()
	{
	}
	void RSkeletalMeshSceneProxy::SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const ESkinnedMeshAsset* InMeshAsset, const ESkeletonAsset* InSkeletonAsset)
	{
		SetupShaders();

		SetPrimitiveSettings(InIsHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);
		UpdateMeshAsset(InMeshAsset);
		UpdateSkeletonAsset(InSkeletonAsset);

		SkeletonRenderResource.InitRenderResource();
		MaterialParameter.SetupParameters();

		UpdateRenderResource();
	}
	BOOL32 RSkeletalMeshSceneProxy::IsRenderValid()const
	{
		//TODO
		return TRUE;
	}
	void RSkeletalMeshSceneProxy::UpdateMeshAsset(const ESkinnedMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
		Check((!!MeshAsset), (ENGINE_RENDER_CORE_ERROR));
		{
			Check((MeshAsset->IsResourceValid()), (ENGINE_RENDER_CORE_ERROR));
			{
				const ESkinnedMesh* SkinnedMesh = MeshAsset->GetStoragedResource();
				const UINT32 BoneNum = SkinnedMesh->GetBindPoseValue().Length();
				Check((BoneNum > 0u), (ENGINE_RENDER_CORE_ERROR));
				SkeletonRenderResource.SetBoneNum(BoneNum);
				if ((BoneNum != BoneValues.Length()))
				{
					BoneValues.Resize(BoneNum);
				}
			}
		}
	}
	void RSkeletalMeshSceneProxy::UpdateSkeletonAsset(const ESkeletonAsset* InSkeletonAsset)
	{
		SkeletonAsset = InSkeletonAsset;
	}
	void RSkeletalMeshSceneProxy::UpdateRenderResource()
	{
#if _EDITOR_ONLY
		if (SkeletonRenderResource.IsRenderResourceValid())
#endif
		{
			SkeletonRenderResource.UpdateRenderResource();
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Skeleton render resource is not valid."));
		}
#endif

		Matrix4x4 InvMat(GetLocalToWorldMatrix().Inverse());
		MaterialParameter["_WorldMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix());
		MaterialParameter["_WorldInvMatrix"] = &TranslateUploadMatrixType(InvMat);
		MaterialParameter["_WorldInvTransposeMatrix"] = &TranslateUploadTransposeMatrixType(InvMat);
		MaterialParameter.UploadBuffer();
	}
	void RSkeletalMeshSceneProxy::BindRenderResource()const
	{

	}
	void RSkeletalMeshSceneProxy::Draw()const
	{

	}
	void RSkeletalMeshSceneProxy::SetupShaders()
	{

	}
	void RSkeletalMeshSceneProxy::BindVertexShader()const
	{

	}
	void RSkeletalMeshSceneProxy::BindPixelShader()const
	{

	}
	void RSkeletalMeshSceneProxy::BindMeshResource()const
	{

	}
	void RSkeletalMeshSceneProxy::BindMaterialParameter(const UINT32 InSlot)const
	{

	}

};