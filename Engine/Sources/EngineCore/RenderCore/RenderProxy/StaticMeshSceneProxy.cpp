#include "StaticMeshSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <MeshAsset/MeshAsset.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RStaticMeshMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddPrimitiveMaterialParameter();
		AddMeshMaterialParameter();
		EndSetupParameter();
		CreateBuffer();
	}

	RStaticMeshSceneProxy::RStaticMeshSceneProxy(PStaticMeshComponent* InComponent)
		: VertexShader(nullptr), PixelShader(nullptr), MeshAsset(nullptr), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create static mesh scene proxy failed"), (!!Component));
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
	void RStaticMeshSceneProxy::SetupProxy(const BOOL32 InIsHidden, const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow, const ERenderPrimitiveMatrices& InMatrices, const EStaticMeshAsset* InMeshAsset)
	{
		SetupShaders();

		SetPrimitiveSettings(InIsHidden, InIsMovable, InIsCastShadow, InIsReceiveShadow);
		UpdatePrimitiveMatrices(InMatrices);
		UpdateMeshAsset(InMeshAsset);

		MaterialParameter.SetupParameters();

		UpdateRenderResource();
	}
	BOOL32 RStaticMeshSceneProxy::IsRenderValid()const
	{
		//TODO
		return TRUE;
	}
	void RStaticMeshSceneProxy::UpdateMeshAsset(const EStaticMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
	}
	void RStaticMeshSceneProxy::UpdateRenderResource()
	{
		Matrix4x4 InvMat(GetLocalToWorldMatrix().Inverse());
		MaterialParameter["_WorldMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix());
		MaterialParameter["_WorldInvMatrix"] = &TranslateUploadMatrixType(InvMat);
		MaterialParameter["_WorldInvTransposeMatrix"] = &TranslateUploadTransposeMatrixType(InvMat);
		MaterialParameter.UploadBuffer();
	}
	void RStaticMeshSceneProxy::BindRenderResource()const
	{
		BindVertexShader();
		BindPixelShader();
		BindMeshResource();
		BindMaterialParameter(1u);
	}
	void RStaticMeshSceneProxy::SetupShaders()
	{
		const EString ImportPath(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
		const EString ImportVSName = EString("StaticMesh_") + EEngineSettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
		const RInputLayoutDesc TempShaderInputLayouts[] =
		{
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_POSITION0, sizeof(FLOAT), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT, 0u, 0u),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_NORMAL0, sizeof(FLOAT), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT, 0u, 1u),
			RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0, sizeof(FLOAT), 2u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT, 0u, 2u)
		};
		if (!VertexShader)
		{
			constexpr UINT32 TempShaderInputLayoutNum = PE_ARRAYSIZE(TempShaderInputLayouts);
			TryLoadVertexShader(EBaseSettings::ENGINE_SHADER_PATH, ImportVSName,
				VertexShader,
				&ImportPath, &ImportVSName,
				TempShaderInputLayouts, &TempShaderInputLayoutNum);
		}
		if (!PixelShader)
		{
			const EString ImportPSName = EString("StaticMesh_") + EEngineSettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(EBaseSettings::ENGINE_SHADER_PATH, ImportPSName,
				PixelShader,
				&ImportPath, &ImportPSName);
		}
	}
	void RStaticMeshSceneProxy::BindVertexShader()const
	{
#if _EDITOR_ONLY
		if ((!!VertexShader) && (VertexShader->IsRenderResourceValid()))
#endif
		{
			const RVertexShaderResource* VSResource = VertexShader->GetRenderResource();
			{
				RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
				RenderDevice->SetInputLayout(VSResource->InputLayout);
				RenderDevice->SetVSShader(VSResource->Shader);
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh vertex shader is invalid."));
		}
#endif
	}
	void RStaticMeshSceneProxy::BindPixelShader()const
	{
#if _EDITOR_ONLY
		if ((!!PixelShader) && (PixelShader->IsRenderResourceValid()))
#endif
		{
			const RPixelShaderResource* PSResource = PixelShader->GetRenderResource();
			{
				RDeviceD3D11::GetDeviceSingleton()->SetPSShader(PSResource->Shader);
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh pixel shader is invalid."));
		}
#endif
	}
	void RStaticMeshSceneProxy::BindMeshResource()const
	{
#if _EDITOR_ONLY
		if ((!!VertexShader) && (VertexShader->IsRenderResourceValid()) && (!!PixelShader) && (PixelShader->IsRenderResourceValid()) && (!!MeshAsset) && (MeshAsset->IsRenderResourceValid()))
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			const EStaticMeshRenderResource*	MeshRenderResource	= MeshAsset->GetRenderResource();
			const UINT32						LayoutNum			= VertexShader->GetShaderInputLayoutNum();
			const RInputLayoutDesc*				Layouts				= VertexShader->GetShaderInputLayouts();
			const RIndexBufferResource*			IndexRenderResource	= MeshRenderResource->GetIndexRenderResource();
#if _EDITOR_ONLY
			if ((!!IndexRenderResource) && (IndexRenderResource->IsRenderResourceValid()))
#endif
			{
				RFormatType IndexFormat = IndexRenderResource->UseShort ? RFormatType::FORMAT_R16_UINT : RFormatType::FORMAT_R32_UINT;
				RenderDevice->SetIndexBuffer(IndexRenderResource->Buffer, 0u, IndexFormat);
			}
#if _EDITOR_ONLY
			else
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh index buffer is invalid."));
			}
#endif
			for (UINT32 LayoutIndex = 0u; LayoutIndex < LayoutNum; LayoutIndex++)
			{
				const RInputLayoutDesc&					Layout					= Layouts[LayoutIndex];
				UINT32									ShaderSemanticSlot		= 0u;
				EVertexResourceType						VertexResource			= TranslateInputLayoutToVertexResource(Layout, ShaderSemanticSlot);
				const TArray<RVertexBufferResource*>*	VertexRenderResources	= MeshRenderResource->GetVertexRenderResource(VertexResource);
#if _EDITOR_ONLY
				if ((!!VertexRenderResources) && (ShaderSemanticSlot < VertexRenderResources->Num<UINT32>()))
#endif
				{
					const RVertexBufferResource* VertexRenderResource = (*VertexRenderResources)[ShaderSemanticSlot];
#if _EDITOR_ONLY
					if ((!!VertexRenderResource) && (VertexRenderResource->IsRenderResourceValid()))
#endif
					{
						RenderDevice->SetVertexBuffer(VertexRenderResource->Buffer, VertexRenderResource->Stride, 0u, LayoutIndex);
					}
#if _EDITOR_ONLY
					else
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh vertex buffer is invalid."));
					}
#endif
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
	void RStaticMeshSceneProxy::BindMaterialParameter(const UINT32 InSlot)const
	{
		const RBufferResource& ConstantBuffer = MaterialParameter.GetConstantBuffer();
#if _EDITOR_ONLY
		if (((InSlot >= 0u) && (InSlot < 8u)) && (ConstantBuffer.IsRenderResourceValid()))
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			RenderDevice->BindVSConstantBuffer(ConstantBuffer.Buffer, InSlot);
			RenderDevice->BindPSConstantBuffer(ConstantBuffer.Buffer, InSlot);
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh constant buffer is invalid."));
		}
#endif
	}
	void RStaticMeshSceneProxy::Draw()const
	{
#if _EDITOR_ONLY
		if ((!!MeshAsset) && (MeshAsset->IsRenderResourceValid()))
#endif
		{
			const EStaticMeshRenderResource*	MeshRenderResource	= MeshAsset->GetRenderResource();
			const RIndexBufferResource*			IndexRenderResource	= MeshRenderResource->GetIndexRenderResource();
#if _EDITOR_ONLY
			if ((!!IndexRenderResource) && (IndexRenderResource->IsRenderResourceValid()))
#endif
			{
				RDeviceD3D11::GetDeviceSingleton()->DrawIndexed(IndexRenderResource->IndexCount);
			}
#if _EDITOR_ONLY
			else
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Draw static mesh indexed is invalid."));
			}
#endif
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Static mesh asset is invalid."));
		}
#endif
	}

};