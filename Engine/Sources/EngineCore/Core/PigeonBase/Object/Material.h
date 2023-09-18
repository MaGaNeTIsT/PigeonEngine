#pragma once

#include <CoreMinimal.h>
#include "Object.h"
#include <MaterialAsset/MaterialAsset.h>
#include <ShaderAsset/ShaderAsset.h>

namespace PigeonEngine
{

	class PMaterial : public PObject
	{
	public:
		EMaterialParameter			StaticMaterialParameter;
		EMaterialParameter			DynamicMaterialParameter;
		const EVertexShaderAsset*	VertexShader;
		const EPixelShaderAsset*	PixelShader;
	public:
		PMaterial();
		PMaterial(const PMaterial& Other);
		virtual ~PMaterial();
		PMaterial& operator=(const PMaterial& Other);
	};

};