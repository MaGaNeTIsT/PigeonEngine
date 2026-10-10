#include "LightComponent.h"
#include <RenderProxy/LightSceneProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PLightComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PLightComponent::PLightComponent()
		: UpdateState(PLightUpdateState::LIGHT_UPDATE_STATE_NONE)
	{
		POBJ_DEBUGNAME_SET(this, "LightComponent");
	}
	PLightComponent::~PLightComponent()
	{
	}
	ELightType PLightComponent::GetLightType() const noexcept
	{
		return (LightData.LightType);
	}
	const Color3& PLightComponent::GetLightColor() const noexcept
	{
		return (LightData.LightColor);
	}
	BOOL32 PLightComponent::IsLightCastShadow() const noexcept
	{
		return (LightData.CastShadow);
	}
	const Vector2Int& PLightComponent::GetShadowMapSize() const noexcept
	{
		return (LightData.ShadowMapSize);
	}
	const ELightData& PLightComponent::GetLightData() const noexcept
	{
		return LightData;
	}
	void PLightComponent::SetLightColor(const Color3& InColor)
	{
		LightData.LightColor = InColor;
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PLightComponent::SetShadowMapSize(const BOOL32 InIsCastShadow, const Vector2Int& InShadowMapSize)
	{
		LightData.CastShadow = InIsCastShadow;
		LightData.ShadowMapSize = InShadowMapSize;
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}

	// Render proxy functions START
	UINT8 PLightComponent::GetUpdateRenderState() const noexcept
	{
		return UpdateState;
	}
	void PLightComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			AddToRenderScene();
		}
	}
	void PLightComponent::DestroyRenderState()
	{
		RemoveFromRenderScene();
		PSceneComponent::DestroyRenderState();
	}
	void PLightComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			UpdateInRenderScene();
		}
		PSceneComponent::SendUpdateRenderState();
	}
	void PLightComponent::MarkAsDirty(PLightUpdateState InState)
	{
		if (InState == PLightUpdateState::LIGHT_UPDATE_STATE_MATRIX)
		{
			MarkRenderTransformAsDirty();
		}
		else
		{
			UpdateState |= InState;
			MarkRenderStateAsDirty();
		}
	}
	void PLightComponent::MarkRenderTransformAsDirty()
	{
		UpdateState |= PLightUpdateState::LIGHT_UPDATE_STATE_MATRIX;
		PSceneComponent::MarkRenderTransformAsDirty();
	}
	void PLightComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PLightComponent::CleanMarkRenderStateDirty()
	{
		UpdateState = PLightUpdateState::LIGHT_UPDATE_STATE_NONE;
		PSceneComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

	PLocalLightComponent::PLocalLightComponent()
		: SceneProxy(nullptr)
	{
		LightData.LightType = ELightType::LIGHT_TYPE_POINT;
		POBJ_DEBUGNAME_SET(this, "LocalLightComponent");
	}
	PLocalLightComponent::~PLocalLightComponent()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("SceneProxy is not null in local light component distruction.")), (!SceneProxy));
	}
	FLOAT PLocalLightComponent::GetLuminousIntensityCandela() const noexcept
	{
		return (LightData.LuminousIntensityCandela);
	}
	FLOAT PLocalLightComponent::GetAttenuationRadiusMeters() const noexcept
	{
		return (LightData.AttenuationRadiusMeters);
	}
	FLOAT PLocalLightComponent::GetSourceRadiusMeters() const noexcept
	{
		return (LightData.SourceRadiusMeters);
	}
	void PLocalLightComponent::SetLuminousIntensityCandela(const FLOAT InLuminousIntensityCandela)
	{
		LightData.LuminousIntensityCandela = EMath::Max(0.f, InLuminousIntensityCandela);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PLocalLightComponent::SetLuminousFluxLumen(const FLOAT InLuminousFluxLumen)
	{
		// A point light's flux over the whole sphere. The spot type overrides this, because a
		// spot's flux covers a cone and the same lumens therefore mean a brighter beam.
		SetLuminousIntensityCandela(EMath::Max(0.f, InLuminousFluxLumen) / (4.f * PE_PI));
	}
	void PLocalLightComponent::SetAttenuationRadiusMeters(const FLOAT InAttenuationRadiusMeters)
	{
		// A radius of zero would make the windowed falloff divide by zero, and it also means
		// "light nothing", which is never what someone typing 0 wants.
		LightData.AttenuationRadiusMeters = EMath::Max(0.001f, InAttenuationRadiusMeters);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PLocalLightComponent::SetSourceRadiusMeters(const FLOAT InSourceRadiusMeters)
	{
		LightData.SourceRadiusMeters = EMath::Max(0.f, InSourceRadiusMeters);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}

	// Render proxy functions START
	RLocalLightSceneProxy* PLocalLightComponent::CreateSceneProxy()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Try creating local light scene proxy, but already exist scene proxy.")), (!SceneProxy));
		SceneProxy = new RLocalLightSceneProxy(this);
		return SceneProxy;
	}
	void PLocalLightComponent::AddToRenderScene()
	{
		this->GetWorld()->GetRenderScene()->AddLocalLight(this);
	}
	void PLocalLightComponent::RemoveFromRenderScene()
	{
		this->GetWorld()->GetRenderScene()->RemoveLocalLight(this);
	}
	void PLocalLightComponent::UpdateInRenderScene()
	{
		this->GetWorld()->GetRenderScene()->UpdateLocalLight(this);
	}
	// Render proxy functions END

}   // namespace PigeonEngine
