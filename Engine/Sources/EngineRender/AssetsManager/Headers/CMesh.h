#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CStructCommon.h"

class CMesh
{
public:
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() { return (this->m_VertexBuffer); }
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() { return (this->m_IndexBuffer); }
	const std::vector<CustomStruct::CSubMeshInfo>& GetSubMeshInfo() const { return (this->m_SubMeshInfo); }
	UINT			GetVertexStride() { return (this->m_VertexStride); }
	UINT			GetVertexCount() { return (this->m_VertexCount); }
	UINT			GetIndexCount() { return (this->m_IndexCount); }
public:
	CMesh(const std::string& name, const std::vector<CustomStruct::CVertex3D>& vdata, const std::vector<UINT>& idata, std::vector<CustomStruct::CSubMeshInfo> submeshInfo, Microsoft::WRL::ComPtr<ID3D11Buffer> vbuffer, Microsoft::WRL::ComPtr<ID3D11Buffer> ibuffer);
	virtual ~CMesh();
private:
	std::string								m_MeshPath;
	std::vector<CustomStruct::CSubMeshInfo> m_SubMeshInfo;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	m_IndexBuffer;
	std::vector<CustomStruct::CVertex3D>	m_VertexData;
	std::vector<UINT>						m_IndexData;
	UINT									m_VertexStride;
	UINT									m_VertexCount;
	UINT									m_IndexCount;
};