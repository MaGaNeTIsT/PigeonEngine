#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RScene;

	class RBaseSceneProxy : public EObjectBase
	{
	public:
		const RScene*	GetRenderScene()const;
		PE_INLINE void	SetRenderScene(const RScene* InScene);
	protected:
		const RScene*	Scene;

		RENDER_PROXY_CLASS_BODY(RBaseSceneProxy)

	};

};