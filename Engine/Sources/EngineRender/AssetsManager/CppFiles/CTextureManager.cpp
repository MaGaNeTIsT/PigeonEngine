#include "../../../../../Entry/EngineMain.h"
#include "../Headers/CTextureManager.h"
#include "../Headers/CTextureType.h"
#include "../../RenderBase/Headers/CRenderStructCommon.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

CTextureManager* CTextureManager::m_TextureManager = new CTextureManager();
void CTextureManager::ShutDown()
{
	CTextureManager::ClearTexture2DData();
	delete m_TextureManager;
}
void CTextureManager::ClearTexture2DData()
{
	if (m_TextureManager->m_Texture2DData.size() > 0)
	{
		for (const auto& data : (m_TextureManager->m_Texture2DData))
		{
			if ((data.second) != NULL)
			{
				delete (data.second);
			}
		}
		m_TextureManager->m_Texture2DData.clear();
	}
}
void CTextureManager::ClearTextureCubeData()
{
	if (m_TextureManager->m_TextureCubeData.size() > 0)
	{
		for (const auto& data : (m_TextureManager->m_TextureCubeData))
		{
			if ((data.second) != NULL)
			{
				delete (data.second);
			}
		}
		m_TextureManager->m_TextureCubeData.clear();
	}
}
CTexture2D* CTextureManager::LoadTexture2D(const std::string& name, const BOOL& isSRGB)
{
	const static std::string _tgaName = "tga";

	CTexture2D* resultTexture = CTextureManager::FindTexture2DData(name);
	if (resultTexture != NULL)
	{
		return resultTexture;
	}

	{
		size_t pos = name.find_last_of('.');
		if (pos == std::string::npos)
		{
			return NULL;
		}
		std::string typeName = name.substr(pos + 1, name.length());
		if (typeName == _tgaName)
		{
			resultTexture = (CTextureManager::LoadTGATexture2D(name, isSRGB));
		}
		else
		{
			return NULL;
		}
		if (resultTexture == NULL)
		{
			return NULL;
		}
		CTextureManager::AddTexture2DData(name, resultTexture);
	}
	return resultTexture;
}
CTextureCube* CTextureManager::LoadTextureCubeCombine(const std::vector<std::string>& name, const BOOL& isSRGB)
{
	const static std::string _tgaName = "tga";

	if (name.size() != 6u)
	{
		return NULL;
	}

	CTextureCube* resultTexture = NULL;
	std::string combineName = name[0u];
	{
		for (UINT i = 1u; i < 6u; i++)
		{
			combineName += name[i];
		}
		resultTexture = CTextureManager::FindTextureCubeData(combineName);
		if (resultTexture != NULL)
		{
			return resultTexture;
		}
	}

	{
		std::vector<TBYTE> cubeData;
		UINT cubeWidth, cubeHeight; UINT offset = 0u;
		for (UINT i = 0u; i < 6u; i++)
		{
			size_t pos = name[i].find_last_of('.');
			if (pos == std::string::npos)
			{
				return NULL;
			}
			std::string typeName = name[i].substr(pos + 1, name[i].length());

			if (typeName == _tgaName)
			{
				UINT tempCubeWidth, tempCubeHeight;
				if (!CTextureManager::LoadTGAData(name[i], cubeData, tempCubeWidth, tempCubeHeight, offset))
				{
					return NULL;
				}
				if (i != 0u && (cubeWidth != cubeHeight || tempCubeWidth != tempCubeHeight || cubeWidth != tempCubeWidth || cubeHeight != tempCubeHeight))
				{
					return NULL;
				}
				cubeWidth = tempCubeWidth;
				cubeHeight = tempCubeHeight;
				offset += cubeWidth * cubeHeight * 4u;
			}
			else
			{
				return NULL;
			}
		}

		{
			CRenderDevice::TextureCubeViewInfo tempTextureCube;
			{
				CustomStruct::CRenderFormat sourceFormat = CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM;
				if (isSRGB)
				{
					sourceFormat = CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM_SRGB;
				}
				TBYTE* ptrCubeData = cubeData.data();
				CustomStruct::CRenderSubresourceData subResourceDatas[6u];
				for (UINT i = 0u; i < 6u; i++)
				{
					subResourceDatas[i].pSysMem = static_cast<const void*>(&(ptrCubeData[i * cubeWidth * cubeHeight * 4u]));
					subResourceDatas[i].SysMemPitch = cubeWidth * 4u;
					subResourceDatas[i].SysMemSlicePitch = cubeWidth * cubeHeight * 4u;
				}
				if (!CRenderDevice::CreateTextureCube(
					tempTextureCube,
					CustomStruct::CRenderTextureDesc(
						cubeWidth,
						cubeHeight,
						CustomStruct::CRenderBindFlag::BIND_SHADER_RESOURCE,
						sourceFormat,
						&sourceFormat),
					subResourceDatas))
				{
					return NULL;
				}
			}
			resultTexture = new CTextureCubeCombine(name.data(), tempTextureCube);
			CTextureManager::AddTextureCubeData(combineName, resultTexture);
		}
	}
	return resultTexture;
}
CTexture2D* CTextureManager::LoadTGATexture2D(const std::string& name, const BOOL& isSRGB)
{
	std::vector<TBYTE> textureData; UINT textureWidth, textureHeight;
	if (!LoadTGAData(name, textureData, textureWidth, textureHeight))
	{
		return NULL;
	}
	CRenderDevice::Texture2DViewInfo tempTexture2D;
	{
		CustomStruct::CRenderFormat sourceFormat = CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM;
		if (isSRGB)
		{
			sourceFormat = CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		if (!CRenderDevice::CreateTexture2D(
			tempTexture2D,
			CustomStruct::CRenderTextureDesc(
				textureWidth,
				textureHeight,
				CustomStruct::CRenderBindFlag::BIND_SHADER_RESOURCE,
				sourceFormat,
				&sourceFormat),
			&CustomStruct::CRenderSubresourceData(
				static_cast<const void*>(textureData.data()),
				textureWidth * 4u,
				textureWidth * textureHeight * 4u)))
		{
			return NULL;
		}
	}
	CTexture2D* resultTexture2D = new CTexture2D(name, tempTexture2D);
	return resultTexture2D;
}
BOOL CTextureManager::LoadTGAData(const std::string& name, std::vector<TBYTE>& outputData, UINT& outputWidth, UINT& outputHeight, const UINT& offset)
{
	TBYTE	header[18];
	TBYTE*	rawData;
	TBYTE	depth;
	UINT	width, height, bpp, size;
	FILE*	file;
	fopen_s(&file, name.c_str(), "rb");
	if (file == NULL)
	{
		return FALSE;
	}
	fread_s(header, sizeof(header), sizeof(header), 1u, file);
	width = header[13] * 256 + header[12];
	height = header[15] * 256 + header[14];
	depth = header[16];
	if (depth == 32)
	{
		bpp = 4u;
	}
	else if (depth == 24)
	{
		bpp = 3u;
	}
	else
	{
		bpp = 0u;
	}
	if (!(bpp == 4u || bpp == 3u))
	{
		return FALSE;
	}
	size = width * height * bpp;
	rawData = new TBYTE[size];
	fread_s(rawData, size, size, 1u, file);

	if (outputData.size() != (offset + (width * height * 4u)))
	{
		outputData.resize(offset + (width * height * 4u));
	}
	outputWidth = width;
	outputHeight = height;

	// Copy from raw to output.
	// R<->B
	if (bpp == 3u)
	{
		for (UINT y = 0u; y < height; y++)
		{
			for (UINT x = 0u; x < width; x++)
			{
				outputData[offset + (y * width + x) * 4u + 0u] = rawData[(y * width + x) * 3u + 2u];
				outputData[offset + (y * width + x) * 4u + 1u] = rawData[(y * width + x) * 3u + 1u];
				outputData[offset + (y * width + x) * 4u + 2u] = rawData[(y * width + x) * 3u + 0u];
				outputData[offset + (y * width + x) * 4u + 3u] = 0xffu;
			}
		}
	}
	else if (bpp == 4u)
	{
		for (UINT y = 0u; y < height; y++)
		{
			for (UINT x = 0u; x < width; x++)
			{
				outputData[offset + (y * width + x) * 4u + 0u] = rawData[(y * width + x) * 4u + 2u];
				outputData[offset + (y * width + x) * 4u + 1u] = rawData[(y * width + x) * 4u + 1u];
				outputData[offset + (y * width + x) * 4u + 2u] = rawData[(y * width + x) * 4u + 0u];
				outputData[offset + (y * width + x) * 4u + 3u] = rawData[(y * width + x) * 4u + 3u];
			}
		}
	}
	delete[]rawData;
	fclose(file);
	return TRUE;
}
void CTextureManager::AddTexture2DData(const std::string& name, CTexture2D* ptrData)
{
	m_TextureManager->m_Texture2DData[name] = ptrData;
}
CTexture2D* CTextureManager::FindTexture2DData(const std::string& name)
{
	std::map<std::string, CTexture2D*>::iterator element = m_TextureManager->m_Texture2DData.find(name);
	if (element == m_TextureManager->m_Texture2DData.end())
		return NULL;
	return (element->second);
}
void CTextureManager::AddTextureCubeData(const std::string& name, CTextureCube* ptrData)
{
	m_TextureManager->m_TextureCubeData[name] = ptrData;
}
CTextureCube* CTextureManager::FindTextureCubeData(const std::string& name)
{
	std::map<std::string, CTextureCube*>::iterator element = m_TextureManager->m_TextureCubeData.find(name);
	if (element == m_TextureManager->m_TextureCubeData.end())
		return NULL;
	return (element->second);
}