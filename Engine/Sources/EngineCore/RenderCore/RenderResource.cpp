#include "RenderResource.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RVertexShaderResource, RRenderResourceInterface>();
		RegisterClassType<RPixelShaderResource, RRenderResourceInterface>();
		RegisterClassType<RComputeShaderResource, RRenderResourceInterface>();
		RegisterClassType<RStructuredBuffer, RRenderResourceInterface>();
		RegisterClassType<RRenderTexture2D, RRenderResourceInterface>();
		RegisterClassType<RRenderTexture3D, RRenderResourceInterface>();
		RegisterClassType<RTexture2DResource, RRenderResourceInterface>();
		RegisterClassType<RTextureCubeResource, RRenderResourceInterface>();
		RegisterClassType<RBufferResource, RRenderResourceInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

};