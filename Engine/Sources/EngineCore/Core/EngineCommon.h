#pragma once

#include <CoreMinimal.h>
#include <Config/EngineConfig.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	class EResourceInterface
	{
	public:
		virtual BOOL32 IsResourceValid()const = 0;
		virtual BOOL32 InitResource() = 0;
		virtual void ReleaseResource() = 0;
	public:
		EResourceInterface() = default;
		EResourceInterface(const EResourceInterface&) = default;
		virtual ~EResourceInterface() = default;
		EResourceInterface& operator=(const EResourceInterface&) = default;
	};
	enum EAnimationType : UINT8
	{
		ANIMATION_TYPE_UNKNOWN	= 0,
		ANIMATION_TYPE_SKELETON,
		ANIMATION_TYPE_COUNT
	};
	template<typename TValueType, typename TTimeType>
	struct ETimeKey
	{
		ETimeKey() : Time(TTimeType()), Value(TValueType()) {}
		ETimeKey(const ETimeKey& Other) : Time(Other.Time), Value(Other.Value) {}
		ETimeKey(const TTimeType& InTime, const TValueType& InValue) : Time(InTime), Value(InValue) {}
		ETimeKey& operator=(const ETimeKey& Other)
		{
			Time	= Other.Time;
			Value	= Other.Value;
			return (*this);
		}

		TTimeType	Time;
		TValueType	Value;
	};
	template<typename TValueType, typename TTimeType>
	BOOL32 operator>(const ETimeKey<TValueType, TTimeType>& InKey0, const ETimeKey<TValueType, TTimeType>& InKey1)
	{
		return (InKey0.Time > InKey1.Time);
	}
	template<typename TValueType, typename TTimeType>
	BOOL32 operator<(const ETimeKey<TValueType, TTimeType>& InKey0, const ETimeKey<TValueType, TTimeType>& InKey1)
	{
		return (InKey0.Time < InKey1.Time);
	}
	template<typename TValueType, typename TTimeType>
	BOOL32 operator>=(const ETimeKey<TValueType, TTimeType>& InKey0, const ETimeKey<TValueType, TTimeType>& InKey1)
	{
		return (InKey0.Time >= InKey1.Time);
	}
	template<typename TValueType, typename TTimeType>
	BOOL32 operator<=(const ETimeKey<TValueType, TTimeType>& InKey0, const ETimeKey<TValueType, TTimeType>& InKey1)
	{
		return (InKey0.Time <= InKey1.Time);
	}
	template<typename TTimeType>
	struct ETransformTimeCurve
	{
		using TPositionKey	= ETimeKey<Vector3, TTimeType>;
		using TRotationKey	= ETimeKey<Quaternion, TTimeType>;
		using TScalingKey	= ETimeKey<Vector3, TTimeType>;

		ETransformTimeCurve() = default;
		ETransformTimeCurve(const ETransformTimeCurve& Other)
		{
			CopyFromOtherCurve(Other);
		}
		ETransformTimeCurve& operator=(const ETransformTimeCurve& Other)
		{
			if (PositionKeys.Length() > 0u)
			{
				PositionKeys.Clear();
			}
			if (RotationKeys.Length() > 0u)
			{
				RotationKeys.Clear();
			}
			if (ScalingKeys.Length() > 0u)
			{
				ScalingKeys.Clear();
			}
			CopyFromOtherCurve(Other);
			return (*this);
		}
		void CopyFromOtherCurve(const ETransformTimeCurve& Other)
		{
			if (Other.PositionKeys.Num() > 0)
			{
				for (INT32 i = 0, n = Other.PositionKeys.Num(); i < n; i++)
				{
					PositionKeys.Add(Other.PositionKeys[i]);
				}
			}
			if (Other.RotationKeys.Num() > 0)
			{
				for (INT32 i = 0, n = Other.RotationKeys.Num(); i < n; i++)
				{
					RotationKeys.Add(Other.RotationKeys[i]);
				}
			}
			if (Other.ScalingKeys.Num() > 0)
			{
				for (INT32 i = 0, n = Other.ScalingKeys.Num(); i < n; i++)
				{
					ScalingKeys.Add(Other.ScalingKeys[i]);
				}
			}
		}

		TArray<TPositionKey>	PositionKeys;
		TArray<TRotationKey>	RotationKeys;
		TArray<TScalingKey>		ScalingKeys;
	};
	struct EBoundAABB
	{
		EBoundAABB()noexcept : AABBMin(Vector3(-0.5f, -0.5f, -0.5f)), AABBMax(Vector3(0.5f, 0.5f, 0.5f)), IsValid(TRUE) {}
		EBoundAABB(const EBoundAABB& Other)noexcept : AABBMin(Other.AABBMin), AABBMax(Other.AABBMax), IsValid(Other.IsValid)  {}
		EBoundAABB(const Vector3& Min, const Vector3& Max, const BOOL32& Valid) noexcept :  AABBMin(Min), AABBMax(Max), IsValid(Valid)  {}
		EBoundAABB& operator+=(const EBoundAABB& Other)
		{
			if (IsValid)
			{
				AABBMin = Vector3(EMath::Min(AABBMin.x, Other.AABBMin.x), EMath::Min(AABBMin.y, Other.AABBMin.y), EMath::Min(AABBMin.z, Other.AABBMin.z));
				AABBMax = Vector3(EMath::Max(AABBMin.x, Other.AABBMin.x), EMath::Max(AABBMin.y, Other.AABBMin.y), EMath::Max(AABBMin.z, Other.AABBMin.z));
			}
			else if (Other.IsValid)
			{
				*this = Other;
			}
			return (*this);
		}
		EBoundAABB& operator=(const EBoundAABB& Other)
		{
			AABBMin = Other.AABBMin;
			AABBMax = Other.AABBMax;
			IsValid = Other.IsValid;
			return (*this);
		}
		void CalculateBoundWithSpace(const Vector3& InLocation, const Quaternion& InRotation, const Vector3& InScaling, TArray<Vector3>& OutPoints)
		{
			if (OutPoints.Num() != 8)
			{
				OutPoints.SetNum(8);
			}
			const Matrix4x4 SpaceMatrix(MakeMatrix4x4(InLocation, InRotation, InScaling));
			OutPoints[0] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMin.x, AABBMax.y, AABBMax.z));
			OutPoints[1] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMax.x, AABBMax.y, AABBMax.z));
			OutPoints[2] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMin.x, AABBMax.y, AABBMin.z));
			OutPoints[3] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMax.x, AABBMax.y, AABBMin.z));
			OutPoints[4] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMin.x, AABBMin.y, AABBMax.z));
			OutPoints[5] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMax.x, AABBMin.y, AABBMax.z));
			OutPoints[6] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMin.x, AABBMin.y, AABBMin.z));
			OutPoints[7] = Matrix4x4TransformPosition(SpaceMatrix, Vector3(AABBMax.x, AABBMin.y, AABBMin.z));
		}
		static void CalculateSeparatingProjectionWorldSpace(const TArray<Vector3>& InPoints, const TArray<Vector3>& InSeparatingAxis, TArray<Vector2>& OutProjections)
		{
			const UINT32 SeparatingAxisNum = InSeparatingAxis.Num<UINT32>();
			if (OutProjections.Num() > 0)
			{
				OutProjections.Empty();
			}
			if ((SeparatingAxisNum == 0u) || (InPoints.Num() != 8))
			{
				return;
			}

			Vector2 TempData;
			for (UINT32 SeparatingAxisIndex = 0u; SeparatingAxisIndex < SeparatingAxisNum; SeparatingAxisIndex++)
			{
				TempData.x = PE_FLOAT32_MAX;
				TempData.y = -PE_FLOAT32_MAX;
				for (UINT32 i = 0u; i < 8u; i++)
				{
					const FLOAT TempProjection = Vector3::Dot(InPoints[i], InSeparatingAxis[SeparatingAxisIndex]);
					TempData.x = EMath::Min(TempProjection, TempData.x);
					TempData.y = EMath::Max(TempProjection, TempData.y);
				}
				OutProjections.Add(TempData);
			}
		}
		static BOOL32 IsProjectionOverlap(const TArray<Vector2>& AProjections, const TArray<Vector2>& BProjections)
		{
			const UINT32 ProjectionNum = AProjections.Num<UINT32>();
			if ((ProjectionNum == 0u) || (ProjectionNum != BProjections.Num<UINT32>()))
			{
				return FALSE;
			}
			for (UINT32 i = 0u; i < ProjectionNum; i++)
			{
				if ((AProjections[i].x > BProjections[i].y) || (AProjections[i].y < BProjections[i].x))
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		EString AsString() const
		{
			return EString(EString("{ AABBMin : ") + ToString(AABBMin.x) + " " + ToString(AABBMin.y) + " " + ToString(AABBMin.z) + EString(" AABBMax : ") + ToString(AABBMax.x) + " " + ToString(AABBMax.y) + " " + ToString(AABBMax.z) + EString(" }"));
		}

		BOOL32		IsValid;
		Vector3		AABBMin;
		Vector3		AABBMax;
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

#pragma warning(push)
#pragma warning(disable : 26495)

	struct ERect
	{
		ERect()noexcept : Left(0.f), Top(0.f), Right(0.f), Bottom(0.f) {}
		ERect(const ERect& Other)noexcept : Left(Other.Left), Top(Other.Top), Right(Other.Right), Bottom(Other.Bottom) {}
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
			: PlaneTop(Vector4::Zero()), PlaneBottom(Vector4::Zero()), PlaneLeft(Vector4::Zero()), PlaneRight(Vector4::Zero())
			, FarPlaneTopLeft(Vector3::Zero()), FarPlaneTopRight(Vector3::Zero())
			, FarPlaneBottomLeft(Vector3::Zero()), FarPlaneBottomRight(Vector3::Zero())
			, NearPlaneTopLeft(Vector3::Zero()), NearPlaneTopRight(Vector3::Zero())
			, NearPlaneBottomLeft(Vector3::Zero()), NearPlaneBottomRight(Vector3::Zero())
			, SeparateProjectionX(Vector2::Zero()), SeparateProjectionY(Vector2::Zero()), SeparateProjectionZ(Vector2::Zero()) {}
		EFrustum(const EFrustum& Other)noexcept
			: PlaneTop(Other.PlaneTop), PlaneBottom(Other.PlaneBottom), PlaneLeft(Other.PlaneLeft), PlaneRight(Other.PlaneRight)
			, FarPlaneTopLeft(Other.FarPlaneTopLeft), FarPlaneTopRight(Other.FarPlaneTopRight)
			, FarPlaneBottomLeft(Other.FarPlaneBottomLeft), FarPlaneBottomRight(Other.FarPlaneBottomRight)
			, NearPlaneTopLeft(Other.NearPlaneTopLeft), NearPlaneTopRight(Other.NearPlaneTopRight)
			, NearPlaneBottomLeft(Other.NearPlaneBottomLeft), NearPlaneBottomRight(Other.NearPlaneBottomRight)
			, SeparateProjectionX(Other.SeparateProjectionX), SeparateProjectionY(Other.SeparateProjectionY), SeparateProjectionZ(Other.SeparateProjectionZ) {}
		EFrustum& operator=(const EFrustum& Other)
		{
			for (UINT32 i = 0u; i < 4u; i++)
			{
				Plane[i] = Other.Plane[i];
			}
			for (UINT32 i = 0u; i < 8u; i++)
			{
				FarNearPlanePoint[i] = Other.FarNearPlanePoint[i];
			}
			for (UINT32 i = 0u; i < 3u; i++)
			{
				SeparateProjection[i] = Other.SeparateProjection[i];
			}
			return (*this);
		}
		void GeneratePerspectiveFrustumInfo(FLOAT InFovAngleY, FLOAT InAspectRatio, FLOAT InNearDist, FLOAT InFarDist)
		{
			Vector3 TempPosVec[4] = { Vector3(0.f), Vector3(0.f), Vector3(0.f), Vector3(0.f) };
			{
				FLOAT SinHalfAngleY, CosHalfAngleY;
				EMath::SinCos(SinHalfAngleY, CosHalfAngleY, EMath::DegreesToRadians(0.5f * InFovAngleY));
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
				for (UINT32 i = 4u; i < 8u; i++)
				{
					FarNearPlanePoint[i] = TempPosVec[i - 4];
				}
				FLOAT t = InNearDist / InFarDist;
				for (UINT32 i = 0u; i < 4u; i++)
				{
					FarNearPlanePoint[i] = Vector3::Lerp(0.f, TempPosVec[i], t);
				}
			}
			{
				Vector3 TempPlaneNormal[4];
				for (UINT32 i = 0u; i < 4u; i++)
				{
					TempPosVec[i].Normalize();
				}
				TempPlaneNormal[0] = Vector3::Cross(TempPosVec[1], TempPosVec[0]);
				TempPlaneNormal[1] = Vector3::Cross(TempPosVec[2], TempPosVec[3]);
				TempPlaneNormal[2] = Vector3::Cross(TempPosVec[0], TempPosVec[2]);
				TempPlaneNormal[3] = Vector3::Cross(TempPosVec[3], TempPosVec[1]);
				for (UINT32 i = 0u; i < 4u; i++)
				{
					TempPlaneNormal[i].Normalize();
					Plane[i] = MakeVector4(TempPlaneNormal[i], 0.f);
				}
			}
		}
		void GenerateOrthographicFrustumInfo(FLOAT InLeft, FLOAT InTop, FLOAT InRight, FLOAT InBottom, FLOAT InMinDepth, FLOAT InMaxDepth)
		{
			{
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

				NearPlaneTopLeft = Vector3(InLeft, InTop, InMinDepth);
				NearPlaneTopRight = Vector3(InRight, InTop, InMinDepth);
				NearPlaneBottomLeft = Vector3(InLeft, InBottom, InMinDepth);
				NearPlaneBottomRight = Vector3(InRight, InBottom, InMinDepth);

				FarPlaneTopLeft = Vector3(InLeft, InTop, InMaxDepth);
				FarPlaneTopRight = Vector3(InRight, InTop, InMaxDepth);
				FarPlaneBottomLeft = Vector3(InLeft, InBottom, InMaxDepth);
				FarPlaneBottomRight = Vector3(InRight, InBottom, InMaxDepth);
			}
			{
				Vector3 TempPlaneNormal[4];
				TempPlaneNormal[0] = Vector3::Cross(NearPlaneTopRight - NearPlaneTopLeft, FarPlaneTopLeft - NearPlaneTopLeft);
				TempPlaneNormal[1] = Vector3::Cross(FarPlaneBottomLeft - NearPlaneBottomLeft, NearPlaneBottomRight - NearPlaneBottomLeft);
				TempPlaneNormal[2] = Vector3::Cross(FarPlaneTopLeft - NearPlaneTopLeft, NearPlaneBottomLeft - NearPlaneTopLeft);
				TempPlaneNormal[3] = Vector3::Cross(FarPlaneBottomRight - NearPlaneBottomRight, NearPlaneTopRight - NearPlaneBottomRight);
				for (UINT32 i = 0u; i < 4u; i++)
				{
					TempPlaneNormal[i].Normalize();
					Plane[i] = MakeVector4(TempPlaneNormal[i], 0.f);
				}
			}
		}
		void GeneratePlaneWorldSpace(const Vector3& InLocation, const Quaternion& InRotation, const Vector3& InScaling = Vector3::One())
		{
			{
				Matrix4x4 TempLocalToWorld(MakeMatrix4x4(InLocation, InRotation, InScaling));
				for (UINT32 i = 0u; i < 8u; i++)
				{
					FarNearPlanePoint[i] = Matrix4x4TransformPosition(TempLocalToWorld, FarNearPlanePoint[i]);
				}
			}
			{
				Vector3 TempPlaneNormal[4] =
				{
					Vector3(Plane[0].x, Plane[0].y, Plane[0].z),
					Vector3(Plane[1].x, Plane[1].y, Plane[1].z),
					Vector3(Plane[2].x, Plane[2].y, Plane[2].z),
					Vector3(Plane[3].x, Plane[3].y, Plane[3].z)
				};
				for (UINT32 i = 0u; i < 4u; i++)
				{
					TempPlaneNormal[i] = QuaternionTransformVector(InRotation, TempPlaneNormal[i]);
					TempPlaneNormal[i].Normalize();
				}
				Plane[0] = MakeVector4(TempPlaneNormal[0], Vector3::Dot(NearPlaneTopLeft, TempPlaneNormal[0]));
				Plane[1] = MakeVector4(TempPlaneNormal[0], Vector3::Dot(NearPlaneBottomRight, TempPlaneNormal[0]));
				Plane[2] = MakeVector4(TempPlaneNormal[0], Vector3::Dot(NearPlaneTopLeft, TempPlaneNormal[0]));
				Plane[3] = MakeVector4(TempPlaneNormal[0], Vector3::Dot(NearPlaneBottomRight, TempPlaneNormal[0]));
			}
		}
		void GenerateSeparatingProjectionWorldSpace()
		{
			Vector3 SeparateAxis[3] = { Vector3::XVector(), Vector3::YVector(), Vector3::ZVector() };
			for (UINT32 AxisIndex = 0u; AxisIndex < 3u; AxisIndex++)
			{
				SeparateProjection[AxisIndex].x = PE_FLOAT32_MAX;
				SeparateProjection[AxisIndex].y = -PE_FLOAT32_MAX;
				for (UINT32 i = 0u; i < 8u; i++)
				{
					const FLOAT TempProjection = Vector3::Dot(FarNearPlanePoint[i], SeparateAxis[AxisIndex]);
					SeparateProjection[AxisIndex].x = EMath::Min(TempProjection, SeparateProjection[AxisIndex].x);
					SeparateProjection[AxisIndex].y = EMath::Max(TempProjection, SeparateProjection[AxisIndex].y);
				}
			}
		}

		union
		{
			struct
			{
				Vector4	PlaneTop;
				Vector4	PlaneBottom;
				Vector4	PlaneLeft;
				Vector4	PlaneRight;
			};
			Vector4	Plane[4];
		};
		union
		{
			struct
			{
				Vector3	NearPlaneTopLeft;
				Vector3	NearPlaneTopRight;
				Vector3	NearPlaneBottomLeft;
				Vector3	NearPlaneBottomRight;
				Vector3	FarPlaneTopLeft;
				Vector3	FarPlaneTopRight;
				Vector3	FarPlaneBottomLeft;
				Vector3	FarPlaneBottomRight;
			};
			Vector3	FarNearPlanePoint[8];
		};
		union
		{
			struct
			{
				Vector2	SeparateProjectionX;
				Vector2	SeparateProjectionY;
				Vector2	SeparateProjectionZ;
			};
			Vector2	SeparateProjection[3];
		};
	};

#pragma warning(pop)

	struct EViewport
	{
		EViewport() noexcept : TopLeftX(0.f), TopLeftY(0.f), Width(0.f), Height(0.f), MinDepth(RCommonSettings::RENDER_DEPTH_MIN), MaxDepth(RCommonSettings::RENDER_DEPTH_MAX) {}
		EViewport(const EViewport& Other) noexcept : TopLeftX(Other.TopLeftX), TopLeftY(Other.TopLeftY), Width(Other.Width), Height(Other.Height), MinDepth(Other.MinDepth), MaxDepth(Other.MaxDepth) {}
		constexpr EViewport(const Vector2& startPos, const Vector2& rectSize, const Vector2& depthSize) noexcept : TopLeftX(startPos.x), TopLeftY(startPos.y), Width(rectSize.x), Height(rectSize.y), MinDepth(depthSize.x), MaxDepth(depthSize.y) {}
		EViewport& operator=(const EViewport& Other)
		{
			TopLeftX	= Other.TopLeftX;
			TopLeftY	= Other.TopLeftY;
			Width		= Other.Width;
			Height		= Other.Height;
			MinDepth	= Other.MinDepth;
			MaxDepth	= Other.MaxDepth;
			return (*this);
		}

		FLOAT	TopLeftX;
		FLOAT	TopLeftY;
		FLOAT	Width;
		FLOAT	Height;
		FLOAT	MinDepth;
		FLOAT	MaxDepth;
	};
	struct EViewMatrix
	{
		struct EViewPart
		{
			EViewPart()noexcept : ViewMatrix(Matrix4x4::Identity()), InverseViewMatrix(Matrix4x4::Identity()) {}
			EViewPart(const EViewPart& Other)noexcept : ViewMatrix(Other.ViewMatrix), InverseViewMatrix(Other.InverseViewMatrix) {}
			EViewPart& operator=(const EViewPart& Other)
			{
				ViewMatrix			= Other.ViewMatrix;
				InverseViewMatrix	= Other.InverseViewMatrix;
				return (*this);
			}

			Matrix4x4	ViewMatrix;
			Matrix4x4	InverseViewMatrix;
		};
		struct EProjectionPart
		{
			EProjectionPart()noexcept : ProjectionMatrix(Matrix4x4::Identity()), InverseProjectionMatrix(Matrix4x4::Identity()), DeviceZToViewZMulAdd(Vector2::XVector()) {}
			EProjectionPart(const EProjectionPart& Other)noexcept : ProjectionMatrix(Other.ProjectionMatrix), InverseProjectionMatrix(Other.InverseProjectionMatrix), DeviceZToViewZMulAdd(Other.DeviceZToViewZMulAdd) {}
			EProjectionPart& operator=(const EProjectionPart& Other)
			{
				ProjectionMatrix		= Other.ProjectionMatrix;
				InverseProjectionMatrix	= Other.InverseProjectionMatrix;
				DeviceZToViewZMulAdd	= Other.DeviceZToViewZMulAdd;
				return (*this);
			}

			Matrix4x4	ProjectionMatrix;
			Matrix4x4	InverseProjectionMatrix;
			Vector2		DeviceZToViewZMulAdd;
		};
		EViewMatrix()noexcept : ViewProjectionMatrix(Matrix4x4::Identity()), InverseViewProjectionMatrix(Matrix4x4::Identity()) {}
		EViewMatrix(const EViewMatrix& Other)noexcept : ViewPart(Other.ViewPart), ProjectionPart(Other.ProjectionPart), ViewProjectionMatrix(Other.ViewProjectionMatrix), InverseViewProjectionMatrix(Other.InverseViewProjectionMatrix) {}
		EViewMatrix& operator=(const EViewMatrix& Other)
		{
			ViewPart					= Other.ViewPart;
			ProjectionPart				= Other.ProjectionPart;
			ViewProjectionMatrix		= Other.ViewProjectionMatrix;
			InverseViewProjectionMatrix	= Other.InverseViewProjectionMatrix;
			return (*this);
		}
		void TransformWorldPointToView(const Vector3& InPositionWS, Vector3& OutPositionVS)const
		{
			OutPositionVS = Matrix4x4TransformPosition(ViewPart.ViewMatrix, InPositionWS);
		}
		void TransformViewPointToWorld(const Vector3& InPositionVS, Vector3& OutPositionWS)const
		{
			OutPositionWS = Matrix4x4TransformPosition(ViewPart.InverseViewMatrix, InPositionVS);
		}
		/*
		* Return value : if return false, the point is out of screen coord, else in screen.
		* Params InPosition[SpaceName] : specific space position of the point.
		* Params output : [0.f, screen max] range of screen coord which has anchor(zero point) in left top of RECT.
		*/
		BOOL32 TransformWorldPointToScreen(const EViewport& InViewport, const Vector3& InPositionWS, Vector2& OutPositionSS)const
		{
			const Vector2 PointScl(0.5f, -0.5f);
			const Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionWS.x, InPositionWS.y, InPositionWS.z, 1.f);
			TempPoint = Matrix4x4TransformVector(ViewProjectionMatrix, TempPoint);

			BOOL32 bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RCommonSettings::RENDER_DEPTH_MIN) ||
				(TempPoint.z > RCommonSettings::RENDER_DEPTH_MAX);

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
		BOOL32 TransformWorldPointToScreen(const EViewport& InViewport, const Vector3& InPositionWS, Vector3& OutPositionSS)const
		{
			PE_CONSTEXPR_STATIC Vector2 PointScl(0.5f, -0.5f);
			PE_CONSTEXPR_STATIC Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionWS.x, InPositionWS.y, InPositionWS.z, 1.f);
			TempPoint = Matrix4x4TransformVector(ViewProjectionMatrix, TempPoint);
			TempPoint = TempPoint / TempPoint.w;

			BOOL32 bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RCommonSettings::RENDER_DEPTH_MIN) ||
				(TempPoint.z > RCommonSettings::RENDER_DEPTH_MAX);

			PE_CONSTEXPR_STATIC Vector3 _StaticPointScl(PointScl.x, PointScl.y, 1.f);
			PE_CONSTEXPR_STATIC Vector3 _StaticPointAdd(PointAdd.x, PointAdd.y, 0.f);

			Vector3 ViewPortPointMul(InViewport.Width, InViewport.Height, 1.f);
			Vector3 ViewPortPointAdd(InViewport.TopLeftX, InViewport.TopLeftY, 0.f);

			Vector3 Result(TempPoint.x, TempPoint.y, TempPoint.z);
			OutPositionSS = (Result * _StaticPointScl + _StaticPointAdd) * ViewPortPointMul + ViewPortPointAdd;

			return (!bIfOutScreen);
		}
		BOOL32 TransformViewPointToScreen(const EViewport& InViewport, const Vector3& InPositionVS, Vector2& OutPositionSS)const
		{
			const Vector2 PointScl(0.5f, -0.5f);
			const Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionVS.x, InPositionVS.y, InPositionVS.z, 1.f);
			TempPoint = Matrix4x4TransformVector(ProjectionPart.ProjectionMatrix, TempPoint);

			BOOL32 bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RCommonSettings::RENDER_DEPTH_MIN) ||
				(TempPoint.z > RCommonSettings::RENDER_DEPTH_MAX);

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
		BOOL32 TransformViewPointToScreen(const EViewport& InViewport, const Vector3& InPositionVS, Vector3& OutPositionSS)const
		{
			PE_CONSTEXPR_STATIC Vector2 PointScl(0.5f, -0.5f);
			PE_CONSTEXPR_STATIC Vector2 PointAdd(0.5f, 0.5f);

			Vector4 TempPoint(InPositionVS.x, InPositionVS.y, InPositionVS.z, 1.f);
			TempPoint = Matrix4x4TransformVector(ProjectionPart.ProjectionMatrix, TempPoint);
			TempPoint = TempPoint / TempPoint.w;

			BOOL32 bIfOutScreen =
				(TempPoint.x < -1.f) ||
				(TempPoint.x > 1.f) ||
				(TempPoint.y < -1.f) ||
				(TempPoint.y > 1.f) ||
				(TempPoint.z < RCommonSettings::RENDER_DEPTH_MIN) ||
				(TempPoint.z > RCommonSettings::RENDER_DEPTH_MAX);

			PE_CONSTEXPR_STATIC Vector3 _StaticPointScl(PointScl.x, PointScl.y, 1.f);
			PE_CONSTEXPR_STATIC Vector3 _StaticPointAdd(PointAdd.x, PointAdd.y, 0.f);

			Vector3 ViewPortPointMul(InViewport.Width, InViewport.Height, 1.f);
			Vector3 ViewPortPointAdd(InViewport.TopLeftX, InViewport.TopLeftY, 0.f);

			Vector3 Result(TempPoint.x, TempPoint.y, TempPoint.z);
			OutPositionSS = (Result * _StaticPointScl + _StaticPointAdd) * ViewPortPointMul + ViewPortPointAdd;

			return (!bIfOutScreen);
		}
		/*
		* Params input X Y : screen space position of the point (range : [0.f, screen max]).
		* Params output : output space direction from view to point. WARNING! output direction is NOT normalized.
		*/
		void TransformScreenPointToWorld(const EViewport& InViewport, const Vector2& InPositionSS, Vector3& OutPositionWS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				1.f, 1.f);
			TempPoint = Matrix4x4TransformVector(InverseViewProjectionMatrix, TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionWS = Vector3(TempPoint.x, TempPoint.y, TempPoint.z);
		}
		void TransformScreenPointToWorld(const EViewport& InViewport, const Vector3& InPositionSS, Vector3& OutPositionWS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				EMath::Clamp(InPositionSS.z, RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX), 1.f);
			TempPoint = Matrix4x4TransformVector(InverseViewProjectionMatrix, TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionWS = Vector3(TempPoint.x, TempPoint.y, TempPoint.z);
		}
		void TransformScreenPointToView(const EViewport& InViewport, const Vector2& InPositionSS, Vector3& OutPositionVS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				1.f, 1.f);
			TempPoint = Matrix4x4TransformVector(ProjectionPart.InverseProjectionMatrix, TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionVS = Vector3(TempPoint.x, TempPoint.y, TempPoint.z);
		}
		void TransformScreenPointToView(const EViewport& InViewport, const Vector3& InPositionSS, Vector3& OutPositionVS)const
		{
			Vector4 TempPoint(
				((InPositionSS.x - InViewport.TopLeftX) / InViewport.Width - 0.5f) * 2.f,
				((InPositionSS.y - InViewport.TopLeftY) / InViewport.Height - 0.5f) * -2.f,
				EMath::Clamp(InPositionSS.z, RCommonSettings::RENDER_DEPTH_MIN, RCommonSettings::RENDER_DEPTH_MAX), 1.f);
			TempPoint = Matrix4x4TransformVector(ProjectionPart.InverseProjectionMatrix, TempPoint);
			TempPoint /= TempPoint.w;
			OutPositionVS = Vector3(TempPoint.x, TempPoint.y, TempPoint.z);
		}
		void GenerateViewPart(const Vector3& InViewWorldLocation, const Quaternion& InViewWorldRotation)
		{
			ViewPart.InverseViewMatrix = MakeMatrix4x4(InViewWorldLocation, InViewWorldRotation);
			ViewPart.ViewMatrix = ViewPart.InverseViewMatrix.Inverse();
		}
		void GeneratePerspectiveProjectPart(const EViewport& InViewport, FLOAT InFovAngleY, FLOAT InNearDist, FLOAT InFarDist)
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
		void GenerateOrthographicProjectPart(const EViewport& InViewport, FLOAT InMinDepth, FLOAT InMaxDepth)
		{
			ProjectionPart.ProjectionMatrix = Matrix4x4::OrthographicMatrix(InViewport.TopLeftX, InViewport.TopLeftY, InViewport.TopLeftX + InViewport.Width, InViewport.TopLeftY + InViewport.Height, InMinDepth, InMaxDepth);
			ProjectionPart.InverseProjectionMatrix = ProjectionPart.ProjectionMatrix.Inverse();

			//TODO Orthographic projection can not use far or near directly.
			ProjectionPart.DeviceZToViewZMulAdd = Vector2(0.0f, 0.0f);
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
	struct EViewDomainInfo
	{
		EViewDomainInfo() = default;
		EViewDomainInfo(const EViewDomainInfo& Other) : RenderViewport(Other.RenderViewport), ViewMatrix(Other.ViewMatrix), ViewFrustum(Other.ViewFrustum) {}
		EViewDomainInfo& operator=(const EViewDomainInfo& Other)
		{
			RenderViewport	= Other.RenderViewport;
			ViewMatrix		= Other.ViewMatrix;
			ViewFrustum		= Other.ViewFrustum;
			return (*this);
		}

		EViewport		RenderViewport;
		EViewMatrix		ViewMatrix;
		EFrustum		ViewFrustum;
	};
	enum ELightType : UINT8
	{
		LIGHT_TYPE_DIRECTIONAL = 0,
		LIGHT_TYPE_POINT,
		LIGHT_TYPE_SPOT,
		LIGHT_TYPE_COUNT
	};
	struct ELightData
	{
		ELightData()noexcept : LightType(ELightType::LIGHT_TYPE_COUNT), LightColor(Color3::Black()), LightIntensity(1.f), CastShadow(FALSE), ShadowMapSize(Vector2Int(2, 2)) {}
		ELightData(const ELightData& Other)noexcept : LightType(Other.LightType), LightColor(Other.LightColor), LightIntensity(Other.LightIntensity), CastShadow(Other.CastShadow), ShadowMapSize(Other.ShadowMapSize) {}
		ELightData(ELightType	InLightType, FLOAT InColorR, FLOAT InColorG, FLOAT InColorB, FLOAT InIntensity, BOOL32 InCastShadow, INT32 InShadowMapWidth, INT32 InShadowMapHeight)noexcept : LightType(InLightType), LightColor(Color3(InColorR, InColorG, InColorB)), LightIntensity(InIntensity), CastShadow(InCastShadow), ShadowMapSize(Vector2Int(InShadowMapWidth, InShadowMapHeight)) {}
		ELightData& operator=(const ELightData& Other)
		{
			LightType		= Other.LightType;
			LightColor		= Other.LightColor;
			LightIntensity	= Other.LightIntensity;
			CastShadow		= Other.CastShadow;
			ShadowMapSize	= Other.ShadowMapSize;
			return (*this);
		}

		ELightType	LightType;
		Color3		LightColor;
		FLOAT		LightIntensity;
		BOOL32		CastShadow;
		Vector2Int	ShadowMapSize;
	};
	struct ECascadeShadowData
	{
		ECascadeShadowData() : IsUseShadow(FALSE) {}
		ECascadeShadowData(const ECascadeShadowData& Other)noexcept : IsUseShadow(Other.IsUseShadow), Layers(Other.Layers), Borders(Other.Borders) {}
		ECascadeShadowData(const BOOL32 InIsUseShadow, const FLOAT* InLayers, const FLOAT* InBorders, const UINT32 InCascadeNum)
			: IsUseShadow(InIsUseShadow)
		{
			if (!InIsUseShadow)
			{
				return;
			}
			PE_CHECK((ENGINE_WORLD_ERROR), ("Check CSM failed that layer or border or cascade num is invalid."), ((!!InLayers) && (!!InBorders) && (InCascadeNum > 0u)));
			for (UINT32 i = 0u; i < InCascadeNum; i++)
			{
#if _EDITOR_ONLY
				if (InLayers)
#endif
				{
					Layers.Add(InLayers[i]);
				}
				if (i < (InCascadeNum - 1u))
				{
#if _EDITOR_ONLY
					if (InBorders)
#endif
					{
						Borders.Add(InBorders[i]);
					}
				}
			}
		}
		ECascadeShadowData& operator=(const ECascadeShadowData& Other)
		{
			if (Layers.Num() > 0)
			{
				Layers.Empty();
			}
			if (Borders.Num() > 0)
			{
				Borders.Empty();
			}
			IsUseShadow	= Other.IsUseShadow;
			Layers		= Other.Layers;
			Borders		= Other.Borders;
			return (*this);
		}
		void Setup(const BOOL32 InIsUseShadow, const TArray<FLOAT>& InLayers, const TArray<FLOAT>& InBorders)
		{
			if (Layers.Num() > 0)
			{
				Layers.Empty();
			}
			if (Borders.Num() > 0)
			{
				Borders.Empty();
			}
			IsUseShadow = InIsUseShadow;
			if (!InIsUseShadow)
			{
				return;
			}
			PE_CHECK((ENGINE_WORLD_ERROR), ("Check CSM failed that layer or border or cascade num is invalid."), ((InLayers.Num() > 0) && (InLayers.Num() == (InLayers.Num() + 1))));
			Layers	= InLayers;
			Borders	= InBorders;
		}

		BOOL32			IsUseShadow;
		TArray<FLOAT>	Layers;
		TArray<FLOAT>	Borders;
	};

};