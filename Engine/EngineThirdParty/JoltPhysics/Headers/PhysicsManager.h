#pragma once
#include <CoreMinimal.h>
#include "JoltPhysics.h"

namespace PigeonEngine
{
	typedef CPhysics_Jolt CPhysicsInterface;

	class CPhysicsManager : public CPhysicsInterface
	{
		CLASS_SINGLETON_BODY(CPhysicsManager)
	public:
		static void		Initialize(HWND hWnd)		{}
		static void		ShutDown()					{}
		static void		StaticUpdate()				{}
	public:
		static void		Init()						{ GetSingleton()->InitPhysics(); }
		static void		Uninit()					{ GetSingleton()->UninitPhysics(); }
		static void		Update()					{}
		static void		FixedUpdate()
		{
			GetSingleton()->PrePhysicsUpdate();
			GetSingleton()->PhysicsUpdate();
			GetSingleton()->PostPhysicsUpdate();
		}
		static void		Draw()						{}
#ifdef _DEVELOPMENT_EDITOR
	public:
		static void		EditorUpdate() {}
	private:
		BOOL			m_EditorOpen;
#endif
	};
};