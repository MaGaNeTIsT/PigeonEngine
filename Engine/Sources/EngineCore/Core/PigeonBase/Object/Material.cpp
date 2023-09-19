#include "Material.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PMaterial, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PMaterial::PMaterial()
		: VertexShader(nullptr), PixelShader(nullptr)
	{
	}
	PMaterial::PMaterial(const PMaterial& Other)
		: VertexShader(Other.VertexShader), PixelShader(Other.PixelShader)
	{
	}
	PMaterial::~PMaterial()
	{
	}
	PMaterial& PMaterial::operator=(const PMaterial& Other)
	{
		MaterialParameter.ClearParameter();
		VertexShader				= Other.VertexShader;
		PixelShader					= Other.PixelShader;
		return (*this);
	}

};