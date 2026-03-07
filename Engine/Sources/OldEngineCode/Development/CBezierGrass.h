/*
#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"

class CBezierGrass
{
public:
	struct GrassPropertyInfo
	{
		GrassPropertyInfo() : SplitNum(0u), StepLength(0.f), LeafWidth(0.f) {}
		UINT32	SplitNum;
		FLOAT	StepLength;
		FLOAT	LeafWidth;
		CustomStruct::CColor	RootColor;
		CustomStruct::CColor	TipColor;
	};
	struct GrassInstanceInfo
	{
		GrassInstanceInfo() : Tilt(0.f), Bend(0.f) {}
		CustomType::Vector3		Origin;
		CustomType::Vector3		Direction;
		CustomType::Vector2		Tip;
		FLOAT					Tilt;
		FLOAT					Bend;
	};
private:
	struct GrassInstanceBuffer
	{
		DirectX::XMFLOAT4 Params0;
		DirectX::XMFLOAT4 Params1;
		DirectX::XMFLOAT2 Params2;
	};
	struct GrassPropertyBuffer
	{
		DirectX::XMFLOAT4 Params0;
		DirectX::XMFLOAT4 Params1;
		DirectX::XMFLOAT4 Params2;
	};
	const static UINT32	_GrassInstancePerGroupMaxNum = 65536u;
public:
	void	Init();
	void	Update();
	void	Render();
	void	Uninit();
private:
	void	ReCreateGrassInstanceData(const CustomType::Vector3& origin, const FLOAT& BaseHeight, const FLOAT& offsetHeight, const FLOAT& lengthX, const FLOAT& lengthZ, const UINT32& numX, const UINT32& numZ);
	void	ReCreateGrassIndexBuffer(const UINT32& num);
	void	ReCreateGrassInstanceConstantBuffer();
	void	UploadGrassInstanceConstantBuffer();
	void	UploadGrassPropertyConstantBuffer();
private:
	static void		TranslateGrassInstanceInfoToBuffer(GrassInstanceBuffer& output, const GrassInstanceInfo& input);
	static void		TranslateGrassInstanceBufferToInfo(GrassInstanceInfo& output, const GrassInstanceBuffer& input);
	static void		TranslateGrassPropertyInfoToBuffer(GrassPropertyBuffer& output, const GrassPropertyInfo& input);
	static void		TranslateGrassPropertyBufferToInfo(GrassPropertyInfo& output, const GrassPropertyBuffer& input);
private:
	UINT32											m_GrassInstanceNumInBuffer;
	std::vector<GrassInstanceInfo>					m_GrassInstanceData;
	CRenderDevice::StructuredBufferInfo				m_GrassInstanceBuffer;
	GrassPropertyInfo								m_GrassPropertyData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_GrassPropertyCBuffer;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>		m_GrassInputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>		m_GrassVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>		m_GrassPixelShader;
	UINT32											m_GrassIndexNum;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_GrassIndexBuffer;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_PipelineRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_GrassRS;
#ifdef _DEVELOPMENT_EDITOR
private:
	struct DebugCBufferData
	{
		DirectX::XMINT4		Parameter;
		DirectX::XMFLOAT4	StartEndPoint;
		DirectX::XMFLOAT4	ControlPoint;
	};
public:
	void	DrawDebug();
private:
	BOOL32											m_IsShowGrassDebugRS;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	m_GrassDebugRS;
	FLOAT											m_DebugGrassRot;
	BOOL32											m_IsShowDebugWnd;
	class CScreenPolygon2D*							m_DebugWnd;
	UINT32											m_DebugBufferSizeX;
	UINT32											m_DebugBufferSizeY;
	DebugCBufferData								m_DebugCBufferData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_DebugCBuffer;
	CRenderDevice::RenderTexture2DViewInfo			m_DebugTexBuffer;
	CRenderDevice::RenderTexture2DViewInfo			m_DebugScreenBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_DebugCS;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_DebugScreenCS;
#endif
public:
	CBezierGrass();
	~CBezierGrass();
};
*/