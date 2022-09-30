#include "../../../../Entry/EngineMain.h"
#include "../Headers/CSkyBox.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/AssetsManager/Headers/CShader.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRenderer.h"

std::shared_ptr<CVertexShader> CSkyBox::m_VertexShader = nullptr;

CSkyBox::CSkyBox()
{
	if (!m_VertexShader)
	{
		m_VertexShader = CShaderManager::LoadVertexShader();
	}
	this->m_SkyBoxInfo = SkyBoxInfo(static_cast<FLOAT>(1000000));
}
void CSkyBox::Init()
{

}
void CSkyBox::Uninit()
{

}
void CSkyBox::Update()
{

}
void CSkyBox::PrepareDraw()
{

}
void CSkyBox::Draw()
{

}