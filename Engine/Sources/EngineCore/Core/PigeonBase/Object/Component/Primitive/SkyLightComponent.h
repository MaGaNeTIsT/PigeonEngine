#pragma once

#include <CoreMinimal.h>
#include <Config/EngineConfig.h>
#include <TextureAsset/TextureAsset.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	class RSkyLightSceneProxy;

	struct ERenderSkyLightMatrices : public ERenderTransformInfo
	{
	public:

	public:
		ERenderSkyLightMatrices(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
			: ERenderTransformInfo(InWorldLocation, InWorldRotation, InWorldScaling)
		{
		}
		ERenderSkyLightMatrices() = default;
		ERenderSkyLightMatrices(const ERenderSkyLightMatrices& Other)
			: ERenderTransformInfo(Other)
		{
		}
		ERenderSkyLightMatrices& operator=(const ERenderSkyLightMatrices& Other)
		{
			ERenderTransformInfo::CopyFromOther(Other);
			return (*this);
		}
	};

	struct ERenderSkyLightParams
	{
	public:
		Color3		LightAdjustColor;
		FLOAT		LightAdjustIntensity;
		// The cube's mean linear luminance, which the render side divides every sample by
		// before applying the intensity: the pair is what makes a tint/brightness edit free.
		FLOAT		CubeMeanLinearLuminance;
		// Base resolution exponent of the prefiltered environment cube (8 = 256).
		UINT32		PrefilterResolutionExponent;
	public:
		ERenderSkyLightParams(const Color3& InLightAdjustColor, const FLOAT InLightAdjustIntensity
			, const FLOAT InCubeMeanLinearLuminance, const UINT32 InPrefilterResolutionExponent)
			: LightAdjustColor(InLightAdjustColor), LightAdjustIntensity(InLightAdjustIntensity)
			, CubeMeanLinearLuminance(InCubeMeanLinearLuminance), PrefilterResolutionExponent(InPrefilterResolutionExponent)
		{
		}
		ERenderSkyLightParams() = default;
		ERenderSkyLightParams(const ERenderSkyLightParams& Other)
			: LightAdjustColor(Other.LightAdjustColor), LightAdjustIntensity(Other.LightAdjustIntensity)
			, CubeMeanLinearLuminance(Other.CubeMeanLinearLuminance), PrefilterResolutionExponent(Other.PrefilterResolutionExponent)
		{
		}
		ERenderSkyLightParams& operator=(const ERenderSkyLightParams& Other)
		{
			LightAdjustColor				= Other.LightAdjustColor;
			LightAdjustIntensity			= Other.LightAdjustIntensity;
			CubeMeanLinearLuminance			= Other.CubeMeanLinearLuminance;
			PrefilterResolutionExponent		= Other.PrefilterResolutionExponent;
			return (*this);
		}
	};

	class PSkyLightComponent : public PSceneComponent
	{
	public:
		const ETextureCubeAsset*	GetCubeMapAsset() const;
		const Color3&				GetLightAdjust() const;
		// The mean luminance in cd/m2 the environment is adjusted to. The render side normalizes
		// the cube by its own mean first, so this is a physical mean rather than a scale.
		FLOAT						GetIntensity() const;
		// The prefiltered environment cube's base resolution, as a power of two. 8 means 256.
		PE_NODISCARD UINT32			GetPrefilterResolutionExponent() const noexcept;
		PE_NODISCARD FLOAT			GetCubeMeanLinearLuminance() const noexcept;
	public:
		void	SetCubeMapAsset(const ETextureCubeAsset* InCubeMapAsset);
		/*
		* Game thread: binds an environment cube to this sky light without waiting for it. The asset is
		* asked for now, applied before this call returns when it is already loaded, and retried every
		* frame until it is; InImportSource is the editor's raw source, six faces in the order the
		* importer expects.
		*/
		void	RequestCubeMapAsset(const EString& InAssetName, const FAssetImportCubeSource* InImportSource = nullptr);
		void	SetLightAdjust(const Color3& InColor);
		void	SetIntensity(const FLOAT InLuminanceInCandelaPerSquareMeter);
		void	SetPrefilterResolutionExponent(const UINT32 InExponent);
	protected:
		const ETextureCubeAsset*	CubeMapAsset;
		Color3						LightAdjust;
		FLOAT						Intensity;
		UINT32						PrefilterResolutionExponent;
		// The cube's solid angle weighted mean linear luminance, computed when the asset is set.
		// The render side divides every environment sample by it, which is what makes Intensity
		// a true mean rather than a scale.
		FLOAT						CubeMeanLinearLuminance;

		// Render proxy functions START
	public:
		enum PSkyLightUpdateState : UINT8
		{
			SKY_LIGHT_UPDATE_STATE_NONE		= 0,
			SKY_LIGHT_UPDATE_STATE_TEXTURE	= (1 << 0),
			SKY_LIGHT_UPDATE_STATE_PARAMS	= (1 << 1),
			SKY_LIGHT_UPDATE_STATE_MATRIX	= (1 << 2)
		};
	public:
		RSkyLightSceneProxy*			SceneProxy;	// ONLY render thread could use this proxy pointer.
	public:
		UINT8							GetUpdateRenderState() const;
		RSkyLightSceneProxy*			CreateSceneProxy();
		virtual void					CreateRenderState() override;
		virtual void					DestroyRenderState() override;
		virtual void					SendUpdateRenderState() override;
	protected:
		void			MarkAsDirty(PSkyLightUpdateState InState);
		virtual void	MarkRenderTransformAsDirty() override;
		virtual void	MarkRenderStateAsDirty() override;
		virtual void	CleanMarkRenderStateDirty() override;
	protected:
		UINT8							UpdateState;
		// Render proxy functions END

	// ImGui
#if ENGINE_EDITOR_ONLY
	public:
		virtual void	GenerateComponentDetail() override;
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(PSkyLightComponent)
	};

};
