#pragma once

#include "../Game/CGameObject.h"

class CScreenPolygon2D;

class CGTAOComputeShader : public CGameObject
{
public:
	struct GTAOConstantBuffer
	{
		GTAOConstantBuffer() { ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4 ResultBufferParams;
		DirectX::XMFLOAT4 DepthBufferParams;
		DirectX::XMFLOAT4 FadeAttenParams;				// x y is fade radius mul add coefficient. z is fade distance. w is falloff's attenuation factory.
		DirectX::XMFLOAT4 AdjustAngleThicknessParams;	// x is world space radius adjust. y is sin rotatation delta angle. z is cos rotatation delta angle. w is thickness blend factory.
		DirectX::XMFLOAT4 IntensityPowerParams;			// x is ambient occlusion adjust intensity. y is ambient occlusion adjust power. z w is not used.
	};
	struct GTAOUserArguments
	{
		GTAOUserArguments() { ZeroMemory(this, sizeof(*this)); }
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
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	Draw()override;
protected:
	virtual void	PrepareDraw()override;
public:
	CGTAOComputeShader();
	virtual ~CGTAOComputeShader();
protected:
	CScreenPolygon2D*							m_Polygon2D;

	INT											m_DebugType;
	CRenderDevice::RenderTexture2DViewInfo		m_DebugBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_DebugComputeShader;

	GTAOUserArguments							m_UserArguments;
	GTAOConstantBuffer							m_ConstantData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_ConstantBuffer;
	CRenderDevice::RenderTexture2DViewInfo		m_IntegralBuffer;
	CRenderDevice::RenderTexture2DViewInfo		m_FilterBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_IntegralComputeShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_FilterComputeShader;
};