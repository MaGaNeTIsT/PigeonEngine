#pragma once

#include <CoreMinimal.h>
#include "LightComponent.h"

namespace PigeonEngine
{

	/*
	* A spot light: a local light with a cone.
	*
	* The cone is authored as two half angles from the axis, in degrees, because that is the
	* pair a person can picture. The renderer stores their cosines - a cosine is what the
	* shading compares a direction against, and computing it once per light costs less than
	* computing it once per light per pixel.
	*
	* The inner angle is the lip: everything inside it is at full intensity. Between the two the
	* light falls to nothing at the outer angle, which is the penumbra. An inner angle equal to
	* the outer one is a hard edge, which is legal but shows aliasing along the cone.
	*/
	class PSpotLightComponent : public PLocalLightComponent
	{
	public:
		FLOAT	GetSpotInnerAngleDegrees() const noexcept;
		FLOAT	GetSpotOuterAngleDegrees() const noexcept;
	public:
		// Half angles from the axis. Clamped to (0, 90): zero is a degenerate cone and past
		// ninety the "cone" is wider than the half space the falloff assumes.
		void	SetSpotInnerAngleDegrees(const FLOAT InSpotInnerAngleDegrees);
		void	SetSpotOuterAngleDegrees(const FLOAT InSpotOuterAngleDegrees);
		// Both at once, which is the only way to be sure the pair stays ordered: an inner angle
		// wider than the outer one inverts the penumbra, and the renderer clamps it back rather
		// than producing a cone that brightens towards its edge.
		void	SetSpotConeAnglesDegrees(const FLOAT InSpotInnerAngleDegrees, const FLOAT InSpotOuterAngleDegrees);
	public:
		// A spot spreads its flux over a cone rather than over the whole sphere, so the same
		// lumens mean a brighter beam. The cone's solid angle is 2 pi (1 - cos(outer)), which is
		// what turns an intensity back into a flux and the reverse.
		virtual void	SetLuminousFluxLumen(const FLOAT InLuminousFluxLumen) override;

		CLASS_VIRTUAL_NOCOPY_BODY(PSpotLightComponent)
	};

}   // namespace PigeonEngine
