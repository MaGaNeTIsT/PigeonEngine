#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CStructCommon.h"

template<typename IndexType>
class CMesh
{
public:
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() { return (this->m_VertexBuffer); }
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() { return (this->m_IndexBuffer); }
	const std::vector<CustomStruct::CRenderInputLayoutDesc>& GetInputLayoutDesc()const { return (this->m_InputLayoutDesc); }
	const std::vector<CustomStruct::CSubMeshInfo>& GetSubMeshInfo() const { return (this->m_SubMeshInfo); }
	const void*		GetVertexData() const { return (this->m_VertexData); }
	UINT			GetVertexStride() { return (this->m_VertexStride); }
	UINT			GetVertexCount() { return (this->m_VertexCount); }
	UINT			GetIndexCount() { return (this->m_IndexCount); }
	BOOL			IsHaveVertexData() { return (this->m_VertexData != nullptr); }
public:
	CMesh(const std::string& name, const CustomStruct::CRenderInputLayoutDesc* inputLayoutDesc, const UINT& inputLayoutNum, void* vdata, const UINT& vNum, const std::vector<IndexType>& idata, const std::vector<CustomStruct::CSubMeshInfo>& submeshInfo, Microsoft::WRL::ComPtr<ID3D11Buffer> vbuffer, Microsoft::WRL::ComPtr<ID3D11Buffer> ibuffer)
	{
		this->m_MeshPath = name;
		this->m_SubMeshInfo		= submeshInfo;
		this->m_VertexData		= vdata;
		this->m_IndexData		= idata;
		this->m_VertexBuffer	= vbuffer;
		this->m_IndexBuffer		= ibuffer;
		this->m_VertexCount		= vNum;
		this->m_IndexCount		= static_cast<UINT>(idata.size());
		this->m_VertexStride	= 0u;
		this->m_InputLayoutDesc.resize(inputLayoutNum);
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			this->m_InputLayoutDesc[i] = inputLayoutDesc[i];
			this->m_VertexStride += inputLayoutDesc[i].GetSemanticSizeByByte();
		}
	}
	virtual ~CMesh()
	{
		if (this->m_VertexData != nullptr)
		{
			delete[](this->m_VertexData);
			this->m_VertexData = nullptr;
		}
	}
private:
	std::string											m_MeshPath;
	std::vector<CustomStruct::CRenderInputLayoutDesc>	m_InputLayoutDesc;
	std::vector<CustomStruct::CSubMeshInfo>				m_SubMeshInfo;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				m_IndexBuffer;
	const void*											m_VertexData;
	std::vector<IndexType>								m_IndexData;
	UINT												m_VertexStride;
	UINT												m_VertexCount;
	UINT												m_IndexCount;
};