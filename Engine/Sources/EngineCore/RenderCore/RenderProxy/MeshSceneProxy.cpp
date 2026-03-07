#include "MeshSceneProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RMeshSceneProxy, RPrimitiveSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EVertexResourceType TranslateInputLayoutToVertexResource(const RInputLayoutDesc& InLayout, UINT32& OutSlot)
	{
		RShaderSemanticType ShaderSemantic = GetShaderSemanticBaseType(InLayout.SemanticName);
		OutSlot = GetShaderSemanticTypeSlot(InLayout.SemanticName);
		EVertexLayoutType VertexLayout = TranslateSemanticBaseTypeToVertexBaseLayout(ShaderSemantic);
		return (TranslateVertexLayoutTypeToVertexResourceType(VertexLayout));
	}

	void RMeshMaterialParameter::AddMeshMaterialParameter()
	{

	}

	RMeshSceneProxy::RMeshSceneProxy()
	{
	}
	RMeshSceneProxy::RMeshSceneProxy(const RMeshSceneProxy& Other)
		: RPrimitiveSceneProxy(Other)
	{
	}
	RMeshSceneProxy::~RMeshSceneProxy()
	{
	}

};