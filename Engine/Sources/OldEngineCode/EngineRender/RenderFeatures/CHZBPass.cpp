/*
#include "../Headers/CHZBPass.h"
#include "../../RenderBase/Headers/CMeshRendererComponent.h"
#include "../../../EngineGame/Headers/CCamera.h"
#include "../../../EngineGame/Headers/CScene.h"

#ifdef _DEVELOPMENT_EDITOR
#include "../../../EngineGame/Headers/CScreenPolygon2D.h"
#endif

CHZBPass::CHZBPass()
{
#ifdef _DEVELOPMENT_EDITOR
	this->m_Polygon2D = NULL;
	this->m_DebugType = FALSE;
	this->m_DebugLevel = 0;
#endif
	this->m_PipelineSize = CustomType::Vector2Int(0, 0);
	this->m_HZBMipLevels = 0;
}
CHZBPass::~CHZBPass()
{
#ifdef _DEVELOPMENT_EDITOR
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
#endif
}
void CHZBPass::Init(const CustomType::Vector2Int& pipelineSize)
{
	this->m_PipelineSize = pipelineSize;
	this->CalculateHZBLevels();
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/BuildHZBComputeShader.cso", this->m_BuildHZBComputeShader);
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/RawDownSamplingComputeShader.cso", this->m_RawDownSamplingComputeShader);
	this->InitHZBBuffers();

#ifdef _DEVELOPMENT_EDITOR
	this->m_Polygon2D = new CScreenPolygon2D(TRUE, nullptr, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, CustomType::Vector4(0, 0, pipelineSize.X(), pipelineSize.Y()));
	this->m_Polygon2D->Init();
#endif
}
void CHZBPass::Uninit()
{
#ifdef _DEVELOPMENT_EDITOR
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
#endif
}
void CHZBPass::Update()
{

}
void CHZBPass::ComputeHZB(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& sceneDepth)
{
	CRenderDevice::SetCSShader(this->m_RawDownSamplingComputeShader);
	CRenderDevice::BindCSShaderResourceView(sceneDepth, 5u);
	CRenderDevice::BindCSUnorderedAccessView(this->m_HZBBuffers[0].UnorderedAccessView, 0u);
	CRenderDevice::Dispatch(static_cast<UINT32>((this->m_HZBSizes[0].X() + 7) / 8), static_cast<UINT32>((this->m_HZBSizes[0].Y() + 7) / 8), 1u);
	CRenderDevice::BindNoCSUnorderedAccessView(0u);
	CRenderDevice::SetNoCSShader();

	CRenderDevice::SetCSShader(this->m_BuildHZBComputeShader);
	for (UINT32 i = 1u; i < this->m_HZBBuffers.size(); i++)
	{
		CRenderDevice::BindCSShaderResourceView(this->m_HZBBuffers[i - 1u].ShaderResourceView, 5u);
		CRenderDevice::BindCSUnorderedAccessView(this->m_HZBBuffers[i].UnorderedAccessView, 0u);
		CRenderDevice::Dispatch(static_cast<UINT32>((this->m_HZBSizes[i].X() + 7) / 8), static_cast<UINT32>((this->m_HZBSizes[i].Y() + 7) / 8), 1u);
		CRenderDevice::BindNoCSUnorderedAccessView(0u);
	}
	CRenderDevice::SetNoCSShader();
}
#ifdef _DEVELOPMENT_EDITOR
void CHZBPass::EditorUpdate()
{
	bool tempDebugType = this->m_DebugType;

	ImGui::Begin("HZB parameters");
	ImGui::Checkbox("Debug Enable", &tempDebugType);
	ImGui::SliderInt("Debug Level", &(this->m_DebugLevel), 0, static_cast<INT32>(this->m_HZBBuffers.size() - 1u));
	for (UINT32 i = 0u; i < (this->m_HZBSizes.size()); i++)
	{
		ImGui::Text("Resolution[%d] : x = %d, y = %d.", i, this->m_HZBSizes[i].X(), this->m_HZBSizes[i].Y());
	}
	ImGui::End();

	this->m_DebugType = tempDebugType;
}
void CHZBPass::DrawDebug()
{
	if (this->m_DebugType)
	{
		CRenderDevice::BindPSShaderResourceView(this->m_HZBBuffers[this->m_DebugLevel].ShaderResourceView, ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT);
		{
			const CMeshRendererComponent* meshRenderer = this->m_Polygon2D->GetMeshRendererComponent<CMeshRendererComponent>();
			if (meshRenderer)
			{
				meshRenderer->Draw();
			}
		}
	}
}
#endif
void CHZBPass::GetHZBBufferByMipIndex(CRenderDevice::RenderTexture2DViewInfo& buffer, const UINT32& idx)
{
	if (idx < this->m_HZBBuffers.size())
	{
		buffer = this->m_HZBBuffers[idx];
	}
}
void CHZBPass::GetHZBBuffers(std::vector<CRenderDevice::RenderTexture2DViewInfo>& buffers)
{
	buffers = this->m_HZBBuffers;
}
void CHZBPass::GetHZBSizeByMipIndex(CustomType::Vector2Int& size, const UINT32& idx)
{
	if (idx < this->m_HZBSizes.size())
	{
		size = this->m_HZBSizes[idx];
	}
}
void CHZBPass::GetHZBSizes(std::vector<CustomType::Vector2Int>& sizes)
{
	sizes = this->m_HZBSizes;
}
void CHZBPass::CalculateHZBLevels()
{
	INT32 wCount = CustomType::CMath::Log2Floor(this->m_PipelineSize.X());
	INT32 hCount = CustomType::CMath::Log2Floor(this->m_PipelineSize.Y());
	this->m_HZBMipLevels = CustomType::CMath::Min(wCount, hCount) + 1u;
	this->m_HZBSizes.resize(static_cast<size_t>(this->m_HZBMipLevels));
	INT32 w = CustomType::CMath::Exp2(wCount - (this->m_HZBMipLevels - 1));
	INT32 h = CustomType::CMath::Exp2(hCount - (this->m_HZBMipLevels - 1));
	this->m_HZBSizes[this->m_HZBMipLevels - 1] = CustomType::Vector2Int(w, h);
	for (INT32 i = this->m_HZBMipLevels - 2; i >= 0; i--)
	{
		this->m_HZBSizes[i] = this->m_HZBSizes[i + 1] * 2;
	}
}
void CHZBPass::InitHZBBuffers()
{
	this->m_HZBBuffers.resize(this->m_HZBSizes.size());
	for (UINT32 i = 0u; i < this->m_HZBSizes.size(); i++)
	{
		CRenderDevice::CreateRenderTexture2D(
			this->m_HZBBuffers[i],
			CustomStruct::CRenderTextureDesc(
				static_cast<UINT32>(this->m_HZBSizes[i].X()),
				static_cast<UINT32>(this->m_HZBSizes[i].Y()),
				CustomStruct::CRenderBindFlag::BIND_SRV_UAV,
				CustomStruct::CRenderFormat::FORMAT_R32_FLOAT));
	}
}
*/