/*
#include "../Headers/CRenderPipeline.h"
#include "../../../EngineBase/Headers/CManager.h"
#include "../../../EngineGame/Headers/CGameObject.h"
#include "../../../EngineGame/Headers/CCamera.h"
#include "../../../EngineGame/Headers/CLightType.h"
#include "../Headers/CGPUQuery.h"
#include "../Headers/CGPUQueryManager.h"
#include "../Headers/CGPUCulling.h"
#include "../../AssetsManager/Headers/CMeshManager.h"
#include "../../RenderBase/Headers/CMaterialBase.h"
#include "../../../EngineGame/Headers/CComponent.h"
#include "../Headers/CMeshRendererComponent.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../AssetsManager/Headers/CTextureType.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../../EngineGame/Headers/CSkyBox.h"
#include "../../RenderFeatures/Headers/CGTAOPass.h"
#include "../../RenderFeatures/Headers/CHZBPass.h"
#include "../../RenderFeatures/Headers/CDynamicWind.h"
#include "../../RenderFeatures/Headers/CPostProcessBase.h"

#ifdef _DEVELOPMENT_EDITOR
#include "../../../Development/Headers/CDebugScreen.h"
#include "../../../Development/Headers/CGPUProfiler.h"
#endif

#ifdef _DEVELOPMENT_EDITOR
#include "../../../Development/Headers/CBezierGrass.h"

CBezierGrass _GBezierGrass;
#endif

CTexture2D*							CRenderPipeline::m_DefaultTexture[CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_COUNT] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
std::shared_ptr<CPixelShader>		CRenderPipeline::m_DefaultEmptyPS		= nullptr;
std::shared_ptr<CVertexShader>		CRenderPipeline::m_FullScreenPolygonVS	= nullptr;
std::shared_ptr<CPixelShader>		CRenderPipeline::m_ScreenPolygonShader	= nullptr;
std::shared_ptr<CPixelShader>		CRenderPipeline::m_DirectLightShader	= nullptr;
std::shared_ptr<CPostProcessBase>	CRenderPipeline::m_PostProcessBase		= nullptr;
std::shared_ptr<CSkyBox>			CRenderPipeline::m_SkyBox				= nullptr;
std::shared_ptr<CGPUCulling>		CRenderPipeline::m_GPUCulling			= nullptr;
std::shared_ptr<CGTAOPass>			CRenderPipeline::m_GTAOPass				= nullptr;
std::shared_ptr<CHZBPass>			CRenderPipeline::m_HZBPass				= nullptr;
std::shared_ptr<CDynamicWind>		CRenderPipeline::m_DynamicWind			= nullptr;
#ifdef _DEVELOPMENT_EDITOR
std::shared_ptr<CDebugScreen>		CRenderPipeline::m_DebugScreen			= nullptr;
#endif

CRenderPipeline::CRenderPipeline()
{
	for (INT i = 0; i < CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_COUNT; i++)
	{
		if (CRenderPipeline::m_DefaultTexture[i] == NULL)
		{
			CRenderPipeline::m_DefaultTexture[i] = CTextureManager::LoadTexture2D(CustomStruct::CEngineDefaultDefines::GetDefaultTexturePath(i), FALSE);
		}
	}
	if (CRenderPipeline::m_DefaultEmptyPS == nullptr)
	{
		CRenderPipeline::m_DefaultEmptyPS = std::shared_ptr<CPixelShader>(CShaderManager::LoadPixelShader(ENGINE_SHADER_EMPTY_PS));
	}
	if (CRenderPipeline::m_FullScreenPolygonVS == nullptr)
	{
		CustomStruct::CRenderInputLayoutDesc desc[2u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		CRenderPipeline::m_FullScreenPolygonVS = std::shared_ptr<CVertexShader>(CShaderManager::LoadVertexShader(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, desc, 2u));
	}
	if (CRenderPipeline::m_ScreenPolygonShader == nullptr)
	{
		CRenderPipeline::m_ScreenPolygonShader = std::shared_ptr<CPixelShader>(CShaderManager::LoadPixelShader(ENGINE_SHADER_SCREEN_POLYGON_2D_PS));
	}
	if (CRenderPipeline::m_DirectLightShader == nullptr)
	{
		CRenderPipeline::m_DirectLightShader = std::shared_ptr<CPixelShader>(CShaderManager::LoadPixelShader(ENGINE_SHADER_DIRECT_LIGHT_PS));
	}
	if (CRenderPipeline::m_PostProcessBase == nullptr)
	{
		CRenderPipeline::m_PostProcessBase = std::shared_ptr<CPostProcessBase>(new CPostProcessBase());
	}
	if (CRenderPipeline::m_SkyBox == nullptr)
	{
		CRenderPipeline::m_SkyBox = std::shared_ptr<CSkyBox>(new CSkyBox());
	}
	if (CRenderPipeline::m_GPUCulling == nullptr)
	{
		CRenderPipeline::m_GPUCulling = std::shared_ptr<CGPUCulling>(new CGPUCulling());
	}
	if (CRenderPipeline::m_GTAOPass == nullptr)
	{
		CRenderPipeline::m_GTAOPass = std::shared_ptr<CGTAOPass>(new CGTAOPass());
	}
	if (CRenderPipeline::m_HZBPass == nullptr)
	{
		CRenderPipeline::m_HZBPass = std::shared_ptr<CHZBPass>(new CHZBPass());
	}
	if (CRenderPipeline::m_DynamicWind == nullptr)
	{
		CRenderPipeline::m_DynamicWind = std::shared_ptr<CDynamicWind>(new CDynamicWind());
	}
#ifdef _DEVELOPMENT_EDITOR
	if (CRenderPipeline::m_DebugScreen == nullptr)
	{
		CRenderPipeline::m_DebugScreen = std::shared_ptr<CDebugScreen>(new CDebugScreen());
	}
#endif

	m_Scene				= NULL;
	m_SceneCamera		= NULL;
	m_FrameIndex		= 0;
	m_GlobalBufferSize	= CustomType::Vector2Int(0, 0);
	m_ShadowBufferSize	= CustomType::Vector2Int(0, 0);
	m_FullScreenPolygon = NULL;

#ifdef _DEVELOPMENT_EDITOR
	m_ShadowPassGPUAverageTime = static_cast<DOUBLE>(0);
	m_ShowGPUProfiler = FALSE;
	m_ShowCPUCullingInfo = FALSE;
	m_ShowGPUCullingInfo = FALSE;
	m_ShowDebugScreen = FALSE;
	m_ShowGTAOEditor = FALSE;
	m_ShowHZBEditor = FALSE;
	m_ShowDynamicWindEditor = FALSE;
#endif
}
CRenderPipeline::~CRenderPipeline()
{
}
void CRenderPipeline::Init(const CScene* scene, const CustomType::Vector2Int& bufferSize)
{
#ifdef _DEVELOPMENT_EDITOR
	m_ShadowPassGPUAverageTime = static_cast<DOUBLE>(0);
	m_ShowGPUProfiler = FALSE;
	m_ShowCPUCullingInfo = FALSE;
	m_ShowGPUCullingInfo = FALSE;
	m_ShowDebugScreen = FALSE;
	m_ShowGTAOEditor = FALSE;
	m_ShowHZBEditor = FALSE;
	m_ShowDynamicWindEditor = FALSE;
#endif

	m_Scene = scene;
	m_GlobalBufferSize = bufferSize;
	m_ShadowBufferSize = CustomType::Vector2Int(2048, 2048);
	m_GlobalCullingInfo.Distance = ENGINE_DEFAULT_CULLING_DISTANCE;
	m_GlobalCullingInfo.ClipOffset = ENGINE_DEFAULT_CULLING_OFFSET;

	{
		CustomType::Vector4Int fullScreenSize(0, 0, bufferSize.X(), bufferSize.Y());
		CustomStruct::CRenderInputLayoutDesc desc[2u] = {
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_POSITION),
			CustomStruct::CRenderInputLayoutDesc(CustomStruct::CRenderShaderSemantic::SHADER_SEMANTIC_TEXCOORD) };
		m_FullScreenPolygon = CMeshManager::LoadPolygon2D(fullScreenSize, desc, 2u);
	}

	for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
	{
		if (m_ScenePrimitives[i].size() != 0)
		{
			m_ScenePrimitives[i].clear();
		}
	}
	if (m_SceneLightList.size() != 0)
	{
		m_SceneLightList.clear();
	}
	if (m_CameraCullingResults.size() != 0)
	{
		m_CameraCullingResults.clear();
	}
	for (INT i = 0; i < CullingResultsLayer::CULLINGRESULTS_COUNT; i++)
	{
		if (m_RenderingCullingResults[i].size() != 0)
		{
			m_RenderingCullingResults[i].clear();
		}
	}
	if (m_DirLightCullingResults.size() != 0)
	{
		m_DirLightCullingResults.clear();
	}

	{
		CRenderDevice::CreateBuffer(
			m_RenderPerFrameInfo.PerFrameBuffer,
			CustomStruct::CRenderBufferDesc(
				sizeof(CustomStruct::CShaderGlobalPerFrame),
				CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER,
				sizeof(FLOAT)));
		CRenderDevice::CreateBuffer(
			m_RenderLightDataInfo.LightDataBuffer,
			CustomStruct::CRenderBufferDesc(
				sizeof(CustomStruct::CShaderGlobalLightData),
				CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER,
				sizeof(FLOAT)));
	}

	{
		UINT bufferWidth = static_cast<UINT>(bufferSize.X());
		UINT bufferHeight = static_cast<UINT>(bufferSize.Y());
		for (UINT i = 0u; i < CRenderPipeline::GEOMETRY_BUFFER_COUNT; i++)
		{
			CRenderDevice::CreateRenderTexture2D(
				m_RTGBuffer[i],
				CustomStruct::CRenderTextureDesc(
					bufferWidth,
					bufferHeight,
					CustomStruct::CRenderBindFlag::BIND_RTV_SRV,
					CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_FLOAT));
		}
		CRenderDevice::CreateRenderTexture2D(
			m_RTSceneColor,
			CustomStruct::CRenderTextureDesc(
				bufferWidth,
				bufferHeight,
				CustomStruct::CRenderBindFlag::BIND_RTV_SRV,
				CustomStruct::CRenderFormat::FORMAT_R11G11B10_FLOAT));
		CustomStruct::CRenderFormat format = CustomStruct::CRenderFormat::FORMAT_UNKNOWN;
		CRenderDevice::CreateRenderTexture2D(
			m_RTSceneDepth,
			CustomStruct::CRenderTextureDesc(
				bufferWidth,
				bufferHeight,
				CustomStruct::CRenderBindFlag::BIND_NONE,
				format, &format, &format, &format,
				32u));
		for (INT i = 0; i < 2; i++)
		{
			CRenderDevice::CreateRenderTexture2D(
				m_RTPostProcess[i],
				CustomStruct::CRenderTextureDesc(
					bufferWidth,
					bufferHeight,
					CustomStruct::CRenderBindFlag::BIND_RTV_SRV,
					CustomStruct::CRenderFormat::FORMAT_R11G11B10_FLOAT));
		}
	}

	{
		CRenderDevice::CreateRasterizerState(m_PipelineRS,
			CustomStruct::CRenderRasterizerState(
				CustomStruct::CRenderCullMode::CULL_BACK,
				CustomStruct::CRenderFillMode::FILL_SOLID));

		std::vector<CustomStruct::CRenderBlendState> blendStates;
		blendStates.resize(1);
		blendStates[0] = CustomStruct::CRenderBlendState();
		CRenderDevice::CreateBlendState(m_ShadowSkyForwardPrePassBS, blendStates);
		blendStates.resize(4);
		blendStates[0] = CustomStruct::CRenderBlendState();
		blendStates[1] = CustomStruct::CRenderBlendState();
		blendStates[2] = CustomStruct::CRenderBlendState();
		blendStates[3] = CustomStruct::CRenderBlendState();
		CRenderDevice::CreateBlendState(m_GBufferPassBS, blendStates);
		blendStates.resize(1);
		blendStates[0] = CustomStruct::CRenderBlendState();
		CRenderDevice::CreateBlendState(m_DirectLightPostPassBS, blendStates);
		blendStates.resize(1);
		blendStates[0] = CustomStruct::CRenderBlendState(
			CustomStruct::CRenderBlendOption::BLEND_SRC_ALPHA,
			CustomStruct::CRenderBlendOption::BLEND_INV_SRC_ALPHA,
			CustomStruct::CRenderBlendOperation::BLEND_OP_ADD,
			CustomStruct::CRenderBlendOption::BLEND_ONE,
			CustomStruct::CRenderBlendOption::BLEND_ONE,
			CustomStruct::CRenderBlendOperation::BLEND_OP_MAX);
		CRenderDevice::CreateBlendState(m_TransparentPassBS, blendStates);

		CRenderDevice::CreateDepthStencilState(m_ShadowPrePassDSS,
			CustomStruct::CRenderDepthState(
				CustomStruct::CRenderComparisonFunction::COMPARISON_LESS));
		CRenderDevice::CreateDepthStencilState(m_GBufferForwardPassDSS,
			CustomStruct::CRenderDepthState(
				CustomStruct::CRenderComparisonFunction::COMPARISON_LESS_EQUAL));
		CRenderDevice::CreateDepthStencilState(m_DirectLightPostPassDSS,
			CustomStruct::CRenderDepthState(
				CustomStruct::CRenderComparisonFunction::COMPARISON_ALWAYS));
	}

	{
		CRenderDevice::CreateSamplerState(m_PipelineSampler[0],
			CustomStruct::CRenderSamplerState(
				CustomStruct::CRenderFilter::FILTER_POINT,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP));
		CRenderDevice::CreateSamplerState(m_PipelineSampler[1],
			CustomStruct::CRenderSamplerState(
				CustomStruct::CRenderFilter::FILTER_POINT,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP));
		CRenderDevice::CreateSamplerState(m_PipelineSampler[2],
			CustomStruct::CRenderSamplerState(
				CustomStruct::CRenderFilter::FILTER_LINEAR,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP));
		CRenderDevice::CreateSamplerState(m_PipelineSampler[3],
			CustomStruct::CRenderSamplerState(
				CustomStruct::CRenderFilter::FILTER_LINEAR,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP,
				CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP));
	}
}
void CRenderPipeline::PostInit()
{
	if (m_Scene == NULL)
	{
		return;
	}

	{
		m_SceneCamera = reinterpret_cast<CCamera*>(m_Scene->m_MainCamera);

		CLightDirectional* sceneMainDirectionalLight = NULL;
		for (auto& lightObj : m_Scene->m_Lights)
		{
			if (lightObj.second && lightObj.second->IsActive())
			{
				CLightDirectional* tempLight = reinterpret_cast<CLightDirectional*>(lightObj.second);
				if (tempLight)
				{
					sceneMainDirectionalLight = tempLight;
					break;
				}
			}
		}
		if (sceneMainDirectionalLight)
		{
			if (!(sceneMainDirectionalLight->SetShadowInfo(m_ShadowBufferSize, 32u)))
			{
				//TODO
			}
		}
	}

	{
		m_FrameIndex = 0u;

		m_SkyBox->Init(m_GlobalBufferSize, CSkyBox::SkyBoxInfo(1000000.f));

		m_GTAOPass->Init(m_SceneCamera, m_GlobalBufferSize);
		m_HZBPass->Init(m_GlobalBufferSize);
		m_GPUCulling->Init(m_HZBPass, 1u, 50u);
		m_DynamicWind->Init();
#ifdef _DEVELOPMENT_EDITOR
		m_DebugScreen->Init(m_GlobalBufferSize);
#endif

#ifdef _DEVELOPMENT_EDITOR
		_GBezierGrass.Init();
#endif
	}
}
void CRenderPipeline::Uninit()
{
	m_Scene = NULL;
	m_SceneCamera = NULL;
	for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
	{
		if (m_ScenePrimitives[i].size() != 0)
		{
			m_ScenePrimitives[i].clear();
		}
	}
	if (m_SceneLightList.size() != 0)
	{
		m_SceneLightList.clear();
	}
	if (m_CameraCullingResults.size() != 0)
	{
		m_CameraCullingResults.clear();
	}
	for (INT i = 0; i < CullingResultsLayer::CULLINGRESULTS_COUNT; i++)
	{
		if (m_RenderingCullingResults[i].size() != 0)
		{
			m_RenderingCullingResults[i].clear();
		}
	}
	if (m_DirLightCullingResults.size() != 0)
	{
		m_DirLightCullingResults.clear();
	}

#ifdef _DEVELOPMENT_EDITOR
	_GBezierGrass.Uninit();
#endif
#ifdef _DEVELOPMENT_EDITOR
	m_DebugScreen->Uninit();
#endif
	m_DynamicWind->Uninit();
	m_GTAOPass->Uninit();
	m_HZBPass->Uninit();
	m_GPUCulling->Uninit();

	m_SkyBox->Uninit();
}
void CRenderPipeline::PostUpdate()
{
	if (m_Scene == NULL || m_SceneCamera == NULL)
	{
		return;
	}

	m_FrameIndex += 1u;

	{
		for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
		{
			if (m_ScenePrimitives[i].size() != 0)
			{
				m_ScenePrimitives[i].clear();
			}
		}
		if (m_SceneLightList.size() != 0)
		{
			m_SceneLightList.clear();
		}
		if (m_CameraCullingResults.size() != 0)
		{
			m_CameraCullingResults.clear();
		}
		for (INT i = 0; i < CullingResultsLayer::CULLINGRESULTS_COUNT; i++)
		{
			if (m_RenderingCullingResults[i].size() != 0)
			{
				m_RenderingCullingResults[i].clear();
			}
		}
		if (m_DirLightCullingResults.size() != 0)
		{
			m_DirLightCullingResults.clear();
		}
	}

	{
		for (auto& obj : m_Scene->m_Lights)
		{
			if (!(obj.second) || !(obj.second->IsActive()))
			{
				continue;
			}
			CLightBase* tempLightBase = reinterpret_cast<CLightBase*>(obj.second);
			if (!tempLightBase)
			{
				continue;
			}
			m_SceneLightList.push_back(tempLightBase);
			if (tempLightBase->IsTransmitShadow())
			{
				{
					CLightDirectional* tempLightDir = reinterpret_cast<CLightDirectional*>(tempLightBase);
					if (tempLightDir)
					{
						DirectionalCascadeShadowPerLightPrimitives tempCullingResults;
						m_DirLightCullingResults.insert_or_assign(tempLightDir, tempCullingResults);

						std::vector<FLOAT> cascadeDistances(3u, 0.f);
						cascadeDistances[0] = 1000.f;
						cascadeDistances[1] = 10000.f;
						cascadeDistances[2] = 80000.f;
						std::vector<FLOAT> cascadeBorders(2u, 0.15f);
						cascadeBorders[0] = 0.15f;
						cascadeBorders[1] = 0.1f;
						CLightDirectional::ShadowCascadeSettings cascadeSettings(3u, cascadeDistances, cascadeBorders);
						tempLightDir->PrepareCascadeShadowInfo(m_SceneCamera, &cascadeSettings);
						tempLightDir->UpdateCascadeShadowInfo();
					}
				}
				//TODO other light type's shadow culling list.
			}
		}
		for (auto& obj : m_Scene->m_GameObjects[CScene::SceneLayout::LAYOUT_TERRAIN])
		{
			if (obj.second && obj.second->IsActive())
			{
				CGameObject* tempGameObjectPtr = reinterpret_cast<CGameObject*>(obj.second);
				if (tempGameObjectPtr && tempGameObjectPtr->HasMeshComponent() && tempGameObjectPtr->HasMeshRendererComponent())
				{
					const CMeshComponent* mesh = tempGameObjectPtr->GetMeshComponent<CMeshComponent>();
					const CMeshRendererComponent* meshRenderer = tempGameObjectPtr->GetMeshRendererComponent<CMeshRendererComponent>();
					if (mesh && meshRenderer && mesh->IsActive() && mesh->HasMesh() && meshRenderer->IsActive() && meshRenderer->HasMesh() && meshRenderer->HasMaterial())
					{
						m_ScenePrimitives[CScene::SceneLayout::LAYOUT_TERRAIN].push_back(tempGameObjectPtr);
						break;
					}
				}
			}
		}
		for (auto& obj : m_Scene->m_GameObjects[CScene::SceneLayout::LAYOUT_OPAQUE])
		{
			if (obj.second && obj.second->IsActive())
			{
				CGameObject* tempGameObjectPtr = reinterpret_cast<CGameObject*>(obj.second);
				if (tempGameObjectPtr && tempGameObjectPtr->HasMeshComponent() && tempGameObjectPtr->HasMeshRendererComponent())
				{
					const CMeshComponent* mesh = tempGameObjectPtr->GetMeshComponent<CMeshComponent>();
					if (mesh == nullptr)
					{
						mesh = tempGameObjectPtr->GetMeshComponent<CSkeletonMeshComponent>();
					}
					const CMeshRendererComponent* meshRenderer = tempGameObjectPtr->GetMeshRendererComponent<CMeshRendererComponent>();
					if (mesh && meshRenderer && mesh->IsActive() && mesh->HasMesh() && meshRenderer->IsActive() && meshRenderer->HasMesh() && meshRenderer->HasMaterial())
					{
						m_ScenePrimitives[CScene::SceneLayout::LAYOUT_OPAQUE].push_back(tempGameObjectPtr);
					}
				}
			}
		}
		for (auto& obj : m_Scene->m_GameObjects[CScene::SceneLayout::LAYOUT_TRANSPARENT])
		{
			if (obj.second && obj.second->IsActive())
			{
				CGameObject* tempGameObjectPtr = reinterpret_cast<CGameObject*>(obj.second);
				if (tempGameObjectPtr && tempGameObjectPtr->HasMeshComponent() && tempGameObjectPtr->HasMeshRendererComponent())
				{
					const CMeshComponent* mesh = tempGameObjectPtr->GetMeshComponent<CMeshComponent>();
					const CMeshRendererComponent* meshRenderer = tempGameObjectPtr->GetMeshRendererComponent<CMeshRendererComponent>();
					if (mesh && meshRenderer && mesh->IsActive() && mesh->HasMesh() && meshRenderer->IsActive() && meshRenderer->HasMesh() && meshRenderer->HasMaterial())
					{
						m_ScenePrimitives[CScene::SceneLayout::LAYOUT_TRANSPARENT].push_back(tempGameObjectPtr);
					}
				}
			}
		}
	}

	Culling();

	m_SkyBox->Update();

	m_GTAOPass->Update();
	m_HZBPass->Update();
	m_DynamicWind->Update();
	m_GPUCulling->Update(m_FrameIndex);
#ifdef _DEVELOPMENT_EDITOR
	m_DebugScreen->Update();
#endif
#ifdef _DEVELOPMENT_EDITOR
	_GBezierGrass.Update();
#endif
}
void CRenderPipeline::Render()
{
	if (m_Scene == NULL || m_SceneCamera == NULL)
	{
		return;
	}

	{
		CRenderDevice::ClearRenderTargetView(m_RTSceneColor.RenderTargetView);
		for (UINT i = 0u; i < CRenderPipeline::GEOMETRY_BUFFER_COUNT; i++)
		{
			CRenderDevice::ClearRenderTargetView(m_RTGBuffer[i].RenderTargetView);
		}
		for (UINT i = 0u; i < 2u; i++)
		{
			CRenderDevice::ClearRenderTargetView(m_RTPostProcess[i].RenderTargetView);
		}
		CRenderDevice::ClearDepthStencilView(m_RTSceneDepth.DepthStencilView);
		CRenderDevice::ClearFinalOutput();

		CRenderDevice::BindVSSamplerStates(m_PipelineSampler, ENGINE_SAMPLER_FIRST_START_SLOT, 4u);
		CRenderDevice::BindPSSamplerStates(m_PipelineSampler, ENGINE_SAMPLER_FIRST_START_SLOT, 4u);
		CRenderDevice::BindCSSamplerStates(m_PipelineSampler, ENGINE_SAMPLER_FIRST_START_SLOT, 4u);

		CRenderDevice::SetRasterizerState(m_PipelineRS);
		CRenderDevice::SetPrimitiveTopology(CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	{
		PrepareLightDataRender();
		CRenderDevice::UploadBuffer(m_RenderLightDataInfo.LightDataBuffer, &(m_RenderLightDataInfo.LightData));
		CRenderDevice::BindVSConstantBuffer(m_RenderLightDataInfo.LightDataBuffer, ENGINE_CBUFFER_VS_PS_CS_LIGHT_DATA_START_SLOT);
		CRenderDevice::BindPSConstantBuffer(m_RenderLightDataInfo.LightDataBuffer, ENGINE_CBUFFER_VS_PS_CS_LIGHT_DATA_START_SLOT);
		CRenderDevice::BindCSConstantBuffer(m_RenderLightDataInfo.LightDataBuffer, ENGINE_CBUFFER_VS_PS_CS_LIGHT_DATA_START_SLOT);
	}

	CLightDirectional* lightShadow = NULL;
	BOOL needShadowPass = FALSE;
	{
		for (auto& lightObj : m_SceneLightList)
		{
			CLightDirectional* tempLight = reinterpret_cast<CLightDirectional*>(lightObj);
			if (tempLight && tempLight->IsTransmitShadow())
			{
				DirectionalCascadeShadowCullingResults::iterator element = m_DirLightCullingResults.find(tempLight);
				if (element == m_DirLightCullingResults.end())
				{
					continue;
				}
				lightShadow = tempLight;
				needShadowPass = (m_DirLightCullingResults[tempLight]).size() > 0;
				if (needShadowPass)
				{
					break;
				}
			}
		}
		//Shadow pass
		if (needShadowPass)
		{
			UINT layerNum = lightShadow->GetCurrentShadowMapLayerNum();
			for (UINT layerIndex = 0u; layerIndex < layerNum; layerIndex++)
			{
				std::string passShadowName = "Shadow_Pass_";
				passShadowName += std::to_string(layerIndex);
				PrepareDirectionalLightPerFrameRender(lightShadow, layerIndex);
				CRenderDevice::UploadBuffer(m_RenderPerFrameInfo.PerFrameBuffer, &(m_RenderPerFrameInfo.PerFrameData));
				CRenderDevice::BindVSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CBUFFER_VS_PS_CS_PER_FRAME_START_SLOT);
				CRenderDevice::BindPSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CBUFFER_VS_PS_CS_PER_FRAME_START_SLOT);
				CRenderDevice::BindCSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CBUFFER_VS_PS_CS_PER_FRAME_START_SLOT);
				CustomStruct::CRenderViewport viewportShadow = lightShadow->GetCurrentShadowMapViewport(layerIndex);
				const CRenderDevice::RenderTexture2DViewInfo& shadowMap = lightShadow->GetCurrentShadowMap(layerIndex).Texture;
				CGPUProfilerManager::AddPass(passShadowName, m_FrameIndex, [&]() {
					CRenderDevice::SetDepthStencilState(m_ShadowPrePassDSS);
					CRenderDevice::SetBlendState(m_ShadowSkyForwardPrePassBS);
					CRenderDevice::ClearDepthStencilView(shadowMap.DepthStencilView, CustomStruct::CRenderClearDepthStencilFlag::CLEAR_DEPTH);
					CRenderDevice::SetRenderTarget(shadowMap.DepthStencilView);
					CRenderDevice::SetViewport(viewportShadow);
					if (m_DirLightCullingResults[lightShadow].find(layerIndex) != m_DirLightCullingResults[lightShadow].end())
					{
						for (UINT i = 0u; i < (m_DirLightCullingResults[lightShadow])[layerIndex].size(); i++)
						{
							const CMeshRendererComponent* meshRenderer = ((m_DirLightCullingResults[lightShadow])[layerIndex])[i]->GetMeshRendererComponent<CMeshRendererComponent>();
							if (meshRenderer)
							{
								meshRenderer->DrawExtra();
							}
						}
					}
					});
			}
		}
	}

	{
		PreparePerFrameRender(lightShadow, 0u);
		CRenderDevice::UploadBuffer(m_RenderPerFrameInfo.PerFrameBuffer, &(m_RenderPerFrameInfo.PerFrameData));
		CRenderDevice::BindVSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CBUFFER_VS_PS_CS_PER_FRAME_START_SLOT);
		CRenderDevice::BindPSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CBUFFER_VS_PS_CS_PER_FRAME_START_SLOT);
		CRenderDevice::BindCSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CBUFFER_VS_PS_CS_PER_FRAME_START_SLOT);
		CRenderDevice::SetViewport(m_SceneCamera->GetViewport());
	}

	{
		//Pre depth pass
		CGPUProfilerManager::AddPass("Pre-Depth_Pass", m_FrameIndex, [&]() {
			if (!needShadowPass)
			{
				CRenderDevice::SetDepthStencilState(m_ShadowPrePassDSS);
				CRenderDevice::SetBlendState(m_ShadowSkyForwardPrePassBS);
			}
			CRenderDevice::SetRenderTarget(m_RTSceneDepth.DepthStencilView);
			for (INT i = 0; i < CullingResultsLayer::CULLINGRESULTS_COUNT; ++i)
			{
				for (auto& obj : m_RenderingCullingResults[i])
				{
					const CMeshRendererComponent* meshRenderer = obj->GetMeshRendererComponent<CMeshRendererComponent>();
					if (meshRenderer)
					{
						meshRenderer->DrawExtra();
					}
				}
			}
			});
	}

	{
		//HZB GTAO pass
		CGPUProfilerManager::AddPass("HZB_Pass", m_FrameIndex, [&]() {
			CRenderDevice::SetNoRenderTarget();
			m_HZBPass->ComputeHZB(m_RTSceneDepth.ShaderResourceView);
			});
		CGPUProfilerManager::AddPass("GTAO_Pass", m_FrameIndex, [&]() {
			m_GTAOPass->ComputeGTAO(m_RTSceneDepth.ShaderResourceView);
			});
	}

	{
		//Occlusion culling pass
		CGPUProfilerManager::AddPass("Occlusion_Culling_Pass", m_FrameIndex, [&]() {
			m_GPUCulling->ComputeCulling(m_FrameIndex);
			});
	}

	{
		//Geometry buffer pass
		CGPUProfilerManager::AddPass("Geometry_Buffer_Pass", m_FrameIndex, [&]() {
			CRenderDevice::SetDepthStencilState(m_GBufferForwardPassDSS);
			CRenderDevice::SetBlendState(m_GBufferPassBS);
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> tempGBuffersRTV[CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u];
			tempGBuffersRTV[0] = m_RTSceneColor.RenderTargetView;
			for (UINT i = 0u; i < CRenderPipeline::GEOMETRY_BUFFER_COUNT; i++)
			{
				tempGBuffersRTV[i + 1u] = m_RTGBuffer[i].RenderTargetView;
			}
			CRenderDevice::SetRenderTargets(tempGBuffersRTV, CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u, m_RTSceneDepth.DepthStencilView);
			CRenderDevice::BindPSShaderResourceView(m_GTAOPass->GetResultShaderResourceView(), ENGINE_AO_GLOBAL_INPUT_START_SLOT);
			for (auto& obj : m_RenderingCullingResults[CullingResultsLayer::CULLINGRESULTS_DEFERRED])
			{
				const CMeshRendererComponent* meshRenderer = obj->GetMeshRendererComponent<CMeshRendererComponent>();
				if (meshRenderer)
				{
					meshRenderer->Draw();
				}
			}
			});
	}

	{
		//Direct light pass
		UINT layerNum = needShadowPass ? lightShadow->GetCurrentShadowMapLayerNum() : 0u;
		CGPUProfilerManager::AddPass("Direct_Light_Pass", m_FrameIndex, [&]() {
			CRenderDevice::SetDepthStencilState(m_DirectLightPostPassDSS);
			CRenderDevice::SetBlendState(m_DirectLightPostPassBS);
			CRenderDevice::SetRenderTarget(m_RTSceneColor.RenderTargetView);
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tempGBuffersSRV[CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u];
			tempGBuffersSRV[0] = m_RTSceneColor.ShaderResourceView;
			for (UINT i = 0u; i < CRenderPipeline::GEOMETRY_BUFFER_COUNT; i++)
			{
				tempGBuffersSRV[i + 1u] = m_RTGBuffer[i].ShaderResourceView;
			}
			CRenderDevice::BindPSShaderResourceViews(tempGBuffersSRV, ENGINE_GBUFFER_FIRST_START_SLOT, CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u);
			for (UINT i = 0u; i < 4u; i++)
			{
				if (i < layerNum)
				{
					CRenderDevice::BindPSShaderResourceView(lightShadow->GetCurrentShadowMap(i).Texture.ShaderResourceView, ENGINE_LIGHT_SHADOW_MAP_FIRST_START_SLOT + i);
				}
				else
				{
					CRenderDevice::BindPSShaderResourceView(CRenderPipeline::m_DefaultTexture[CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE]->GetShaderResourceView(), ENGINE_LIGHT_SHADOW_MAP_FIRST_START_SLOT + i);
				}
			}
			DrawFullScreenPolygon(CRenderPipeline::m_DirectLightShader);
			});
	}

	{
		//Opaque forward pass
		CGPUProfilerManager::AddPass("Opaque_Forward_Pass", m_FrameIndex, [&]() {
			CRenderDevice::SetDepthStencilState(m_GBufferForwardPassDSS);
			CRenderDevice::SetBlendState(m_ShadowSkyForwardPrePassBS);
			CRenderDevice::SetRenderTarget(m_RTSceneColor.RenderTargetView, m_RTSceneDepth.DepthStencilView);
			for (auto& obj : m_RenderingCullingResults[CullingResultsLayer::CULLINGRESULTS_FORWARD])
			{
				const CMeshRendererComponent* meshRenderer = obj->GetMeshRendererComponent<CMeshRendererComponent>();
				if (meshRenderer)
				{
					meshRenderer->Draw();
				}
			}
#ifdef _DEVELOPMENT_EDITOR
			_GBezierGrass.Render();
#endif
			});
	}

	{
		//Sky box pass
		CGPUProfilerManager::AddPass("Sky_Box_Pass", m_FrameIndex, [&]() {
			m_SkyBox->Draw();
			});
	}

	{
		//Transparent forward pass
		CGPUProfilerManager::AddPass("Transparent_Forward_Pass", m_FrameIndex, [&]() {
			CRenderDevice::SetBlendState(m_TransparentPassBS);
			for (auto& obj : m_RenderingCullingResults[CullingResultsLayer::CULLINGRESULTS_TRANSPARENT])
			{
				const CMeshRendererComponent* meshRenderer = obj->GetMeshRendererComponent<CMeshRendererComponent>();
				if (meshRenderer)
				{
					meshRenderer->Draw();
				}
			}
			});
	}

	{
		//TODO Test and debug.
		CGPUProfilerManager::AddPass("Debug_Pass", m_FrameIndex, [&]() {
#ifdef _DEVELOPMENT_EDITOR
			CRenderDevice::SetDepthStencilState(m_GBufferForwardPassDSS);
			CRenderDevice::SetBlendState(m_ShadowSkyForwardPrePassBS);
			CRenderDevice::SetRenderTarget(m_RTSceneColor.RenderTargetView);
			if (m_ShowDebugScreen)
			{
				if (needShadowPass)
				{
					m_DebugScreen->SetShadowMap(lightShadow->GetCurrentShadowMap(0u).Texture.ShaderResourceView);
				}
				else
				{
					m_DebugScreen->SetShadowMap(CRenderPipeline::m_DefaultTexture[CustomStruct::CEngineDefaultTexture2DType::ENGINE_DEFAULT_TEXTURE2D_TYPE_WHITE]->GetShaderResourceView());
				}
				m_DebugScreen->Draw();
			}
			m_HZBPass->DrawDebug();
			m_GTAOPass->DrawDebug();
			m_DynamicWind->DrawDebug(m_GlobalBufferSize);
			_GBezierGrass.DrawDebug();
#endif
			});
	}

	{
		//Post process pass
		CGPUProfilerManager::AddPass("Post_Process_Pass", m_FrameIndex, [&]() {
			CRenderDevice::SetDepthStencilState(m_DirectLightPostPassDSS);
			CRenderDevice::SetBlendState(m_DirectLightPostPassBS);
			CRenderDevice::SetNoRenderTarget();
			CRenderDevice::CopyTexture2DResource(m_RTSceneColor.Texture2D, m_RTPostProcess[0].Texture2D);
			CRenderDevice::BindPSShaderResourceView(m_RTSceneDepth.ShaderResourceView, ENGINE_SCENE_DEPTH);
			m_PostProcessBase->InitPerFrame(&(m_RTPostProcess[1]), &(m_RTPostProcess[0]));
			m_PostProcessBase->Draw(CPostProcessBase::PostProcessType::POST_PROCESS_COLOR_GRADING);
			DrawFullScreenPolygon(nullptr);
			});
	}

	{
		//Final output
		CRenderDevice::SetDefaultDepthStencilState();
		CRenderDevice::SetDefaultBlendState();
		CRenderDevice::SetFinalOutput();
		CRenderDevice::BindPSShaderResourceView(m_PostProcessBase->GetResultTarget()->ShaderResourceView, ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT);
		DrawFullScreenPolygon(CRenderPipeline::m_ScreenPolygonShader);
	}

#ifdef _DEVELOPMENT_EDITOR
	if (m_ShowGPUProfiler)
	{
		DOUBLE shadowPassTime = static_cast<DOUBLE>(0);
		DOUBLE timeScale = static_cast<DOUBLE>(1000);
		if (needShadowPass)
		{
			UINT layerNum = lightShadow->GetCurrentShadowMapLayerNum();
			for (UINT i = 0u; i < layerNum; i++)
			{
				std::string passShadowName = "Shadow_Pass_";
				passShadowName += std::to_string(i);
				shadowPassTime += CGPUProfilerManager::GetPassAverageTime(passShadowName);
			}
			shadowPassTime *= timeScale;
		}
		m_ShadowPassGPUAverageTime = shadowPassTime;
	}
#endif
}
#ifdef _DEVELOPMENT_EDITOR
void CRenderPipeline::EditorUpdate()
{
	bool showGPUProfiler = m_ShowGPUProfiler;
	bool showCPUCullingInfo = m_ShowCPUCullingInfo;
	bool showGPUCullingInfo = m_ShowGPUCullingInfo;
	bool showDebugScreen = m_ShowDebugScreen;
	bool showGTAOEditor = m_ShowGTAOEditor;
	bool showHZBEditor = m_ShowHZBEditor;
	bool showDynamicWindEditor = m_ShowDynamicWindEditor;

	ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
	if (ImGui::TreeNode("CurrentRenderPipelineInfo"))
	{
		ImGui::Checkbox("ShowGPUProfiler", &showGPUProfiler);
		ImGui::Checkbox("ShowCPUCullingInfo", &showCPUCullingInfo);
		ImGui::Checkbox("ShowGPUCullingInfo", &showGPUCullingInfo);
		ImGui::Checkbox("ShowDebugScreen", &showDebugScreen);
		ImGui::Checkbox("ShowGTAOEditor", &showGTAOEditor);
		ImGui::Checkbox("ShowHZBEditor", &showHZBEditor);
		ImGui::Checkbox("ShowDynamicWindEditor", &showDynamicWindEditor);
		ImGui::TreePop();
	}
	if (showGPUProfiler)
	{
		DOUBLE timeScale = static_cast<DOUBLE>(1000);
		ImGui::Begin("GPU Profiler");
		ImGui::Text("Shadow Pass          : %f ms.", m_ShadowPassGPUAverageTime);
		ImGui::Text("Pre-Depth Pass       : %f ms.", CGPUProfilerManager::GetPassAverageTime("Pre-Depth_Pass") * timeScale);
		ImGui::Text("Build HZB            : %f ms.", CGPUProfilerManager::GetPassAverageTime("HZB_Pass") * timeScale);
		ImGui::Text("Ambient Occlusion    : %f ms.", CGPUProfilerManager::GetPassAverageTime("GTAO_Pass") * timeScale);
		ImGui::Text("Occlusion Culling    : %f ms.", CGPUProfilerManager::GetPassAverageTime("Occlusion_Culling_Pass") * timeScale);
		ImGui::Text("Geometry Pass        : %f ms.", CGPUProfilerManager::GetPassAverageTime("Geometry_Buffer_Pass") * timeScale);
		ImGui::Text("Sky Pass             : %f ms.", CGPUProfilerManager::GetPassAverageTime("Sky_Box_Pass") * timeScale);
		ImGui::Text("Direct Light Pass    : %f ms.", CGPUProfilerManager::GetPassAverageTime("Direct_Light_Pass") * timeScale);
		ImGui::Text("Opaque Forward Pass  : %f ms.", CGPUProfilerManager::GetPassAverageTime("Opaque_Forward_Pass") * timeScale);
		ImGui::Text("Transparent Pass     : %f ms.", CGPUProfilerManager::GetPassAverageTime("Transparent_Forward_Pass") * timeScale);
		ImGui::Text("Post-process Pass    : %f ms.", CGPUProfilerManager::GetPassAverageTime("Post_Process_Pass") * timeScale);
		ImGui::Text("Debug Screen Info    : %f ms.", CGPUProfilerManager::GetPassAverageTime("Debug_Pass") * timeScale);
		ImGui::Text("Total Pass           : %f ms.", m_ShadowPassGPUAverageTime +
			(CGPUProfilerManager::GetPassAverageTime("Pre-Depth_Pass") +
				CGPUProfilerManager::GetPassAverageTime("HZB_Pass") +
				CGPUProfilerManager::GetPassAverageTime("GTAO_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Occlusion_Culling_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Geometry_Buffer_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Sky_Box_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Direct_Light_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Opaque_Forward_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Transparent_Forward_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Post_Process_Pass") +
				CGPUProfilerManager::GetPassAverageTime("Debug_Pass")) * timeScale);
		ImGui::End();
	}
	if (showGPUCullingInfo)
	{
		m_GPUCulling->EditorUpdate();
	}
	if (showGTAOEditor)
	{
		m_GTAOPass->EditorUpdate();
	}
	if (showHZBEditor)
	{
		m_HZBPass->EditorUpdate();
	}
	if (showDynamicWindEditor)
	{
		m_DynamicWind->EditorUpdate();
	}

	m_ShowGPUProfiler = showGPUProfiler;
	m_ShowCPUCullingInfo = showCPUCullingInfo;
	m_ShowGPUCullingInfo = showGPUCullingInfo;
	m_ShowDebugScreen = showDebugScreen;
	m_ShowGTAOEditor = showGTAOEditor;
	m_ShowHZBEditor = showHZBEditor;
	m_ShowDynamicWindEditor = showDynamicWindEditor;
}
#endif
void CRenderPipeline::DrawFullScreenPolygon(const std::shared_ptr<CPixelShader>& shader)
{
	m_FullScreenPolygonVS->Bind();
	if (shader)
	{
		shader->Bind();
	}
	CRenderDevice::SetVertexBuffer(m_FullScreenPolygon->GetVertexBuffer(), m_FullScreenPolygon->GetVertexStride());
	CRenderDevice::SetIndexBuffer(m_FullScreenPolygon->GetIndexBuffer());
	CRenderDevice::DrawIndexed(m_FullScreenPolygon->GetIndexCount());
}
void CRenderPipeline::PreparePerFrameRender(CLightDirectional* light, const UINT& index)
{
	CustomType::Vector2 cameraViewportMin = m_SceneCamera->GetViewportMinSize();
	CustomStruct::CRenderViewport vp = m_SceneCamera->GetViewport();
	CustomType::Matrix4x4 lightViewProjMatrix(CustomType::Matrix4x4::Identity());
	if (light != NULL)
	{
		lightViewProjMatrix = light->GetCurrentViewMatrix(index);
		lightViewProjMatrix = CustomType::Matrix4x4::MultiplyMatrix(lightViewProjMatrix, light->GetCurrentProjectionMatrix(0u));
	}

	m_RenderPerFrameInfo.PerFrameData.ProjectionMatrix = m_SceneCamera->GetProjectionMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ProjectionInvMatrix = m_SceneCamera->GetProjectionInverseMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.DepthMultiAdd = m_SceneCamera->GetDeviceZToViewZMulAdd().GetXMFLOAT4();
	m_RenderPerFrameInfo.PerFrameData.ScreenToViewSpaceParams = m_SceneCamera->GetScreenToViewParameters(CustomType::Vector2Int(CRenderDevice::GetViewport().Width, CRenderDevice::GetViewport().Height), CustomType::Vector2Int(CRenderDevice::GetViewport().Width, CRenderDevice::GetViewport().Height)).GetXMFLOAT4();
	m_RenderPerFrameInfo.PerFrameData.CameraViewportMinSizeAndInvBufferSize = DirectX::XMFLOAT4(cameraViewportMin.X(), cameraViewportMin.Y(), 1.f / static_cast<FLOAT>(m_GlobalBufferSize.X()), 1.f / static_cast<FLOAT>(m_GlobalBufferSize.Y()));
	m_RenderPerFrameInfo.PerFrameData.CameraViewportSizeAndInvSize = m_SceneCamera->GetViewportSizeAndInvSize().GetXMFLOAT4();
	m_RenderPerFrameInfo.PerFrameData.CameraViewportRect = DirectX::XMFLOAT4(vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height);

	m_RenderPerFrameInfo.PerFrameData.ViewMatrix = m_SceneCamera->GetViewMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewInvMatrix = m_SceneCamera->GetViewInverseMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewProjectionMatrix = m_SceneCamera->GetViewProjectionMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewProjectionInvMatrix = m_SceneCamera->GetViewProjectionInverseMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.LightViewProjectionMatrix = lightViewProjMatrix.GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.TimeParams = DirectX::XMFLOAT4(static_cast<FLOAT>(CManager::GetGameTimer()->GetClockTime()), static_cast<FLOAT>(CManager::GetGameTimer()->GetDeltaTime()), 1.f, 1.f);
	m_RenderPerFrameInfo.PerFrameData.CameraWorldPosition = m_SceneCamera->GetWorldPosition().GetXMFLOAT4();
}
void CRenderPipeline::PrepareLightDataRender()
{
	std::vector<CLightDirectional*> lightDirectional;
	std::vector<CLightPoint*> lightPoint;
	std::vector<CLightSpot*> lightSpot;
	for (auto& obj : m_SceneLightList)
	{
		CLightBase::LightType lightType = obj->GetLightType();
		if (lightType == CLightBase::LightType::LIGHT_TYPE_DIRECTIONAL)
		{
			CLightDirectional* tempLight = reinterpret_cast<CLightDirectional*>(obj);
			if (tempLight)
			{
				lightDirectional.push_back(tempLight);
			}
		}
		else if (lightType == CLightBase::LightType::LIGHT_TYPE_POINT)
		{
			CLightPoint* tempLight = reinterpret_cast<CLightPoint*>(obj);
			if (tempLight)
			{
				lightPoint.push_back(tempLight);
			}
		}
		else if (lightType == CLightBase::LightType::LIGHT_TYPE_SPOT)
		{
			CLightSpot* tempLight = reinterpret_cast<CLightSpot*>(obj);
			if (tempLight)
			{
				lightSpot.push_back(tempLight);
			}
		}
	}
	m_RenderLightDataInfo.LightData.LightCount = DirectX::XMINT4(static_cast<INT>(lightDirectional.size()), static_cast<INT>(lightPoint.size()), static_cast<INT>(lightSpot.size()), 0);
	UINT lightIndex = 0u;
	for (INT i = 0; i < m_RenderLightDataInfo.LightData.LightCount.x; i++)
	{
		if (lightIndex >= CustomStruct::CShaderGlobalLightData::GetSupportLightMaxCount())
		{
			break;
		}

		CustomStruct::CColor color; FLOAT intensity;
		lightDirectional[i]->GetColor(color, intensity);
		CustomType::Vector3 direction(lightDirectional[i]->GetForwardVector());

		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Color = DirectX::XMFLOAT4(color.r, color.g, color.b, intensity);
		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Params0 = DirectX::XMFLOAT4(direction.X(), direction.Y(), direction.Z(), 0.f);
		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Params1 = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);

		lightIndex += 1u;
	}
	for (INT i = 0; i < m_RenderLightDataInfo.LightData.LightCount.y; i++)
	{
		if (lightIndex >= CustomStruct::CShaderGlobalLightData::GetSupportLightMaxCount())
		{
			break;
		}

		CustomStruct::CColor color; FLOAT intensity;
		lightPoint[i]->GetColor(color, intensity);
		CustomType::Vector3 position(lightPoint[i]->GetWorldPosition());

		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Color = DirectX::XMFLOAT4(color.r, color.g, color.b, intensity);
		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Params0 = DirectX::XMFLOAT4(position.X(), position.Y(), position.Z(), lightPoint[i]->GetRadius());
		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Params1 = DirectX::XMFLOAT4(0.f, 0.f, 0.f, lightPoint[i]->GetAttenuationExponent());

		lightIndex += 1u;
	}
	for (INT i = 0; i < m_RenderLightDataInfo.LightData.LightCount.z; i++)
	{
		if (lightIndex >= CustomStruct::CShaderGlobalLightData::GetSupportLightMaxCount())
		{
			break;
		}

		CustomStruct::CColor color; FLOAT intensity;
		lightSpot[i]->GetColor(color, intensity);
		CustomType::Vector3 position(lightSpot[i]->GetWorldPosition());
		CustomType::Vector3 direction(lightSpot[i]->GetForwardVector());

		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Color = DirectX::XMFLOAT4(color.r, color.g, color.b, intensity);
		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Params0 = DirectX::XMFLOAT4(position.X(), position.Y(), position.Z(), lightSpot[i]->GetRange());
		m_RenderLightDataInfo.LightData.LightParams[lightIndex].Params1 = DirectX::XMFLOAT4(direction.X(), direction.Y(), direction.Z(), lightSpot[i]->GetCosHalfRadian());

		lightIndex += 1u;
	}
}
void CRenderPipeline::PrepareDirectionalLightPerFrameRender(CLightBase* light, const UINT& index)
{
	CustomType::Matrix4x4 viewMatrix(light->GetCurrentViewMatrix(index));
	CustomType::Matrix4x4 projectionMatrix(light->GetCurrentProjectionMatrix(index));
	CustomType::Matrix4x4 viewProjectionMatrix(CustomType::Matrix4x4::MultiplyMatrix(viewMatrix, projectionMatrix));

	::ZeroMemory(&(m_RenderPerFrameInfo.PerFrameData), sizeof(m_RenderPerFrameInfo.PerFrameData));

	m_RenderPerFrameInfo.PerFrameData.ProjectionMatrix = projectionMatrix.GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ProjectionInvMatrix = projectionMatrix.Inverse().GetGPUUploadFloat4x4();

	m_RenderPerFrameInfo.PerFrameData.ViewMatrix = viewMatrix.GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewInvMatrix = viewMatrix.Inverse().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewProjectionMatrix = viewProjectionMatrix.GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewProjectionInvMatrix = viewProjectionMatrix.Inverse().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.TimeParams = DirectX::XMFLOAT4(static_cast<FLOAT>(CManager::GetGameTimer()->GetClockTime()), static_cast<FLOAT>(CManager::GetGameTimer()->GetDeltaTime()), 1.f, 1.f);
}
void CRenderPipeline::PrepareCameraCullingInfo()
{
	std::vector<CustomType::Vector3> cameraPlane(m_SceneCamera->GetCullingPlane());
	m_GlobalCullingInfo.OriginPosition = m_SceneCamera->GetWorldPosition();
	m_GlobalCullingInfo.ClipPlane[0] = m_SceneCamera->GetForwardVector();
	m_GlobalCullingInfo.ClipPlane[1] = -cameraPlane[0];
	m_GlobalCullingInfo.ClipPlane[2] = -cameraPlane[1];
	m_GlobalCullingInfo.ClipPlane[3] = -cameraPlane[2];
	m_GlobalCullingInfo.ClipPlane[4] = -cameraPlane[3];
	for (INT i = 0; i < 5; i++)
	{
		m_GlobalCullingInfo.ClipDot[i] = CustomType::Vector3::Dot(m_GlobalCullingInfo.ClipPlane[i], m_GlobalCullingInfo.OriginPosition) + m_GlobalCullingInfo.ClipOffset;
	}
}
void CRenderPipeline::Culling()
{
	//Camera distance culling & frustum culling
	{
		PrepareCameraCullingInfo();
		for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
		{
			for (auto& obj : m_ScenePrimitives[i])
			{
				CustomType::Vector3 boundAnchor; FLOAT boundRadius;
				obj->GetRenderWorldBoundingSphere(boundAnchor, boundRadius);
				if (PerObjectDistanceFrustumCulling(boundAnchor, boundRadius))
				{
					m_CameraCullingResults.push_back(obj);
				}
			}
		}
	}

	//HZB occlusion culling
	{
		std::vector<BOOL> objectHZBCullingResults;
		m_GPUCulling->ReadBackAndPrepareCullingResult(m_FrameIndex, m_CameraCullingResults, objectHZBCullingResults);
		for (UINT i = 0; i < m_CameraCullingResults.size(); i++)
		{
			if (objectHZBCullingResults[i] == TRUE)
			{
				const CMeshRendererComponent* meshRenderer = m_CameraCullingResults[i]->GetMeshRendererComponent<CMeshRendererComponent>();
				CMaterialBase::MaterialType tempMaterialType = meshRenderer->GetMaterialType();
				if (tempMaterialType == CMaterialBase::MaterialType::MATERIAL_TYPE_OPAQUE_DEFERRED)
				{
					m_RenderingCullingResults[CullingResultsLayer::CULLINGRESULTS_DEFERRED].push_back(m_CameraCullingResults[i]);
				}
				else if (tempMaterialType == CMaterialBase::MaterialType::MATERIAL_TYPE_OPAQUE_FORWARD)
				{
					m_RenderingCullingResults[CullingResultsLayer::CULLINGRESULTS_FORWARD].push_back(m_CameraCullingResults[i]);
				}
				else if (tempMaterialType == CMaterialBase::MaterialType::MATERIAL_TYPE_TRANSPARENT)
				{
					m_RenderingCullingResults[CullingResultsLayer::CULLINGRESULTS_TRANSPARENT].push_back(m_CameraCullingResults[i]);
				}
			}
		}
	}

	//Light orthographic culling
	{
		if (m_DirLightCullingResults.size() == 0)
		{
			//This is function's ending.
			return;
		}
		for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
		{
			for (auto& obj : m_ScenePrimitives[i])
			{
				CustomType::Vector3 boundAnchor; FLOAT boundRadius;
				obj->GetRenderWorldBoundingSphere(boundAnchor, boundRadius);
				for (auto& cullingList : m_DirLightCullingResults)
				{
					CLightDirectional* tempLight = cullingList.first;
					std::vector<UINT> layerIndex;
					if (!PerObjectDirectionalCascadeShadowCulling(layerIndex, tempLight, boundAnchor, boundRadius))
					{
						continue;
					}
					for (UINT index = 0u; index < layerIndex.size(); index++)
					{
						DirectionalCascadeShadowPerLightPrimitives::iterator element = (cullingList.second).find(layerIndex[index]);
						if (element == (cullingList.second).end())
						{
							std::vector<CGameObject*> tempResults;
							cullingList.second.insert_or_assign(layerIndex[index], tempResults);
						}
						((cullingList.second)[layerIndex[index]]).push_back(obj);
					}
				}
			}
		}
	}

#ifdef _DEVELOPMENT_EDITOR
	if (m_ShowCPUCullingInfo)
	{
		INT tempPrimitivesCount = static_cast<INT>(m_ScenePrimitives[0].size());
		for (INT i = 1; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
		{
			tempPrimitivesCount += static_cast<INT>(m_ScenePrimitives[i].size());
		}
		INT tempCascadeShadowCount = static_cast<INT>(m_DirLightCullingResults.size());
		ImGui::Begin("CPU culling information");
		ImGui::Text("Render pipeline primitives = %d", tempPrimitivesCount);
		ImGui::Text("Pass camera culling = %d", static_cast<INT>(m_CameraCullingResults.size()));
		ImGui::Text("Cascade shadow count = %d", tempCascadeShadowCount);
		INT tempCascadeShadowIndex = 0;
		for (auto& tempResult : m_DirLightCullingResults)
		{
			UINT tempLayerNum = tempResult.first->GetCurrentShadowMapLayerNum();
			ImGui::Text("Cascade shadow index = %d", tempCascadeShadowIndex);
			ImGui::Text("Layer count = %d", static_cast<INT>(tempLayerNum));
			for (UINT layerIndex = 0u; layerIndex < tempLayerNum; layerIndex++)
			{
				INT tempLayerPassNum = static_cast<INT>((tempResult.second)[layerIndex].size());
				ImGui::Text("Layer index = %d, Pass light culling = %d", static_cast<INT>(layerIndex), tempLayerPassNum);
			}
			tempCascadeShadowIndex++;
		}
		ImGui::End();
	}
#endif
}
BOOL CRenderPipeline::PerObjectDistanceFrustumCulling(const CustomType::Vector3& pos, const FLOAT& radius)
{
	auto planeCulling = [](const CRenderCameraCullingInfo& cullingInfo, const CustomType::Vector3& pos, const FLOAT& radius)->BOOL {
		for (UINT i = 0u; i < 5u; i++)
		{
			if ((CustomType::Vector3::Dot(pos, cullingInfo.ClipPlane[i]) + radius) < cullingInfo.ClipDot[i])
			{
				return FALSE;
			}
		}
		return TRUE; };

	if ((CustomType::Vector3::Distance(pos, m_GlobalCullingInfo.OriginPosition) - radius) < m_GlobalCullingInfo.Distance)
	{
		return (planeCulling(m_GlobalCullingInfo, pos, radius));
	}
	return FALSE;
}
BOOL CRenderPipeline::PerObjectDirectionalCascadeShadowCulling(std::vector<UINT>& layerIndex, CLightDirectional* light, const CustomType::Vector3& pos, const FLOAT& radius)
{
#if 0
	static CustomType::Vector3 offset[8] = {
		CustomType::Vector3(-1, -1, -1),CustomType::Vector3(1, -1, -1),
		CustomType::Vector3(-1, -1, 1),CustomType::Vector3(1, -1, 1),
		CustomType::Vector3(-1, 1, -1),CustomType::Vector3(1, 1, -1),
		CustomType::Vector3(-1, 1, 1),CustomType::Vector3(1, 1, 1) };

	BOOL result = FALSE;
	UINT layerNum = light->GetCurrentShadowMapLayerNum();
	if (layerNum < 1u)
	{
		return result;
	}
	CustomType::Vector3 boundAnchor((light->GetCurrentViewMatrix()).MultiplyPosition(pos));
	for (UINT i = 0u; i < layerNum; i++)
	{
		BOOL currentDefinite = FALSE;
		CustomType::Matrix4x4 projMatrix(light->GetCurrentProjectionMatrix(i));
		CustomType::Vector3 tempAnchor(boundAnchor); CustomType::Vector4 tempScreenPos;
		//First calc
		{
			tempScreenPos = projMatrix.MultiplyVector(CustomType::Vector4(tempAnchor.X(), tempAnchor.Y(), tempAnchor.Z(), 1.f));
			tempScreenPos = tempScreenPos / tempScreenPos.W();
			currentDefinite = (tempScreenPos.Z() > 0.f && tempScreenPos.Z() < 1.f) && (tempScreenPos.X() > -1.f && tempScreenPos.X() < 1.f) && (tempScreenPos.Y() > -1.f && tempScreenPos.Y() < 1.f);
		}
		//Conservative calc
		for (UINT boundIndex = 0u; boundIndex < 8u; boundIndex++)
		{
			if (currentDefinite)
			{
				break;
			}
			tempAnchor = boundAnchor + (offset[boundIndex] * radius);
			tempScreenPos = projMatrix.MultiplyVector(CustomType::Vector4(tempAnchor.X(), tempAnchor.Y(), tempAnchor.Z(), 1.f));
			tempScreenPos = tempScreenPos / tempScreenPos.W();
			currentDefinite = (tempScreenPos.Z() > 0.f && tempScreenPos.Z() < 1.f) && (tempScreenPos.X() > -1.f && tempScreenPos.X() < 1.f) && (tempScreenPos.Y() > -1.f && tempScreenPos.Y() < 1.f);
		}
		if (currentDefinite)
		{
			result = TRUE;
			layerIndex.push_back(i);
		}
	}
	return result;
#else
	BOOL result = FALSE;
	UINT layerNum = light->GetCurrentShadowMapLayerNum();
	if (layerNum < 1u)
	{
		return result;
	}
	for (UINT i = 0u; i < layerNum; i++)
	{
		CustomType::Vector3 lightBoundAnchor; FLOAT lightBoundRadius;
		{
			CustomType::Vector4 lightBound(light->GetCurrentProjectionSphereBounding(i));
			lightBoundAnchor = CustomType::Vector3(lightBound);
			lightBoundRadius = lightBound.W();
		}
		FLOAT distanceSq = lightBoundRadius + radius;
		distanceSq = distanceSq * distanceSq;
		if (CustomType::Vector3::DistanceSquare(lightBoundAnchor, pos) < distanceSq)
		{
			result = TRUE;
			layerIndex.push_back(i);
		}
	}
	return result;
#endif
}
CTexture2D* CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DType input)
{
	return (CRenderPipeline::m_DefaultTexture[input]);
}
std::shared_ptr<CPixelShader> CRenderPipeline::GetDefaultEmptyPS()
{
	return (CRenderPipeline::m_DefaultEmptyPS);
}
*/