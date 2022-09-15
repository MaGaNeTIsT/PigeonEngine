#include "../Headers/CRenderPipeline.h"
#include "../../../EngineBase/Headers/CManager.h"
#include "../../../EngineGame/Headers/CGameObject.h"
#include "../../../EngineGame/Headers/CCamera.h"
#include "../../../EngineGame/Headers/CLight.h"
#include "../Headers/CRenderDevice.h"
#include "../../AssetsManager/Headers/CMesh.h"
#include "../../AssetsManager/Headers/CMeshManager.h"
#include "../Headers/CMeshRenderer.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../AssetsManager/Headers/CTexture2D.h"
#include "../../AssetsManager/Headers/CTextureManager.h"
#include "../../RenderFeatures/Headers/CGTAOComputeShader.h"
#include "../../RenderFeatures/Headers/CHZBBuffer.h"

#include "../../../Development/Headers/CDebugScreen.h"

CTexture2D*							CRenderPipeline::m_DefaultTexture[CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_COUNT] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
std::shared_ptr<CPixelShader>		CRenderPipeline::m_DefaultEmptyPS		= nullptr;
std::shared_ptr<CVertexShader>		CRenderPipeline::m_FullScreenPolygonVS	= nullptr;
std::shared_ptr<CPixelShader>		CRenderPipeline::m_ScreenPolygonShader	= nullptr;
std::shared_ptr<CPixelShader>		CRenderPipeline::m_DirectLightShader	= nullptr;
std::shared_ptr<CDebugScreen>		CRenderPipeline::m_DebugScreen			= nullptr;
std::shared_ptr<CGTAOComputeShader>	CRenderPipeline::m_GTAOComputeShader	= nullptr;
std::shared_ptr<CHZBBuffer>			CRenderPipeline::m_HZB					= nullptr;

