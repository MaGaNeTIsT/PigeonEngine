#pragma once

class CTexture2D
{
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetShaderResourceView() { return (this->m_ShaderResourceView); }
public:
	CTexture2D(const std::string& name, Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	virtual ~CTexture2D();
protected:
	std::string											m_TexturePath;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_Texture2D;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView;
};