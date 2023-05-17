#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

class CPostProcessBase
{
public:
	enum PostProcessType
	{
		POST_PROCESS_COLOR_GRADING,
		POST_PROCESS_COUNT
	};
public:
	const CRenderDevice::RenderTexture2DViewInfo* GetResultTarget()const { return (this->m_SwapRenderTarget[this->m_SwapIndex]); }
	void	InitPerFrame(const CRenderDevice::RenderTexture2DViewInfo* firstDst, const CRenderDevice::RenderTexture2DViewInfo* firstSrc);
	void	Draw(PostProcessType type);
private:
	const CRenderDevice::RenderTexture2DViewInfo*	m_SwapRenderTarget[2];
	class CPixelShader*								m_PostShader[POST_PROCESS_COUNT];
	INT												m_SwapIndex;
public:
	CPostProcessBase();
	~CPostProcessBase();
};