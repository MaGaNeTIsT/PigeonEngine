#pragma once

#include "../Game/CGameObject.h"

class CShader;

class CField : public CGameObject
{
private:
	static const int	FIELD_X			= 100;
	static const int	FIELD_Z			= 100;

	ID3D11Buffer*		m_VertexBuffer	= NULL;
	ID3D11Buffer*		m_IndexBuffer	= NULL;

	CustomStruct::CVertex3D			m_Vertex[FIELD_X * FIELD_Z];

	CShader*			m_Shader;

	float				m_LightRotation;
	XMFLOAT3			m_LightDirection;
public:
	void	Init();
	void	Uninit();
	void	Update();
	void	Draw();
	void	DrawDeferred();
	void	DrawShadow();
};