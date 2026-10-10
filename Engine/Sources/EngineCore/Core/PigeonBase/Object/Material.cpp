#include "Material.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PMaterial, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PMaterial::PMaterial()
		: MaterialInstance(nullptr)
	{
	}
	PMaterial::PMaterial(const PMaterial& Other)
		: MaterialInstance(Other.MaterialInstance)
	{
	}
	PMaterial::~PMaterial()
	{
	}
	PMaterial& PMaterial::operator=(const PMaterial& Other)
	{
		MaterialInstance			= Other.MaterialInstance;
		return (*this);
	}
	EShaderInstance* PMaterial::GetMaterialInstance() const
	{
		return MaterialInstance;
	}
	void PMaterial::SetMaterialInstance(EShaderInstance* InMaterialInstance)
	{
		MaterialInstance = InMaterialInstance;
	}

};
