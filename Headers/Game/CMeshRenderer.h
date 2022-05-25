#pragma once

#include "../../Entry/MyMain.h"
#include "../Base/CStructCommon.h"

class CShader;
class CGameObject;

class CMeshRenderer
{
public:
	void	LoadShader();
	BOOL	CreateConstantBuffer(const UINT& sizeData);
	void	UploadConstantBuffer(const void* ptrData);
	void	BindConstantBuffer(const UINT& startSlot);
public:
	void	LoadExtraShader(const std::string& vertexShaderName, const std::string& pixelShaderName);
public:
	void	Draw();
	void	DrawExtra();
public:
	CMeshRenderer(CGameObject* gameObject, const std::string& vertexShaderName, const std::string& pixelShaderName);
	virtual ~CMeshRenderer();
protected:
	std::string			m_VertexShaderName;
	std::string			m_PixelShaderName;
	CShader*			m_Shader;
	CShader*			m_ExtraShader;
	CGameObject*		m_GameObject;
};