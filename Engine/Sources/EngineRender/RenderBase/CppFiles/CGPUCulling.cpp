#include "../Headers/CGPUCulling.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../RenderFeatures/Headers/CHZBPass.h"
#include "../../../EngineGame/Headers/CGameObject.h"

CGPUCulling::CGPUCulling()
{
	this->m_CachedCount = 0u;
}
CGPUCulling::~CGPUCulling()
{
}
void CGPUCulling::Init(std::shared_ptr<CHZBPass> hzb, const UINT& waitingFrameNum, const UINT& baseSceneObjectsNum)
{
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/EngineGPUCulling.cso", this->m_GPUCullingComputeShader);
	this->m_CachedCount = CustomType::CMath::Clamp(waitingFrameNum, 0u, 4u);
	this->m_CachedCount = this->m_CachedCount + 2u;
	this->m_MappedUIDCullingResult.clear();
	this->m_CachedCullingInfo.resize(this->m_CachedCount);
	for (UINT i = 0u; i < this->m_CachedCount; i++)
	{
		CGPUCulling::ReCreateBufferSize(i, baseSceneObjectsNum);
	}
	this->m_CullingData.Parameters = XMINT4(baseSceneObjectsNum, 0, 0, 0);
	CRenderDevice::CreateBuffer(
		this->m_CullingConstantBuffer,
		CustomStruct::CRenderBufferDesc(
			sizeof(CullingConstantBufferData),
			CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER,
			sizeof(FLOAT)));
	
}
void CGPUCulling::Uninit()
{
}
void CGPUCulling::ReadBackAndPrepareCullingResult(const ULONGLONG& frameIndex, const std::vector<CGameObject*>& inputCullingResult, std::vector<BOOL>& outputCullingResult)
{
	UINT index = frameIndex % (this->m_CachedCount);

	outputCullingResult.resize(inputCullingResult.size(), TRUE);
	if (frameIndex > this->m_CachedCount)
	{
		if (this->ReadBackFromResource(frameIndex))
		{
			for (UINT i = 0u; i < inputCullingResult.size(); i++)
			{
				std::map<ULONGLONG, UINT>::iterator occluded = this->m_MappedUIDCullingResult.find(inputCullingResult[i]->GetGameObjectID());
				if (occluded != this->m_MappedUIDCullingResult.end())
				{
					outputCullingResult[i] = (occluded->second) == 1u;
				}
			}
		}
	}

	this->PrepareCullingInfo(frameIndex, inputCullingResult);
	this->m_CullingData.Parameters = XMINT4(static_cast<int32_t>(this->m_CachedCullingInfo[index].NumObjects), 0, 0, 0);
}
void CGPUCulling::ComputeCulling(const ULONGLONG& frameIndex)
{
	UINT index = frameIndex % (this->m_CachedCount);

	{
		CRenderDevice::UploadBuffer(this->m_CullingConstantBuffer, static_cast<void*>(&(this->m_CullingData)));
		CRenderDevice::BindCSConstantBuffer(this->m_CullingConstantBuffer, 1u);
	}

	{
		CRenderDevice::SetCSShader(this->m_GPUCullingComputeShader);
		CRenderDevice::BindCSShaderResourceView(this->m_CachedCullingInfo[index].CullingBoundingInfo.ShaderResourceView, 0u);
		CRenderDevice::BindCSUnorderedAccessView(this->m_CachedCullingInfo[index].CullingResultInfo.UnorderedAccessView, 0u);
		CRenderDevice::Dispatch((this->m_CachedCullingInfo[index].NumObjects + 63) / 64, 1u, 1u);
		CRenderDevice::BindNoCSUnorderedAccessView(0u);
		CRenderDevice::SetNoCSShader();
	}
}
void CGPUCulling::ReCreateBufferSize(const UINT& idx, const UINT& num)
{
	if (idx >= this->m_CachedCullingInfo.size())
	{
		return;
	}
	this->m_CachedCullingInfo[idx].NumObjects = num;
	if (num > this->m_CachedCullingInfo[idx].MappingCullingResult.size())
	{
		this->m_CachedCullingInfo[idx].MappingCullingResult.resize(num);
		CRenderDevice::CreateStructuredBuffer(
			this->m_CachedCullingInfo[idx].CullingBoundingInfo,
			CustomStruct::CRenderStructuredBufferDesc(
				sizeof(PrimitivesAABBBoxInfo), num));
		CRenderDevice::CreateStructuredBuffer(
			this->m_CachedCullingInfo[idx].CullingResultInfo,
			CustomStruct::CRenderStructuredBufferDesc(
				sizeof(UINT), num, TRUE, CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_READ));
	}
}
void CGPUCulling::PrepareCullingInfo(const ULONGLONG& frameIndex, const std::vector<CGameObject*>& fromCPUCullingResult)
{
	this->m_MappedUIDCullingResult.clear();
	{
		std::vector<PrimitivesAABBBoxInfo> objectsAABB(fromCPUCullingResult.size());
		std::vector<ULONGLONG> objectsUID(fromCPUCullingResult.size());
		UINT numCounter = 0u;
		{
			CustomType::Vector3 min, max;
			for (UINT i = 0u; i < fromCPUCullingResult.size(); i++)
			{
				if (fromCPUCullingResult[i]->GetBoundingBox() != NULL)
				{
					fromCPUCullingResult[i]->GetAABBBoundingBox(min, max);
					objectsAABB[numCounter].Min[0] = min.X();
					objectsAABB[numCounter].Min[1] = min.Y();
					objectsAABB[numCounter].Min[2] = min.Z();
					objectsAABB[numCounter].Max[0] = max.X();
					objectsAABB[numCounter].Max[1] = max.Y();
					objectsAABB[numCounter].Max[2] = max.Z();
					objectsUID[numCounter] = fromCPUCullingResult[i]->GetGameObjectID();
					numCounter++;
				}
			}
		}
		UINT index = frameIndex % (this->m_CachedCount);
		{
			this->ReCreateBufferSize(index, numCounter);
			CachedCullingInfo* currentUsedInfo = &(this->m_CachedCullingInfo[index]);
			memcpy_s(currentUsedInfo->MappingCullingResult.data(), sizeof(ULONGLONG) * currentUsedInfo->MappingCullingResult.size(), objectsUID.data(), sizeof(ULONGLONG) * numCounter);
			//TODO Upload action is not running in rendering but before rendering.
			CRenderDevice::UploadBuffer(currentUsedInfo->CullingBoundingInfo.Buffer, static_cast<void*>(objectsAABB.data()), sizeof(PrimitivesAABBBoxInfo) * numCounter, sizeof(PrimitivesAABBBoxInfo) * numCounter);
		}
	}
}
BOOL CGPUCulling::ReadBackFromResource(const ULONGLONG& frameIndex)
{
	INT readBackIndex;
	{
		UINT index = frameIndex % (this->m_CachedCount);
		INT totalCount = static_cast<INT>(this->m_CachedCount);
		readBackIndex = static_cast<INT>(index) - totalCount + 1;
		readBackIndex = (readBackIndex < 0) ? (readBackIndex + totalCount) : readBackIndex;
	}
	BOOL result = FALSE;
	CachedCullingInfo* currentUsedInfo = &(this->m_CachedCullingInfo[readBackIndex]);
	std::vector<UINT> currentCullingResult(currentUsedInfo->NumObjects, TRUE);
	{
		CustomStruct::CRenderMappedResource mr;
		result = CRenderDevice::Map(currentUsedInfo->CullingResultInfo, 0u, CustomStruct::CRenderMapType::MAP_READ, CustomStruct::CRenderMapFlag::MAP_FLAG_DO_NOT_WAIT, mr);
		if (result)
		{
			memcpy_s(currentCullingResult.data(), sizeof(UINT) * currentUsedInfo->NumObjects, mr.pData, sizeof(UINT) * currentUsedInfo->NumObjects);
		}
		CRenderDevice::Unmap(currentUsedInfo->CullingResultInfo, 0u);
	}
	if (result)
	{
		for (UINT i = 0u; i < currentUsedInfo->NumObjects; i++)
		{
			this->m_MappedUIDCullingResult[currentUsedInfo->MappingCullingResult[i]] = currentCullingResult[i];
		}
	}
	return result;
}