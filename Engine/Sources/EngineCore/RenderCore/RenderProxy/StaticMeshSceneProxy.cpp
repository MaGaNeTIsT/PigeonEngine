#include "StaticMeshSceneProxy.h"

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RStaticMeshMaterialParameter::SetupParameters()
	{
		BeginSetupParameter();
		AddPrimitiveMaterialParameter();
		AddMeshMaterialParameter();
		EndSetupParameter();
	}

	RStaticMeshSceneProxy::RStaticMeshSceneProxy(PStaticMeshComponent* InComponent)
		: VertexShader(nullptr), PixelShader(nullptr), MeshAsset(nullptr), Component(InComponent)
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Create static mesh scene proxy failed"), (!!Component));
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy()
		: VertexShader(nullptr), PixelShader(nullptr), MeshAsset(nullptr), Component(nullptr)
	{
	}
	RStaticMeshSceneProxy::RStaticMeshSceneProxy(const RStaticMeshSceneProxy& Other)
		: RMeshSceneProxy(Other), VertexShader(Other.VertexShader), PixelShader(Other.PixelShader), MeshAsset(Other.MeshAsset), Component(Other.Component)
	{
	}
	RStaticMeshSceneProxy::~RStaticMeshSceneProxy()
	{
	}
	void RStaticMeshSceneProxy::SetupProxy(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset)
	{
		SetupShaders();

		SetPrimitiveSettings(InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);
		UpdateMeshAsset(InMeshAsset);

		MaterialParameter.SetupParameters();
	}
	void RStaticMeshSceneProxy::UpdateMeshAsset(const EStaticMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
	}
	void RStaticMeshSceneProxy::UpdateRenderResource()
	{
		Matrix4x4 InvMat(LocalToWorldMatrix.Inverse());
		MaterialParameter["_WorldMatrix"] = &TranslateUploadMatrixType(LocalToWorldMatrix);
		MaterialParameter["_WorldInvMatrix"] = &TranslateUploadMatrixType(InvMat);
		MaterialParameter["_WorldInvTransposeMatrix"] = &TranslateUploadTransposeMatrixType(InvMat);
	}
	void RStaticMeshSceneProxy::BindRenderResource()
	{
		BindMaterialParameter(1u);
#if _EDITOR_ONLY
		if (((!!VertexShader) && (!!MeshAsset)) && (MeshAsset->IsRenderResourceValid()))
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			const EStaticMeshRenderResource*	MeshRenderResource	= MeshAsset->GetRenderResource();
			const UINT32						LayoutNum			= VertexShader->GetShaderInputLayoutNum();
			const RInputLayoutDesc*				Layouts				= VertexShader->GetShaderInputLayouts();
			for (UINT32 LayoutIndex = 0u; LayoutIndex < LayoutNum; LayoutIndex++)
			{
				const RInputLayoutDesc&			Layout					= Layouts[LayoutIndex];
				UINT32							ShaderSemanticSlot		= 0u;
				EVertexResourceType				VertexResource			= TranslateInputLayoutToVertexResource(Layout, ShaderSemanticSlot);
				const TArray<RBufferResource*>*	VertexRenderResource	= MeshRenderResource->GetVertexRenderResource(VertexResource);
#if _EDITOR_ONLY
				if ((!!VertexRenderResource) && (ShaderSemanticSlot < VertexRenderResource->Length()))
#endif
				{
					const RBufferResource* RenderResource = (*VertexRenderResource)[ShaderSemanticSlot];
#if _EDITOR_ONLY

#endif
					RenderDevice->SetVertexBuffer(RenderResource->Buffer,);
				}
#if _EDITOR_ONLY
				else
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check static mesh scene proxy vertex render resource is invalid."));
				}
#endif
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check static mesh scene proxy is invalid."));
		}
#endif
	}
	void RStaticMeshSceneProxy::SetupShaders()
	{
		const EString ImportPath(ESettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
		const EString ImportVSName = EString("StaticMesh_") + ESettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
		const RInputLayoutDesc TempShaderInputLayouts[] =
		{
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_POSITION0),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_NORMAL0),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0)
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
			const EString ImportPSName = EString("StaticMesh_") + ESettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(ESettings::ENGINE_SHADER_PATH, ImportPSName,
				PixelShader,
				&ImportPath, &ImportPSName);
		}
	}
	void RStaticMeshSceneProxy::BindMaterialParameter(const UINT32 InSlot)
	{
		RBufferResource& ConstantBuffer = MaterialParameter.GetConstantBuffer();
#if _EDITOR_ONLY
		if (((InSlot >= 0u) && (InSlot < 16u)) && (ConstantBuffer.IsRenderResourceValid()))
#endif
		{
			RDeviceD3D11::GetDeviceSingleton()->BindVSConstantBuffer(ConstantBuffer.Buffer, InSlot);
			RDeviceD3D11::GetDeviceSingleton()->BindPSConstantBuffer(ConstantBuffer.Buffer, InSlot);
		}
	}

};