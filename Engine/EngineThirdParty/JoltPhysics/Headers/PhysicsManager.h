#pragma once
#include <CoreMinimal.h>
#include "JoltPhysics.h"

PIGEONENGINE_NAMESPACE_BEGIN
typedef FPhysics_Jolt FPhysicsInterface;

class FPhysicsManager final : public FPhysicsInterface
{
	CLASS_SINGLETON_BODY(FPhysicsManager)
public:
	static void		Initialize()				{}
	static void		ShutDown()					{}
	static void		StaticUpdate()				{}
public:
	static void		Init()						{ GetSingleton()->InitPhysics(); }
	static void		Uninit()					{ GetSingleton()->UninitPhysics(); }
	static void		Update()					{}
	static void		FixedUpdate(FLOAT DeltaTime)
	{
		GetSingleton()->PrePhysicsUpdate();
		GetSingleton()->PhysicsUpdate(DeltaTime);
		GetSingleton()->PostPhysicsUpdate();
	}
	static void		Draw()						{}
#if _EDITOR_ONLY
public:
	static void		EditorUpdate() {}
private:
	BOOL32			m_EditorOpen = FALSE;
#endif
};

PIGEONENGINE_NAMESPACE_END