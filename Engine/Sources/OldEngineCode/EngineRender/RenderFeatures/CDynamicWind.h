/*
#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

class CDynamicWind
{
public:
	void	Init();
	void	Uninit();
	void	Update();
	void	ComputeDynamicWind();
protected:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_ComputeDynamicWindShader;
	CRenderDevice::RenderTexture3DViewInfo			m_DynamicWindBuffer;
#ifdef _DEVELOPMENT_EDITOR
public:
	void	EditorUpdate();
	void	DrawDebug(const CustomType::Vector2Int& pipelineSize);
protected:
	void	RunCPUWind(const FLOAT& deltaTime);
	void	AddCPUWindSource(const FLOAT& deltaTime);
protected:
	struct DynamicWindCPUDebugData
	{
		DirectX::XMINT4	DebugType;
	};
protected:
	INT												m_DebugWindCPUType;
	DynamicWindCPUDebugData							m_ComputeDynamicWindCPUDebugData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_ComputeDynamicWindCPUDebugCBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_ComputeDynamicWindCPUDebugShader;
	CRenderDevice::RenderTexture2DViewInfo			m_DebugWindCPUResultBuffer;
	CRenderDevice::RenderTexture2DViewInfo			m_DebugWindCPUDensityBuffer;
	CRenderDevice::RenderTexture2DViewInfo			m_DebugWindCPUUVectorBuffer;
	CRenderDevice::RenderTexture2DViewInfo			m_DebugWindCPUVVectorBuffer;
	class CScreenPolygon2D*							m_DebugWindCPUWnd;
protected:
	FLOAT*		m_CellU;
	FLOAT*		m_CellV;
	FLOAT*		m_CellD;
	FLOAT*		m_CellPrevU;
	FLOAT*		m_CellPrevV;
	FLOAT*		m_CellPrevD;
	FLOAT		m_Viscosity;
	FLOAT		m_DiffuseCoefficient;
	FLOAT		m_DSource;
	FLOAT		m_USource;
	FLOAT		m_VSource;
	BOOL		m_DrawDebugWindCPU;
	BOOL		m_AddSource;
	BOOL		m_ResetSource;
protected:
	inline static UINT		TransformCellCoordToIndex(const UINT& i, const UINT& j, const UINT& w);
	inline static void		TransformCellIndexToCoord(const UINT& index, const UINT& w, const UINT& h, UINT& i, UINT& j);
	inline static INT		TransformCellCoordToIndex(const INT& i, const INT& j, const UINT& w);
	inline static void		TransformCellIndexToCoord(const INT& index, const UINT& w, const UINT& h, INT& i, INT& j);
	inline static void		Swap(FLOAT*& a, FLOAT*& b);
	inline static void		InitPointer(FLOAT*& ptr, const UINT& num, const FLOAT& init = 0.f);
	inline static void		CreatePointer(FLOAT*& ptr, const UINT& num);
	inline static void		DeletePointer(FLOAT*& ptr);
protected:
	static void		AddSource(const UINT& w, const UINT& h, FLOAT* x, FLOAT* s, const FLOAT& dt);
	static void		SetBoundary(const UINT& w, const UINT& h, const INT& b, FLOAT* x);
	static void		DiffusionSolver(const UINT& iter, const UINT& w, const UINT& h, const INT& b, FLOAT* x, FLOAT* x0, const FLOAT& diffuse, const FLOAT& dt);
	static void		AdvectionSolver(const UINT& w, const UINT& h, const INT& b, FLOAT* d, FLOAT* d0, FLOAT* u, FLOAT* v, const FLOAT& dt);
	static void		ProjectionSolver(const UINT& iter, const UINT& w, const UINT& h, FLOAT* u, FLOAT* v, FLOAT* p, FLOAT* div);
	static void		DensityStep(const UINT& iter, const UINT& w, const UINT& h, FLOAT* x, FLOAT* x0, FLOAT* u, FLOAT* v, const FLOAT& diffuse, const FLOAT& dt);
	static void		VelocityStep(const UINT& iter, const UINT& w, const UINT& h, FLOAT* u, FLOAT* v, FLOAT* u0, FLOAT* v0, const FLOAT& viscosity, const FLOAT& dt);
protected:
	const static UINT		_StaticIterationCount	= 20u;
	const static UINT		_StaticCellWidth		= 126u;
	const static UINT		_StaticCellHeight		= 126u;
	constexpr static UINT	_StaticCellCount		= (_StaticCellWidth + 2u) * (_StaticCellHeight + 2u);
#endif
public:
	CDynamicWind();
	virtual ~CDynamicWind();
};
*/