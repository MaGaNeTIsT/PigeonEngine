#pragma once

#include <CoreMinimal.h>
#include "LightComponent.h"

namespace PigeonEngine
{

	/*
	* A point light: a local light with no cone.
	*
	* It radiates into the whole sphere, so its falloff is the inverse square law and nothing
	* else. Its intensity is a luminous intensity in candela, and SetLuminousFluxLumen is the
	* way in for anyone holding a lamp rating: I = Phi / (4 pi).
	*
	* The type adds no state and no behaviour over PLocalLightComponent - that is the point of
	* the base. What it adds is the name, the type tag that the shading side branches on, and
	* the debug name an editor shows.
	*/
	class PPointLightComponent : public PLocalLightComponent
	{
	public:
		CLASS_VIRTUAL_NOCOPY_BODY(PPointLightComponent)
	};

}   // namespace PigeonEngine
