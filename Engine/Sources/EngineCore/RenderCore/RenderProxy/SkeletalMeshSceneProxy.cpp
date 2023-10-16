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
	void RSkeletalMeshSceneProxy::SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const ESkinnedMeshAsset* InMeshAsset, const ESkeletonAsset* InSkeletonAsset, const TArray<Matrix4x4>& InBoneToRootMatrices)
	{
		SetupShaders();

		SetPrimitiveSettings(InIsHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);
		UpdateMeshAsset(InMeshAsset);
		UpdateSkeletonAsset(InSkeletonAsset);
		UpdateSkeletonRenderResource(InBoneToRootMatrices);

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
#if _EDITOR_ONLY
		if (!!MeshAsset)
#endif
		{
			Check((MeshAsset->IsResourceValid()), (ENGINE_RENDER_CORE_ERROR));
			{
				const ESkinnedMesh* SkinnedMesh = MeshAsset->GetStoragedResource();
				Check((!!SkinnedMesh), (ENGINE_RENDER_CORE_ERROR));
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
	void RSkeletalMeshSceneProxy::UpdateSkeletonRenderResource(const TArray<Matrix4x4>& InBoneToRootMatrices)
	{
#if _EDITOR_ONLY
		if ((!!SkeletonAsset) && (SkeletonRenderResource.IsRenderResourceValid()))
#endif
		{
			Check((SkeletonAsset->IsResourceValid()), (ENGINE_RENDER_CORE_ERROR));
			const ESkeleton* Skeleton = SkeletonAsset->GetStoragedResource();
			Check((Skeleton->GetBoneCount() > 0u), (ENGINE_RENDER_CORE_ERROR));

			Check((!!MeshAsset), (ENGINE_RENDER_CORE_ERROR));
			Check((MeshAsset->IsResourceValid()), (ENGINE_RENDER_CORE_ERROR));
			const ESkinnedMesh* SkinnedMesh = MeshAsset->GetStoragedResource();
			Check((!!SkinnedMesh), (ENGINE_RENDER_CORE_ERROR));
			const ESkinnedMesh::EBindPoseValue& BindPoses = SkinnedMesh->GetBindPoseValue();
			const ESkinnedMesh::EBindPoseIndex& BindPoseMappings = SkinnedMesh->GetBindPoseIndex();

			Check((BindPoses.Length() <= InBoneToRootMatrices.Length()), (ENGINE_RENDER_CORE_ERROR));
			Check(((BindPoses.Length() > 0u) && (BindPoses.Length() == BindPoseMappings.Length())), (ENGINE_RENDER_CORE_ERROR));
			Check((BindPoses.Length() == BoneValues.Length()), (ENGINE_RENDER_CORE_ERROR));

#if _EDITOR_ONLY
			BOOL32 CheckBoneUpdate = FALSE;
#endif
			for (auto It = BindPoseMappings.Begin(); It != BindPoseMappings.End(); It++)
			{
				const Matrix4x4* BindPosePtr = BindPoses.FindValueAsPtr(It->first);
				const USHORT BoneIndex = Skeleton->GetBoneIndex(It->first);
#if _EDITOR_ONLY
				if ((!!BindPosePtr) && (BoneIndex < InBoneToRootMatrices.Length()))
#endif
				{
					BoneValues[It->second] = (InBoneToRootMatrices[BoneIndex].Transpose()) * (*BindPosePtr);
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
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Skeleton render resource is not valid."));
		}
#endif
	}
	void RSkeletalMeshSceneProxy::UpdateRenderResource()
	{
		Matrix4x4 InvMat(GetLocalToWorldMatrix().Inverse());
		MaterialParameter["_WorldMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix());
		MaterialParameter["_WorldInvMatrix"] = &TranslateUploadMatrixType(InvMat);
		MaterialParameter["_WorldInvTransposeMatrix"] = &TranslateUploadTransposeMatrixType(InvMat);
		MaterialParameter.UploadBuffer();
	}
	void RSkeletalMeshSceneProxy::BindRenderResource()const
	{
		BindVertexShader();
		BindPixelShader();
		BindMeshResource();
		BindMaterialParameter(1u);
		BindSkeletonRenderResource(0u);
	}
	void RSkeletalMeshSceneProxy::Draw()const
	{

	}
	void RSkeletalMeshSceneProxy::SetupShaders()
	{
		const EString ImportPath(ESettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
		const EString ImportVSName = EString("SkeletalMesh_") + ESettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
		const RInputLayoutDesc TempShaderInputLayouts[] =
		{
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_POSITION0, sizeof(FLOAT), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT, 0u, 0u),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_NORMAL0, sizeof(FLOAT), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT, 0u, 1u),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0, sizeof(FLOAT), 2u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT, 0u, 2u),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES0, sizeof(USHORT), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT, 0u, 3u),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT0, sizeof(FLOAT), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT, 0u, 4u)
		};
		if (!VertexShader)
		{
			const UINT32 TempShaderInputLayoutNum = 3u;
			TryLoadVertexShader(ESettings::ENGINE_SHADER_PATH, ImportVSName,
				VertexShader,
				&ImportPath, &ImportVSName,
				TempShaderInputLayouts, &TempShaderInputLayoutNum);
		}
		if (!PixelShader)
		{
			const EString ImportPSName = EString("SkeletalMesh_") + ESettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(ESettings::ENGINE_SHADER_PATH, ImportPSName,
				PixelShader,
				&ImportPath, &ImportPSName);
		}
	}
	void RSkeletalMeshSceneProxy::BindVertexShader()const
	{
#if _EDITOR_ONLY
		BOOL32 IsResourceValid = !!VertexShader;
		if (IsResourceValid)
		{
			IsResourceValid = VertexShader->IsRenderResourceValid();
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh vertex shader is invalid."), (IsResourceValid));
		if (IsResourceValid)
#endif
		{
			const RVertexShaderResource* VSResource = VertexShader->GetRenderResource();
			{
				RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
				RenderDevice->SetInputLayout(VSResource->InputLayout);
				RenderDevice->SetVSShader(VSResource->Shader);
			}
		}
	}
	void RSkeletalMeshSceneProxy::BindPixelShader()const
	{
#if _EDITOR_ONLY
		BOOL32 IsResourceValid = !!PixelShader;
		if (IsResourceValid)
		{
			IsResourceValid = PixelShader->IsRenderResourceValid();
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh pixel shader is invalid."), (IsResourceValid));
		if (IsResourceValid)
#endif
		{
			const RPixelShaderResource* PSResource = PixelShader->GetRenderResource();
			{
				RDeviceD3D11::GetDeviceSingleton()->SetPSShader(PSResource->Shader);
			}
		}
	}
	void RSkeletalMeshSceneProxy::BindMeshResource()const
	{
#if _EDITOR_ONLY
		BOOL32 IsResourceValid = (!!VertexShader) && (!!PixelShader) && (!!MeshAsset);
		if (IsResourceValid)
		{
			IsResourceValid = (VertexShader->IsRenderResourceValid()) && (PixelShader->IsRenderResourceValid()) && (MeshAsset->IsRenderResourceValid());
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check skeletal mesh scene proxy is invalid."), (IsResourceValid));
		if (IsResourceValid)
#endif
		{
			RDeviceD3D11*						RenderDevice		= RDeviceD3D11::GetDeviceSingleton();
			const ESkinnedMeshRenderResource*	MeshRenderResource	= MeshAsset->GetRenderResource();
			const UINT32						LayoutNum			= VertexShader->GetShaderInputLayoutNum();
			const RInputLayoutDesc*				Layouts				= VertexShader->GetShaderInputLayouts();
			const RIndexBufferResource*			IndexRenderResource	= MeshRenderResource->GetIndexRenderResource();
#if _EDITOR_ONLY
			BOOL32 IsIndexBufferValid = !!IndexRenderResource;
			if (IsIndexBufferValid)
			{
				IsIndexBufferValid = IndexRenderResource->IsRenderResourceValid();
			}
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh index buffer is invalid."), (IsIndexBufferValid));
			if (IsIndexBufferValid)
#endif
			{
				RFormatType IndexFormat = IndexRenderResource->UseShort ? RFormatType::FORMAT_R16_UINT : RFormatType::FORMAT_R32_UINT;
				RenderDevice->SetIndexBuffer(IndexRenderResource->Buffer, 0u, IndexFormat);
			}
			for (UINT32 LayoutIndex = 0u; LayoutIndex < LayoutNum; LayoutIndex++)
			{
				const RInputLayoutDesc& Layout = Layouts[LayoutIndex];
				const BOOL32 IsSkinIndex = (Layout.SemanticName >= RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES) && (Layout.SemanticName <= RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES15);
				const BOOL32 IsSkinWeight = (Layout.SemanticName >= RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT) && (Layout.SemanticName <= RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT15);
				CheckSlow((MeshAsset ? (MeshAsset->GetStoragedResource() ? (Layout.MemberNum == MeshAsset->GetStoragedResource()->GetEffectBoneNum()) : TRUE) : TRUE), (ENGINE_RENDER_CORE_ERROR));
				const BOOL32 IsSkinLayout = IsSkinIndex || IsSkinWeight
#if _EDITOR_ONLY
					|| (Layout.MemberNum == MeshAsset->GetStoragedResource()->GetEffectBoneNum())
#endif
					;
				if (IsSkinLayout)
				{
					const TArray<RVertexBufferResource>& SkinRenderResources = MeshRenderResource->GetSkinRenderResource();
					//TODO We only use first skin data for rendering for now.
#if _EDITOR_ONLY
					BOOL32 IsSkinRenderResourcesValid = (SkinRenderResources.Length() > 0u) && ((SkinRenderResources.Length() & 0x1u) == 0u);
					Check((IsSkinRenderResourcesValid), (ENGINE_RENDER_CORE_ERROR));
					if (IsSkinRenderResourcesValid)
#endif
					{
						if (IsSkinIndex)
						{
#if _EDITOR_ONLY
							PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh skin index buffer is invalid."), (SkinRenderResources[0].IsRenderResourceValid()));
							if (SkinRenderResources[0].IsRenderResourceValid())
#endif
							{
								RenderDevice->SetVertexBuffer(SkinRenderResources[0].Buffer, SkinRenderResources[0].Stride, 0u, LayoutIndex);
							}
						}
						else if (IsSkinWeight)
						{
#if _EDITOR_ONLY
							PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh skin weight buffer is invalid."), (SkinRenderResources[1].IsRenderResourceValid()));
							if (SkinRenderResources[1].IsRenderResourceValid())
#endif
							{
								Check((SkinRenderResources[1].Stride == (sizeof(FLOAT) * 4u)), (ENGINE_RENDER_CORE_ERROR));
								RenderDevice->SetVertexBuffer(SkinRenderResources[1].Buffer, SkinRenderResources[1].Stride, 0u, LayoutIndex);
							}
						}
#if _EDITOR_ONLY
						else
						{
							PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh skin data error."));
						}
#endif
					}
				}
				else
				{
					UINT32									ShaderSemanticSlot		= 0u;
					EVertexResourceType						VertexResource			= TranslateInputLayoutToVertexResource(Layout, ShaderSemanticSlot);
					const TArray<RVertexBufferResource*>*	VertexRenderResources	= MeshRenderResource->GetVertexRenderResource(VertexResource);
#if _EDITOR_ONLY
					BOOL32 IsVertexBuffersValid = (!!VertexRenderResources) && (ShaderSemanticSlot < VertexRenderResources->Length());
					PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check skeletal mesh scene proxy vertex render resources is invalid."), (IsVertexBuffersValid));
					if (IsVertexBuffersValid)
#endif
					{
						const RVertexBufferResource* VertexRenderResource = (*VertexRenderResources)[ShaderSemanticSlot];
#if _EDITOR_ONLY
						BOOL32 IsVertexBufferValid = !!VertexRenderResource;
						if (IsVertexBufferValid)
						{
							IsVertexBufferValid = VertexRenderResource->IsRenderResourceValid();
						}
						PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh vertex buffer is invalid."), (IsVertexBufferValid));
						if (IsVertexBufferValid)
#endif
						{
							RenderDevice->SetVertexBuffer(VertexRenderResource->Buffer, VertexRenderResource->Stride, 0u, LayoutIndex);
						}
					}
				}
			}
		}
	}
	void RSkeletalMeshSceneProxy::BindMaterialParameter(const UINT32 InSlot)const
	{
		const RBufferResource& ConstantBuffer = MaterialParameter.GetConstantBuffer();
#if _EDITOR_ONLY
		BOOL32 IsBufferValid = (InSlot >= 0u) && (InSlot < 8u);
		if (IsBufferValid)
		{
			IsBufferValid = ConstantBuffer.IsRenderResourceValid();
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skeletal mesh constant buffer is invalid."), (IsBufferValid));
		if (IsBufferValid)
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			RenderDevice->BindVSConstantBuffer(ConstantBuffer.Buffer, InSlot);
			RenderDevice->BindPSConstantBuffer(ConstantBuffer.Buffer, InSlot);
		}
	}
	void RSkeletalMeshSceneProxy::BindSkeletonRenderResource(const UINT32 InSlot)const
	{

	}

};