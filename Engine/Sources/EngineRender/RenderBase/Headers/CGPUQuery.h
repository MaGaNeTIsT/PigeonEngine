#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "./CStructCommon.h"

class CGPUQuery
{
public:
	Microsoft::WRL::ComPtr<ID3D11Query> GetQuery() { return (this->m_Query); }
public:
	CGPUQuery(const std::string& name, const CustomStruct::CRenderQueryDesc& desc, const Microsoft::WRL::ComPtr<ID3D11Query>& query);
	CGPUQuery(const CGPUQuery& input);
	virtual ~CGPUQuery();
protected:
    std::string							m_Name;
    CustomStruct::CRenderQueryDesc		m_QueryDesc;
	Microsoft::WRL::ComPtr<ID3D11Query>	m_Query;
};