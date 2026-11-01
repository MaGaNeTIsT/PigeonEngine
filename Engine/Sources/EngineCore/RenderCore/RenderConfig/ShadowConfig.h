#pragma once

#include <CoreMinimal.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	/*
	* Shadow quality presets. The preset picks the filter cost, which is a compile time define in
	* the shadow shaders: changing it re-assembles those programs (a startup setting, not a per
	* frame one).
	*/
	enum EShadowQualityType : UINT8
	{
		SHADOW_QUALITY_LOW		= 0,	// 3x3 taps
		SHADOW_QUALITY_MEDIUM	= 1,	// 3x3 taps
		SHADOW_QUALITY_HIGH		= 2,	// 5x5 taps
		SHADOW_QUALITY_COUNT
	};

	/*
	* The shadow budget and quality knobs, in one place.
	*
	* Everything a shadow pass needs to size itself comes from here, so no shader and no renderer
	* source states a resolution, a cascade count or a bias. The defaults are the sizings this
	* project ships with; a game is expected to adjust the struct before the first frame.
	*
	* The total pixel budget is what every per light request is clamped against, together with the
	* atlas edge the budget implies. A light may ask for 2048, the type cap may allow it, and the
	* budget can still hand it 1024 or 512 - that clamp is the point of the budget.
	*/
	class RShadowSettings final
	{
	public:
		// ---- global budget ----
		// Every shadow tile has to fit in this many pixels. 24M with a 5120 edge holds the default
		// layout: four cascades at 2048/1024/512/256, eight spot tiles at 1024 and four point
		// lights at six 512 faces each.
		UINT32	TotalPixelBudget			= 24u * 1024u * 1024u;
		// Hard cap on the atlas edge a budget may grow to. The edge is the budget's square root,
		// rounded up to a multiple of 256 and clamped here.
		UINT32	MaxAtlasSize				= 8192u;
		// Depth precision of the shadow atlas. 32 for large orthographic ranges, 16 where memory
		// matters more than precision.
		UINT32	ShadowDepthBits				= 32u;

		// ---- directional (CSM) ----
		BOOL32			bEnableDirectionalShadow		= TRUE;
		UINT32			DirectionalCascadeNumMax		= 4u;
		// Multiplied onto the light's own cascade thicknesses, so a level can scale the whole
		// distribution without editing the light.
		FLOAT			DirectionalCascadeDistanceScale	= 1.0f;
		// Resolution of cascade 0. Later cascades halve, down to 256.
		UINT32			DirectionalTileBaseSize			= 2048u;
		UINT32			DirectionalMaxResolution		= 2048u;
		EShadowQualityType	DirectionalQuality			= SHADOW_QUALITY_MEDIUM;
		// Fraction of a cascade's far range over which it blends into the next one.
		FLOAT			CascadeBlendFraction			= 0.1f;
		// Metres added to the near side of every cascade's orthographic box, so a caster outside
		// the slice but projecting into it is still rendered.
		FLOAT			DepthPadding					= 200.f;

		// ---- local lights ----
		BOOL32			bEnableLocalShadow				= TRUE;
		// Independent caps per light type: a spot needs one view, a point needs six.
		UINT32			LocalSpotShadowMaxCount			= 8u;
		UINT32			LocalPointShadowMaxCount		= 4u;
		UINT32			SpotMaxResolution				= 1024u;
		UINT32			PointFaceMaxResolution			= 512u;
		EShadowQualityType	LocalQuality				= SHADOW_QUALITY_MEDIUM;
		FLOAT			LocalNear						= 0.5f;

		// ---- receiver side bias (software) ----
		FLOAT			ConstantDepthBias				= 0.f;
		FLOAT			SlopeScaledDepthBias			= 2.f;
		FLOAT			NormalOffsetTexels				= 1.5f;

		// ---- directional shadow mask ----
		// One RGBA8 target carries four directional lights. More than one set exists behind
		// PE_SHADOW_MASK_SET_NUM, which the default configuration compiles out.
		UINT32			DirectionalShadowMaskSetNum		= 1u;
		FLOAT			MaskResolutionScale				= 1.f;

	public:
		static RShadowSettings* GetSingleton()
		{
			static RShadowSettings _StaticSingletonObject;
			return (&_StaticSingletonObject);
		}
	public:
		// The atlas edge the budget implies: sqrt(budget) rounded up to 256, clamped to the cap.
		PE_NODISCARD UINT32 GetAtlasSize()const
		{
			UINT32 Edge = static_cast<UINT32>(EMath::Sqrt(static_cast<FLOAT>(TotalPixelBudget)));
			Edge = (((Edge + 255u) / 256u) * 256u);
			Edge = EMath::Clamp(Edge, 256u, EMath::Max(MaxAtlasSize, 256u));
			return Edge;
		}
		// Filter radius of a quality preset, in texels. The shadow shaders turn this into the
		// PCF loop bound through their own define.
		PE_NODISCARD static UINT32 GetFilterRadius(const EShadowQualityType InQuality)
		{
			return ((InQuality == EShadowQualityType::SHADOW_QUALITY_HIGH) ? 2u : 1u);
		}
		PE_NODISCARD UINT32 GetDirectionalFilterRadius()const
		{
			return GetFilterRadius(DirectionalQuality);
		}
		PE_NODISCARD UINT32 GetLocalFilterRadius()const
		{
			return GetFilterRadius(LocalQuality);
		}
		// Cascade resolution for a cascade index: the base size halved per level, floored at 256.
		PE_NODISCARD UINT32 GetCascadeResolution(const UINT32 InCascadeIndex)const
		{
			const UINT32 Shift = EMath::Min(InCascadeIndex, 8u);
			return EMath::Max(DirectionalTileBaseSize >> Shift, 256u);
		}
	public:
		RShadowSettings(const RShadowSettings&) = delete;
		RShadowSettings& operator=(const RShadowSettings&) = delete;
	private:
		RShadowSettings() {}
		~RShadowSettings() {}
	};

} // namespace PigeonEngine
