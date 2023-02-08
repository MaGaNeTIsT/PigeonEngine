#include "../Headers/CDynamicWind.h"
#include "../../RenderBase/Headers/CRenderStructCommon.h"

#ifdef _DEVELOPMENT_EDITOR
#include "../../../EngineBase/Headers/CManager.h"
#include "../../../EngineBase/Headers/CTimer.h"
#include "../../../EngineGame/Headers/CScene.h"
#include "../../RenderBase/Headers/CMeshRendererComponent.h"
#include "../../../EngineGame/Headers/CScreenPolygon2D.h"
#endif

#ifdef _DEVELOPMENT_EDITOR
const CGameTimer* _GGameTimer = CManager::GetGameTimer();
#endif

CDynamicWind::CDynamicWind()
{
	this->m_ComputeDynamicWindShader = nullptr;
	//CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/BuildHZBComputeShader.cso", this->m_ComputeDynamicWindShader);
#ifdef _DEVELOPMENT_EDITOR
	this->m_DebugWindCPUType = 0;
	this->m_ComputeDynamicWindCPUDebugData.DebugType = DirectX::XMINT4(0, 0, 0, 0);
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/DynamicWindCPUDebug.cso", this->m_ComputeDynamicWindCPUDebugShader);
	CRenderDevice::CreateBuffer(this->m_ComputeDynamicWindCPUDebugCBuffer, CustomStruct::CRenderBufferDesc(sizeof(DynamicWindCPUDebugData), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	CRenderDevice::CreateRenderTexture2D(this->m_DebugWindCPUDensityBuffer, CustomStruct::CRenderTextureDesc(
		(CDynamicWind::_StaticCellWidth + 2u), (CDynamicWind::_StaticCellHeight + 2u),
		CustomStruct::CRenderBindFlag::BIND_SRV_UAV, CustomStruct::CRenderFormat::FORMAT_R32_FLOAT));
	CRenderDevice::CreateRenderTexture2D(this->m_DebugWindCPUUVectorBuffer, CustomStruct::CRenderTextureDesc(
		(CDynamicWind::_StaticCellWidth + 2u), (CDynamicWind::_StaticCellHeight + 2u),
		CustomStruct::CRenderBindFlag::BIND_SRV_UAV, CustomStruct::CRenderFormat::FORMAT_R32_FLOAT));
	CRenderDevice::CreateRenderTexture2D(this->m_DebugWindCPUVVectorBuffer, CustomStruct::CRenderTextureDesc(
		(CDynamicWind::_StaticCellWidth + 2u), (CDynamicWind::_StaticCellHeight + 2u),
		CustomStruct::CRenderBindFlag::BIND_SRV_UAV, CustomStruct::CRenderFormat::FORMAT_R32_FLOAT));
	CRenderDevice::CreateRenderTexture2D(this->m_DebugWindCPUResultBuffer, CustomStruct::CRenderTextureDesc(
		(CDynamicWind::_StaticCellWidth + 2u), (CDynamicWind::_StaticCellHeight + 2u),
		CustomStruct::CRenderBindFlag::BIND_SRV_UAV, CustomStruct::CRenderFormat::FORMAT_B8G8R8A8_UNORM));
	this->m_DebugWindCPUWnd		= nullptr;
	this->m_CellU				= nullptr;
	this->m_CellV				= nullptr;
	this->m_CellD				= nullptr;
	this->m_CellPrevU			= nullptr;
	this->m_CellPrevV			= nullptr;
	this->m_CellPrevD			= nullptr;
	this->m_Viscosity			= 0.f;
	this->m_DiffuseCoefficient	= 0.f;
	this->m_DSource				= 0.f;
	this->m_USource				= 0.f;
	this->m_VSource				= 0.f;
	this->m_DrawDebugWindCPU	= FALSE;
	this->m_AddSource			= FALSE;
	this->m_ResetSource			= FALSE;
	CDynamicWind::CreatePointer(this->m_CellU, CDynamicWind::_StaticCellCount);
	CDynamicWind::CreatePointer(this->m_CellV, CDynamicWind::_StaticCellCount);
	CDynamicWind::CreatePointer(this->m_CellD, CDynamicWind::_StaticCellCount);
	CDynamicWind::CreatePointer(this->m_CellPrevU, CDynamicWind::_StaticCellCount);
	CDynamicWind::CreatePointer(this->m_CellPrevV, CDynamicWind::_StaticCellCount);
	CDynamicWind::CreatePointer(this->m_CellPrevD, CDynamicWind::_StaticCellCount);
	CDynamicWind::InitPointer(this->m_CellU, CDynamicWind::_StaticCellCount);
	CDynamicWind::InitPointer(this->m_CellV, CDynamicWind::_StaticCellCount);
	CDynamicWind::InitPointer(this->m_CellD, CDynamicWind::_StaticCellCount);
	CDynamicWind::InitPointer(this->m_CellPrevU, CDynamicWind::_StaticCellCount);
	CDynamicWind::InitPointer(this->m_CellPrevV, CDynamicWind::_StaticCellCount);
	CDynamicWind::InitPointer(this->m_CellPrevD, CDynamicWind::_StaticCellCount);
#endif
}
CDynamicWind::~CDynamicWind()
{
#ifdef _DEVELOPMENT_EDITOR
	if (this->m_DebugWindCPUWnd != nullptr)
	{
		delete (this->m_DebugWindCPUWnd);
		this->m_DebugWindCPUWnd = nullptr;
	}
	CDynamicWind::DeletePointer(this->m_CellU);
	CDynamicWind::DeletePointer(this->m_CellV);
	CDynamicWind::DeletePointer(this->m_CellD);
	CDynamicWind::DeletePointer(this->m_CellPrevU);
	CDynamicWind::DeletePointer(this->m_CellPrevV);
	CDynamicWind::DeletePointer(this->m_CellPrevD);
#endif
}
void CDynamicWind::Init()
{
#ifdef _DEVELOPMENT_EDITOR
	this->m_DrawDebugWindCPU = FALSE;
	this->m_AddSource = FALSE;
	this->m_ResetSource = FALSE;
#endif
}
void CDynamicWind::Uninit()
{
#ifdef _DEVELOPMENT_EDITOR
	if (this->m_DebugWindCPUWnd != nullptr)
	{
		this->m_DebugWindCPUWnd->Uninit();
	}
#endif
}
void CDynamicWind::Update()
{
#ifdef _DEVELOPMENT_EDITOR
	if (this->m_DebugWindCPUWnd != nullptr)
	{
		this->m_DebugWindCPUWnd->Update();
	}
#endif
}
void CDynamicWind::ComputeDynamicWind()
{

}
#ifdef _DEVELOPMENT_EDITOR
void CDynamicWind::EditorUpdate()
{
	bool bAddSource = this->m_AddSource;
	bool bDrawDebugWindCPU = this->m_DrawDebugWindCPU;

	ImGui::Begin("DynamicWindParameters");
	ImGui::Checkbox("DrawDebugWindCPU", &bDrawDebugWindCPU);
	ImGui::Checkbox("AddSource", &bAddSource);
	if (ImGui::Button("ResetSource"))
	{
		this->m_ResetSource = TRUE;
	}
	ImGui::SliderInt("DebugWindCPUType", &(this->m_DebugWindCPUType), 0, 1, "%d", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
	ImGui::SliderFloat("ViscosityCoefficient", &(this->m_Viscosity), -10.f, 10.f, "%.4f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
	ImGui::SliderFloat("DiffuseCoefficient", &(this->m_DiffuseCoefficient), -1.f, 1.f, "%.4f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
	ImGui::SliderFloat("DensitySource", &(this->m_DSource), 0.f, 2.f, "%.4f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
	ImGui::SliderFloat("UVectorSource", &(this->m_USource), -1000.f, 1000.f, "%.4f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
	ImGui::SliderFloat("VVectorSource", &(this->m_VSource), -1000.f, 1000.f, "%.4f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
	ImGui::End();

	this->m_AddSource = bAddSource;
	this->m_DrawDebugWindCPU = bDrawDebugWindCPU;
	this->m_ComputeDynamicWindCPUDebugData.DebugType = DirectX::XMINT4(this->m_DebugWindCPUType, 0, 0, 0);
}
void CDynamicWind::DrawDebug(const CustomType::Vector2Int& pipelineSize)
{
	if (this->m_DebugWindCPUWnd == nullptr)
	{
		this->m_DebugWindCPUWnd = new CScreenPolygon2D(TRUE, nullptr, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, CustomType::Vector4(0, 0, pipelineSize.X(), pipelineSize.Y()));
		this->m_DebugWindCPUWnd->Init();
	}
	else
	{
		CustomType::Vector4Int wndSize(this->m_DebugWindCPUWnd->GetScreenPosition());
		if (wndSize != CustomType::Vector4Int(0, 0, pipelineSize.X(), pipelineSize.Y()))
		{
			delete (this->m_DebugWindCPUWnd);
			this->m_DebugWindCPUWnd = new CScreenPolygon2D(TRUE, nullptr, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, CustomType::Vector4(0, 0, pipelineSize.X(), pipelineSize.Y()));
			this->m_DebugWindCPUWnd->Init();
		}
	}
	if (this->m_CellU == nullptr ||
		this->m_CellV == nullptr ||
		this->m_CellD == nullptr ||
		this->m_CellPrevU == nullptr ||
		this->m_CellPrevV == nullptr ||
		this->m_CellPrevD == nullptr)
	{
		return;
	}

	if (!this->m_DrawDebugWindCPU)
	{
		return;
	}

	{
		FLOAT deltaTime = static_cast<FLOAT>(_GGameTimer->GetDeltaTime());
		CDynamicWind::RunCPUWind(deltaTime);
	}

	if (this->m_DrawDebugWindCPU)
	{
		{
			CRenderDevice::SetCSShader(this->m_ComputeDynamicWindCPUDebugShader);
			CRenderDevice::UploadBuffer(this->m_ComputeDynamicWindCPUDebugCBuffer, &(this->m_ComputeDynamicWindCPUDebugData));
			CRenderDevice::BindCSConstantBuffer(this->m_ComputeDynamicWindCPUDebugCBuffer, 3u);
			if (this->m_DebugWindCPUType == 0)
			{
				CRenderDevice::UploadResource(this->m_DebugWindCPUDensityBuffer.Texture2D, this->m_CellD, 4 * (CDynamicWind::_StaticCellWidth + 2u), 4 * CDynamicWind::_StaticCellCount);
				CRenderDevice::BindCSShaderResourceView(this->m_DebugWindCPUDensityBuffer.ShaderResourceView, 5u);
			}
			else if (this->m_DebugWindCPUType == 1)
			{
				CRenderDevice::UploadResource(this->m_DebugWindCPUUVectorBuffer.Texture2D, this->m_CellU, 4 * (CDynamicWind::_StaticCellWidth + 2u), 4 * CDynamicWind::_StaticCellCount);
				CRenderDevice::UploadResource(this->m_DebugWindCPUVVectorBuffer.Texture2D, this->m_CellV, 4 * (CDynamicWind::_StaticCellWidth + 2u), 4 * CDynamicWind::_StaticCellCount);
				CRenderDevice::BindCSShaderResourceView(this->m_DebugWindCPUUVectorBuffer.ShaderResourceView, 5u);
				CRenderDevice::BindCSShaderResourceView(this->m_DebugWindCPUVVectorBuffer.ShaderResourceView, 6u);
			}

			CRenderDevice::BindCSUnorderedAccessView(this->m_DebugWindCPUResultBuffer.UnorderedAccessView, 0u);
			CRenderDevice::Dispatch(((CDynamicWind::_StaticCellWidth + 2u) + 3u) / 4u, ((CDynamicWind::_StaticCellHeight + 2u) + 3u) / 4u, 1u);
			CRenderDevice::BindNoCSUnorderedAccessView(0u);
		}

		{
			CRenderDevice::BindPSShaderResourceView(this->m_DebugWindCPUResultBuffer.ShaderResourceView, ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT);
			const CMeshRendererComponent* meshRenderer = this->m_DebugWindCPUWnd->GetMeshRendererComponent<CMeshRendererComponent>();
			if (meshRenderer)
			{
				meshRenderer->Draw();
			}
		}
	}
}
void CDynamicWind::RunCPUWind(const FLOAT& deltaTime)
{
	if (this->m_ResetSource)
	{
		CDynamicWind::InitPointer(this->m_CellU, CDynamicWind::_StaticCellCount);
		CDynamicWind::InitPointer(this->m_CellV, CDynamicWind::_StaticCellCount);
		CDynamicWind::InitPointer(this->m_CellD, CDynamicWind::_StaticCellCount);
		CDynamicWind::InitPointer(this->m_CellPrevU, CDynamicWind::_StaticCellCount);
		CDynamicWind::InitPointer(this->m_CellPrevV, CDynamicWind::_StaticCellCount);
		CDynamicWind::InitPointer(this->m_CellPrevD, CDynamicWind::_StaticCellCount);
		this->m_ResetSource = FALSE;
	}
	if (this->m_AddSource)
	{
		this->AddCPUWindSource(deltaTime);
	}
	CDynamicWind::VelocityStep(CDynamicWind::_StaticIterationCount, CDynamicWind::_StaticCellWidth, CDynamicWind::_StaticCellHeight, this->m_CellU, this->m_CellV, this->m_CellPrevU, this->m_CellPrevV, this->m_Viscosity, deltaTime);
	CDynamicWind::DensityStep(CDynamicWind::_StaticIterationCount, CDynamicWind::_StaticCellWidth, CDynamicWind::_StaticCellHeight, this->m_CellD, this->m_CellPrevD, this->m_CellU, this->m_CellV, this->m_DiffuseCoefficient, deltaTime);
}
void CDynamicWind::AddCPUWindSource(const FLOAT& deltaTime)
{
	INT i = 64, j = 64; FLOAT d = this->m_DSource, u = this->m_USource, v = this->m_VSource;
	FLOAT* tempDSource = new FLOAT[CDynamicWind::_StaticCellCount];
	FLOAT* tempUSource = new FLOAT[CDynamicWind::_StaticCellCount];
	FLOAT* tempVSource = new FLOAT[CDynamicWind::_StaticCellCount];
	for (UINT index = 0u; index < CDynamicWind::_StaticCellCount; index++)
	{
		tempDSource[index] = 0.f;
		tempUSource[index] = 0.f;
		tempVSource[index] = 0.f;
	}
	for (INT x = -5; x <= 5; x++)
	{
		for (INT y = -5; y <= 5; y++)
		{
			tempDSource[CDynamicWind::TransformCellCoordToIndex(x + i, y + j, CDynamicWind::_StaticCellWidth)] = d;
			tempUSource[CDynamicWind::TransformCellCoordToIndex(x + i, y + j, CDynamicWind::_StaticCellWidth)] = u;
			tempVSource[CDynamicWind::TransformCellCoordToIndex(x + i, y + j, CDynamicWind::_StaticCellWidth)] = v;
		}
	}
	CDynamicWind::AddSource(CDynamicWind::_StaticCellWidth, CDynamicWind::_StaticCellHeight, this->m_CellPrevD, tempDSource, deltaTime);
	CDynamicWind::AddSource(CDynamicWind::_StaticCellWidth, CDynamicWind::_StaticCellHeight, this->m_CellPrevU, tempUSource, deltaTime);
	CDynamicWind::AddSource(CDynamicWind::_StaticCellWidth, CDynamicWind::_StaticCellHeight, this->m_CellPrevV, tempVSource, deltaTime);
	delete[]tempDSource;
	delete[]tempUSource;
	delete[]tempVSource;
}
inline UINT CDynamicWind::TransformCellCoordToIndex(const UINT& i, const UINT& j, const UINT& w)
{
	return (i + (w + 2u) * j);
}
inline void CDynamicWind::TransformCellIndexToCoord(const UINT& index, const UINT& w, const UINT& h, UINT& i, UINT& j)
{
	i = index % (w + 2u);
	j = index / (h + 2u);
}
inline INT CDynamicWind::TransformCellCoordToIndex(const INT& i, const INT& j, const UINT& w)
{
	return (i + (static_cast<INT>(w + 2u)) * j);
}
inline void CDynamicWind::TransformCellIndexToCoord(const INT& index, const UINT& w, const UINT& h, INT& i, INT& j)
{
	i = index % (static_cast<INT>(w + 2u));
	j = index / (static_cast<INT>(h + 2u));
}
inline void CDynamicWind::Swap(FLOAT*& a, FLOAT*& b)
{
	FLOAT* temp = a;
	a = b;
	b = temp;
}
inline void CDynamicWind::InitPointer(FLOAT*& ptr, const UINT& num, const FLOAT& init)
{
	if (ptr != nullptr)
	{
		for (UINT i = 0u; i < num; i++)
		{
			ptr[i] = init;
		}
	}
}
inline void CDynamicWind::CreatePointer(FLOAT*& ptr, const UINT& num)
{
	if (ptr == nullptr)
	{
		ptr = new FLOAT[num];
	}
}
inline void CDynamicWind::DeletePointer(FLOAT*& ptr)
{
	if (ptr != nullptr)
	{
		delete[]ptr;
		ptr = nullptr;
	}
}
void CDynamicWind::AddSource(const UINT& w, const UINT& h, FLOAT* x, FLOAT* s, const FLOAT& dt)
{
	const UINT size = (w + 2u) * (h + 2u);
	for (UINT i = 0u; i < size; i++)
	{
		x[i] += dt * s[i];
	}
}
void CDynamicWind::SetBoundary(const UINT& w, const UINT& h, const INT& b, FLOAT* x)
{
	const INT iW = static_cast<INT>(w);
	const INT iH = static_cast<INT>(h);
	const BOOL clearBound = TRUE;
	if (clearBound)
	{
		for (INT i = 1; i < iW; i++)
		{
			x[CDynamicWind::TransformCellCoordToIndex(i, 0, w)] = 0.f;
			x[CDynamicWind::TransformCellCoordToIndex(i, iH + 1, w)] = 0.f;
		}
		for (INT j = 1; j < iH; j++)
		{
			x[CDynamicWind::TransformCellCoordToIndex(0, j, w)] = 0.f;
			x[CDynamicWind::TransformCellCoordToIndex(iW + 1, j, w)] = 0.f;
		}
		x[CDynamicWind::TransformCellCoordToIndex(0, 0, w)] = 0.f;
		x[CDynamicWind::TransformCellCoordToIndex(0, iH + 1, w)] = 0.f;
		x[CDynamicWind::TransformCellCoordToIndex(iW + 1, 0, w)] = 0.f;
		x[CDynamicWind::TransformCellCoordToIndex(iW + 1, iH + 1, w)] = 0.f;
	}
	else
	{
		for (INT i = 1; i < iW; i++)
		{
			FLOAT temp = x[CDynamicWind::TransformCellCoordToIndex(i, 1, w)];
			x[CDynamicWind::TransformCellCoordToIndex(i, 0, w)] = b == 2 ? -temp : temp;
			temp = x[CDynamicWind::TransformCellCoordToIndex(i, iH, w)];
			x[CDynamicWind::TransformCellCoordToIndex(i, iH + 1, w)] = b == 2 ? -temp : temp;
		}
		for (INT j = 1; j < iH; j++)
		{
			FLOAT temp = x[CDynamicWind::TransformCellCoordToIndex(1, j, w)];
			x[CDynamicWind::TransformCellCoordToIndex(0, j, w)] = b == 1 ? -temp : temp;
			temp = x[CDynamicWind::TransformCellCoordToIndex(iW, j, w)];
			x[CDynamicWind::TransformCellCoordToIndex(iW + 1, j, w)] = b == 1 ? -temp : temp;
		}
		x[CDynamicWind::TransformCellCoordToIndex(0, 0, w)] = 0.5f * (x[CDynamicWind::TransformCellCoordToIndex(1, 0, w)] + x[CDynamicWind::TransformCellCoordToIndex(0, 1, w)]);
		x[CDynamicWind::TransformCellCoordToIndex(0, iH + 1, w)] = 0.5f * (x[CDynamicWind::TransformCellCoordToIndex(1, iH + 1, w)] + x[CDynamicWind::TransformCellCoordToIndex(0, iH, w)]);
		x[CDynamicWind::TransformCellCoordToIndex(iW + 1, 0, w)] = 0.5f * (x[CDynamicWind::TransformCellCoordToIndex(iW, 0, w)] + x[CDynamicWind::TransformCellCoordToIndex(iW + 1, 1, w)]);
		x[CDynamicWind::TransformCellCoordToIndex(iW + 1, iH + 1, w)] = 0.5f * (x[CDynamicWind::TransformCellCoordToIndex(iW, iH + 1, w)] + x[CDynamicWind::TransformCellCoordToIndex(iW + 1, iH, w)]);
	}
}
void CDynamicWind::DiffusionSolver(const UINT& iter, const UINT& w, const UINT& h, const INT& b, FLOAT* x, FLOAT* x0, const FLOAT& diffuse, const FLOAT& dt)
{
	const INT iW = static_cast<INT>(w);
	const INT iH = static_cast<INT>(h);
	FLOAT a = dt * diffuse * w * h;
	for (UINT k = 0u; k < iter; k++)
	{
		for (INT i = 1; i <= iW; i++)
		{
			for (INT j = 1; j <= iH; j++)
			{
				INT targetCoord = CDynamicWind::TransformCellCoordToIndex(i, j, w);
				x[targetCoord] = (x0[targetCoord] + a * (x[CDynamicWind::TransformCellCoordToIndex(i, j - 1, w)] + x[CDynamicWind::TransformCellCoordToIndex(i, j + 1, w)] + x[CDynamicWind::TransformCellCoordToIndex(i - 1, j, w)] + x[CDynamicWind::TransformCellCoordToIndex(i + 1, j, w)])) / (1.f + 4.f * a);
			}
		}
		CDynamicWind::SetBoundary(w, h, b, x);
	}
}
void CDynamicWind::AdvectionSolver(const UINT& w, const UINT& h, const INT& b, FLOAT* d, FLOAT* d0, FLOAT* u, FLOAT* v, const FLOAT& dt)
{
	const INT iW = static_cast<INT>(w);
	const INT iH = static_cast<INT>(h);
	const FLOAT fW = static_cast<FLOAT>(w);
	const FLOAT fH = static_cast<FLOAT>(h);
	FLOAT dtu0 = dt * w;
	FLOAT dtv0 = dt * h;
	for (INT i = 1; i <= iW; i++)
	{
		for (INT j = 1; j <= iH; j++)
		{
			FLOAT x = i - dtu0 * u[CDynamicWind::TransformCellCoordToIndex(i, j, w)];
			FLOAT y = j - dtv0 * v[CDynamicWind::TransformCellCoordToIndex(i, j, w)];
			if (x < 0.5f) { x = 0.5f; }
			if (x > (fW + 0.5f)) { x = fW + 0.5f; }
			if (y < 0.5f) { y = 0.5f; }
			if (y > (fH + 0.5f)) { y = fH + 0.5f; }
			INT i0 = static_cast<INT>(x); INT i1 = i0 + 1;
			INT j0 = static_cast<INT>(y); INT j1 = j0 + 1;
			FLOAT s1 = x - static_cast<FLOAT>(i0); FLOAT s0 = 1.f - s1;
			FLOAT t1 = y - static_cast<FLOAT>(j0); FLOAT t0 = 1.f - t1;
			d[CDynamicWind::TransformCellCoordToIndex(i, j, w)] =
				s0 * (t0 * d0[CDynamicWind::TransformCellCoordToIndex(i0, j0, w)] + t1 * d0[CDynamicWind::TransformCellCoordToIndex(i0, j1, w)]) +
				s1 * (t0 * d0[CDynamicWind::TransformCellCoordToIndex(i1, j0, w)] + t1 * d0[CDynamicWind::TransformCellCoordToIndex(i1, j1, w)]);
		}
	}
	CDynamicWind::SetBoundary(w, h, b, d);
}
void CDynamicWind::ProjectionSolver(const UINT& iter, const UINT& w, const UINT& h, FLOAT* u, FLOAT* v, FLOAT* p, FLOAT* div)
{
	const INT iW = static_cast<INT>(w);
	const INT iH = static_cast<INT>(h);
	const FLOAT fW = static_cast<FLOAT>(w);
	const FLOAT fH = static_cast<FLOAT>(h);
	FLOAT hW = 1.f / fW;
	FLOAT hH = 1.f / fH;
	for (INT i = 1; i <= iW; i++)
	{
		for (INT j = 1; j <= iH; j++)
		{
			div[CDynamicWind::TransformCellCoordToIndex(i, j, w)] = -0.5f * (
				hW * (u[CDynamicWind::TransformCellCoordToIndex(i + 1, j, w)] - v[CDynamicWind::TransformCellCoordToIndex(i - 1, j, w)]) +
				hH * (v[CDynamicWind::TransformCellCoordToIndex(i, j + 1, w)] - v[CDynamicWind::TransformCellCoordToIndex(i, j - 1, w)]));
			p[CDynamicWind::TransformCellCoordToIndex(i, j, w)] = 0.f;
		}
	}
	CDynamicWind::SetBoundary(w, h, 0, div);
	CDynamicWind::SetBoundary(w, h, 0, p);
	for (UINT k = 0u; k < iter; k++)
	{
		for (INT i = 1; i <= iW; i++)
		{
			for (INT j = 1; j <= iH; j++)
			{
				p[CDynamicWind::TransformCellCoordToIndex(i, j, w)] = (div[CDynamicWind::TransformCellCoordToIndex(i, j, w)] +
					p[CDynamicWind::TransformCellCoordToIndex(i - 1, j, w)] + p[CDynamicWind::TransformCellCoordToIndex(i + 1, j, w)] +
					p[CDynamicWind::TransformCellCoordToIndex(i, j - 1, w)] + p[CDynamicWind::TransformCellCoordToIndex(i, j + 1, w)]) / 4.f;
			}
		}
		CDynamicWind::SetBoundary(w, h, 0, p);
	}

	for (INT i = 1; i <= iW; i++)
	{
		for (INT j = 1; j <= iH; j++)
		{
			u[CDynamicWind::TransformCellCoordToIndex(i, j, w)] -= 0.5f * (p[CDynamicWind::TransformCellCoordToIndex(i + 1, j, w)] - p[CDynamicWind::TransformCellCoordToIndex(i - 1, j, w)]) / hW;
			v[CDynamicWind::TransformCellCoordToIndex(i, j, w)] -= 0.5f * (p[CDynamicWind::TransformCellCoordToIndex(i, j + 1, w)] - p[CDynamicWind::TransformCellCoordToIndex(i, j - 1, w)]) / hH;
		}
	}
	CDynamicWind::SetBoundary(w, h, 1, u);
	CDynamicWind::SetBoundary(w, h, 2, v);
}
void CDynamicWind::DensityStep(const UINT& iter, const UINT& w, const UINT& h, FLOAT* x, FLOAT* x0, FLOAT* u, FLOAT* v, const FLOAT& diffuse, const FLOAT& dt)
{
	CDynamicWind::AddSource(w, h, x, x0, dt);
	CDynamicWind::Swap(x0, x);
	CDynamicWind::DiffusionSolver(iter, w, h, 0, x, x0, diffuse, dt);
	CDynamicWind::Swap(x0, x);
	CDynamicWind::AdvectionSolver(w, h, 0, x, x0, u, v, dt);
}
void CDynamicWind::VelocityStep(const UINT& iter, const UINT& w, const UINT& h, FLOAT* u, FLOAT* v, FLOAT* u0, FLOAT* v0, const FLOAT& viscosity, const FLOAT& dt)
{
	CDynamicWind::AddSource(w, h, u, u0, dt);
	CDynamicWind::AddSource(w, h, v, v0, dt);
	CDynamicWind::Swap(u0, u);
	CDynamicWind::DiffusionSolver(iter, w, h, 1, u, u0, viscosity, dt);
	CDynamicWind::Swap(v0, v);
	CDynamicWind::DiffusionSolver(iter, w, h, 2, v, v0, viscosity, dt);
	CDynamicWind::ProjectionSolver(iter, w, h, u, v, u0, v0);
	CDynamicWind::Swap(u0, u);
	CDynamicWind::Swap(v0, v);
	CDynamicWind::AdvectionSolver(w, h, 1, u, u0, u0, v0, dt);
	CDynamicWind::AdvectionSolver(w, h, 2, v, v0, u0, v0, dt);
	CDynamicWind::ProjectionSolver(iter, w, h, u, v, u0, v0);
}
#endif