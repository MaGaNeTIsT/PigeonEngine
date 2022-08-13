#include "../../Headers/Game/CMesh.h"

CMesh::CMesh(const std::string& name, const std::vector<CustomStruct::CVertex3D>& vdata, const std::vector<UINT>& idata, std::vector<CustomStruct::CSubMeshInfo> submeshInfo, Microsoft::WRL::ComPtr<ID3D11Buffer> vbuffer, Microsoft::WRL::ComPtr<ID3D11Buffer> ibuffer)
{
	this->m_MeshPath		= name;
	this->m_SubMeshInfo		= submeshInfo;
	this->m_VertexData		= vdata;
	this->m_IndexData		= idata;
	this->m_VertexBuffer	= vbuffer;
	this->m_IndexBuffer		= ibuffer;
	this->m_VertexCount		= static_cast<UINT>(vdata.size());
	this->m_IndexCount		= static_cast<UINT>(idata.size());
}
CMesh::~CMesh()
{
}