/*
#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

class CGTAOPass
{
public:
	struct GTAOConstantBuffer
	{
		GTAOConstantBuffer() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4 ResultBufferParams;
		DirectX::XMFLOAT4 DepthBufferParams;
		DirectX::XMFLOAT4 FadeAttenParams;				// x y is fade radius mul add coefficient. z is fade distance. w is falloff's attenuation factory.
		DirectX::XMFLOAT4 AdjustAngleThicknessParams;	// x is world space radius adjust. y is sin rotatation delta angle. z is cos rotatation delta angle. w is thickness blend factory.
		DirectX::XMFLOAT4 IntensityPowerParams;			// x is ambient occlusion adjust intensity. y is ambient occlusion adjust power. z w is not used.
	};
	struct GTAOUserArguments
	{
		GTAOUserArguments() { ::ZeroMemory(this, sizeof(*this)); }
		UINT	NumAngles;
		FLOAT	FallOffEnd;
		//FLOAT	FallOffStartRatio;
		FLOAT	ThicknessBlend;
		FLOAT	FadeRadius;
		FLOAT	FadeDistance;
		FLOAT	Intensity;
		FLOAT	Power;
	};
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetResultShaderResourceView() { return m_FilterBuffer.ShaderResourceView; }
public:
	void	Init(class CCamera* mainCamera, const CustomType::Vector2Int& pipelineSize);
	void	Uninit();
	void	Update();
	void	ComputeGTAO(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& sceneDepth);
#ifdef _DEVELOPMENT_EDITOR
public:
	void	EditorUpdate();
	void	DrawDebug();
#endif
public:
	CGTAOPass();
	~CGTAOPass();
private:
#ifdef _DEVELOPMENT_EDITOR
	class CScreenPolygon2D*						m_Polygon2D;
	INT											m_DebugType;
	CRenderDevice::RenderTexture2DViewInfo		m_DebugBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_DebugComputeShader;
#endif
	class CCamera*								m_MainCamera;
	CustomType::Vector2Int						m_BufferSize;
	CustomType::Vector2Int						m_PipelineSize;
	GTAOUserArguments							m_UserArguments;
	GTAOConstantBuffer							m_ConstantData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_ConstantBuffer;
	CRenderDevice::RenderTexture2DViewInfo		m_IntegralBuffer;
	CRenderDevice::RenderTexture2DViewInfo		m_FilterBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_IntegralComputeShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_FilterComputeShader;
};
*/