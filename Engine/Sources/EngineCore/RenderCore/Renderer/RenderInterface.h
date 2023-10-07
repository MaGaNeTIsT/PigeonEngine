#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class PCameraComponent;
	class PSkyLightComponent;
	class PDirectionalLightComponent;
	class PStaticMeshComponent;
	class PSkeletalMeshComponent;

	class RSceneInterface
	{
	public:
		virtual void	ClearAll()			= 0;
		virtual void	UnbindErrorCheck()	= 0;
	public:
		virtual void	AddCamera(PCameraComponent* InComponent)						= 0;
		virtual void	RemoveCamera(PCameraComponent* InComponent)						= 0;
		virtual void	UpdateCamera(PCameraComponent* InComponent)						= 0;

		virtual void	AddSkyLight(PSkyLightComponent* InComponent)					= 0;
		virtual void	RemoveSkyLight(PSkyLightComponent* InComponent)					= 0;
		virtual void	UpdateSkyLight(PSkyLightComponent* InComponent)					= 0;

		virtual void	AddDirectionalLight(PDirectionalLightComponent* InComponent)	= 0;
		virtual void	RemoveDirectionalLight(PDirectionalLightComponent* InComponent)	= 0;
		virtual void	UpdateDirectionalLight(PDirectionalLightComponent* InComponent)	= 0;

		virtual void	AddStaticMesh(PStaticMeshComponent* InComponent)				= 0;
		virtual void	RemoveStaticMesh(PStaticMeshComponent* InComponent)				= 0;
		virtual void	UpdateStaticMesh(PStaticMeshComponent* InComponent)				= 0;
		virtual void	AddSkeletalMesh(PSkeletalMeshComponent* InComponent)			= 0;
		virtual void	RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)			= 0;
		virtual void	UpdateSkeletalMesh(PSkeletalMeshComponent* InComponent)			= 0;
	public:
		RSceneInterface() {}
		RSceneInterface(const RSceneInterface& Other) {}
		virtual ~RSceneInterface() {}
		RSceneInterface& operator=(const RSceneInterface& Other) { return (*this); }
	};

};