/*
#pragma once

#include "../../RenderBase/Headers/CRenderDevice.h"

class CTextureBase
{
public:
	const std::string& GetName()const { return (this->m_Name); }
	virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetShaderResourceView()const = 0;
public:
	CTextureBase(const std::string& name);
	CTextureBase(const CTextureBase& v);
	virtual ~CTextureBase() {}
protected:
	std::string m_Name;
};

class CTexture2D : public CTextureBase
{
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetShaderResourceView()const override;
public:
	CTexture2D(const std::string& name, const CRenderDevice::Texture2DViewInfo& v);
	CTexture2D(const CTexture2D& v);
	virtual ~CTexture2D() {}
protected:
	CRenderDevice::Texture2DViewInfo m_Texture2D;
};
*/

/*
* // Create from texutre file that has texture array with 6 faces.
* // Face[0] : +X Right face.
* // Face[1] : -X Left face.
* // Face[2] : +Y Top face.
* // Face[3] : -Y Bottom face.
* // Face[4] : +Z Forward face.
* // Face[5] : -Z Back face.
*/
//class CTextureCube : public CTextureBase
//{
//public:
//	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetShaderResourceView()const override;
//public:
//	CTextureCube(const std::string& name, const CRenderDevice::TextureCubeViewInfo& v);
//	CTextureCube(const CTextureCube& v);
//	virtual ~CTextureCube() {}
//protected:
//	CRenderDevice::TextureCubeViewInfo m_TextureCube;
//};

/*
* // Create from different texutre file into texture cube.
* // Face[0] : +X Right face.
* // Face[1] : -X Left face.
* // Face[2] : +Y Top face.
* // Face[3] : -Y Bottom face.
* // Face[4] : +Z Forward face.
* // Face[5] : -Z Back face.
*/
//class CTextureCubeCombine : public CTextureCube
//{
//public:
//	CTextureCubeCombine(const std::string* nameFace, const CRenderDevice::TextureCubeViewInfo& v);
//	CTextureCubeCombine(const CTextureCubeCombine& v);
//	virtual ~CTextureCubeCombine() {}
//protected:
//	std::string m_NameFace[5u];
//};