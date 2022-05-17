#pragma once

#include "../Game/CGameObject.h"

class CShader;

class CRayMarching : public CGameObject
{
private:
	ID3D11Buffer*				m_VertexBuffer			= NULL;
	ID3D11Buffer*				m_IndexBuffer			= NULL;
	ID3D11RasterizerState*		m_RenderStateNoCull		= NULL;
	ID3D11RasterizerState*		m_RenderStateCull		= NULL;

	CShader*					m_Shader;

	FLOAT						m_Time;

	FLOAT						m_LightRotation;
	XMFLOAT3					m_LightDirection;
public:
	void	Init();
	void	Uninit();
	void	Update();
	void	Draw();
};