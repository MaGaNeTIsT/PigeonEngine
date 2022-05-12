#pragma once

#include "../Object/CGameObject.h"

class CCamera : public CGameObject
{
private:
	static CCamera* m_Instance;

	RECT		m_Viewport;
	XMFLOAT4X4	m_ViewMatrix;
	XMFLOAT4X4	m_InvViewMatrix;
	XMFLOAT4X4	m_ProjectionMatrix;
public:
	CCamera() { m_Instance = this; }
	static CCamera* GetInstance() { return m_Instance; }

	void	Init();
	void	Uninit();
	void	Update();
	void	Draw();
	void	DrawShadow();

	XMFLOAT4X4	GetViewMatrix() { return m_ViewMatrix; }
	XMFLOAT4X4	GetInvViewMatrix() { return m_InvViewMatrix; }
	XMFLOAT4X4	GetProjectionMatrix() { return m_ProjectionMatrix; }
};