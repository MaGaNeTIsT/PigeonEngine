#include "SkyLightProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <RenderDevice/DeviceD3D11.h>
#include "MeshSceneProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSkyLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RSkyLightMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_WorldMatrix"));
		AddParameter<Matrix4x4, EShaderParameterValueType::SHADER_PARAMETER_TYPE_MATRIX44>(("_WorldInvMatrix"));
		AddParameter<Vector3, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT4>(("_LightAdjustColor"));
		AddParameter<FLOAT, EShaderParameterValueType::SHADER_PARAMETER_TYPE_FLOAT1>(("_LightAdjustIntensity"));
		EndSetupParameter();
		CreateBuffer();
	}

	const EStaticMeshAsset*		RSkyLightSceneProxy::SkyDomeMeshAsset		= nullptr;
	const EVertexShaderAsset*	RSkyLightSceneProxy::SkyDomeVertexShader	= nullptr;
	const EPixelShaderAsset*	RSkyLightSceneProxy::SkyDomePixelShader		= nullptr;

	void RSkyLightSceneProxy::CreateSkyDomeAsset()
	{
		if (!(RSkyLightSceneProxy::SkyDomeMeshAsset))
		{
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineModels/BaseShapes/");
			EString ImportName("SphereUV");
			EString ImportFileType("obj");
			TryLoadStaticMesh(EBaseSettings::ENGINE_MESH_PATH, "SkyDome", RSkyLightSceneProxy::SkyDomeMeshAsset, &ImportPath, &ImportName, &ImportFileType, TRUE);
		}
		if (!(RSkyLightSceneProxy::SkyDomeVertexShader))
		{
			EString ImportPath(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
			EString VSName("SkyLight_");
			VSName += EEngineSettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
			RInputLayoutDesc VSInputLayout[] = { RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_POSITION0, sizeof(FLOAT), 4u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT) };
			constexpr UINT32 VSInputLayoutNum = PE_ARRAYSIZE(VSInputLayout);
			TryLoadVertexShader(EBaseSettings::ENGINE_SHADER_PATH, VSName, RSkyLightSceneProxy::SkyDomeVertexShader, &ImportPath, &VSName, VSInputLayout, &VSInputLayoutNum);
		}
		if (!(RSkyLightSceneProxy::SkyDomePixelShader))
		{
			EString ImportPath(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
			EString PSName("SkyLight_");
			PSName += EEngineSettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(EBaseSettings::ENGINE_SHADER_PATH, PSName, RSkyLightSceneProxy::SkyDomePixelShader, &ImportPath, &PSName);
		}
	}

	RSkyLightSceneProxy::RSkyLightSceneProxy(PSkyLightComponent* InComponent)
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f), Component(InComponent)
	{
		CreateSkyDomeAsset();
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create directional light scene proxy failed"), (!!Component));
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy()
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f), Component(nullptr)
	{
		CreateSkyDomeAsset();
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy(const RSkyLightSceneProxy& Other)
		: RBaseSceneProxy(Other), TextureCubeAsset(Other.TextureCubeAsset), AdjustColor(Other.AdjustColor), AdjustIntensity(Other.AdjustIntensity), Component(Other.Component)
	{
		CreateSkyDomeAsset();
	}
	RSkyLightSceneProxy::~RSkyLightSceneProxy()
	{
	}
	void RSkyLightSceneProxy::SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset)
	{
		UpdateTextureCubeAsset(InTextureCubeAsset);
		UpdateSkyLightParams(InSkyLightParams);
		UpdateMatrices(InMatrices);

		SkyLightMaterialParameter.SetupParameters();

		UpdateRenderResource();
	}
	void RSkyLightSceneProxy::UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset)
	{
		TextureCubeAsset = InTextureCubeAsset;
	}
	void RSkyLightSceneProxy::UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams)
	{
		AdjustColor		= InSkyLightParams.LightAdjustColor;
		AdjustIntensity	= InSkyLightParams.LightAdjustIntensity;
	}
	void RSkyLightSceneProxy::UpdateMatrices(const ERenderSkyLightMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, InMatrices.WorldScaling);
	}
	void RSkyLightSceneProxy::UpdateRenderResource()
	{
		FLOAT LightAdjustIntensity = TranslateUploadBaseType(AdjustIntensity);
		SkyLightMaterialParameter["_WorldMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix());
		SkyLightMaterialParameter["_WorldInvMatrix"] = &TranslateUploadMatrixType(GetLocalToWorldMatrix().Inverse());
		SkyLightMaterialParameter["_LightAdjustColor"] = &TranslateUploadColorType(AdjustColor);
		SkyLightMaterialParameter["_LightAdjustIntensity"] = &LightAdjustIntensity;
		SkyLightMaterialParameter.UploadBuffer();
	}
	BOOL32 RSkyLightSceneProxy::IsRenderValid()const
	{
#if _EDITOR_ONLY
		BOOL32 IsValid = (!!(RSkyLightSceneProxy::SkyDomeVertexShader)) && (!!(RSkyLightSceneProxy::SkyDomePixelShader)) && (!!(RSkyLightSceneProxy::SkyDomeMeshAsset));
		if (IsValid)
		{
			IsValid = (RSkyLightSceneProxy::SkyDomeVertexShader->IsRenderResourceValid()) && (RSkyLightSceneProxy::SkyDomePixelShader->IsRenderResourceValid()) && (RSkyLightSceneProxy::SkyDomeMeshAsset->IsRenderResourceValid());
		}
#else
		BOOL32 IsValid = TRUE;
#endif
		IsValid = IsValid && SkyLightMaterialParameter.GetConstantBuffer().IsRenderResourceValid() && (!!TextureCubeAsset);
		if (IsValid)
		{
			IsValid = TextureCubeAsset->IsRenderResourceValid() && (AdjustIntensity >= 0.001f);
		}
		return IsValid;
	}
	void RSkyLightSceneProxy::BindRenderResource()const
	{
		BindVertexShader();
		BindPixelShader();
		BindMeshResource();
		BindMaterialParameter(1u);
		RDeviceD3D11::GetDeviceSingleton()->BindPSShaderResourceView(TextureCubeAsset->GetRenderResource()->ShaderResourceView, 0u);
	}
	void RSkyLightSceneProxy::Draw()const
	{
#if _EDITOR_ONLY
		if (!!(RSkyLightSceneProxy::SkyDomeMeshAsset))
#endif
		{
#if _EDITOR_ONLY
			if (RSkyLightSceneProxy::SkyDomeMeshAsset->IsRenderResourceValid())
#endif
			{
				const EStaticMeshRenderResource* MeshRenderResource = RSkyLightSceneProxy::SkyDomeMeshAsset->GetRenderResource();
				const RIndexBufferResource* IndexRenderResource = MeshRenderResource->GetIndexRenderResource();
#if _EDITOR_ONLY
				if (!!IndexRenderResource)
#endif
				{
#if _EDITOR_ONLY
					if (IndexRenderResource->IsRenderResourceValid())
#endif
					{
						RDeviceD3D11::GetDeviceSingleton()->DrawIndexed(IndexRenderResource->IndexCount);
					}
#if _EDITOR_ONLY
					else
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome draw indexed is invalid."));
					}
#endif
				}
#if _EDITOR_ONLY
				else
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome draw indexed is invalid."));
				}
#endif
			}
#if _EDITOR_ONLY
			else
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome mesh asset is invalid."));
			}
