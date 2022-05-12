#pragma once

#include "./CRenderDevice.h"

struct CONSTANT
{
	XMFLOAT4X4				WorldMatrix;
	XMFLOAT4X4				ViewMatrix;
	XMFLOAT4X4				ProjectionMatrix;

	XMFLOAT4				CameraPosition;
	XMFLOAT4				Parameter;
	XMFLOAT4X4				WorldInvTranspose;
	XMFLOAT4				Parameter2;
	XMFLOAT4X4				LightViewMatrix;
	XMFLOAT4X4				LightProjectionMatrix;
};

class CShader
{
private:
	ID3D11VertexShader*		m_VertexShader;
	ID3D11PixelShader*		m_PixelShader;
	ID3D11InputLayout*		m_VertexLayout;

	ID3D11Buffer*			m_ConstantBuffer;
	CONSTANT				m_Constant;

	ID3D11Buffer*			m_LightBuffer;
	LIGHT					m_Light;

public:
	void Init( const CHAR* VertexShader, const CHAR* PixelShader );
	void Uninit();
	void Bind();

	void SetWorldMatrix(XMFLOAT4X4* WorldMatrix) { m_Constant.WorldMatrix = Transpose(WorldMatrix); }
	void SetViewMatrix(XMFLOAT4X4* ViewMatrix) { m_Constant.ViewMatrix = Transpose(ViewMatrix); }
	void SetProjectionMatrix(XMFLOAT4X4* ProjectionMatrix) { m_Constant.ProjectionMatrix = Transpose(ProjectionMatrix); }
	void SetPrameter(XMFLOAT4 Parameter) { m_Constant.Parameter = Parameter; }
	void SetPrameter2(XMFLOAT4 Parameter) { m_Constant.Parameter2 = Parameter; }
	void SetWorldInvTranspose(XMFLOAT4X4* WorldInvTranspose) { m_Constant.WorldInvTranspose = Transpose(WorldInvTranspose); }
	void SetLightMatrix(XMFLOAT4X4* matView, XMFLOAT4X4* matProj) { m_Constant.LightViewMatrix = Transpose(matView); m_Constant.LightProjectionMatrix = Transpose(matProj); }

	void SetCameraPosition(XMFLOAT3* CameraPosition)
	{
		m_Constant.CameraPosition.x = CameraPosition->x;
		m_Constant.CameraPosition.y = CameraPosition->y;
		m_Constant.CameraPosition.z = CameraPosition->z;
		m_Constant.CameraPosition.w = 1.0f;
	}

	XMFLOAT4X4 Transpose(XMFLOAT4X4* Matrix)
	{
		XMFLOAT4X4 outMatrix;

		outMatrix._11 = Matrix->_11;
		outMatrix._12 = Matrix->_21;
		outMatrix._13 = Matrix->_31;
		outMatrix._14 = Matrix->_41;

		outMatrix._21 = Matrix->_12;
		outMatrix._22 = Matrix->_22;
		outMatrix._23 = Matrix->_32;
		outMatrix._24 = Matrix->_42;

		outMatrix._31 = Matrix->_13;
		outMatrix._32 = Matrix->_23;
		outMatrix._33 = Matrix->_33;
		outMatrix._34 = Matrix->_43;

		outMatrix._41 = Matrix->_14;
		outMatrix._42 = Matrix->_24;
		outMatrix._43 = Matrix->_34;
		outMatrix._44 = Matrix->_44;

		return outMatrix;
	}
};