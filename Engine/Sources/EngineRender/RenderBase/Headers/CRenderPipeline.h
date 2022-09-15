#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "./CStructCommon.h"
#include "../../../EngineGame/Headers/CScene.h"

class CRenderPipeline
{
public:
	struct RenderPerFrameInfo
	{
		RenderPerFrameInfo() { ::ZeroMemory(this, sizeof(*this)); }
		CustomStruct::ConstantBufferPerFrame PerFrameData;
		Microsoft::WRL::ComPtr<ID3D11Buffer> PerFrameBuffer;
	};
public:
	const static UINT GEOMETRY_BUFFER_COUNT = 3;
public:
	virtual void	Init(const CScene* scene, const CustomType::Vector2Int& bufferSize);
	virtual void	PostInit();
	virtual void	Uninit();
	virtual void	PostUpdate();
	virtual void	Render();
protected:
	void			DrawFullScreenPolygon(const std::shared_ptr<class CPixelShader>& shader);
	virtual void	PreparePerFrameRender(class CCamera* camera);
protected:
	const CScene*						m_CurrentScene;
	std::vector<class CGameObject*>		m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_COUNT];
protected:
	RenderPerFrameInfo					m_RenderPerFrameInfo;
	CustomType::Vector2Int				m_GlobalBufferSize;
protected:
	class CMesh*						m_FullScreenPolygon;
protected:
	CRenderDevice::RenderTexture2DViewInfo	m_SceneColor;
	CRenderDevice::RenderTexture2DViewInfo	m_SceneDepth;
	CRenderDevice::RenderTexture2DViewInfo	m_PostProcessColor[2];
	CRenderDevice::RenderTexture2DViewInfo	m_GBuffer[GEOMETRY_BUFFER_COUNT];
	CRenderDevice::RenderTexture2DViewInfo	m_ShadowBuffer;
protected:
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_PipelineRS;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			m_ShadowPrePassBS;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			m_GBufferPassBS;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			m_DirectLightPassBS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_ShadowPrePassDSS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_GBufferPassDSS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_DirectLightPassDSS;
protected:
	static std::shared_ptr<class CVertexShader>			m_FullScreenPolygonVS;
	static std::shared_ptr<class CPixelShader>			m_ScreenPolygonShader;
	static std::shared_ptr<class CPixelShader>			m_DirectLightShader;
protected:
	static std::shared_ptr<class CDebugScreen>			m_DebugScreen;
	static std::shared_ptr<class CGTAOComputeShader>	m_GTAOComputeShader;
	static std::shared_ptr<class CHZBBuffer>			m_HZB;
protected:
	static class CTexture2D* m_DefaultTexture[CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_COUNT];
public:
	CRenderPipeline();
	virtual ~CRenderPipeline();
};