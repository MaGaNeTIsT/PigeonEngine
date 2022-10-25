#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "./CStructCommon.h"
#include "../../AssetsManager/Headers/CMesh.h"
#include "../../../EngineGame/Headers/CScene.h"

class CRenderPipeline
{
public:
	struct CRenderCameraCullingInfo
	{
		CRenderCameraCullingInfo()
		{
			Distance		= 0.f;
			OriginPosition	= CustomType::Vector3::Zero();
			ClipOffset		= 0.f;
			for (INT i = 0; i < 5; i++)
			{
				ClipDot[i]		= 0.f;
				ClipPlane[i]	= CustomType::Vector3::Zero();
			}
		}
		FLOAT				Distance;
		CustomType::Vector3 OriginPosition;
		FLOAT				ClipOffset;
		FLOAT				ClipDot[5];
		CustomType::Vector3 ClipPlane[5];
	};
public:
	struct RenderPerFrameInfo
	{
		RenderPerFrameInfo() { ::ZeroMemory(this, sizeof(*this)); }
		CustomStruct::CShaderGlobalPerFrame		PerFrameData;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	PerFrameBuffer;
	};
	struct RenderLightDataInfo
	{
		RenderLightDataInfo() { ::ZeroMemory(this, sizeof(*this)); }
		CustomStruct::CShaderGlobalLightData	LightData;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	LightDataBuffer;
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
	virtual void	PreparePerFrameRender(class CCamera* camera);
	virtual void	PrepareDirectionalLightPerFrameRender(class CLightBase* light, const UINT& index);
	void			PrepareCameraCullingInfo(CRenderCameraCullingInfo& cullingInfo, class CCamera* camera);
	BOOL			CullingCameraPlane(const CustomType::Vector3& pos, const FLOAT& radius, const CRenderCameraCullingInfo& cullingInfo);
	void			Culling(std::vector<class CGameObject*>& cullingResult, const CRenderCameraCullingInfo& cullingInfo, const std::vector<class CGameObject*>& primitives);
	void			CullingDirectionalCascadeShadow(std::vector<class CGameObject*>& cullingResult, CLightDirectional* lightInfo, const std::vector<class CGameObject*>& primitives);
	virtual void	PrepareLightDataRender(class CCamera* camera);
	void			DrawFullScreenPolygon(const std::shared_ptr<class CPixelShader>& shader);
protected:
	using DirectionalCascadeShadowPrimitives = std::map<class CLightDirectional*, std::map<UINT, std::vector<class CGameObject*>>>;
protected:
	const CScene*							m_CurrentScene;
	std::vector<class CGameObject*>			m_CurrentScenePrimitives[CScene::SceneLayout::LAYOUT_COUNT];
	std::vector<class CGameObject*>			m_CurrentCPUCullingPrimitives;
	DirectionalCascadeShadowPrimitives		m_CurrentDirCSMCullingPrimitives;
protected:
	ULONGLONG								m_FrameIndex;
	CRenderCameraCullingInfo				m_GlobalCullingInfo;
	RenderPerFrameInfo						m_RenderPerFrameInfo;
	RenderLightDataInfo						m_RenderLightDataInfo;
	CustomType::Vector2Int					m_GlobalBufferSize;
	CustomType::Vector2Int					m_ShadowBufferSize;
protected:
	std::shared_ptr<CMesh<UINT>>			m_FullScreenPolygon;
protected:
	CRenderDevice::RenderTexture2DViewInfo	m_SceneColor;
	CRenderDevice::RenderTexture2DViewInfo	m_SceneDepth;
	CRenderDevice::RenderTexture2DViewInfo	m_PostProcessColor[2];
	CRenderDevice::RenderTexture2DViewInfo	m_GBuffer[GEOMETRY_BUFFER_COUNT];
protected:
	Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_PipelineSampler[4];
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_PipelineRS;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			m_ShadowSkyForwardPrePassBS;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			m_GBufferPassBS;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			m_DirectLightPassBS;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			m_TransparentPassBS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_ShadowPrePassDSS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_GBufferForwardPassDSS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		m_DirectLightPassDSS;
protected:
	static std::shared_ptr<class CVertexShader>			m_FullScreenPolygonVS;
	static std::shared_ptr<class CPixelShader>			m_ScreenPolygonShader;
	static std::shared_ptr<class CPixelShader>			m_DirectLightShader;
protected:
	static std::shared_ptr<class CSkyBox>				m_SkyBox;
	static std::shared_ptr<class CGPUCulling>			m_GPUCulling;
	static std::shared_ptr<class CGTAOPass>				m_GTAOPass;
	static std::shared_ptr<class CHZBPass>				m_HZBPass;
	static std::shared_ptr<class CDebugScreen>			m_DebugScreen;
public:
	static class CTexture2D* GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum input);
	static std::shared_ptr<class CPixelShader> GetDefaultEmptyPS();
protected:
	static class CTexture2D* m_DefaultTexture[CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_COUNT];
	static std::shared_ptr<class CPixelShader> m_DefaultEmptyPS;
public:
	CRenderPipeline();
	virtual ~CRenderPipeline();
private:
	friend class CDebugScreen;
};