#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <Shader/ShaderParameter.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>

namespace PigeonEngine
{

	class RScene;

	class RViewMaterialParameter : public EMaterialParameter
	{
	public:
		virtual void	SetupParameters()override;

		CLASS_MATERIAL_PARAMETER(RViewMaterialParameter)
	};

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
		void	UpdateRenderResource();
		void	BindRenderResource(const UINT32 InSolt)const;
	protected:
		void	BindViewMaterialParameter(const UINT32 InSlot)const;
	protected:
		RViewMaterialParameter		ViewMaterialParameter;
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