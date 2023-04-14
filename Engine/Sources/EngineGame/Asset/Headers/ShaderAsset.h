#pragma once

#include "./BaseAsset.h"
#include "../../../EngineCore/RenderCore/Headers/DeviceD3D11.h"

namespace PigeonEngine
{

	struct EShaderCPUResource
	{
		std::string		ShaderPath;
		void*			ShaderByteCode;
	};

	template<typename TShaderResourceType>
	class ShaderAsset : TBaseAsset<std::string, TShaderResourceType>
	{

	};

};