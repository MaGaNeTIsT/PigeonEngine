#include "../../Headers/Object/CDebugScreen.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Object/CPolygon2D.h"

BOOL Linear_Interpolation(const INT& x0, const INT& y0, const INT& x1, const INT& y1, const INT& t, INT& Phi)
{
	if (t < x0 || t > x1)
		return false;
	Phi = (INT)(((FLOAT)(t - x1)) / ((FLOAT)(x0 - x1)) * (FLOAT)y0 + ((FLOAT)(t - x0)) / ((FLOAT)(x1 - x0)) * (FLOAT)y1);
	return true;
}

void DebugScreen::Init()
{
	ID3D11ShaderResourceView* srv[DEBUGPOLYGON_COUNT] = {
		CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_WORLDPOSITION),
		CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_WORLDNORMAL),
		CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_DIFFUSE),
		CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_SPECULAR),
		CRenderDevice::GetDeferredBuffer()->GetDepthStencilShaderResourceView(DeferredBuffer::DEPTHSTENCILBUFFER_LIGHT),
		CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_WORLDPOSITION),
		CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_DIFFUSE)
	};
	SetShaderResourceView(DEBUGPOLYGON_COUNT, srv);

	INT x = 0, y = 0, l = SCREEN_HEIGHT / DEBUGPOLYGON_COUNT - 1;
	for (INT i = 0; i < DEBUGPOLYGON_COUNT; ++i)
	{
		if (i == 5 || i == 6)
		{
			m_Polygon[i] = new DebugQuadAlpha();
			Linear_Interpolation(0, 0, DEBUGPOLYGON_COUNT, SCREEN_HEIGHT, i, y);
			m_Polygon[i]->SetScreenPosition(XMFLOAT4(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + l), static_cast<FLOAT>(y + l)));
			m_Polygon[i]->Init();
		}
		else
		{
			m_Polygon[i] = new CPolygon2D();
			Linear_Interpolation(0, 0, DEBUGPOLYGON_COUNT, SCREEN_HEIGHT, i, y);
			m_Polygon[i]->SetScreenPosition(XMFLOAT4(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + l), static_cast<FLOAT>(y + l)));
			m_Polygon[i]->Init();
		}
	}
}

void DebugScreen::Uninit()
{
	for (INT i = 0; i < DEBUGPOLYGON_COUNT; ++i)
	{
		m_Polygon[i]->Uninit();
		delete m_Polygon[i];
	}
}

void DebugScreen::Update()
{
	for (INT i = 0; i < DEBUGPOLYGON_COUNT; ++i)
		m_Polygon[i]->Update();
}

void DebugScreen::Draw()
{
	for (INT i = 0; i < DEBUGPOLYGON_COUNT; ++i)
	{
		CRenderDevice::SetTexture(m_SRV[i], 6);
		m_Polygon[i]->Draw();
	}
}

void DebugScreen::SetShaderResourceView(INT num, ID3D11ShaderResourceView** srv)
{
	if (num < 0)
		return;
	INT n = num > DEBUGPOLYGON_COUNT ? DEBUGPOLYGON_COUNT : num;
	for (INT i = 0; i < n; ++i)
	{
		m_SRV[i] = srv[i];
	}
}