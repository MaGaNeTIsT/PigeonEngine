#include "SceneRenderer.h"
#include <RenderProxy/ViewProxy.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSceneRenderer, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RSceneRenderer::RSceneRenderer()
		: Scene(nullptr)
	{
	}
	RSceneRenderer::~RSceneRenderer()
	{
	}
	void RSceneRenderer::Initialize()
	{
	}
	void RSceneRenderer::ShutDown()
	{
	}
	void RSceneRenderer::InitNewFrame()
	{
		InitViews();
	}
	void RSceneRenderer::Render()
	{

	}
	void RSceneRenderer::InitViews()
	{
		// Update requires
		{
			RCommand& UpdateCommands = Scene->GetUpdateCommands();
			UpdateCommands.DoCommands();
			UpdateCommands.EmptyQueue();
		}

		// Add or remove requires
		{
			RCommand& AddOrRemoveCommands = Scene->GetAddOrRemoveCommands();
			AddOrRemoveCommands.DoCommands();
			AddOrRemoveCommands.EmptyQueue();
		}

		// Visible culling
		{
			OctreeCull();
		}

	}
	void RSceneRenderer::OctreeCull()
	{
		for (UINT ViewIndex = 0u, ViewNum = ViewProxies.Length(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if _EDITOR_ONLY
			Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that view proxy can not be null"), (!!ViewProxy));
			if (!ViewProxy)
			{
				continue;
			}
#endif
			const EFrustum& ViewFrustum = ViewProxy->GetViewFrustum();
			ROctree& SceneOctree = Scene->GetSceneOctree();
			if (SceneOctree.GetPrimitiveNum() == 0u)
			{
				return;
			}
			INT32 TempDeep = 0;
			SceneOctree.BackwardRecursionNode(0, TempDeep,
				[](ROctreeNode& InNode, INT32 InDeep)->INT32
				{
					return (InDeep + 1);
				},
				[&ViewFrustum](ROctreeNode& InNode, INT32 InDeep)->BOOL32
				{
					//TODO
					return TRUE;
				});
		}
	}
	void RSceneRenderer::PrimitiveCull()
	{

	}

};