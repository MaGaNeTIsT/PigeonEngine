#pragma once

#include <CoreMinimal.h>
#include "Object.h"
#include <ShaderAsset/ShaderAsset.h>
#include <Shader/ShaderParameter.h>

namespace PigeonEngine
{

	class PMaterial : public PObject
	{
	public:
		enum EMaterialParameterType : UINT32
		{
			MATERIAL_PARAMETER_TYPE_FLOAT1,
			MATERIAL_PARAMETER_TYPE_FLOAT2,
			MATERIAL_PARAMETER_TYPE_FLOAT3,
			MATERIAL_PARAMETER_TYPE_FLOAT4,
			MATERIAL_PARAMETER_TYPE_INT1,
			MATERIAL_PARAMETER_TYPE_INT2,
			MATERIAL_PARAMETER_TYPE_INT3,
			MATERIAL_PARAMETER_TYPE_INT4,
			MATERIAL_PARAMETER_TYPE_UINT1,
			MATERIAL_PARAMETER_TYPE_UINT2,
			MATERIAL_PARAMETER_TYPE_UINT3,
			MATERIAL_PARAMETER_TYPE_UINT4,
			MATERIAL_PARAMETER_TYPE_MATRIX33,
			MATERIAL_PARAMETER_TYPE_MATRIX34,
			MATERIAL_PARAMETER_TYPE_MATRIX44,
			MATERIAL_PARAMETER_TYPE_TEXTURE2D,
			MATERIAL_PARAMETER_TYPE_TEXTURECUBE,
			MATERIAL_PARAMETER_TYPE_TEXTURE3D,
			MATERIAL_PARAMETER_TYPE_COUNT
		};
	public:
		void	AddParameter(EMaterialParameterType InType, const EString& InParamName = "_Param");
	protected:
		struct EParameterUnit
		{
			EMaterialParameterType	Type;
			TArray<BYTE>			Data;
		};
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