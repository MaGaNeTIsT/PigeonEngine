#include "PointLightComponent.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PPointLightComponent, PLocalLightComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PPointLightComponent::PPointLightComponent()
	{
		LightData.LightType = ELightType::LIGHT_TYPE_POINT;
		// A point light stores no cone. The shading side branches on the type rather than on a
		// sentinel cosine, so these values are never read - they are left at a defined state
		// rather than a suggestive one, so nothing accidentally depends on them.
		LightData.SpotInnerAngleDegrees = 0.f;
		LightData.SpotOuterAngleDegrees = 0.f;
		POBJ_DEBUGNAME_SET(this, "PointLightComponent");
	}
	PPointLightComponent::~PPointLightComponent()
	{
	}

}   // namespace PigeonEngine
