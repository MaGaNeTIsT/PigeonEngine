#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <Shader/ShaderEngineParameters.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>

namespace PigeonEngine
{

	class RScene;
	class EShaderProgram;
	class IRRHIDevice;
	class IRCommandList;

	/*
	* The view parameters are published into the ViewParams block the shader source declares,
	* by name, and bound at the root the reading program's binding plan reports. The block is
	* shared by every program that declares it, so one instance of this proxy publishes it for
	* the whole frame rather than per material.
	*/
	class RViewProxy : public RBaseSceneProxy
	{
	public:
		using RVisibilityMapType	= TMap<ObjectIdentityType, BOOL32>;
	public:
		RViewProxy(const PCameraComponent* InComponent);
		void	SetupProxy(const BOOL32 InIsMainView, const ERenderViewMatrices& InMatrices, const ERenderViewParams& InParams);
	public:
		BOOL32						IsMainRenderView()const;
		RVisibilityMapType&			GetVisibilityMap();
		const RVisibilityMapType&	GetVisibilityMap()const;
		ECameraViewInfo&			GetViewInfo();
		const ECameraViewInfo&		GetViewInfo()const;
		EViewport&					GetRenderViewport();
		const EViewport&			GetRenderViewport()const;
		EViewMatrix&				GetViewMatrix();
		const EViewMatrix&			GetViewMatrix()const;
		EFrustum&					GetViewFrustum();
		const EFrustum&				GetViewFrustum()const;
	public:
		void	ResetVisibilityMap();
		void	UpdateViewSettings(const BOOL32 InIsMainView);
		void	UpdateMatrices(const ERenderViewMatrices& InMatrices);
		void	UpdateViewParams(const ERenderViewParams& InParams);
		// Builds the ViewParams mirrors and buffers from the first program that declares the
		// block. FALSE while no such program was seen: the view is simply not published in
		// that frame, and the next program gets to supply the layout.
		BOOL32	EnsureParameterBlock(IRRHIDevice* InRenderDevice, const EShaderProgram& InProgram);
		// Writes the values and pushes them into the gpu buffers. Early outs while nothing
		// moved, so a still camera costs one flag test per frame.
		void	UpdateRenderResource();
		// Binds the view block at the root the program's plan reports. Must run after the
		// pipeline state is set: switching a root signature voids every root binding.
		void	BindParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram)const;
	protected:
		// Named at construction: the name is the block's identity, so there is no unnamed state
		// for a later edit to leave it in (an unnamed block never initializes, uploads or binds).
		RShaderParameterBlock		ViewParameterBlock{ EName(SHADER_BLOCK_VIEW_PARAMS) };
		// TRUE while a value the block carries has moved since the last publish.
		BOOL32						bParameterValuesDirty;
		BOOL32						IsMainView;
		RVisibilityMapType			VisibilityMap;
	protected:
		ECameraViewInfo				CameraViewInfo;
		EViewDomainInfo				ViewDomainInfo;
	protected:
		const PCameraComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RViewProxy)

	};

};
