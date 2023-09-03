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
	INT32												m_DebugWindCPUType;
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
	BOOL32		m_DrawDebugWindCPU;
	BOOL32		m_AddSource;
	BOOL32		m_ResetSource;
protected:
	inline static UINT32		TransformCellCoordToIndex(const UINT32& i, const UINT32& j, const UINT32& w);
	inline static void		TransformCellIndexToCoord(const UINT32& index, const UINT32& w, const UINT32& h, UINT32& i, UINT32& j);
	inline static INT32		TransformCellCoordToIndex(const INT32& i, const INT32& j, const UINT32& w);
	inline static void		TransformCellIndexToCoord(const INT32& index, const UINT32& w, const UINT32& h, INT32& i, INT32& j);
	inline static void		Swap(FLOAT*& a, FLOAT*& b);
	inline static void		InitPointer(FLOAT*& ptr, const UINT32& num, const FLOAT& init = 0.f);
	inline static void		CreatePointer(FLOAT*& ptr, const UINT32& num);
	inline static void		DeletePointer(FLOAT*& ptr);
protected:
	static void		AddSource(const UINT32& w, const UINT32& h, FLOAT* x, FLOAT* s, const FLOAT& dt);
	static void		SetBoundary(const UINT32& w, const UINT32& h, const INT32& b, FLOAT* x);
	static void		DiffusionSolver(const UINT32& iter, const UINT32& w, const UINT32& h, const INT32& b, FLOAT* x, FLOAT* x0, const FLOAT& diffuse, const FLOAT& dt);
	static void		AdvectionSolver(const UINT32& w, const UINT32& h, const INT32& b, FLOAT* d, FLOAT* d0, FLOAT* u, FLOAT* v, const FLOAT& dt);
	static void		ProjectionSolver(const UINT32& iter, const UINT32& w, const UINT32& h, FLOAT* u, FLOAT* v, FLOAT* p, FLOAT* div);
	static void		DensityStep(const UINT32& iter, const UINT32& w, const UINT32& h, FLOAT* x, FLOAT* x0, FLOAT* u, FLOAT* v, const FLOAT& diffuse, const FLOAT& dt);
	static void		VelocityStep(const UINT32& iter, const UINT32& w, const UINT32& h, FLOAT* u, FLOAT* v, FLOAT* u0, FLOAT* v0, const FLOAT& viscosity, const FLOAT& dt);
protected:
	const static UINT32		_StaticIterationCount	= 20u;
	const static UINT32		_StaticCellWidth		= 126u;
	const static UINT32		_StaticCellHeight		= 126u;
	constexpr static UINT32	_StaticCellCount		= (_StaticCellWidth + 2u) * (_StaticCellHeight + 2u);
#endif
public:
	CDynamicWind();
	virtual ~CDynamicWind();
};
*/