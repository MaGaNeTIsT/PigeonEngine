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
	public:

		RENDER_PROXY_CLASS_BODY(RMeshSceneProxy)

	};

	extern EVertexResourceType TranslateInputLayoutToVertexResource(const RInputLayoutDesc& InLayout, UINT32& OutSlot);

};