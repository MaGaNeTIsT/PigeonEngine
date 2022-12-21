#include "../Headers/CGPUCulling.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../RenderFeatures/Headers/CHZBPass.h"
#include "../../../EngineGame/Headers/CGameObject.h"

CGPUCulling::CGPUCulling()
{
	this->m_CachedCount = 0u;
	this->m_MostDetailHZBIndex = 0;
	this->m_UsedHZBNum = 0;
	this->m_CachedAABBNum = 0u;
	this->m_PassingCount = 0u;

#ifdef _DEVELOPMENT_EDITOR
	this->m_EditorFrameCount = 0u;
#endif
}
CGPUCulling::~CGPUCulling()
{
}
void CGPUCulling::Init(std::shared_ptr<CHZBPass> hzb, const UINT& waitingFrameNum, const UINT& baseSceneObjectsNum)
{
	this->m_HZBPass = hzb;
	this->m_MostDetailHZBIndex = hzb->GetHZBBufferNum() - 1u;
	this->m_UsedHZBNum = 1;
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/EngineGPUCulling.cso", this->m_GPUCullingComputeShader);
	this->m_CachedCount = CustomType::CMath::Clamp(waitingFrameNum, 0u, 4u);
	this->m_CachedCount = this->m_CachedCount + 2u;
	this->m_CachedAABBInfo.clear();
	this->m_CachedAABBNum = 0u;
	this->m_PassingCount = 0u;
	this->m_MappedUIDCullingResult.clear();
	this->m_CachedCullingInfo.resize(this->m_CachedCount);
	for (UINT i = 0u; i < this->m_CachedCount; i++)
	{
		CGPUCulling::ReCreateBufferSize(i, baseSceneObjectsNum);
	}
	this->m_CullingData.Parameters = DirectX::XMINT4(baseSceneObjectsNum, 0, 0, 0);
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
void CGPUCulling::Update(const ULONGLONG& frameIndex)
{
#ifdef _DEVELOPMENT_EDITOR
	this->m_EditorFrameCount = frameIndex;
#endif
}
void CGPUCulling::ReadBackAndPrepareCullingResult(const ULONGLONG& frameIndex, const std::vector<CGameObject*>& inputCullingResult, std::vector<BOOL>& outputCullingResult)
{
	UINT index = frameIndex % (this->m_CachedCount);
	outputCullingResult.resize(inputCullingResult.size(), TRUE);
	if (frameIndex > this->m_CachedCount)
	{
		this->m_PassingCount = 0u;	//TODO Delte counter for shipping.
		if (this->ReadBackFromResource(frameIndex))
		{
			for (UINT i = 0u; i < inputCullingResult.size(); i++)
			{
				std::map<ULONGLONG, UINT>::iterator occluded = this->m_MappedUIDCullingResult.find(inputCullingResult[i]->GetUniqueID());
				if (occluded != this->m_MappedUIDCullingResult.end())
				{
					outputCullingResult[i] = (occluded->second) == 1u;
					////TODO Delte counter for shipping.
					if (outputCullingResult[i])
					{
						this->m_PassingCount++;
					}
				}
			}
		}
	}

	this->PrepareCullingInfo(frameIndex, inputCullingResult);
}
void CGPUCulling::ComputeCulling(const ULONGLONG& frameIndex)
{
	UINT index = frameIndex % (this->m_CachedCount);

	{		
		INT layerCount = static_cast<INT>(this->m_HZBPass->GetHZBBufferNum());
		INT mostDetail = CustomType::CMath::Clamp(this->m_MostDetailHZBIndex, 0, layerCount - 1);
		INT numLayer = CustomType::CMath::Clamp(this->m_UsedHZBNum, 1, layerCount - mostDetail);
		numLayer = CustomType::CMath::Clamp(numLayer, 1, 7);
		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> hzbBuffers(numLayer);
		INT indexHZB = mostDetail;
		for (INT i = 0; i < numLayer; i++)
		{
			CRenderDevice::RenderTexture2DViewInfo usedHZBBuffer;
			this->m_HZBPass->GetHZBBufferByMipIndex(usedHZBBuffer, indexHZB);
			hzbBuffers[(numLayer - 1) - i] = usedHZBBuffer.ShaderResourceView;
			indexHZB++;
		}
		this->m_CullingData.Parameters = DirectX::XMINT4(static_cast<int32_t>(this->m_CachedCullingInfo[index].NumObjects), mostDetail, numLayer, 0);
		CRenderDevice::UploadBuffer(this->m_CachedCullingInfo[index].CullingBoundingInfo.Buffer, static_cast<void*>(this->m_CachedAABBInfo.data()), sizeof(PrimitivesAABBBoxInfo) * this->m_CachedAABBNum, sizeof(PrimitivesAABBBoxInfo) * this->m_CachedAABBNum);
		CRenderDevice::UploadBuffer(this->m_CullingConstantBuffer, static_cast<void*>(&(this->m_CullingData)));
		CRenderDevice::BindCSConstantBuffer(this->m_CullingConstantBuffer, 1u);
		for (UINT i = 0u; i < static_cast<UINT>(numLayer); i++)
		{
			CRenderDevice::BindCSShaderResourceView(hzbBuffers[i], 1u + i);
		}
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
#ifdef _DEVELOPMENT_EDITOR
void CGPUCulling::EditorUpdate()
{
	INT index, readBackIndex;
	{
		index = static_cast<INT>(this->m_EditorFrameCount % (this->m_CachedCount));
		INT totalCount = static_cast<INT>(this->m_CachedCount);
		readBackIndex = static_cast<INT>(index) - totalCount + 1;
		readBackIndex = (readBackIndex < 0) ? (readBackIndex + totalCount) : readBackIndex;
	}

	INT mostDetailHZBIndex = this->m_MostDetailHZBIndex;
	INT usedHZBNum = this->m_UsedHZBNum;
	INT layerCount = static_cast<INT>(this->m_HZBPass->GetHZBBufferNum());
	ImGui::Begin("Occlusion test parameters");
	ImGui::Text("Current  index = %d number of objects = %d", index, this->m_CachedCullingInfo[index].NumObjects);
	ImGui::Text("ReadBack index = %d number of objects = %d", readBackIndex, this->m_CachedCullingInfo[readBackIndex].NumObjects);
	ImGui::Text("Passing number of objects = %d", this->m_PassingCount);
	ImGui::SliderInt("Most HZB layer", &mostDetailHZBIndex, 0, static_cast<INT>(this->m_HZBPass->GetHZBBufferNum() - 1u));
	ImGui::SliderInt("Used number of HZB layers", &usedHZBNum, 1, layerCount - mostDetailHZBIndex);
	ImGui::End();
	this->m_MostDetailHZBIndex = mostDetailHZBIndex;
	this->m_UsedHZBNum = usedHZBNum;
}
#endif
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
		this->m_CachedAABBInfo.resize(fromCPUCullingResult.size());
		std::vector<ULONGLONG> objectsUID(fromCPUCullingResult.size());
		UINT numCounter = 0u;
		{
			CustomType::Vector3 min, max;
			for (UINT i = 0u; i < fromCPUCullingResult.size(); i++)
			{
				if (fromCPUCullingResult[i]->HasRenderBoundingBox())
				{
					fromCPUCullingResult[i]->GetRenderWorldAABBBoundingBox(min, max);
					this->m_CachedAABBInfo[numCounter].Min[0] = min.X();
					this->m_CachedAABBInfo[numCounter].Min[1] = min.Y();
					this->m_CachedAABBInfo[numCounter].Min[2] = min.Z();
					this->m_CachedAABBInfo[numCounter].Max[0] = max.X();
					this->m_CachedAABBInfo[numCounter].Max[1] = max.Y();
					this->m_CachedAABBInfo[numCounter].Max[2] = max.Z();
					objectsUID[numCounter] = fromCPUCullingResult[i]->GetUniqueID();
					numCounter++;
				}
			}
			this->m_CachedAABBNum = numCounter;
		}
		UINT index = frameIndex % (this->m_CachedCount);
		{
			this->ReCreateBufferSize(index, numCounter);
			CachedCullingInfo* currentUsedInfo = &(this->m_CachedCullingInfo[index]);
			memcpy_s(currentUsedInfo->MappingCullingResult.data(), sizeof(ULONGLONG) * currentUsedInfo->MappingCullingResult.size(), objectsUID.data(), sizeof(ULONGLONG) * numCounter);
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
			this->m_MappedUIDCullingResult[currentUsedInfo->MappingCullingResult[i]] = currentCullingResult[i] & 0x1u;
		}
	}
	return result;
}