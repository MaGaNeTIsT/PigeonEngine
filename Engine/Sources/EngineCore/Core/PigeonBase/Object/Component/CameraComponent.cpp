#include "CameraComponent.h"

namespace PigeonEngine
{

	PCameraComponent::PCameraComponent()
	{
	}
	PCameraComponent::~PCameraComponent()
	{
	}
	void PCameraComponent::SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight)
	{
		Viewport.TopLeftX = InTopLeftX;
		Viewport.TopLeftY = InTopLeftY;
		Viewport.Width = InWidth;
		Viewport.Height = InHeight;
	}
	void PCameraComponent::SetViewport(const Vector4& InViewRect)
	{
		Viewport.TopLeftX = InViewRect.x;
		Viewport.TopLeftY = InViewRect.y;
		Viewport.Width = InViewRect.z;
		Viewport.Height = InViewRect.w;
	}

};