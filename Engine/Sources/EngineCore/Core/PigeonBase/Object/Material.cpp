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
		: StaticMaterialParameter(Other.StaticMaterialParameter), DynamicMaterialParameter(Other.DynamicMaterialParameter)
		, VertexShader(Other.VertexShader), PixelShader(Other.PixelShader)
	{

	}
	PMaterial::~PMaterial()
	{
		StaticMaterialParameter.ReleaseParameter();
		DynamicMaterialParameter.ReleaseParameter();
	}
	PMaterial& PMaterial::operator=(const PMaterial& Other)
	{
		StaticMaterialParameter.ReleaseParameter();
		DynamicMaterialParameter.ReleaseParameter();
		StaticMaterialParameter		= Other.StaticMaterialParameter;
		DynamicMaterialParameter	= Other.DynamicMaterialParameter;
		VertexShader				= Other.VertexShader;
		PixelShader					= Other.PixelShader;
		return (*this);
	}

};