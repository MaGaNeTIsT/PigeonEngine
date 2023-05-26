#pragma once

#include <CoreMinimal.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	struct EBoundAABB
	{
		EBoundAABB()noexcept : AABBMin(Vector3(-0.5f, -0.5f, -0.5f)), AABBMax(Vector3(0.5f, 0.5f, 0.5f)) {}
		EBoundAABB(const EBoundAABB& Other)noexcept : AABBMin(Other.AABBMin), AABBMax(Other.AABBMax) {}

		Vector3	AABBMin;
		Vector3	AABBMax;
	};
	struct EBox
	{
		EBox()noexcept : Origin(Vector3(0.f, 0.f, 0.f)), Extent(Vector3(0.5f, 0.5f, 0.5f)) {}
		EBox(const EBox& Other)noexcept : Origin(Other.Origin), Extent(Other.Extent) {}

		Vector3	Origin;
		Vector3	Extent;
	};
	struct ESphere
	{
		ESphere()noexcept : Origin(Vector3(0.f, 0.f, 0.f)), Radius(0.5f) {}
		ESphere(const ESphere& Other)noexcept : Origin(Other.Origin), Radius(Other.Radius) {}

		Vector3	Origin;
		FLOAT	Radius;
	};
	struct ERect
	{
		ERect()noexcept : Left(0.f), Top(0.f), Right(0.f), Bottom(0.f) {}
		ERect(const ERect& Other)noexcept : Left(Other.Left), Top(Other.Top), Right(Other.Right), Bottom(Other.Bottom) {}
		ERect(FLOAT InLeft, FLOAT InTop, FLOAT InRight, FLOAT InBottom)noexcept : Left(InLeft), Top(InTop), Right(InRight), Bottom(InBottom) {}
		constexpr ERect()noexcept : Left(0.f), Top(0.f), Right(0.f), Bottom(0.f) {}
		constexpr ERect(const ERect& Other)noexcept : Left(Other.Left), Top(Other.Top), Right(Other.Right), Bottom(Other.Bottom) {}
		constexpr ERect(FLOAT InLeft, FLOAT InTop, FLOAT InRight, FLOAT InBottom)noexcept : Left(InLeft), Top(InTop), Right(InRight), Bottom(InBottom) {}

		union
		{
			struct
			{
				FLOAT	Left;
				FLOAT	Top;
				FLOAT	Right;
				FLOAT	Bottom;
			};
			FLOAT	Edge[4];
		};
	};
	struct EFrustum
	{
		EFrustum()noexcept
			: PlaneTop(Vector3::Zero()), PlaneBottom(Vector3::Zero()), PlaneLeft(Vector3::Zero()), PlaneRight(Vector3::Zero())
			, FarPlaneTopLeft(Vector3::Zero()), FarPlaneTopRight(Vector3::Zero())
			, FarPlaneBottomLeft(Vector3::Zero()), FarPlaneBottomRight(Vector3::Zero())
			, NearPlaneTopLeft(Vector3::Zero()), NearPlaneTopRight(Vector3::Zero())
			, NearPlaneBottomLeft(Vector3::Zero()), NearPlaneBottomRight(Vector3::Zero()) {}
		EFrustum(const EFrustum& other)noexcept
			: PlaneTop(other.PlaneTop), PlaneBottom(other.PlaneBottom), PlaneLeft(other.PlaneLeft), PlaneRight(other.PlaneRight)
			, FarPlaneTopLeft(other.FarPlaneTopLeft), FarPlaneTopRight(other.FarPlaneTopRight)
			, FarPlaneBottomLeft(other.FarPlaneBottomLeft), FarPlaneBottomRight(other.FarPlaneBottomRight)
			, NearPlaneTopLeft(other.NearPlaneTopLeft), NearPlaneTopRight(other.NearPlaneTopRight)
			, NearPlaneBottomLeft(other.NearPlaneBottomLeft), NearPlaneBottomRight(other.NearPlaneBottomRight) {}
		void GenerateFrustumInfo(FLOAT InFovAngleY, FLOAT InAspectRatio, FLOAT InNearDist, FLOAT InFarDist)
		{
			Vector3 TempPosVec[4];
			{
				FLOAT SinHalfAngleY, CosHalfAngleY;
				EMath::SinCos(SinHalfAngleY, CosHalfAngleY, (0.5f * InFovAngleY) * EMath::GetDegToRad());
				FLOAT HalfHeight = (SinHalfAngleY / CosHalfAngleY) * InFarDist;
				FLOAT HalfWidth = InAspectRatio * HalfHeight;
				//
				//          Y
				//          ^
				//          |
				//          |
				//  0---------------1
				//  |               |
				//  |               |----->X
				//  |               |
				//  2---------------3
				//
				TempPosVec[0] = Vector3(-HalfWidth, HalfHeight, InFarDist);
				TempPosVec[1] = Vector3(HalfWidth, HalfHeight, InFarDist);
				TempPosVec[2] = Vector3(-HalfWidth, -HalfHeight, InFarDist);
				TempPosVec[3] = Vector3(HalfWidth, -HalfHeight, InFarDist);
			}
			{
				for (UINT i = 4u; i < 8u; i++)
				{
					FarNearPlanePoint[i] = TempPosVec[i - 4];
				}
				FLOAT t = InNearDist / InFarDist;
				for (UINT i = 0u; i < 4u; i++)
				{
					FarNearPlanePoint[i] = Vector3::Lerp(0.f, TempPosVec[i], t);
				}
			}
			{
				for (INT i = 0; i < 4; i++)
				{
					TempPosVec[i].Normalize();
				}
				Plane[0] = Vector3::Cross(TempPosVec[0], TempPosVec[1]);
				Plane[1] = Vector3::Cross(TempPosVec[3], TempPosVec[2]);
				Plane[2] = Vector3::Cross(TempPosVec[2], TempPosVec[0]);
				Plane[3] = Vector3::Cross(TempPosVec[1], TempPosVec[3]);
				for (UINT i = 0u; i < 4u; i++)
				{
					Plane[i].Normalize();
				}
			}
		}

		union
		{
			struct
			{
				Vector3		PlaneTop;
				Vector3		PlaneBottom;
				Vector3		PlaneLeft;
				Vector3		PlaneRight;
			};
			Vector3		Plane[4];
		};
		union
		{
			struct
			{
				Vector3		FarPlaneTopLeft;
				Vector3		FarPlaneTopRight;
				Vector3		FarPlaneBottomLeft;
				Vector3		FarPlaneBottomRight;
				Vector3		NearPlaneTopLeft;
				Vector3		NearPlaneTopRight;
				Vector3		NearPlaneBottomLeft;
				Vector3		NearPlaneBottomRight;
			};
			Vector3		FarNearPlanePoint[8];
		};
	};
	struct EViewport
	{
		EViewport() noexcept : TopLeftX(0.f), TopLeftY(0.f), Width(0.f), Height(0.f), MinDepth(RENDER_DEPTH_MIN), MaxDepth(RENDER_DEPTH_MAX) {}
		EViewport(const EViewport& other) noexcept : TopLeftX(other.TopLeftX), TopLeftY(other.TopLeftY), Width(other.Width), Height(other.Height), MinDepth(other.MinDepth), MaxDepth(other.MaxDepth) {}
		EViewport(const Vector2& startPos, const Vector2& rectSize, const Vector2& depthSize) noexcept : TopLeftX(rectSize.x), TopLeftY(rectSize.y), Width(rectSize.x), Height(rectSize.y), MinDepth(depthSize.x), MaxDepth(depthSize.y) {}
		constexpr EViewport() noexcept : TopLeftX(0.f), TopLeftY(0.f), Width(0.f), Height(0.f), MinDepth(0.f), MaxDepth(0.f) {}
		constexpr EViewport(const Vector2& startPos, const Vector2& rectSize, const Vector2& depthSize) noexcept : TopLeftX(rectSize.x), TopLeftY(rectSize.y), Width(rectSize.x), Height(rectSize.y), MinDepth(depthSize.x), MaxDepth(depthSize.y) {}
		constexpr EViewport(const EViewport& other) noexcept : TopLeftX(other.TopLeftX), TopLeftY(other.TopLeftY), Width(other.Width), Height(other.Height), MinDepth(other.MinDepth), MaxDepth(other.MaxDepth) {}

		FLOAT	TopLeftX;
		FLOAT	TopLeftY;
		FLOAT	Width;
		FLOAT	Height;
		FLOAT	MinDepth;
		FLOAT	MaxDepth;
	};
	struct ECameraMatrix
	{
		struct EViewPart
		{
			EViewPart()noexcept : ViewMatrix(Matrix4x4::Identity()), InverseViewMatrix(Matrix4x4::Identity()) {}
			EViewPart(const EViewPart& Other)noexcept : ViewMatrix(Other.ViewMatrix), InverseViewMatrix(Other.InverseViewMatrix) {}

			Matrix4x4	ViewMatrix;
			Matrix4x4	InverseViewMatrix;
		};
		struct EProjectionPart
		{
			EProjectionPart()noexcept : ProjectionMatrix(Matrix4x4::Identity()), InverseProjectionMatrix(Matrix4x4::Identity()), DeviceZToViewZMulAdd(Vector2::XVector()) {}
			EProjectionPart(const EProjectionPart& Other)noexcept : ProjectionMatrix(Other.ProjectionMatrix), InverseProjectionMatrix(Other.InverseProjectionMatrix), DeviceZToViewZMulAdd(Other.DeviceZToViewZMulAdd) {}

			Matrix4x4	ProjectionMatrix;
			Matrix4x4	InverseProjectionMatrix;
			Vector2		DeviceZToViewZMulAdd;
		};
		ECameraMatrix()noexcept : ViewProjectionMatrix(Matrix4x4::Identity()), InverseViewProjectionMatrix(Matrix4x4::Identity()) {}
		ECameraMatrix(const ECameraMatrix& Other)noexcept : ViewPart(Other.ViewPart), ProjectionPart(Other.ProjectionPart), ViewProjectionMatrix(Other.ViewProjectionMatrix), InverseViewProjectionMatrix(Other.InverseViewProjectionMatrix) {}
		void TransformWorldPointToView(const Vector3& InPositionWS, Vector3& OutPositionVS)const
		{
			OutPositionVS = ViewPart.ViewMatrix.MultiplyPosition(InPositionWS);
		}
		void TransformViewPointToWorld(const Vector3& InPositionVS, Vector3& OutPositionWS)const
		{
			OutPositionWS = ViewPart.InverseViewMatrix.MultiplyPosition(InPositionVS);
		}
		/*
		* Return value : if return false, the point is out of screen coord, else in screen.
		* Params InPosition[SpaceName] : specific space position of the point.
		* Params output : [0.f, screen max] range of screen coord which has anchor(zero point) in left top of RECT.
		*/
		BOOL TransformWorldPointToScreen(const EViewport& InViewport, const Vector3& InPositionWS, Vector2& OutPositionSS)const
		{
			const Vector2 PointScl(0.5f, -0.5f);
			const Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionWS.x, InPositionWS.y, InPositionWS.z, 1.f);
			TempPoint = ViewProjectionMatrix.MultiplyVector(TempPoint);

			BOOL bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RENDER_DEPTH_MIN) ||
				(TempPoint.z > RENDER_DEPTH_MAX);

			FLOAT ScreenCoordX = EMath::Clamp(TempPoint.x / TempPoint.w, -1.f, 1.f);
			FLOAT ScreenCoordY = EMath::Clamp(TempPoint.y / TempPoint.w, -1.f, 1.f);

			Vector2 ViewPortPointMul(InViewport.Width, InViewport.Height);
			Vector2 ViewPortPointAdd(InViewport.TopLeftX, InViewport.TopLeftY);

			Vector2 Result(ScreenCoordX, ScreenCoordY);
			if (TempPoint.z < 0.f)
			{
				Result *= -1.f;
			}
			OutPositionSS = (Result * PointScl + PointAdd) * ViewPortPointMul + ViewPortPointAdd;

			return (!bIfOutScreen);
		}
		BOOL TransformWorldPointToScreen(const EViewport& InViewport, const Vector3& InPositionWS, Vector3& OutPositionSS)const
		{
			const Vector2 PointScl(0.5f, -0.5f);
			const Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionWS.x, InPositionWS.y, InPositionWS.z, 1.f);
			TempPoint = ViewProjectionMatrix.MultiplyVector(TempPoint);
			TempPoint = TempPoint / TempPoint.w;

			BOOL bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RENDER_DEPTH_MIN) ||
				(TempPoint.z > RENDER_DEPTH_MAX);

			const static Vector3 _StaticPointScl(PointScl.x, PointScl.y, 1.f);
			const static Vector3 _StaticPointAdd(PointAdd.x, PointAdd.y, 0.f);

			Vector3 ViewPortPointMul(InViewport.Width, InViewport.Height, 1.f);
			Vector3 ViewPortPointAdd(InViewport.TopLeftX, InViewport.TopLeftY, 0.f);

			Vector3 Result(TempPoint);
			OutPositionSS = (Result * _StaticPointScl + _StaticPointAdd) * ViewPortPointMul + ViewPortPointAdd;

			return (!bIfOutScreen);
		}
		BOOL TransformViewPointToScreen(const EViewport& InViewport, const Vector3& InPositionVS, Vector2& OutPositionSS)const
		{
			const Vector2 PointScl(0.5f, -0.5f);
			const Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionVS.x, InPositionVS.y, InPositionVS.z, 1.f);
			TempPoint = ProjectionPart.ProjectionMatrix.MultiplyVector(TempPoint);

			BOOL bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RENDER_DEPTH_MIN) ||
				(TempPoint.z > RENDER_DEPTH_MAX);

			FLOAT ScreenCoordX = EMath::Clamp(TempPoint.x / TempPoint.w, -1.f, 1.f);
			FLOAT ScreenCoordY = EMath::Clamp(TempPoint.y / TempPoint.w, -1.f, 1.f);

			Vector2 ViewPortPointMul(InViewport.Width, InViewport.Height);
			Vector2 ViewPortPointAdd(InViewport.TopLeftX, InViewport.TopLeftY);

			Vector2 Result(ScreenCoordX, ScreenCoordY);
			if (TempPoint.z < 0.f)
			{
				Result *= -1.f;
			}
			OutPositionSS = (Result * PointScl + PointAdd) * ViewPortPointMul + ViewPortPointAdd;

			return (!bIfOutScreen);
		}
		BOOL TransformViewPointToScreen(const EViewport& InViewport, const Vector3& InPositionVS, Vector3& OutPositionSS)const
		{
			const Vector2 PointScl(0.5f, -0.5f);
			const Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionVS.x, InPositionVS.y, InPositionVS.z, 1.f);
			TempPoint = ProjectionPart.ProjectionMatrix.MultiplyVector(TempPoint);
			TempPoint = TempPoint / TempPoint.w;

			BOOL bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RENDER_DEPTH_MIN) ||
				(TempPoint.z > RENDER_DEPTH_MAX);

			const static Vector3 _StaticPointScl(PointScl.x, PointScl.y, 1.f);
			const static Vector3 _StaticPointAdd(PointAdd.x, PointAdd.y, 0.f);

			Vector3 ViewPortPointMul(InViewport.Width, InViewport.Height, 1.f);
			Vector3 ViewPortPointAdd(InViewport.TopLeftX, InViewport.TopLeftY, 0.f);

			Vector3 Result(TempPoint);
			OutPositionSS = (Result * _StaticPointScl + _StaticPointAdd) * ViewPortPointMul + ViewPortPointAdd;

			return (!bIfOutScreen);
		}
		/*
		* Params input X Y : screen space position of the point (range : [0.f, screen max]).
		* Params output : output space direction from camera to point. WARNING! output direction is NOT normalized.
		*/
		void TransformScreenPointToWorld(const EViewport& InViewport, const Vector2& InPositionSS, Vector3& OutPositionWS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				1.f, 1.f);
			InverseViewProjectionMatrix.MultiplyVector(TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionWS = Vector3(TempPoint);
		}
		void TransformScreenPointToWorld(const EViewport& InViewport, const Vector3& InPositionSS, Vector3& OutPositionWS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				EMath::Clamp(InPositionSS.z, RENDER_DEPTH_MIN, RENDER_DEPTH_MAX), 1.f);
			InverseViewProjectionMatrix.MultiplyVector(TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionWS = Vector3(TempPoint);
		}
		void TransformScreenPointToView(const EViewport& InViewport, const Vector2& InPositionSS, Vector3& OutPositionVS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				1.f, 1.f);
			ProjectionPart.InverseProjectionMatrix.MultiplyVector(TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionVS = Vector3(TempPoint);
		}
		void TransformScreenPointToView(const EViewport& InViewport, const Vector3& InPositionSS, Vector3& OutPositionVS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				EMath::Clamp(InPositionSS.z, RENDER_DEPTH_MIN, RENDER_DEPTH_MAX), 1.f);
			ProjectionPart.InverseProjectionMatrix.MultiplyVector(TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionVS = Vector3(TempPoint);
		}
		void GenerateViewPart(const Vector3& InCameraWorldLocation, const Quaternion& InCameraWorldRotation)
		{
			ViewPart.InverseViewMatrix = Matrix4x4(InCameraWorldLocation, InCameraWorldRotation);
			ViewPart.ViewMatrix = ViewPart.InverseViewMatrix.Inverse();
		}
		void GenerateProjectPart(const EViewport& InViewport, FLOAT InFovAngleY, FLOAT InNearDist, FLOAT InFarDist)
		{
			FLOAT ViewportW = InViewport.Width;
			FLOAT ViewportH = InViewport.Height;
			FLOAT AspectRatio = ViewportW / ViewportH;

			ProjectionPart.ProjectionMatrix = Matrix4x4::PerspectiveMatrix(InFovAngleY, AspectRatio, InNearDist, InFarDist);
			ProjectionPart.InverseProjectionMatrix = ProjectionPart.ProjectionMatrix.Inverse();

			FLOAT DepthAdd = ProjectionPart.ProjectionMatrix._33;
			FLOAT DepthMul = ProjectionPart.ProjectionMatrix._43;
			if (EMath::Abs(DepthMul) < 1e-6f)
			{
				DepthMul = 1e-6f;
			}
			DepthMul = 1.f / DepthMul;
			DepthAdd = -DepthAdd * DepthMul;
			ProjectionPart.DeviceZToViewZMulAdd = Vector2(DepthMul, DepthAdd);

			//TODO
			//Vector4 ViewportSizeAndInvSize(ViewportW, ViewportH, 1.f / ViewportW, 1.f / ViewportH);
			//Vector2 ViewportMinSize(InViewport.TopLeftX, InViewport.TopLeftY);
		}
		void GenerateFinalMatrix()
		{
			ViewProjectionMatrix = ViewPart.ViewMatrix * ProjectionPart.ProjectionMatrix;
			InverseViewProjectionMatrix = ViewProjectionMatrix.Inverse();
		}

		EViewPart		ViewPart;
		EProjectionPart	ProjectionPart;
		Matrix4x4		ViewProjectionMatrix;
		Matrix4x4		InverseViewProjectionMatrix;
	};

};