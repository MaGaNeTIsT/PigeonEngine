#include "../Headers/CHZBBuffer.h"
#include "../../../EngineGame/Headers/CCamera.h"
#include "../../../EngineGame/Headers/CScene.h"
#include "../../../EngineGame/Headers/CScreenPolygon2D.h"

CHZBBuffer::CHZBBuffer()
{
	this->m_Polygon2D = NULL;
	this->m_BufferSize = CustomType::Vector2Int(0, 0);
	this->m_PipelineSize = CustomType::Vector2Int(0, 0);

	this->m_HZBMipLevels = 0;
}
CHZBBuffer::~CHZBBuffer()
{
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
}
void CHZBBuffer::Init(const CustomType::Vector2Int& bufferSize, const CustomType::Vector2Int& pipelineSize)
{
	this->m_BufferSize = bufferSize;
	this->m_PipelineSize = pipelineSize;

	this->m_HZBMipLevels = 1;

	CRenderDevice::CreateRenderTexture2D(
		this->m_HZBBuffer,
		CustomStruct::CRenderTextureDesc(
			static_cast<UINT>(bufferSize.X()),
			static_cast<UINT>(bufferSize.Y()),
			CustomStruct::CRenderBindFlag::BIND_SRV_UAV,
			CustomStruct::CRenderFormat::FORMAT_R32_FLOAT));

	this->m_Polygon2D = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, CustomType::Vector4(0, 0, pipelineSize.X(), pipelineSize.Y()));
	this->m_Polygon2D->Init();

	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/BuildHZBComputeShader.cso", this->m_BuildHZBComputeShader);
}
void CHZBBuffer::Uninit()
{
	if (this->m_Polygon2D != NULL)
	{
		delete (this->m_Polygon2D);
		this->m_Polygon2D = NULL;
	}
}
void CHZBBuffer::Update()
{

}
void CHZBBuffer::PrepareDraw(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& sceneDepth)
{
	CRenderDevice::BindCSShaderResourceView(sceneDepth, 0u);
	CRenderDevice::BindCSUnorderedAccessView(this->m_HZBBuffer.UnorderedAccessView, 0u);
	CRenderDevice::SetCSShader(this->m_BuildHZBComputeShader);
	CRenderDevice::Dispatch(static_cast<UINT>((this->m_BufferSize.X() + 7) / 8), static_cast<UINT>((this->m_BufferSize.Y() + 7) / 8), 1u);
	CRenderDevice::BindCSUnorderedAccessView(nullptr, 0u);
	CRenderDevice::SetCSShader(nullptr);

	CRenderDevice::BindPSShaderResourceView(this->m_HZBBuffer.ShaderResourceView, ENGINE_TEXTURE2D_ALBEDO_START_SLOT);
}
void CHZBBuffer::Draw(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& sceneDepth)
{
	this->PrepareDraw(sceneDepth);
	this->m_Polygon2D->Draw();
}