#pragma once

#include "../Game/CGameObject.h"

class CShader;

class CSky : public CGameObject
{
private:
	static const int	SKY_X			= 32;
	static const int	SKY_Z			= 32;

	ID3D11Buffer*		m_VertexBuffer	= NULL;
	ID3D11Buffer*		m_IndexBuffer	= NULL;

	CustomStruct::CVertex3DData			m_Vertex[SKY_X * SKY_Z];

	CShader*			m_Shader;

	FLOAT				m_Time;

	FLOAT				m_LightRotation;
	XMFLOAT3			m_LightDirection;
public:
	void	Init();
	void	Uninit();
	void	Update();
	void	Draw();
};