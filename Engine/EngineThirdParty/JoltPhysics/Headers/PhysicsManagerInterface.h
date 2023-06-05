#pragma once

namespace PigeonEngine
{
	class IPhysicsManagerInterface
	{
	public:
		IPhysicsManagerInterface()					{}
		virtual ~IPhysicsManagerInterface()			= default;

	public:
		virtual void InitPhysics()					{}
		virtual void UninitPhysics()				{}
		virtual void PrePhysicsUpdate()				{}
		virtual void PhysicsUpdate()				{}
		virtual void PostPhysicsUpdate()			{}
	};
};