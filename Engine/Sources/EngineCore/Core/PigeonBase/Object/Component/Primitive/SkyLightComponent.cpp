#include "SkyLightComponent.h"
#include <Asset/AssetLoadManager.h>
#include <RenderProxy/SkyLightProxy.h>
#include <TextureAsset/TextureAsset.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSkyLightComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PSkyLightComponent::PSkyLightComponent()
		: CubeMapAsset(nullptr), LightAdjust(Color3::White()), Intensity(EEngineSettings::ENGINE_SKY_LUMINANCE), SceneProxy(nullptr), UpdateState(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_NONE)
	{
		POBJ_DEBUGNAME_SET(this, "SkyLightComponent");
	}
	PSkyLightComponent::~PSkyLightComponent()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in sky light component distruction."), (!SceneProxy));
	}
	const ETextureCubeAsset* PSkyLightComponent::GetCubeMapAsset()const
	{
		return CubeMapAsset;
	}
	const Color3& PSkyLightComponent::GetLightAdjust()const
	{
		return LightAdjust;
	}
	FLOAT PSkyLightComponent::GetIntensity()const
	{
		return Intensity;
	}
	void PSkyLightComponent::SetCubeMapAsset(const ETextureCubeAsset* InCubeMapAsset)
	{
		CubeMapAsset = InCubeMapAsset;
		MarkAsDirty(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_TEXTURE);
	}
	void PSkyLightComponent::RequestCubeMapAsset(const EString& InAssetName, const FAssetImportCubeSource* InImportSource)
	{
		// Copied by value: the binding outlives this call.
		FAssetImportCubeSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}
		PSkyLightComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, InAssetName, ImportSource]() -> BOOL32
		{
			const ETextureCubeAsset* CubeMapAsset = nullptr;
			if (!ETextureAssetManager::GetManagerSingleton()->RequireLoadTextureCubeAsset(
				EBaseSettings::ENGINE_TEXTURE_PATH, InAssetName, CubeMapAsset, &ImportSource))
			{
				return FALSE;	// not loaded yet: the component retries on its next tick
			}
			Component->SetCubeMapAsset(CubeMapAsset);
			return TRUE;
		};
		// Tried once here as well, so an already loaded cube is applied - and the proxy told about it -
		// before this call returns rather than a frame later.
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}
	void PSkyLightComponent::SetLightAdjust(const Color3& InColor)
	{
		LightAdjust = InColor;
		MarkAsDirty(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_PARAMS);
	}
	void PSkyLightComponent::SetIntensity(const FLOAT InLuminanceInCandelaPerSquareMeter)
	{
		Intensity = EMath::Max(0.001f, InLuminanceInCandelaPerSquareMeter);
		MarkAsDirty(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_PARAMS);
	}

	// Render proxy functions START
	UINT8 PSkyLightComponent::GetUpdateRenderState()const
	{
		return UpdateState;
	}
	RSkyLightSceneProxy* PSkyLightComponent::CreateSceneProxy()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating sky light scene proxy, but already exist scene proxy."), (!SceneProxy));
		SceneProxy = new RSkyLightSceneProxy(this);
		return SceneProxy;
	}
	void PSkyLightComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			this->GetWorld()->GetRenderScene()->AddSkyLight(this);
		}
	}
	void PSkyLightComponent::DestroyRenderState()
	{
		this->GetWorld()->GetRenderScene()->RemoveSkyLight(this);
		PSceneComponent::DestroyRenderState();
	}
	void PSkyLightComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			this->GetWorld()->GetRenderScene()->UpdateSkyLight(this);
		}
		PSceneComponent::SendUpdateRenderState();
	}
	void PSkyLightComponent::MarkAsDirty(PSkyLightUpdateState InState)
	{
		if (InState == PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_MATRIX)
		{
			MarkRenderTransformAsDirty();
		}
		else
		{
			UpdateState |= InState;
			MarkRenderStateAsDirty();
		}
	}
	void PSkyLightComponent::MarkRenderTransformAsDirty()
	{
		UpdateState |= PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_MATRIX;
		PSceneComponent::MarkRenderTransformAsDirty();
	}
	void PSkyLightComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PSkyLightComponent::CleanMarkRenderStateDirty()
	{
		UpdateState = PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_NONE;
		PSceneComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

};
