#include "../Headers/CGPUQuery.h"

CGPUQuery::CGPUQuery(const std::string& name, const CustomStruct::CRenderQueryDesc& desc, const Microsoft::WRL::ComPtr<ID3D11Query>& query)
{
	this->m_Name		= name;
	this->m_QueryDesc	= desc;
	this->m_Query		= query;
}
CGPUQuery::CGPUQuery(const CGPUQuery& input)
{
	this->m_Name		= input.m_Name;
	this->m_QueryDesc	= input.m_QueryDesc;
	this->m_Query		= input.m_Query;
}
CGPUQuery::~CGPUQuery()
{
}