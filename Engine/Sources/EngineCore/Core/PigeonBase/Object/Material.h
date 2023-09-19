#pragma once

#include <CoreMinimal.h>
#include "Object.h"
#include <MaterialAsset/MaterialAsset.h>
#include <ShaderAsset/ShaderAsset.h>
#include <Shader/ShaderParameter.h>

namespace PigeonEngine
{

	class PMaterial : public PObject
	{
	protected:
		EMaterialParameter			MaterialParameter;
		const EVertexShaderAsset*	VertexShader;
		const EPixelShaderAsset*	PixelShader;
	public:
		PMaterial();
		PMaterial(const PMaterial& Other);
		virtual ~PMaterial();
		PMaterial& operator=(const PMaterial& Other);
	};

};