#endif
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome mesh asset is invalid."));
		}
#endif
	}
	void RSkyLightSceneProxy::BindVertexShader()const
	{
#if _EDITOR_ONLY
		if (!!(RSkyLightSceneProxy::SkyDomeVertexShader))
#endif
		{
#if _EDITOR_ONLY
			if (RSkyLightSceneProxy::SkyDomeVertexShader->IsRenderResourceValid())
#endif
			{
				const RVertexShaderResource* VSResource = RSkyLightSceneProxy::SkyDomeVertexShader->GetRenderResource();
				{
					RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
					RenderDevice->SetInputLayout(VSResource->InputLayout);
					RenderDevice->SetVSShader(VSResource->Shader);
				}
			}
#if _EDITOR_ONLY
			else
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome vertex shader is invalid."));
			}
#endif
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome vertex shader is invalid."));
		}
#endif
	}
	void RSkyLightSceneProxy::BindPixelShader()const
	{
#if _EDITOR_ONLY
		if (!!(RSkyLightSceneProxy::SkyDomePixelShader))
#endif
		{
#if _EDITOR_ONLY
			if (RSkyLightSceneProxy::SkyDomePixelShader->IsRenderResourceValid())
#endif
			{
				const RPixelShaderResource* PSResource = RSkyLightSceneProxy::SkyDomePixelShader->GetRenderResource();
				{
					RDeviceD3D11::GetDeviceSingleton()->SetPSShader(PSResource->Shader);
				}
			}
#if _EDITOR_ONLY
			else
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome pixel shader is invalid."));
			}
#endif
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome pixel shader is invalid."));
		}
