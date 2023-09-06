#include "BaseSceneProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RBaseSceneProxy, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RBaseSceneProxy::RBaseSceneProxy()
		: Scene(nullptr)
	{
	}
	RBaseSceneProxy::RBaseSceneProxy(const RBaseSceneProxy& Other)
		: Scene(nullptr)
	{
	}
	RBaseSceneProxy::~RBaseSceneProxy()
	{
	}
	const RScene* RBaseSceneProxy::GetRenderScene()const
	{
		return Scene;
	}
	void RBaseSceneProxy::SetRenderScene(const RScene* InScene)
	{
		Scene = nullptr;
		if (!!InScene)
		{
			Scene = InScene;
		}
	}

};