#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include "BaseSceneProxy.h"
#include <Shader/ShaderParameter.h>
#include <PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h>

namespace PigeonEngine
{

	class RScene;
	class RViewProxy;
	class RDirectionalLightSceneProxy;

	class RDirectionalLightMaterialParameter : public EMaterialStructParameter
	{
	public:
		virtual void	SetupParameters()override;
		void			AddLightParameters();
		void			UpdateParameterValue(const UINT32 InIndex, const EViewDomainInfo& InInfo, const ELightData& InData);

		RDirectionalLightMaterialParameter() {}
		virtual ~RDirectionalLightMaterialParameter() {}
		RDirectionalLightMaterialParameter(const RDirectionalLightMaterialParameter& Other) {}
		RDirectionalLightMaterialParameter& operator=(const RDirectionalLightMaterialParameter& Other) { return (*this); }
	};

	class RDirectionalLightSceneProxy : public RBaseSceneProxy
	{
	public:
		using RViewSetType				= TSet<const RViewProxy*>;
		using RPerViewDomainInfoType	= TMap<ObjectIdentityType, TArray<EViewDomainInfo>>;
		using RVisibilityMapType		= TMap<ObjectIdentityType, BOOL32>;
		using RPerViewVisibilityMapType	= TMap<ObjectIdentityType, TArray<RVisibilityMapType>>;
	public:
		RDirectionalLightSceneProxy(const PDirectionalLightComponent* InComponent);
		void	SetupProxy(const ERenderDirectionalLightMatrices& InMatrices, const ERenderLightParams& InParams, const ECascadeShadowData* InCascadeShadowData);
	public:
		void	GenerateViewInfo(const RViewProxy* InViewProxy);
	public:
		const RPerViewVisibilityMapType&	GetVisibilityMap()const;
		RPerViewVisibilityMapType&			GetVisibilityMap();
		const ELightData&					GetLightData()const;
		RPerViewDomainInfoType&				GetViewDomainInfos();
		const RPerViewDomainInfoType&		GetViewDomainInfos()const;
		const ECascadeShadowData*			GetCascadeShadowData()const;
		BOOL32								IsLightCastShadow()const;
		BOOL32								IsLightUseCascadeShadow()const;
		BOOL32								IsNeedUpdateParams()const;
		RViewSetType&						GetViews();
		const RViewSetType&					GetViews()const;
	public:
		void	UpdateMatrices(const ERenderDirectionalLightMatrices& InMatrices);
		void	UpdateLightParams(const ERenderLightParams& InParams);
		void	UpdateCascadeData(const ECascadeShadowData* InCascadeShadowData);
	protected:
		PE_MUTABLE BOOL32					NeedUpdateParams;
		RPerViewVisibilityMapType			VisibilityMap;
	protected:
		ELightData							LightData;
		RPerViewDomainInfoType				ViewDomainInfos;
		ECascadeShadowData*					CascadeShadowData;
	protected:
		BOOL32								IsCascadeShadow;
		RViewSetType						Views;
		const PDirectionalLightComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RDirectionalLightSceneProxy)

	};

};