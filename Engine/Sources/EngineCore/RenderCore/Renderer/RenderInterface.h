#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class PPrimitiveComponent;

	class RSceneInterface
	{
	public:
		virtual void	ClearAll()			= 0;
		virtual void	UnbindErrorCheck()	= 0;
	public:
		virtual void	AddStaticPrimitive(PPrimitiveComponent* InComponent)		= 0;
		virtual void	RemoveStaticPrimitive(PPrimitiveComponent* InComponent)		= 0;
		virtual void	AddDynamicPrimitive(PPrimitiveComponent* InComponent)		= 0;
		virtual void	RemoveDynamicPrimitive(PPrimitiveComponent* InComponent)	= 0;
		virtual void	UpdateDynamicPrimitive(PPrimitiveComponent* InComponent)	= 0;
	public:
		RSceneInterface() {}
		RSceneInterface(const RSceneInterface& Other) {}
		virtual ~RSceneInterface() {}
		RSceneInterface& operator=(const RSceneInterface& Other) { return (*this); }
	};

};