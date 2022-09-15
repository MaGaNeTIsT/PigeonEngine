#pragma once

class CTextureManager
{
public:
	static const CTextureManager* const GetTextureManager() { return m_TextureManager; }
	static void					Uninit();
public:
	static void					ClearTexture2DData();
	static class CTexture2D*	LoadTexture2D(const std::string& name);
private:
	static class CTexture2D*	LoadTGATexture2D(const std::string& name);
private:
	static void					AddTexture2DData(const std::string& name, class CTexture2D* ptrData);
	static class CTexture2D*	FindTexture2DData(const std::string& name);
private:
	std::map<std::string, class CTexture2D*> m_Texture2DData;
private:
	CTextureManager() {}
	CTextureManager(const CTextureManager&) {}
	~CTextureManager() {}
private:
	static CTextureManager* m_TextureManager;
};