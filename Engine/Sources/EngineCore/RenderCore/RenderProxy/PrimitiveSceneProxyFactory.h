#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	class RPrimitiveSceneProxy;

	/*
	* The render side builds a primitive's proxy itself, inside the frame's add command, so a
	* component never hands a game thread object across the boundary: what travels is the
	* snapshot and the proxy type name this table turns back into a constructor.
	*
	* Registration is deferred through the engine's own register hook rather than done by a
	* static initializer calling straight in, so the table is built during
	* EMainManager::Initialize and never during static initialization order roulette.
	*/
	class RPrimitiveSceneProxyFactory
	{
	public:
		typedef RPrimitiveSceneProxy* (*FCreateFunctionType)();
	public:
		static void	Register(const EName& InProxyTypeName, const FCreateFunctionType InCreateFunction);
		// Render thread only. Null when nothing is registered under that name, which the caller
		// reads as "this primitive submits no work".
		PE_NODISCARD static RPrimitiveSceneProxy*	Create(const EName& InProxyTypeName);
		PE_NODISCARD static UINT32	GetRegisteredNum();
	};

	/*
	* How a proxy registers itself: one line inside the class registration function its own
	* translation unit already has, so the engine's existing deferred hook does the timing and
	* no second registration macro can collide with it.
	*
	*   static void RegisterClassTypes()
	*   {
	*       RegisterClassType<RStaticMeshSceneProxy, RMeshSceneProxy>();
	*       RPrimitiveSceneProxyFactory::Register(EName(PE_TEXT("StaticMeshPrimitive")),
	*           []() -> RPrimitiveSceneProxy* { return new RStaticMeshSceneProxy(); });
	*   }
	*
	* The name is the one the component reports through
	* PPrimitiveComponent::GetRenderProxyTypeName, and the proxy has to be default
	* constructible: the snapshot carries everything it needs, so its constructor takes nothing.
	*/

} // namespace PigeonEngine
