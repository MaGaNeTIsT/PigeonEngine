#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include "PrimitiveSceneProxy.h"
#include <PigeonBase/Object/Component/Primitive/MeshComponent.h>

namespace PigeonEngine
{

	class RMeshMaterialParameter : public RPrimitiveMaterialParameter
	{
	public:
		void AddMeshMaterialParameter();

		CLASS_MATERIAL_PARAMETER(RMeshMaterialParameter)
	};

	class RMeshSceneProxy : public RPrimitiveSceneProxy
	{
	protected:
		static EVertexResourceType TranslateInputLayoutToVertexResource(const RInputLayoutDesc& InLayout, UINT32& OutSlot)
		{
			RShaderSemanticType ShaderSemantic = GetShaderSemanticBaseType(InLayout.SemanticName);
			OutSlot = GetShaderSemanticTypeSlot(InLayout.SemanticName);
			EVertexLayoutType VertexLayout = TranslateSemanticBaseTypeToVertexBaseLayout(ShaderSemantic);
			return (TranslateVertexLayoutTypeToVertexResourceType(VertexLayout));
		}

		RENDER_PROXY_CLASS_BODY(RMeshSceneProxy)

	};

};