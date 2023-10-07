#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	class RSkyLightSceneProxy;

	class PSkyLightComponent : public PSceneComponent
	{
	public:
		const Color3&	GetLightAdjust()const;
		FLOAT			GetIntensity()const;
	public:
		void	SetLightAdjust(const Color3& InColor);
		void	SetIntensity(const FLOAT InIntensity);
	protected:
		Color3	LightAdjust;
		FLOAT	Intensity;

		// Render proxy functions START
	public:
		enum PSkyLightUpdateState : UINT8
		{
			SKY_LIGHT_UPDATE_STATE_NONE		= 0,
			SKY_LIGHT_UPDATE_STATE_TEXTURE	= (1 << 0),
			SKY_LIGHT_UPDATE_STATE_DATA		= (1 << 1),
		};
	public:
		RSkyLightSceneProxy*			SceneProxy;	// ONLY render thread could use this proxy pointer.
	public:
		UINT8							GetUpdateRenderState()const;
		RSkyLightSceneProxy*			CreateSceneProxy();
		virtual void					CreateRenderState()override;
		virtual void					DestroyRenderState()override;
		virtual void					SendUpdateRenderState()override;
	protected:
		void			MarkAsDirty(PSkyLightUpdateState InState);
		virtual void	MarkRenderTransformAsDirty()override;
		virtual void	MarkRenderStateAsDirty()override;
		virtual void	CleanMarkRenderStateDirty()override;
	protected:
		UINT8							UpdateState;
		// Render proxy functions END

		CLASS_VIRTUAL_NOCOPY_BODY(PSkyLightComponent)
	};

};