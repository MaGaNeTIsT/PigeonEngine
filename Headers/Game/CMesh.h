#pragma once

#include "../../Entry/MyMain.h"
#include "../Base/CStructCommon.h"

class CMesh
{
public:
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() { return (this->m_VertexBuffer); }
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() { return (this->m_IndexBuffer); }
	UINT			GetVertexCount() { return this->m_VertexCount; }
	UINT			GetIndexCount() { return this->m_IndexCount; }
public:
	CMesh(const std::string& name, const std::vector<CustomStruct::CVertex3D>& vdata, const std::vector<UINT>& idata, Microsoft::WRL::ComPtr<ID3D11Buffer> vbuffer, Microsoft::WRL::ComPtr<ID3D11Buffer> ibuffer);
	virtual ~CMesh();
private:
	std::string								m_MeshPath;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	m_IndexBuffer;
	std::vector<CustomStruct::CVertex3D>	m_VertexData;
	std::vector<UINT>						m_IndexData;
	UINT									m_VertexCount;
	UINT									m_IndexCount;
};