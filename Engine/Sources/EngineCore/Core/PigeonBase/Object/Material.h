#pragma once

#include <CoreMinimal.h>
#include "Object.h"
#include <ShaderAsset/ShaderInstance.h>

namespace PigeonEngine
{

	class PMaterial : public PObject
	{
	public:
		PMaterial();
		PMaterial(const PMaterial& Other);
		virtual ~PMaterial();
		PMaterial& operator=(const PMaterial& Other);
	public:
		// The instance is not owned here: whoever creates it keeps ownership, and a
		// material is allowed to stay without one.
		PE_NODISCARD EShaderInstance*	GetMaterialInstance() const;
		void							SetMaterialInstance(EShaderInstance* InMaterialInstance);
	protected:
		EShaderInstance*	MaterialInstance;
	};

};