#endif
	}
	void RSkyLightSceneProxy::BindMeshResource()const
	{
#if _EDITOR_ONLY
		BOOL32 IsValid = (!!(RSkyLightSceneProxy::SkyDomeVertexShader)) && (!!(RSkyLightSceneProxy::SkyDomePixelShader)) && (!!(RSkyLightSceneProxy::SkyDomeMeshAsset));
		if (IsValid)
		{
			IsValid = (RSkyLightSceneProxy::SkyDomeVertexShader->IsRenderResourceValid()) && (RSkyLightSceneProxy::SkyDomePixelShader->IsRenderResourceValid()) && (RSkyLightSceneProxy::SkyDomeMeshAsset->IsRenderResourceValid());
		}
		if (IsValid)
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			const EStaticMeshRenderResource*	MeshRenderResource	= RSkyLightSceneProxy::SkyDomeMeshAsset->GetRenderResource();
			const UINT32						LayoutNum			= RSkyLightSceneProxy::SkyDomeVertexShader->GetShaderInputLayoutNum();
			const RInputLayoutDesc*				Layouts				= RSkyLightSceneProxy::SkyDomeVertexShader->GetShaderInputLayouts();
			const RIndexBufferResource*			IndexRenderResource	= MeshRenderResource->GetIndexRenderResource();
#if _EDITOR_ONLY
			if (!!IndexRenderResource)
#endif
			{
#if _EDITOR_ONLY
				if (IndexRenderResource->IsRenderResourceValid())
#endif
				{
					RFormatType IndexFormat = IndexRenderResource->UseShort ? RFormatType::FORMAT_R16_UINT : RFormatType::FORMAT_R32_UINT;
					RenderDevice->SetIndexBuffer(IndexRenderResource->Buffer, 0u, IndexFormat);
				}
#if _EDITOR_ONLY
				else
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome mesh index buffer is invalid."));
				}
#endif
			}
#if _EDITOR_ONLY
			else
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome mesh index buffer is invalid."));
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
					if (!!VertexRenderResource)
#endif
					{
#if _EDITOR_ONLY
						if (VertexRenderResource->IsRenderResourceValid())
#endif
						{
							RenderDevice->SetVertexBuffer(VertexRenderResource->Buffer, VertexRenderResource->Stride, 0u, LayoutIndex);
						}
#if _EDITOR_ONLY
						else
						{
							PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome mesh vertex buffer is invalid."));
						}
#endif
					}
#if _EDITOR_ONLY
					else
					{
						PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome mesh vertex buffer is invalid."));
					}
#endif
				}
#if _EDITOR_ONLY
				else
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check sky dome mesh scene proxy vertex render resource is invalid."));
				}
#endif
			}
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Check sky dome mesh scene proxy is invalid."));
		}
#endif
	}
	void RSkyLightSceneProxy::BindMaterialParameter(const UINT32 InSlot)const
	{
		const RBufferResource& ConstantBuffer = SkyLightMaterialParameter.GetConstantBuffer();
#if _EDITOR_ONLY
		if (((InSlot >= 0u) && (InSlot < 16u)) && (ConstantBuffer.IsRenderResourceValid()))
#endif
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			RenderDevice->BindVSConstantBuffer(ConstantBuffer.Buffer, InSlot);
			RenderDevice->BindPSConstantBuffer(ConstantBuffer.Buffer, InSlot);
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Sky dome constant buffer is invalid."));
		}
#endif
	}

};