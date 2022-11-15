#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "./CRenderStructCommon.h"
#include "../Headers/CRenderDevice.h"

class CGPUCulling
{
public:
	struct PrimitivesAABBBoxInfo
	{
		FLOAT Min[3];
		FLOAT Max[3];
		FLOAT Padding[2];
	};
	struct CachedCullingInfo
	{
		CachedCullingInfo()
		{
			NumObjects = 0u;
		}
		UINT									NumObjects;
		std::vector<ULONGLONG>					MappingCullingResult;
		CRenderDevice::StructuredBufferInfo		CullingBoundingInfo;
		CRenderDevice::StructuredBufferInfo		CullingResultInfo;
	};
public:
	/*
	*   waitingFrameNum   : How many frame-interval will be waiting when reads back from frame buffer. Clamp with [0 - 4].
	* baseSceneObjectsNum : Approximate number of need culling objects in current scene for initialize culling buffer.
	*/
	void	Init(std::shared_ptr<class CHZBPass> hzb, const UINT& waitingFrameNum, const UINT& baseSceneObjectsNum);
	void	Uninit();
	void	Update(const ULONGLONG& frameIndex);
	/*
	* inputCullingResult  : Data from cpu culling result.
	* outputCullingResult : Value true means pass culling. Value false means be occluded.
	*/
	void	ReadBackAndPrepareCullingResult(const ULONGLONG& frameIndex, const std::vector<class CGameObject*>& inputCullingResult, std::vector<BOOL>& outputCullingResult);
	void	ComputeCulling(const ULONGLONG& frameIndex);
private:
	struct CullingConstantBufferData
	{
		CullingConstantBufferData() { Parameters = DirectX::XMINT4(0, 0, 0, 0); }
		DirectX::XMINT4 Parameters;
	};
	void	ReCreateBufferSize(const UINT& idx, const UINT& num);
	void	PrepareCullingInfo(const ULONGLONG& frameIndex, const std::vector<class CGameObject*>& fromCPUCullingResult);
	BOOL	ReadBackFromResource(const ULONGLONG& frameIndex);
public:
	CGPUCulling();
	~CGPUCulling();
private:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_GPUCullingComputeShader;
	std::shared_ptr<class CHZBPass>					m_HZBPass;
	UINT											m_CachedCount;
	INT												m_MostDetailHZBIndex;
	INT												m_UsedHZBNum;
	std::vector<CachedCullingInfo>					m_CachedCullingInfo;
	UINT											m_CachedAABBNum;
	std::vector<PrimitivesAABBBoxInfo>				m_CachedAABBInfo;
	std::map<ULONGLONG, UINT>						m_MappedUIDCullingResult;
	UINT											m_PassingCount;
	CullingConstantBufferData						m_CullingData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_CullingConstantBuffer;
};