CRenderPipeline::CRenderPipeline()
{
	for (INT i = 0; i < CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_COUNT; i++)
	{
		if (CRenderPipeline::m_DefaultTexture[i] == NULL)
		{
			CRenderPipeline::m_DefaultTexture[i] = CTextureManager::LoadTexture2D(CustomStruct::CEngineDefaultDefines::GetDefaultTexturePath(i));
		}
	}
	if (CRenderPipeline::m_DefaultEmptyPS == nullptr)
	{
		CRenderPipeline::m_DefaultEmptyPS = CShaderManager::LoadPixelShader(ENGINE_SHADER_EMPTY_PS);
	}
	if (CRenderPipeline::m_FullScreenPolygonVS == nullptr)
	{
		CRenderPipeline::m_FullScreenPolygonVS = CShaderManager::LoadVertexShader(ENGINE_SHADER_SCREEN_POLYGON_2D_VS);
	}
	if (CRenderPipeline::m_ScreenPolygonShader == nullptr)
	{
		CRenderPipeline::m_ScreenPolygonShader = CShaderManager::LoadPixelShader(ENGINE_SHADER_SCREEN_POLYGON_2D_PS);
	}
	if (CRenderPipeline::m_DirectLightShader == nullptr)
	{
		CRenderPipeline::m_DirectLightShader = CShaderManager::LoadPixelShader(ENGINE_SHADER_DIRECT_LIGHT_PS);
	}
	if (CRenderPipeline::m_DebugScreen == nullptr)
	{
		CRenderPipeline::m_DebugScreen = std::shared_ptr<CDebugScreen>(new CDebugScreen());
	}
	if (CRenderPipeline::m_GTAOComputeShader == nullptr)
	{
		CRenderPipeline::m_GTAOComputeShader = std::shared_ptr<CGTAOComputeShader>(new CGTAOComputeShader());
	}
	if (CRenderPipeline::m_HZB == nullptr)
	{
		CRenderPipeline::m_HZB = std::shared_ptr<CHZBBuffer>(new CHZBBuffer());
	}

	m_CurrentScene		= NULL;
	m_GlobalBufferSize	= CustomType::Vector2Int(0, 0);
	m_FullScreenPolygon	= NULL;
}
CRenderPipeline::~CRenderPipeline()
{
}
void CRenderPipeline::Init(const CScene* scene, const CustomType::Vector2Int& bufferSize)
{
	m_CurrentScene = scene;
	m_GlobalBufferSize = bufferSize;

	{
		CustomType::Vector4Int fullScreenSize(0, 0, bufferSize.X(), bufferSize.Y());
		m_FullScreenPolygon = CMeshManager::LoadPolygon2DMesh(fullScreenSize);
	}

	for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
	{
		if (m_CurrentScenePrimitives[i].size() != 0)
		{
			m_CurrentScenePrimitives[i].clear();
		}
	}

	{
		CRenderDevice::CreateBuffer(
			m_RenderPerFrameInfo.PerFrameBuffer,
			CustomStruct::CRenderBufferDesc(
				sizeof(CustomStruct::ConstantBufferPerFrame),
				CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER,
				sizeof(FLOAT)));
	}

	{
		UINT bufferWidth = static_cast<UINT>(bufferSize.X());
		UINT bufferHeight = static_cast<UINT>(bufferSize.Y());
		for (UINT i = 0u; i < CRenderPipeline::GEOMETRY_BUFFER_COUNT; i++)
		{
			CRenderDevice::CreateRenderTexture2D(
				m_GBuffer[i],
				CustomStruct::CRenderTextureDesc(
					bufferWidth,
					bufferHeight,
					CustomStruct::CRenderBindFlag::BIND_RTV_SRV,
					CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_FLOAT));
		}
		CRenderDevice::CreateRenderTexture2D(
			m_SceneColor,
			CustomStruct::CRenderTextureDesc(
				bufferWidth,
				bufferHeight,
				CustomStruct::CRenderBindFlag::BIND_RTV_SRV,
				CustomStruct::CRenderFormat::FORMAT_R11G11B10_FLOAT));
		CustomStruct::CRenderFormat format = CustomStruct::CRenderFormat::FORMAT_UNKNOWN;
		CRenderDevice::CreateRenderTexture2D(
			m_SceneDepth,
			CustomStruct::CRenderTextureDesc(
				bufferWidth,
				bufferHeight,
				CustomStruct::CRenderBindFlag::BIND_NONE,
				format, &format, &format, &format,
				32u));
		for (INT i = 0; i < 2; i++)
		{
			CRenderDevice::CreateRenderTexture2D(
				m_PostProcessColor[i],
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
		CRenderDevice::CreateBlendState(m_ShadowPrePassBS, blendStates);
		blendStates.resize(4);
		blendStates[0] = CustomStruct::CRenderBlendState();
		blendStates[1] = CustomStruct::CRenderBlendState();
		blendStates[2] = CustomStruct::CRenderBlendState();
		blendStates[3] = CustomStruct::CRenderBlendState();
		CRenderDevice::CreateBlendState(m_GBufferPassBS, blendStates);
		blendStates.resize(1);
		blendStates[0] = CustomStruct::CRenderBlendState(
			CustomStruct::CRenderBlendOption::BLEND_ONE,
			CustomStruct::CRenderBlendOption::BLEND_ONE,
			CustomStruct::CRenderBlendOperation::BLEND_OP_ADD,
			CustomStruct::CRenderBlendOption::BLEND_ONE,
			CustomStruct::CRenderBlendOption::BLEND_ONE,
			CustomStruct::CRenderBlendOperation::BLEND_OP_MAX);
		CRenderDevice::CreateBlendState(m_DirectLightPassBS, blendStates);

		CRenderDevice::CreateDepthStencilState(m_ShadowPrePassDSS,
			CustomStruct::CRenderDepthState(
				CustomStruct::CRenderComparisonFunction::COMPARISON_LESS));
		CRenderDevice::CreateDepthStencilState(m_GBufferPassDSS,
			CustomStruct::CRenderDepthState(
				CustomStruct::CRenderComparisonFunction::COMPARISON_LESS_EQUAL));
		CRenderDevice::CreateDepthStencilState(m_DirectLightPassDSS,
			CustomStruct::CRenderDepthState(
				CustomStruct::CRenderComparisonFunction::COMPARISON_ALWAYS));
	}
}
void CRenderPipeline::PostInit()
{
	if (m_CurrentScene == NULL)
		return;

	{
		std::vector<CLight*> rawSceneLights = m_CurrentScene->GetGameObjectAll<CLight>(CScene::SceneLayout::LAYOUT_LIGHT);
		CLight* sceneMainLight = NULL;
		for (INT i = 0; i < rawSceneLights.size(); i++)
		{
			if (rawSceneLights[i]->IsActive())
			{
				sceneMainLight = rawSceneLights[i];
				break;
			}
		}
		UINT bufferWidth = static_cast<UINT>(sceneMainLight->GetLightShadowSize().X());
		UINT bufferHeight = static_cast<UINT>(sceneMainLight->GetLightShadowSize().Y());
		CustomStruct::CRenderFormat format = CustomStruct::CRenderFormat::FORMAT_UNKNOWN;
		CRenderDevice::CreateRenderTexture2D(
			m_ShadowBuffer,
			CustomStruct::CRenderTextureDesc(
				bufferWidth,
				bufferHeight,
				CustomStruct::CRenderBindFlag::BIND_NONE,
				format, &format, &format, &format,
				32u));
	}

	{
		std::vector<CCamera*> rawSceneCameras = m_CurrentScene->GetGameObjectAll<CCamera>(CScene::SceneLayout::LAYOUT_CAMERA);
		CCamera* sceneMainCamera = NULL;
		for (INT i = 0; i < rawSceneCameras.size(); i++)
		{
			if (rawSceneCameras[i]->IsActive())
			{
				sceneMainCamera = rawSceneCameras[i];
				break;
			}
		}
		m_DebugScreen->Init(m_GlobalBufferSize);
		m_GTAOComputeShader->Init(sceneMainCamera, CustomType::Vector2Int((m_GlobalBufferSize.X() + 1) / 2, (m_GlobalBufferSize.Y() + 1) / 2), m_GlobalBufferSize);
		m_HZB->Init(CustomType::Vector2Int((m_GlobalBufferSize.X() + 1) / 2, (m_GlobalBufferSize.Y() + 1) / 2), m_GlobalBufferSize);
	}
}
void CRenderPipeline::Uninit()
{
	for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
	{
		if (m_CurrentScenePrimitives[i].size() != 0)
		{
			m_CurrentScenePrimitives[i].clear();
		}
	}

	m_DebugScreen->Uninit();
	m_GTAOComputeShader->Uninit();
	m_HZB->Uninit();
}
void CRenderPipeline::PostUpdate()
{
	if (m_CurrentScene == NULL)
		return;

	{
		std::vector<CCamera*>		rawSceneCameras			= m_CurrentScene->GetGameObjectAll<CCamera>(CScene::SceneLayout::LAYOUT_CAMERA);
		std::vector<CLight*>		rawSceneLights			= m_CurrentScene->GetGameObjectAll<CLight>(CScene::SceneLayout::LAYOUT_LIGHT);
		std::vector<CGameObject*>	rawSceneTerrains		= m_CurrentScene->GetGameObjectAll<CGameObject>(CScene::SceneLayout::LAYOUT_TERRAIN);
		std::vector<CGameObject*>	rawSceneOpaques			= m_CurrentScene->GetGameObjectAll<CGameObject>(CScene::SceneLayout::LAYOUT_OPAQUE);
		std::vector<CGameObject*>	rawSceneTransparents	= m_CurrentScene->GetGameObjectAll<CGameObject>(CScene::SceneLayout::LAYOUT_TRANSPARENT);
		std::vector<CGameObject*>	rawSceneSkys			= m_CurrentScene->GetGameObjectAll<CGameObject>(CScene::SceneLayout::LAYOUT_SKY);
		for (INT i = 0; i < CScene::SceneLayout::LAYOUT_COUNT; i++)
		{
			if (m_CurrentScenePrimitives[i].size() != 0)
			{
				m_CurrentScenePrimitives[i].clear();
			}
		}
		for (INT i = 0; i < rawSceneCameras.size(); i++)
		{
			if (rawSceneCameras[i]->IsActive())
			{
				m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_CAMERA].push_back(rawSceneCameras[i]);
			}
		}
		for (INT i = 0; i < rawSceneLights.size(); i++)
		{
			if (rawSceneLights[i]->IsActive())
			{
				m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_LIGHT].push_back(rawSceneLights[i]);
			}
		}
		for (INT i = 0; i < rawSceneTerrains.size(); i++)
		{
			if (rawSceneTerrains[i]->IsActive() && rawSceneTerrains[i]->GetMeshRenderer())
			{
				m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_TERRAIN].push_back(rawSceneTerrains[i]);
			}
		}
		for (INT i = 0; i < rawSceneOpaques.size(); i++)
		{
			if (rawSceneOpaques[i]->IsActive() && rawSceneOpaques[i]->GetMeshRenderer())
			{
				m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_OPAQUE].push_back(rawSceneOpaques[i]);
			}
		}
		for (INT i = 0; i < rawSceneTransparents.size(); i++)
		{
			if (rawSceneTransparents[i]->IsActive() && rawSceneTransparents[i]->GetMeshRenderer())
			{
				m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_TRANSPARENT].push_back(rawSceneTransparents[i]);
			}
		}
		for (INT i = 0; i < rawSceneSkys.size(); i++)
		{
			if (rawSceneSkys[i]->IsActive() && rawSceneSkys[i]->GetMeshRenderer())
			{
				m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_SKY].push_back(rawSceneSkys[i]);
			}
		}
	}

	m_DebugScreen->Update();
	m_GTAOComputeShader->Update();
	m_HZB->Update();
}
void CRenderPipeline::Render()
{
	if (m_CurrentScene == NULL)
		return;

	for (INT i = 0; i < (m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_CAMERA]).size(); i++)
	{
		{
			CCamera* camera = reinterpret_cast<CCamera*>((m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_SKY])[i]);
			if (!camera)
				continue;

			this->PreparePerFrameRender(camera);
			CRenderDevice::SetRasterizerState(m_PipelineRS);
		}

		{
			//Shadow pass
			CRenderDevice::SetDepthStencilState(m_ShadowPrePassDSS);
			CRenderDevice::SetBlendState(m_ShadowPrePassBS);
			CRenderDevice::SetPrimitiveTopology(CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CRenderDevice::SetRenderTarget(nullptr, m_ShadowBuffer.DepthStencilView);
			for (INT i = CScene::SceneLayout::LAYOUT_TERRAIN; i < CScene::SceneLayout::LAYOUT_TRANSPARENT; ++i)
			{
				for (CGameObject* obj : m_CurrentScenePrimitives[i])
				{
					obj->DrawExtra();
				}
			}
		}

		{
			//Pre depth pass
			CRenderDevice::SetRenderTarget(nullptr, m_SceneDepth.DepthStencilView);
			for (INT i = CScene::SceneLayout::LAYOUT_TERRAIN; i < CScene::SceneLayout::LAYOUT_TRANSPARENT; ++i)
			{
				for (CGameObject* obj : m_CurrentScenePrimitives[i])
				{
					obj->DrawExtra();
				}
			}
			for (CGameObject* obj : m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_SKY])
			{
				obj->DrawExtra();
			}
		}

		{
			//Geometry buffer pass
			CRenderDevice::SetDepthStencilState(m_GBufferPassDSS);
			CRenderDevice::SetBlendState(m_GBufferPassBS);
			CRenderDevice::SetPrimitiveTopology(CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> tempGBuffersRTV[CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u];
			tempGBuffersRTV[0] = m_SceneColor.RenderTargetView;
			for (UINT i = 0u; i < CRenderPipeline::GEOMETRY_BUFFER_COUNT; i++)
			{
				tempGBuffersRTV[i + 1u] = m_GBuffer[i].RenderTargetView;
			}
			CRenderDevice::SetRenderTargets(tempGBuffersRTV, CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u, m_SceneDepth.DepthStencilView);
			for (INT i = CScene::SceneLayout::LAYOUT_TERRAIN; i < CScene::SceneLayout::LAYOUT_TRANSPARENT; ++i)
			{
				for (CGameObject* obj : m_CurrentScenePrimitives[i])
				{
					if (obj->GetMeshRenderer()->GetRenderType() == CMeshRenderer::RENDER_TYPE_DEFERRED)
						obj->Draw();
				}
			}
			for (CGameObject* obj : m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_SKY])
			{
				if (obj->GetMeshRenderer()->GetRenderType() == CMeshRenderer::RENDER_TYPE_SKY)
					obj->Draw();
			}
		}

		{
			//Direct light pass
			CRenderDevice::SetDepthStencilState(m_DirectLightPassDSS);
			CRenderDevice::SetBlendState(m_DirectLightPassBS);
			CRenderDevice::SetPrimitiveTopology(CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CRenderDevice::SetRenderTarget(m_SceneColor.RenderTargetView, nullptr);
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tempGBuffersSRV[CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u];
			tempGBuffersSRV[0] = m_SceneColor.ShaderResourceView;
			for (UINT i = 0u; i < CRenderPipeline::GEOMETRY_BUFFER_COUNT; i++)
			{
				tempGBuffersSRV[i + 1u] = m_GBuffer[i].ShaderResourceView;
			}
			CRenderDevice::BindPSShaderResourceViews(tempGBuffersSRV, ENGINE_GBUFFER_ALL_START_SLOT, CRenderPipeline::GEOMETRY_BUFFER_COUNT + 1u);
			this->DrawFullScreenPolygon(CRenderPipeline::m_DirectLightShader);
		}

		{
			//Post process pass
		}

		{
			//Final output
			CRenderDevice::SetDepthStencilState(nullptr);
			CRenderDevice::SetBlendState(nullptr);
			CRenderDevice::SetPrimitiveTopology(CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CRenderDevice::SetFinalOutput();
			this->DrawFullScreenPolygon(CRenderPipeline::m_ScreenPolygonShader);

			m_DebugScreen->Draw();
			m_GTAOComputeShader->Draw(m_SceneDepth.ShaderResourceView);
			m_HZB->Draw(m_SceneDepth.ShaderResourceView);
		}
	}
}
void CRenderPipeline::DrawFullScreenPolygon(const std::shared_ptr<class CPixelShader>& shader)
{
	CRenderPipeline::m_FullScreenPolygonVS->Bind();
	shader->Bind();
	CRenderDevice::SetVertexBuffer(CRenderPipeline::m_FullScreenPolygon->GetVertexBuffer());
	CRenderDevice::SetIndexBuffer(CRenderPipeline::m_FullScreenPolygon->GetIndexBuffer());
	CRenderDevice::DrawIndexed(CRenderPipeline::m_FullScreenPolygon->GetIndexCount());
}
void CRenderPipeline::PreparePerFrameRender(CCamera* camera)
{
	CustomType::Vector2 cameraViewportMin = camera->GetViewportMinSize();

	m_RenderPerFrameInfo.PerFrameData.ProjectionMatrix = camera->GetProjectionMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ProjectionInvMatrix = camera->GetProjectionInverseMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.DepthMultiAdd = camera->GetDeviceZToViewZMulAdd().GetXMFLOAT4();
	m_RenderPerFrameInfo.PerFrameData.ScreenToViewSpaceParams = camera->GetScreenToViewParameters(CustomType::Vector2Int(CRenderDevice::GetViewport().Width, CRenderDevice::GetViewport().Height), CustomType::Vector2Int(CRenderDevice::GetViewport().Width, CRenderDevice::GetViewport().Height)).GetXMFLOAT4();
	m_RenderPerFrameInfo.PerFrameData.CameraViewportMinSizeAndInvBufferSize = XMFLOAT4(cameraViewportMin.X(), cameraViewportMin.Y(), 1.f / static_cast<FLOAT>(m_GlobalBufferSize.X()), 1.f / static_cast<FLOAT>(m_GlobalBufferSize.Y()));
	m_RenderPerFrameInfo.PerFrameData.CameraViewportSizeAndInvSize = camera->GetViewportSizeAndInvSize().GetXMFLOAT4();
	m_RenderPerFrameInfo.PerFrameData.CameraViewportRect = camera->GetViewport().GetXMFLOAT4();

	m_RenderPerFrameInfo.PerFrameData.ViewMatrix = camera->GetViewMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewInvMatrix = camera->GetViewInverseMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewProjectionMatrix = camera->GetViewProjectionMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.ViewProjectionInvMatrix = camera->GetViewProjectionInverseMatrix().GetGPUUploadFloat4x4();
	m_RenderPerFrameInfo.PerFrameData.TimeParams = XMFLOAT4(static_cast<FLOAT>(CManager::GetGameTimer()->GetClockTime()), static_cast<FLOAT>(CManager::GetGameTimer()->GetDeltaTime()), 1.f, 1.f);
	m_RenderPerFrameInfo.PerFrameData.CameraWorldPosition = camera->GetPosition().GetXMFLOAT3();

	m_RenderPerFrameInfo.PerFrameData.DirectionalLightCount = static_cast<FLOAT>((m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_LIGHT]).size());
	for (INT i = 0; i < (m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_LIGHT]).size() && i < 4; i++)
	{
		CLight* light = reinterpret_cast<CLight*>((m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_LIGHT])[i]);
		m_RenderPerFrameInfo.PerFrameData.DirectionalLightData[i].Direction = light->GetLightData()->Direction;
		m_RenderPerFrameInfo.PerFrameData.DirectionalLightData[i].Color = light->GetLightData()->Color;
	}

	CRenderDevice::UploadBuffer(m_RenderPerFrameInfo.PerFrameBuffer, &(m_RenderPerFrameInfo.PerFrameData));
	CRenderDevice::BindVSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CONSTANT_BUFFER_PER_FRAME_START_SLOT);
	CRenderDevice::BindPSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CONSTANT_BUFFER_PER_FRAME_START_SLOT);
	CRenderDevice::BindCSConstantBuffer(m_RenderPerFrameInfo.PerFrameBuffer, ENGINE_CONSTANT_BUFFER_PER_FRAME_START_SLOT);
}
CTexture2D* CRenderPipeline::GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum input)
{
	return (CRenderPipeline::m_DefaultTexture[input]);
}
std::shared_ptr<CPixelShader> CRenderPipeline::GetDefaultEmptyPS()
{
	return (CRenderPipeline::m_DefaultEmptyPS);
}