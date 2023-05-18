#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <RenderCommon.h>

namespace PigeonEngine
{

	struct PCameraViewInfo
	{
		PCameraViewInfo() : Viewport(RViewport(Vector2::Zero(), Vector2::One(), Vector2::YVector())) {}

		RViewport	Viewport;
		FLOAT		Fov;
		FLOAT		Far;
		FLOAT		Near;
	};

	struct PCameraFrustumInfo
	{
		union
		{
			struct
			{
				Vector3	PlaneTop;
				Vector3	PlaneBottom;
				Vector3	PlaneLeft;
				Vector3	PlaneRight;
			};
			Vector3	Plane[4];
		};
		union
		{
			struct
			{
				Vector3	FarPlaneTopLeft;
				Vector3	FarPlaneTopRight;
				Vector3	FarPlaneBottomLeft;
				Vector3	FarPlaneBottomRight;
				Vector3	NearPlaneTopLeft;
				Vector3	NearPlaneTopRight;
				Vector3	NearPlaneBottomLeft;
				Vector3	NearPlaneBottomRight;
			};
			Vector3	FarNearPlanePoint[8];
		};
	};

	class PCameraComponent : public PSceneComponent
	{

		EClass(PCameraComponent, PSceneComponent)

		CLASS_VIRTUAL_NOCOPY_BODY(PCameraComponent)

	public:
		const PCameraViewInfo&		GetCameraViewInfo()const { return CameraViewInfo; }
		const PCameraFrustumInfo&	GetCameraFrustumInfo()const { return CameraFrustumInfo; }
	public:
		// View port is a rect that defines anchor(top left point), width(x axis length) and height(y axis length).
		void SetViewport(FLOAT InTopLeftX, FLOAT InTopLeftY, FLOAT InWidth, FLOAT InHeight);
		// Param [InViewRect] : x is top left x. y is top left y. z is width. w is height.
		void SetViewport(const Vector4& InViewRect);
	protected:
		PCameraViewInfo		CameraViewInfo;
		PCameraFrustumInfo	CameraFrustumInfo;
	};

};