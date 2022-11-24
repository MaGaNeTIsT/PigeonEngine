#include "../Headers/CMaterialBase.h"
#include "../../AssetsManager/Headers/CShader.h"

CMaterialBase::CMaterialBase(const std::string& name, MaterialType materialType, const UINT& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName)
{
	this->m_Name				= name;
	this->m_MaterialType		= materialType;
	this->m_ConstantSize		= constantSize;
	if (inputLayoutNum > 0)
	{
		this->m_InputLayout.resize(inputLayoutNum);
		for (UINT i = 0u; i < inputLayoutNum; i++)
		{
			this->m_InputLayout[i] = inputLayout[i];
		}
	}
	this->m_VertexShaderName	= vertexShaderName;
	this->m_PixelShaderName		= pixelShaderName;
	this->m_VertexShader		= NULL;
	this->m_PixelShader			= NULL;
}
CMaterialBase::~CMaterialBase()
{
	if (this->m_VertexShader != NULL)
	{
		delete (this->m_VertexShader);
		this->m_VertexShader = NULL;
	}
	if (this->m_PixelShader != NULL)
	{
		delete (this->m_PixelShader);
		this->m_PixelShader = NULL;
	}
}
const CVertexShader* CMaterialBase::GetVertexShader()const
{
	return (this->m_VertexShader);
}
const CPixelShader* CMaterialBase::GetPixelShader()const
{
	return (this->m_PixelShader);
}
const CMaterialBase::MaterialType& CMaterialBase::GetMaterialType()const
{
	return (this->m_MaterialType);
}
const UINT& CMaterialBase::GetConstantSize()const
{
	return (this->m_ConstantSize);
}
const std::string& CMaterialBase::GetName()const
{
	return (this->m_Name);
}
const std::string& CMaterialBase::GetVertexShaderName()const
{
	return (this->m_VertexShaderName);
}
const std::string& CMaterialBase::GetPixelShaderName()const
{
	return (this->m_PixelShaderName);
}
BOOL CMaterialBase::HasConstantData()const
{
	return (this->m_ConstantSize > 0u && this->GetConstantData() != nullptr);
}
BOOL CMaterialBase::HasInputLayout()const
{
	return (this->m_InputLayout.size() > 0);
}
BOOL CMaterialBase::HasVertexShader()const
{
	return (this->m_VertexShader != NULL);
}
BOOL CMaterialBase::HasPixelShader()const
{
	return (this->m_ConstantSize != NULL);
}