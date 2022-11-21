#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "./CRenderStructCommon.h"
#include "./CRenderDevice.h"
#include "../../AssetsManager/Headers/CMeshComponent.h"
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
	void			DrawFullScreenPolygon(const std::shared_ptr<class CPixelShader>& shader);
	virtual void	PreparePerFrameRender();
	virtual void	PrepareLightDataRender();
	virtual void	PrepareDirectionalLightPerFrameRender(class CLightBase* light, const UINT& index);
protected:
	void			PrepareCameraCullingInfo();
	void			Culling();
	BOOL			PerObjectDistanceFrustumCulling(const CustomType::Vector3& pos, const FLOAT& radius);
	BOOL			PerObjectDirectionalCascadeShadowCulling(std::vector<UINT>& layerIndex, class CLightDirectional* light, const CustomType::Vector3& pos, const FLOAT& radius);
protected:
	using DirectionalCascadeShadowPerLightPrimitives	= std::map<UINT, std::vector<class CGameObject*>>;
	using DirectionalCascadeShadowCullingResults		= std::map<class CLightDirectional*, DirectionalCascadeShadowPerLightPrimitives>;
	enum CullingResultsLayer
	{
		CULLINGRESULTS_DEFERRED		= 0,
		CULLINGRESULTS_FORWARD		= 1,
		CULLINGRESULTS_TRANSPARENT	= 2,
		CULLINGRESULTS_COUNT
	};
protected:
	const CScene*							m_Scene;
	class CCamera*							m_SceneCamera;
	std::vector<class CLightBase*>			m_SceneLightList;
	std::vector<class CGameObject*>			m_ScenePrimitives[CScene::SceneLayout::LAYOUT_COUNT];
protected:
	std::vector<class CGameObject*>			m_CameraCullingResults;
	std::vector<class CGameObject*>			m_RenderingCullingResults[CullingResultsLayer::CULLINGRESULTS_COUNT];
	DirectionalCascadeShadowCullingResults	m_DirLightCullingResults;
protected:
	ULONGLONG								m_FrameIndex;
	CRenderCameraCullingInfo				m_GlobalCullingInfo;
	RenderPerFrameInfo						m_RenderPerFrameInfo;
	RenderLightDataInfo						m_RenderLightDataInfo;
	CustomType::Vector2Int					m_GlobalBufferSize;
	CustomType::Vector2Int					m_ShadowBufferSize;
protected:
	const CBaseMesh<UINT>*					m_FullScreenPolygon;
protected:
	CRenderDevice::RenderTexture2DViewInfo	m_RTSceneColor;
	CRenderDevice::RenderTexture2DViewInfo	m_RTSceneDepth;
	CRenderDevice::RenderTexture2DViewInfo	m_RTPostProcess[2];
	CRenderDevice::RenderTexture2DViewInfo	m_RTGBuffer[GEOMETRY_BUFFER_COUNT];
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
	static class CTexture2D*							GetDefaultTexture(CustomStruct::CEngineDefaultTexture2DEnum input);
	static std::shared_ptr<class CPixelShader>			GetDefaultEmptyPS();
protected:
	static class CTexture2D*							m_DefaultTexture[CustomStruct::CEngineDefaultTexture2DEnum::ENGINE_DEFAULT_TEXTURE2D_COUNT];
	static std::shared_ptr<class CPixelShader>			m_DefaultEmptyPS;
public:
	CRenderPipeline();
	virtual ~CRenderPipeline();
private:
	friend class CDebugScreen;
};