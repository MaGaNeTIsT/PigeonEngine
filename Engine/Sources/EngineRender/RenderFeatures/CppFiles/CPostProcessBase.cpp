#include "../Headers/CPostProcessBase.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"

CPostProcessBase::CPostProcessBase()
{
	this->m_SwapRenderTarget[0] = NULL;
	this->m_SwapRenderTarget[1] = NULL;
	for (INT i = 0; i < PostProcessType::POST_PROCESS_COUNT; i++)
	{
		this->m_PostShader[i] = NULL;
	}
	this->m_SwapIndex = 0;
	this->m_PostShader[PostProcessType::POST_PROCESS_COLOR_GRADING] = CShaderManager::LoadPixelShader("./Engine/Assets/EngineShaders/ColorGrading.cso");
}
CPostProcessBase::~CPostProcessBase()
{
	for (INT i = 0; i < PostProcessType::POST_PROCESS_COUNT; i++)
	{
		if (this->m_PostShader[i] != NULL)
		{
			delete (this->m_PostShader[i]);
			this->m_PostShader[i] = NULL;
		}
	}
}
void CPostProcessBase::InitPerFrame(const CRenderDevice::RenderTexture2DViewInfo* firstDst, const CRenderDevice::RenderTexture2DViewInfo* firstSrc)
{
	this->m_SwapIndex = 0;
	this->m_SwapRenderTarget[0] = firstSrc;
	this->m_SwapRenderTarget[1] = firstDst;
}
void CPostProcessBase::Draw(PostProcessType type)
{
	if (type < 0 || type >= PostProcessType::POST_PROCESS_COUNT)
	{
		return;
	}
	if (this->m_PostShader[type] == NULL)
	{
		return;
	}
	INT currentSrc = this->m_SwapIndex;
	INT currentDst = 1 - this->m_SwapIndex;
	CRenderDevice::SetRenderTarget(this->m_SwapRenderTarget[currentDst]->RenderTargetView);
	CRenderDevice::BindPSShaderResourceView(this->m_SwapRenderTarget[currentSrc]->ShaderResourceView, ENGINE_SCENE_COLOR);
	this->m_PostShader[type]->Bind();
	this->m_SwapIndex = 1 - this->m_SwapIndex;
}