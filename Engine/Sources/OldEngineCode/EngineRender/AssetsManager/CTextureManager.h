#pragma once

class CTextureManager
{
public:
	static const CTextureManager* const GetTextureManager() { return m_TextureManager; }
	static void					ShutDown();
public:
	static void					ClearTexture2DData();
	static void					ClearTextureCubeData();
	static class CTexture2D*	LoadTexture2D(const std::string& name, const BOOL& isSRGB = TRUE);
	static class CTextureCube*	LoadTextureCubeCombine(const std::vector<std::string>& name, const BOOL& isSRGB = TRUE);
private:
	static class CTexture2D*	LoadTGATexture2D(const std::string& name, const BOOL& isSRGB);
	static BOOL					LoadTGAData(const std::string& name, std::vector<TBYTE>& outputData, UINT& outputWidth, UINT& outputHeight, const UINT& offset = 0u);
private:
	static void					AddTexture2DData(const std::string& name, class CTexture2D* ptrData);
	static class CTexture2D*	FindTexture2DData(const std::string& name);
	static void					AddTextureCubeData(const std::string& name, class CTextureCube* ptrData);
	static class CTextureCube*	FindTextureCubeData(const std::string& name);
private:
	std::map<std::string, class CTexture2D*> m_Texture2DData;
	std::map<std::string, class CTextureCube*> m_TextureCubeData;
private:
	CTextureManager() {}
	CTextureManager(const CTextureManager&) {}
	~CTextureManager() {}
private:
	static CTextureManager* m_TextureManager;
};