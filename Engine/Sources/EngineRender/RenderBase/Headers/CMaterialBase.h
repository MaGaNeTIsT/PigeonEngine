#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineGame/Headers/CObjectManager.h"
#include "./CRenderStructCommon.h"

class CMaterialBase : public CObjectBase
{
public:
	enum MaterialType
	{
		MATERIAL_TYPE_NONE				= 0,
		MATERIAL_TYPE_OPAQUE_DEFERRED	= 1,
		MATERIAL_TYPE_OPAQUE_FORWARD	= 2,
		MATERIAL_TYPE_TRANSPARENT		= 3
	};
public:
	virtual void	Init() = 0;
	virtual void	Bind()const = 0;
#ifdef _DEVELOPMENT_EDITOR
	virtual void	SelectedEditorUpdate() {}
#endif
public:
	virtual const void*			GetConstantData()const = 0;
	const class CVertexShader*	GetVertexShader()const;
	const class CPixelShader*	GetPixelShader()const;
	const MaterialType&			GetMaterialType()const;
	const UINT&					GetConstantSize()const;
	const std::string&			GetName()const;
	const std::string&			GetVertexShaderName()const;
	const std::string&			GetPixelShaderName()const;
public:
	BOOL	HasConstantData()const;
	BOOL	HasInputLayout()const;
	BOOL	HasVertexShader()const;
	BOOL	HasPixelShader()const;
protected:
	std::string											m_Name;
	UINT												m_ConstantSize;
	MaterialType										m_MaterialType;
	std::vector<CustomStruct::CRenderInputLayoutDesc>	m_InputLayout;
	std::string											m_VertexShaderName;
	class CVertexShader*								m_VertexShader;
	std::string											m_PixelShaderName;
	class CPixelShader*									m_PixelShader;
public:
	CMaterialBase(const std::string& name, MaterialType materialType, const UINT& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName);
	virtual ~CMaterialBase();
};