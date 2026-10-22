#include "BaseType.h"

#include "Text/String.h"
#include "Container/BitArray.h"

namespace PigeonEngine
{

	PE_INLINE Matrix4x4 MakeTranslationMatrix4x4(const Vector3& InTranslation) { return Matrix4x4(DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
	PE_INLINE Matrix4x4 MakeRotationMatrix4x4(const Quaternion& InRotation) { return Matrix4x4(InRotation.GetDirectXMatrix()); }
	PE_INLINE Matrix4x4 MakeScalingMatrix4x4(const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z)); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Euler& InEuler) { return Matrix4x4(DirectX::XMMatrixRotationRollPitchYaw(EMath::DegreesToRadians(InEuler.Pitch), EMath::DegreesToRadians(InEuler.Yaw), EMath::DegreesToRadians(InEuler.Roll))); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Quaternion& InRotation) { return Matrix4x4(InRotation.GetDirectXMatrix()); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Quaternion& InRotation) { return Matrix4x4(InRotation.GetDirectXMatrix() * DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z) * DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Quaternion& InRotation, const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z) * InRotation.GetDirectXMatrix()); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Quaternion& InRotation, const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z) * InRotation.GetDirectXMatrix() * DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InForwardVector, const Vector3& InUpVector, const Vector3& InRightVector, const Vector3& InTranslation)
	{
		return Matrix4x4(
			InRightVector.x, InRightVector.y, InRightVector.z, 0.f,
			InUpVector.x, InUpVector.y, InUpVector.z, 0.f,
			InForwardVector.x, InForwardVector.y, InForwardVector.z, 0.f,
			InTranslation.x, InTranslation.y, InTranslation.z, 1.f);
	}
	PE_INLINE Matrix4x4 InverseMatrix4x4(const Matrix4x4& InMatrix, Vector4* OutDeterminant)
	{
		if (OutDeterminant != nullptr)
		{
			DirectX::XMVECTOR TempVector = DirectX::XMVectorZero();
			Matrix4x4 Result(DirectX::XMMatrixInverse(&TempVector, InMatrix.GetDirectXMatrix()));
			(*OutDeterminant) = Vector4(TempVector);
			return Result;
		}
		return Matrix4x4(DirectX::XMMatrixInverse(nullptr, InMatrix.GetDirectXMatrix()));
	}
	PE_INLINE Vector4 Matrix4x4TransformVector(const Matrix4x4& m, const Vector4& v) { return Vector4(DirectX::XMVector4Transform(DirectX::XMVectorSet(v.x, v.y, v.z, v.w), m.GetDirectXMatrix())); }
	PE_INLINE Vector3 Matrix4x4TransformPosition(const Matrix4x4& m, const Vector3& v) { return Vector3(DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(v.x, v.y, v.z, 1.f), m.GetDirectXMatrix())); }
	PE_INLINE Vector3 Matrix4x4TransformDirection(const Matrix4x4& m, const Vector3& v) { return Vector3(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(v.x, v.y, v.z, 0.f), m.GetDirectXMatrix())); }
	PE_INLINE Quaternion MakeQuaternion(const Matrix4x4& m) { return Quaternion(m.GetDirectXMatrix()); }
	PE_INLINE Quaternion MakeQuaternion(const Vector4& v) { return Quaternion(v.x, v.y, v.z, v.w); }
	PE_INLINE Quaternion MakeQuaternion(const Vector3& InAxis, FLOAT InRadian) { return Quaternion(DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(InAxis.x, InAxis.y, InAxis.z, 0.f), InRadian)); }
	PE_INLINE Euler MakeEuler(const Vector3& v) { return Euler(v.x, v.y, v.z); }
	PE_INLINE Euler MakeEuler(const Quaternion& InQuat)
	{
		Euler RotatorFromQuat(Euler::Zero());

		const FLOAT xx = InQuat.x * InQuat.x;
		const FLOAT yy = InQuat.y * InQuat.y;
		const FLOAT zz = InQuat.z * InQuat.z;

		const FLOAT m31 = 2.f * InQuat.x * InQuat.z + 2.f * InQuat.y * InQuat.w;
		const FLOAT m32 = 2.f * InQuat.y * InQuat.z - 2.f * InQuat.x * InQuat.w;
		const FLOAT m33 = 1.f - 2.f * xx - 2.f * yy;

		const FLOAT cy = sqrtf(m33 * m33 + m31 * m31);
		const FLOAT cx = atan2f(-m32, cy);

		if (cy > 16.f * PE_FLOAT32_EPSILON)
		{
			const FLOAT m12 = 2.f * InQuat.x * InQuat.y + 2.f * InQuat.z * InQuat.w;
			const FLOAT m22 = 1.f - 2.f * xx - 2.f * zz;

			RotatorFromQuat.Pitch = EMath::RadiansToDegrees(cx);
			RotatorFromQuat.Yaw = EMath::RadiansToDegrees(atan2f(m31, m33));
			RotatorFromQuat.Roll = EMath::RadiansToDegrees(atan2f(m12, m22));
		}
		else
		{
			const FLOAT m11 = 1.f - 2.f * yy - 2.f * zz;
			const FLOAT m21 = 2.f * InQuat.x * InQuat.y - 2.f * InQuat.z * InQuat.w;

			RotatorFromQuat.Pitch = EMath::RadiansToDegrees(cx);
			RotatorFromQuat.Yaw = 0.f;
			RotatorFromQuat.Roll = EMath::RadiansToDegrees(atan2f(-m21, m11));
		}
#if _DEBUG_MODE
		if (RotatorFromQuat.IsContainNaN())
		{
			RotatorFromQuat = Euler::Zero();
		}
#endif
		return RotatorFromQuat;
	}
	PE_INLINE Quaternion MakeQuaternion(const Euler& InEuler) { return Quaternion(EMath::DegreesToRadians(InEuler.Pitch), EMath::DegreesToRadians(InEuler.Yaw), EMath::DegreesToRadians(InEuler.Roll)); }
	PE_INLINE Quaternion MakeQuaternion(const Vector3& InFromVec, const Vector3& InToVec)
	{
		Vector3 Axis = Vector3::Cross(InFromVec, InToVec);
		const FLOAT AxisLength = Axis.Length();
		if (AxisLength <= 1e-3f)
		{
			return Quaternion::Identity();
		}
		Axis = Axis / AxisLength;
		const FLOAT CosRad = Vector3::Dot(InFromVec, InToVec);
		return MakeQuaternion(Axis, EMath::ACos(CosRad));
	}
	PE_INLINE Vector3 QuaternionTransformVector(const Quaternion& q, const Vector3& v) { return Vector3(DirectX::XMVector3Rotate(DirectX::XMVectorSet(v.x, v.y, v.z, 0.f), DirectX::XMVectorSet(q.x, q.y, q.z, q.w))); }
	PE_INLINE Vector3 SplitForwardVector(const Matrix4x4& InMat) { return Vector3(InMat._31, InMat._32, InMat._33); }
	PE_INLINE Vector3 SplitUpVector(const Matrix4x4& InMat) { return Vector3(InMat._21, InMat._22, InMat._23); }
	PE_INLINE Vector3 SplitRightVector(const Matrix4x4& InMat) { return Vector3(InMat._11, InMat._12, InMat._13); }
	PE_INLINE Vector3 SplitForwardVector(const Quaternion& InQuat) { return QuaternionTransformVector(InQuat, Vector3::ZVector()); }
	PE_INLINE Vector3 SplitUpVector(const Quaternion& InQuat) { return QuaternionTransformVector(InQuat, Vector3::YVector()); }
	PE_INLINE Vector3 SplitRightVector(const Quaternion& InQuat) { return QuaternionTransformVector(InQuat, Vector3::XVector()); }
	PE_INLINE Vector3 MakeVector3(const Euler& InEuler) { return Vector3(InEuler.Pitch, InEuler.Yaw, InEuler.Roll); }
	PE_INLINE Vector4 MakeVector4(const Vector3& v, FLOAT w) { return Vector4(v.x, v.y, v.z, w); }
	PE_INLINE Color3 MakeColor3(const Color4& c) { return Color3(c.x, c.y, c.z); }
	PE_INLINE Color4 MakeColor4(const Color3& c) { return Color4(c.x, c.y, c.z, 1.f); }
	PE_INLINE Vector2 MinVector2(const Vector2& A, const Vector2& B)
	{
		return Vector2(
			EMath::Min(A.x, B.x),
			EMath::Min(A.y, B.y));
	}
	PE_INLINE Vector3 MinVector3(const Vector3& A, const Vector3& B)
	{
		return Vector3(
			EMath::Min(A.x, B.x),
			EMath::Min(A.y, B.y),
			EMath::Min(A.z, B.z));
	}
	PE_INLINE Vector4 MinVector4(const Vector4& A, const Vector4& B)
	{
		return Vector4(
			EMath::Min(A.x, B.x),
			EMath::Min(A.y, B.y),
			EMath::Min(A.z, B.z),
			EMath::Min(A.w, B.w));
	}
	PE_INLINE Vector2Int MinVector2Int(const Vector2Int& A, const Vector2Int& B)
	{
		return Vector2Int(
			EMath::Min(A.x, B.x),
			EMath::Min(A.y, B.y));
	}
	PE_INLINE Vector3Int MinVector3Int(const Vector3Int& A, const Vector3Int& B)
	{
		return Vector3Int(
			EMath::Min(A.x, B.x),
			EMath::Min(A.y, B.y),
			EMath::Min(A.z, B.z));
	}
	PE_INLINE Vector4Int MinVector4Int(const Vector4Int& A, const Vector4Int& B)
	{
		return Vector4Int(
			EMath::Min(A.x, B.x),
			EMath::Min(A.y, B.y),
			EMath::Min(A.z, B.z),
			EMath::Min(A.w, B.w));
	}
	PE_INLINE Vector2 MaxVector2(const Vector2& A, const Vector2& B)
	{
		return Vector2(
			EMath::Max(A.x, B.x),
			EMath::Max(A.y, B.y));
	}
	PE_INLINE Vector3 MaxVector3(const Vector3& A, const Vector3& B)
	{
		return Vector3(
			EMath::Max(A.x, B.x),
			EMath::Max(A.y, B.y),
			EMath::Max(A.z, B.z));
	}
	PE_INLINE Vector4 MaxVector4(const Vector4& A, const Vector4& B)
	{
		return Vector4(
			EMath::Max(A.x, B.x),
			EMath::Max(A.y, B.y),
			EMath::Max(A.z, B.z),
			EMath::Max(A.w, B.w));
	}
	PE_INLINE Vector2Int MaxVector2Int(const Vector2Int& A, const Vector2Int& B)
	{
		return Vector2Int(
			EMath::Max(A.x, B.x),
			EMath::Max(A.y, B.y));
	}
	PE_INLINE Vector3Int MaxVector3Int(const Vector3Int& A, const Vector3Int& B)
	{
		return Vector3Int(
			EMath::Max(A.x, B.x),
			EMath::Max(A.y, B.y),
			EMath::Max(A.z, B.z));
	}
	PE_INLINE Vector4Int MaxVector4Int(const Vector4Int& A, const Vector4Int& B)
	{
		return Vector4Int(
			EMath::Max(A.x, B.x),
			EMath::Max(A.y, B.y),
			EMath::Max(A.z, B.z),
			EMath::Max(A.w, B.w));
	}
	PE_INLINE Matrix4x4 LookAtTargetMatrixRotation(const Vector3& InTargetLocation, const Vector3& InViewLocation, const Vector3& InViewUpDirection)
	{
		Vector3 TempForward(InTargetLocation - InViewLocation);
		TempForward.Normalize();
#if _EDITOR_ONLY
		Check((!((Vector3::Dot(TempForward, InViewUpDirection) > 0.95f) || (Vector3::Dot(TempForward, InViewUpDirection) < -0.95f))), (ENGINE_MATH_ERROR));
#endif
		Vector3 TempRight(Vector3::Cross(InViewUpDirection, TempForward));
		TempRight.Normalize();

		Vector3 TempUp(Vector3::Cross(TempForward, TempRight));

		return MakeMatrix4x4(TempForward, TempUp, TempRight, Vector3::Zero());
	}
	PE_INLINE Quaternion LookAtTargetQuaternionRotation(const Vector3& InTargetLocation, const Vector3& InViewLocation, const Vector3& InViewUpDirection)
	{
		return MakeQuaternion(LookAtTargetMatrixRotation(InTargetLocation, InViewLocation, InViewUpDirection));
	}

	EString Vector3::AsString() const
	{
		return EString(ToString(x) + " " + ToString(y) + " " + ToString(z));
	}

	Matrix4x4 operator+(const Matrix4x4& lm, const Matrix4x4& rm)
	{
		return (Matrix4x4(
			lm._11 + rm._11, lm._12 + rm._12, lm._13 + rm._13, lm._14 + rm._14,
			lm._21 + rm._21, lm._22 + rm._22, lm._23 + rm._23, lm._24 + rm._24,
			lm._31 + rm._31, lm._32 + rm._32, lm._33 + rm._33, lm._34 + rm._34,
			lm._41 + rm._41, lm._42 + rm._42, lm._43 + rm._43, lm._44 + rm._44));
	}
	Matrix4x4 operator-(const Matrix4x4& lm, const Matrix4x4& rm)
	{
		return (Matrix4x4(
			lm._11 - rm._11, lm._12 - rm._12, lm._13 - rm._13, lm._14 - rm._14,
			lm._21 - rm._21, lm._22 - rm._22, lm._23 - rm._23, lm._24 - rm._24,
			lm._31 - rm._31, lm._32 - rm._32, lm._33 - rm._33, lm._34 - rm._34,
			lm._41 - rm._41, lm._42 - rm._42, lm._43 - rm._43, lm._44 - rm._44));
	}
	Matrix4x4 operator*(const Matrix4x4& lm, FLOAT rv)
	{
		return (Matrix4x4(
			lm._11 * rv, lm._12 * rv, lm._13 * rv, lm._14 * rv,
			lm._21 * rv, lm._22 * rv, lm._23 * rv, lm._24 * rv,
			lm._31 * rv, lm._32 * rv, lm._33 * rv, lm._34 * rv,
			lm._41 * rv, lm._42 * rv, lm._43 * rv, lm._44 * rv));
	}
	Matrix4x4 operator*(FLOAT lv, const Matrix4x4& rm)
	{
		return (Matrix4x4(
			lv * rm._11, lv * rm._12, lv * rm._13, lv * rm._14,
			lv * rm._21, lv * rm._22, lv * rm._23, lv * rm._24,
			lv * rm._31, lv * rm._32, lv * rm._33, lv * rm._34,
			lv * rm._41, lv * rm._42, lv * rm._43, lv * rm._44));
	}
	Matrix4x4 operator*(const Matrix4x4& lm, const Matrix4x4& rm) { return (Matrix4x4::MultiplyMatrix(lm, rm)); }
	Matrix4x4 operator*(const Matrix4x4& lm, const Quaternion& rq) { return (Matrix4x4::MultiplyMatrix(lm, rq.GetMatrix())); }
	Matrix4x4 operator*(const Quaternion& lq, const Matrix4x4& rm) { return (Matrix4x4::MultiplyMatrix(lq.GetMatrix(), rm)); }
	Quaternion operator*(const Quaternion& lq, const Quaternion& rq) { return (Quaternion::MultiplyQuaternion(lq, rq)); }
	Vector2 operator+(const Vector2& lv, const Vector2& rv) { return (Vector2(lv.x + rv.x, lv.y + rv.y)); }
	Vector2 operator+(const Vector2& lv, FLOAT rv) { return (Vector2(lv.x + rv, lv.y + rv)); }
	Vector2 operator+(FLOAT lv, const Vector2& rv) { return (Vector2(lv + rv.x, lv + rv.y)); }
	Vector2 operator-(const Vector2& lv, const Vector2& rv) { return (Vector2(lv.x - rv.x, lv.y - rv.y)); }
	Vector2 operator-(const Vector2& lv, FLOAT rv) { return (Vector2(lv.x - rv, lv.y - rv)); }
	Vector2 operator-(FLOAT lv, const Vector2& rv) { return (Vector2(lv - rv.x, lv - rv.y)); }
	Vector2 operator*(const Vector2& lv, const Vector2& rv) { return (Vector2(lv.x * rv.x, lv.y * rv.y)); }
	Vector2 operator*(const Vector2& lv, FLOAT rv) { return (Vector2(lv.x * rv, lv.y * rv)); }
	Vector2 operator*(FLOAT lv, const Vector2& rv) { return (Vector2(lv * rv.x, lv * rv.y)); }
	Vector2 operator/(const Vector2& lv, const Vector2& rv) { return (Vector2(lv.x / rv.x, lv.y / rv.y)); }
	Vector2 operator/(const Vector2& lv, FLOAT rv) { return (Vector2(lv.x / rv, lv.y / rv)); }
	Vector2 operator/(FLOAT lv, const Vector2& rv) { return (Vector2(lv / rv.x, lv / rv.y)); }
	Vector3 operator+(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Vector3 operator+(const Vector3& lv, FLOAT rv) { return (Vector3(lv.x + rv, lv.y + rv, lv.z + rv)); }
	Vector3 operator+(FLOAT lv, const Vector3& rv) { return (Vector3(lv + rv.x, lv + rv.y, lv + rv.z)); }
	Vector3 operator-(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Vector3 operator-(const Vector3& lv, FLOAT rv) { return (Vector3(lv.x - rv, lv.y - rv, lv.z - rv)); }
	Vector3 operator-(FLOAT lv, const Vector3& rv) { return (Vector3(lv - rv.x, lv - rv.y, lv - rv.z)); }
	Vector3 operator*(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Vector3 operator*(const Vector3& lv, FLOAT rv) { return (Vector3(lv.x * rv, lv.y * rv, lv.z * rv)); }
	Vector3 operator*(FLOAT lv, const Vector3& rv) { return (Vector3(lv * rv.x, lv * rv.y, lv * rv.z)); }
	Vector3 operator/(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Vector3 operator/(const Vector3& lv, FLOAT rv) { return (Vector3(lv.x / rv, lv.y / rv, lv.z / rv)); }
	Vector3 operator/(FLOAT lv, const Vector3& rv) { return (Vector3(lv / rv.x, lv / rv.y, lv / rv.z)); }
	Vector4 operator+(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Vector4 operator+(const Vector4& lv, FLOAT rv) { return (Vector4(lv.x + rv, lv.y + rv, lv.z + rv, lv.w + rv)); }
	Vector4 operator+(FLOAT lv, const Vector4& rv) { return (Vector4(lv + rv.x, lv + rv.y, lv + rv.z, lv + rv.w)); }
	Vector4 operator-(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Vector4 operator-(const Vector4& lv, FLOAT rv) { return (Vector4(lv.x - rv, lv.y - rv, lv.z - rv, lv.w - rv)); }
	Vector4 operator-(FLOAT lv, const Vector4& rv) { return (Vector4(lv - rv.x, lv - rv.y, lv - rv.z, lv - rv.w)); }
	Vector4 operator*(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Vector4 operator*(const Vector4& lv, FLOAT rv) { return (Vector4(lv.x * rv, lv.y * rv, lv.z * rv, lv.w * rv)); }
	Vector4 operator*(FLOAT lv, const Vector4& rv) { return (Vector4(lv * rv.x, lv * rv.y, lv * rv.z, lv * rv.w)); }
	Vector4 operator/(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Vector4 operator/(const Vector4& lv, FLOAT rv) { return (Vector4(lv.x / rv, lv.y / rv, lv.z / rv, lv.w / rv)); }
	Vector4 operator/(FLOAT lv, const Vector4& rv) { return (Vector4(lv / rv.x, lv / rv.y, lv / rv.z, lv / rv.w)); }
	Vector2Int operator+(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x + rv.x, lv.y + rv.y)); }
	Vector2Int operator+(const Vector2Int& lv, INT32 rv) { return (Vector2Int(lv.x + rv, lv.y + rv)); }
	Vector2Int operator+(INT32 lv, const Vector2Int& rv) { return (Vector2Int(lv + rv.x, lv + rv.y)); }
	Vector2Int operator-(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x - rv.x, lv.y - rv.y)); }
	Vector2Int operator-(const Vector2Int& lv, INT32 rv) { return (Vector2Int(lv.x - rv, lv.y - rv)); }
	Vector2Int operator-(INT32 lv, const Vector2Int& rv) { return (Vector2Int(lv - rv.x, lv - rv.y)); }
	Vector2Int operator*(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x * rv.x, lv.y * rv.y)); }
	Vector2Int operator*(const Vector2Int& lv, INT32 rv) { return (Vector2Int(lv.x * rv, lv.y * rv)); }
	Vector2Int operator*(INT32 lv, const Vector2Int& rv) { return (Vector2Int(lv * rv.x, lv * rv.y)); }
	Vector2Int operator/(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x / rv.x, lv.y / rv.y)); }
	Vector2Int operator/(const Vector2Int& lv, INT32 rv) { return (Vector2Int(lv.x / rv, lv.y / rv)); }
	Vector2Int operator/(INT32 lv, const Vector2Int& rv) { return (Vector2Int(lv / rv.x, lv / rv.y)); }
	Vector3Int operator+(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Vector3Int operator+(const Vector3Int& lv, INT32 rv) { return (Vector3Int(lv.x + rv, lv.y + rv, lv.z + rv)); }
	Vector3Int operator+(INT32 lv, const Vector3Int& rv) { return (Vector3Int(lv + rv.x, lv + rv.y, lv + rv.z)); }
	Vector3Int operator-(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Vector3Int operator-(const Vector3Int& lv, INT32 rv) { return (Vector3Int(lv.x - rv, lv.y - rv, lv.z - rv)); }
	Vector3Int operator-(INT32 lv, const Vector3Int& rv) { return (Vector3Int(lv - rv.x, lv - rv.y, lv - rv.z)); }
	Vector3Int operator*(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Vector3Int operator*(const Vector3Int& lv, INT32 rv) { return (Vector3Int(lv.x * rv, lv.y * rv, lv.z * rv)); }
	Vector3Int operator*(INT32 lv, const Vector3Int& rv) { return (Vector3Int(lv * rv.x, lv * rv.y, lv * rv.z)); }
	Vector3Int operator/(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Vector3Int operator/(const Vector3Int& lv, INT32 rv) { return (Vector3Int(lv.x / rv, lv.y / rv, lv.z / rv)); }
	Vector3Int operator/(INT32 lv, const Vector3Int& rv) { return (Vector3Int(lv / rv.x, lv / rv.y, lv / rv.z)); }
	Vector4Int operator+(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Vector4Int operator+(const Vector4Int& lv, INT32 rv) { return (Vector4Int(lv.x + rv, lv.y + rv, lv.z + rv, lv.w + rv)); }
	Vector4Int operator+(INT32 lv, const Vector4Int& rv) { return (Vector4Int(lv + rv.x, lv + rv.y, lv + rv.z, lv + rv.w)); }
	Vector4Int operator-(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Vector4Int operator-(const Vector4Int& lv, INT32 rv) { return (Vector4Int(lv.x - rv, lv.y - rv, lv.z - rv, lv.w - rv)); }
	Vector4Int operator-(INT32 lv, const Vector4Int& rv) { return (Vector4Int(lv - rv.x, lv - rv.y, lv - rv.z, lv - rv.w)); }
	Vector4Int operator*(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Vector4Int operator*(const Vector4Int& lv, INT32 rv) { return (Vector4Int(lv.x * rv, lv.y * rv, lv.z * rv, lv.w * rv)); }
	Vector4Int operator*(INT32 lv, const Vector4Int& rv) { return (Vector4Int(lv * rv.x, lv * rv.y, lv * rv.z, lv * rv.w)); }
	Vector4Int operator/(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Vector4Int operator/(const Vector4Int& lv, INT32 rv) { return (Vector4Int(lv.x / rv, lv.y / rv, lv.z / rv, lv.w / rv)); }
	Vector4Int operator/(INT32 lv, const Vector4Int& rv) { return (Vector4Int(lv / rv.x, lv / rv.y, lv / rv.z, lv / rv.w)); }
	Color3 operator+(const Color3& lv, FLOAT rv) { return (Color3(lv.x + rv, lv.y + rv, lv.z + rv)); }
	Color3 operator+(FLOAT lv, const Color3& rv) { return (Color3(lv + rv.x, lv + rv.y, lv + rv.z)); }
	Color3 operator+(const Color3& lv, const Color3& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator+(const Color3& lv, const Vector3& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator+(const Vector3& lv, const Color3& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color4 operator+(const Color4& lv, const Color4& rv) { return (Color4(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Color4 operator+(const Color4& lv, const Vector4& rv) { return (Color4(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Color4 operator+(const Vector4& lv, const Color4& rv) { return (Color4(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Color3 operator+(const Color3& lv, const Vector4& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator+(const Vector4& lv, const Color3& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator+(const Color4& lv, const Vector3& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator+(const Vector3& lv, const Color4& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator+(const Color4& lv, const Color3& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator+(const Color3& lv, const Color4& rv) { return (Color3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Color3 operator-(const Color3& lv, FLOAT rv) { return (Color3(lv.x - rv, lv.y - rv, lv.z - rv)); }
	Color3 operator-(FLOAT lv, const Color3& rv) { return (Color3(lv - rv.x, lv - rv.y, lv - rv.z)); }
	Color3 operator-(const Color3& lv, const Color3& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator-(const Color3& lv, const Vector3& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator-(const Vector3& lv, const Color3& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color4 operator-(const Color4& lv, const Color4& rv) { return (Color4(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Color4 operator-(const Color4& lv, const Vector4& rv) { return (Color4(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Color4 operator-(const Vector4& lv, const Color4& rv) { return (Color4(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Color3 operator-(const Color3& lv, const Vector4& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator-(const Vector4& lv, const Color3& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator-(const Color4& lv, const Vector3& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator-(const Vector3& lv, const Color4& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator-(const Color4& lv, const Color3& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator-(const Color3& lv, const Color4& rv) { return (Color3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Color3 operator*(const Color3& lv, FLOAT rv) { return (Color3(lv.x * rv, lv.y * rv, lv.z * rv)); }
	Color3 operator*(FLOAT lv, const Color3& rv) { return (Color3(lv * rv.x, lv * rv.y, lv * rv.z)); }
	Color3 operator*(const Color3& lv, const Color3& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator*(const Color3& lv, const Vector3& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator*(const Vector3& lv, const Color3& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color4 operator*(const Color4& lv, const Color4& rv) { return (Color4(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Color4 operator*(const Color4& lv, const Vector4& rv) { return (Color4(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Color4 operator*(const Vector4& lv, const Color4& rv) { return (Color4(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Color3 operator*(const Color3& lv, const Vector4& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator*(const Vector4& lv, const Color3& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator*(const Color4& lv, const Vector3& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator*(const Vector3& lv, const Color4& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator*(const Color4& lv, const Color3& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator*(const Color3& lv, const Color4& rv) { return (Color3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Color3 operator/(const Color3& lv, FLOAT rv) { return (Color3(lv.x / rv, lv.y / rv, lv.z / rv)); }
	Color3 operator/(FLOAT lv, const Color3& rv) { return (Color3(lv / rv.x, lv / rv.y, lv / rv.z)); }
	Color3 operator/(const Color3& lv, const Color3& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color3 operator/(const Color3& lv, const Vector3& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color3 operator/(const Vector3& lv, const Color3& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color4 operator/(const Color4& lv, const Color4& rv) { return (Color4(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Color4 operator/(const Color4& lv, const Vector4& rv) { return (Color4(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Color4 operator/(const Vector4& lv, const Color4& rv) { return (Color4(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Color3 operator/(const Color3& lv, const Vector4& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color3 operator/(const Vector4& lv, const Color3& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color3 operator/(const Color4& lv, const Vector3& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color3 operator/(const Vector3& lv, const Color4& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color3 operator/(const Color4& lv, const Color3& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Color3 operator/(const Color3& lv, const Color4& rv) { return (Color3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }

	UINT32 GetVariableFormatStride(const EVariableFormat& InFormat)
	{
		UINT32 Result = 0u;
		if ((InFormat.Prefix <= EVariableFormatPrefixType::VARIABLE_FORMAT_PREFIX_UNKNOWN) || (InFormat.Prefix >= EVariableFormatPrefixType::VARIABLE_FORMAT_PREFIX_COUNT))
		{
			PE_FAILED((ENGINE_CORE_ERROR), (PE_TEXT("[GetVariableFormatStride] Invalid VariableFormat")));
		}
		else
		{
			constexpr UINT32 StrideArray[EVariableFormatPrefixType::VARIABLE_FORMAT_PREFIX_COUNT] = {
				0u,
				sizeof(FLOAT),
				sizeof(FLOAT),
				sizeof(INT32),
				sizeof(UINT32),
			};
			Result = StrideArray[InFormat.Prefix];
		}
		return Result;
	}
	UINT32 GetVariableFormatElements(const EVariableFormat& InFormat)
	{
		UINT32 Result = 0u;
		if ((InFormat.Suffix <= EVariableFormatSuffixType::VARIABLE_FORMAT_SUFFIX_UNKNOWN) || (InFormat.Suffix >= EVariableFormatSuffixType::VARIABLE_FORMAT_SUFFIX_COUNT))
		{
			PE_FAILED((ENGINE_CORE_ERROR), (PE_TEXT("[GetVariableFormatElements] Invalid VariableFormat")));
		}
		else
		{
			constexpr UINT32 ElementArray[EVariableFormatSuffixType::VARIABLE_FORMAT_SUFFIX_COUNT] = {
				0u,
				1u, 2u, 3u, 4u,		// 1, 2, 3, 4
				4u,					// 2x2
				6u, 6u,				// 3x2, 2x3
				9u,					// 3x3
				8u, 8u,				// 4x2, 2x4
				12u, 12u,			// 4x3, 3x4
				16u					// 4x4
			};
			Result = ElementArray[InFormat.Suffix];
		}
		return Result;
	}
	UINT32 GetVariableFormatSize(const EVariableFormat& InFormat)
	{
		return (GetVariableFormatStride(InFormat) * GetVariableFormatElements(InFormat));
	}

	EStructVariableInfo::EStructVariableInfo() noexcept
		: Size{ 0u }
		, ElementStride{ 0u }
		, NumElements{ 0u }
		, MemberSize{ 0u }
		, NumMembers{ 0u }
	{
	}
	EStructVariableInfo::EStructVariableInfo(const EStructVariableInfo& Other) noexcept
		: Size{ Other.Size }
		, ElementStride{ Other.ElementStride }
		, NumElements{ Other.NumElements }
		, MemberSize{ Other.MemberSize }
		, NumMembers{ Other.NumMembers }
	{
	}
	EStructVariableInfo::EStructVariableInfo(EStructVariableInfo&& Other) noexcept
		: Size{ Other.Size }
		, ElementStride{ Other.ElementStride }
		, NumElements{ Other.NumElements }
		, MemberSize{ Other.MemberSize }
		, NumMembers{ Other.NumMembers }
	{
	}
	EStructVariableInfo& EStructVariableInfo::operator=(const EStructVariableInfo& Other) noexcept
	{
		CopyFromOther(Other);
		return (*this);
	}
	EStructVariableInfo& EStructVariableInfo::operator=(EStructVariableInfo&& Other) noexcept
	{
		CopyFromOther(Other);
		return (*this);
	}
	void EStructVariableInfo::CopyFromOther(const EStructVariableInfo& Other) noexcept
	{
		Size = Other.Size;
		ElementStride = Other.ElementStride;
		NumElements = Other.NumElements;
		MemberSize = Other.MemberSize;
		NumMembers = Other.NumMembers;
	}
	void EStructVariableInfo::AnalysisFormat(const EVariableFormat& InFormat, UINT32 InNumMembers) noexcept
	{
		ElementStride = GetVariableFormatStride(InFormat);
		NumElements = GetVariableFormatElements(InFormat);
		MemberSize = GetVariableFormatSize(InFormat);
		Check(((MemberSize > 0u) && (InNumMembers > 0u)), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructVariableInfo::AnalysisFormat] Invalid Memory Size")));
		NumMembers = InNumMembers;
		Size = NumMembers * MemberSize;
	}

	EStructDataReference::EStructDataReference() noexcept
		: DataRef{}
		, MemOffset{ 0u }
		, MemSize{ 0u }
	{
	}
	EStructDataReference::EStructDataReference(TSharedPtr<EStructAllocator>& InData, UINT32 InOffset, UINT32 InSize) noexcept
		: DataRef{ InData }
		, MemOffset{ InOffset }
		, MemSize{ InSize }
	{
	}
	EStructDataReference::EStructDataReference(const EStructDataReference& Other) noexcept
		: DataRef{ Other.DataRef }
		, MemOffset{ Other.MemOffset }
		, MemSize{ Other.MemSize }
	{
	}
	EStructDataReference::EStructDataReference(EStructDataReference&& Other) noexcept
		: DataRef{ Other.DataRef }
		, MemOffset{ Other.MemOffset }
		, MemSize{ Other.MemSize }
	{
	}
	EStructDataReference& EStructDataReference::operator=(const EStructDataReference& Other) noexcept
	{
		CopyFromOther(Other);
		return (*this);
	}
	EStructDataReference& EStructDataReference::operator=(EStructDataReference&& Other) noexcept
	{
		CopyFromOther(Other);		// May not need move?
		return (*this);
	}
	void EStructDataReference::Reset() noexcept
	{
		DataRef.reset();
		MemOffset = 0u;
		MemSize = 0u;
	}
	void EStructDataReference::CopyFromOther(const EStructDataReference& Other) noexcept
	{
		DataRef = Other.DataRef;
		MemOffset = Other.MemOffset;
		MemSize = Other.MemSize;
	}
	BOOL8 EStructDataReference::IsValid() const noexcept
	{
		if (auto DataPtr = DataRef.lock(); DataPtr)
		{
			return ((MemSize > 0u) && ((MemOffset + MemSize) <= (DataPtr->HeapSize())));
		}
		return FALSE;
	}
	UINT8* EStructDataReference::GetData() noexcept
	{
		if (auto DataPtr = DataRef.lock(); DataPtr && (MemSize > 0u) && ((MemOffset + MemSize) <= (DataPtr->HeapSize())))
		{
			return (DataPtr->HeapData());
		}
		return nullptr;
	}
	UINT32 EStructDataReference::GetOffset() noexcept
	{
		return MemOffset;
	}
	UINT32 EStructDataReference::GetSize() noexcept
	{
		return MemSize;
	}

	EStructVariable::EStructVariable() noexcept
		: StructData{}
		, Info{}
		, MemOffset{ 0u }
		, Format{}
	{
	}
	EStructVariable::EStructVariable(const EStructVariable& Other) noexcept
		: StructData{ Other.StructData }
		, Info{ Other.Info }
		, MemOffset{ Other.MemOffset }
		, Format{ Other.Format }
	{
	}
	EStructVariable::EStructVariable(EStructVariable&& Other) noexcept
		: StructData{ Other.StructData }
		, Info{ Other.Info }
		, MemOffset{ Other.MemOffset }
		, Format{ Other.Format }
	{
	}
	EStructVariable& EStructVariable::operator=(const EStructVariable& Other) noexcept
	{
		StructData = Other.StructData;
		Info = Other.Info;
		MemOffset = Other.MemOffset;
		Format = Other.Format;
		return (*this);
	}
	EStructVariable& EStructVariable::operator=(EStructVariable&& Other) noexcept
	{
		StructData = Other.StructData;
		Info = Other.Info;
		MemOffset = Other.MemOffset;
		Format = Other.Format;
		return (*this);
	}
	BOOL8 EStructVariable::IsValid() const noexcept
	{
		if (auto StructDataPtr = StructData.lock(); StructDataPtr)
		{
			return ((StructDataPtr->MemSize > 0u) && (Info.Size > 0u) && ((MemOffset + Info.Size) <= (StructDataPtr->MemSize)));
		}
		return FALSE;
	}
	void EStructVariable::Setup(TSharedPtr<EStructDataReference>& InStructData, UINT32 InMemOffset, const EVariableFormat& InFormat, UINT32 InNumArrayMembers) noexcept
	{
		if (!InStructData)
		{
			return;
		}
		StructData = InStructData;
		Info.AnalysisFormat(InFormat, InNumArrayMembers);
		MemOffset = InMemOffset;
		Format = InFormat;
	}
	void EStructVariable::SetValue(const UINT8* InValuePtr, UINT32 InValueSize) noexcept
	{
		if ((!!InValuePtr) && (InValueSize > 0u))
		{
			if (auto StructDataPtr = StructData.lock(); StructDataPtr)
			{
				Check((InValueSize == Info.Size), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructVariable::SetValue] Invalid Value Size")));
				Check((StructDataPtr->IsValid()), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructVariable::SetValue] Invalid Struct")));
				Check(((Info.Size > 0) && ((MemOffset + Info.Size) <= (StructDataPtr->MemSize))), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructVariable::SetValue] Invalid Struct Variable")));
				if (auto HeapPtr = StructDataPtr->DataRef.lock(); HeapPtr)
				{
					UINT8* DstPtr = HeapPtr->HeapData();
					DstPtr = DstPtr + (StructDataPtr->GetOffset());
					DstPtr = DstPtr + MemOffset;
					EMemory::Memcpy(DstPtr, InValuePtr,
#if _EDITOR_ONLY
						EMath::Min(Info.Size, InValueSize)
#else	// !_EDITOR_ONLY
						Info.Size
#endif	// _EDITOR_ONLY
					);
				}
			}
		}
	}
	void EStructVariable::SetupWithInitValue(TSharedPtr<EStructDataReference>& InStructData, UINT32 InMemOffset, const EVariableFormat& InFormat, UINT32 InNumArrayMembers, const UINT8* InInitValuePtr, UINT32 InInitMemSize) noexcept
	{
		Setup(InStructData, InMemOffset, InFormat, InNumArrayMembers);
		SetValue(InInitValuePtr, InInitMemSize);
	}

	EExclusiveStruct::EExclusiveStruct() noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		StructDataRef = EMemory::MakeShared<EStructDataReference>();
		ClearStruct();
	}
	EExclusiveStruct::~EExclusiveStruct() noexcept
	{
		ClearStruct();
	}
	EExclusiveStruct::EExclusiveStruct(const EExclusiveStruct& Other) noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		StructDataRef = EMemory::MakeShared<EStructDataReference>();
		CopyFromOther(Other);
	}
	EExclusiveStruct::EExclusiveStruct(EExclusiveStruct&& Other) noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		StructDataRef = EMemory::MakeShared<EStructDataReference>();
		MoveFromOther(EMemory::Forward<EExclusiveStruct>(Other));
	}
	EExclusiveStruct& EExclusiveStruct::operator=(const EExclusiveStruct& Other) noexcept
	{
		CopyFromOther(Other);
		return (*this);
	}
	EExclusiveStruct& EExclusiveStruct::operator=(EExclusiveStruct&& Other) noexcept
	{
		MoveFromOther(EMemory::Forward<EExclusiveStruct>(Other));
		return (*this);
	}
	UINT32 EExclusiveStruct::GetMemSize() const noexcept
	{
		return (RawData->HeapSize());
	}
	UINT8* EExclusiveStruct::GetData() noexcept
	{
		return (RawData->HeapData());
	}
	const UINT8* EExclusiveStruct::GetData() const noexcept
	{
		return (RawData->HeapData());
	}
	void EExclusiveStruct::SetupStruct() noexcept
	{
		ClearStruct();
		BeginSetupVariable();
		DoSetupVariable();
		StructDataRef->DataRef = RawData;
		StructDataRef->MemOffset = 0u;
		StructDataRef->MemSize = InitStructSize;
		UINT32 HeapSize = 0u;
		EndSetupVariable(HeapSize);
		Check(((HeapSize > 0u) && (HeapSize >= InitStructSize)), (ENGINE_CORE_ERROR), (PE_TEXT("[EExclusiveStruct::SetupStruct] Invalid Heap Size")));
		if (HeapSize > 0u)
		{
			RawData->Reallocate(HeapSize);
			RawData->ZeroHeapMemory();
		}
		AddStructVariableCommands.DoCommands();
		AddStructVariableCommands.EmptyQueue();
	}
	void EExclusiveStruct::ClearStruct() noexcept
	{
		AddStructVariableCommands.DoCommands();
		AddStructVariableCommands.EmptyQueue();
		StructVariableNames.Empty();
		StructVariables.Empty();
		StructDataRef->Reset();
		RawData->Release();
		InitStructSize = 0;
	}
	void EExclusiveStruct::BeginSetupVariable() noexcept
	{
	}
	void EExclusiveStruct::EndSetupVariable(UINT32& OutHeapSize) noexcept
	{
		OutHeapSize = InitStructSize;
	}
	void EExclusiveStruct::AddStructVariable(const EName& InVarName, const EVariableFormat& InFormat, UINT32 InNumArrayMembers, UINT32 InInitMemSize, const UINT8* InInitValuePtr) noexcept
	{
		Check((InNumArrayMembers > 0u), (ENGINE_CORE_ERROR), (PE_TEXT("[EExclusiveStruct::AddStructVariable] Invalid NumMembers")));
#if _EDITOR_ONLY
		if (StructVariableNames.ContainsKey(InVarName))
		{
			PE_FAILED((ENGINE_CORE_ERROR), (PE_TEXT("[EExclusiveStruct::AddStructVariable] Struct variable is exist, so can not add same value.")));
			return;
		}
#endif	// _EDITOR_ONLY
		const UINT32 MemOffset = InitStructSize;
		InitStructSize += GetVariableFormatSize(InFormat) * InNumArrayMembers;
		StructVariableNames.Add(InVarName, StructVariables.Num());
		EStructVariable& NewVar = StructVariables.AddDefaultGetRef();
		NewVar.Setup(StructDataRef, MemOffset, InFormat, InNumArrayMembers);
		if ((!!InInitValuePtr) && (InInitMemSize > 0u))
		{
			EStructVariable* NewVarPtr = &NewVar;
			AddStructVariableCommands.EnqueueCommand([InInitValuePtr, InInitMemSize, NewVarPtr]()->void
			{
				NewVarPtr->SetValue(InInitValuePtr, InInitMemSize);
			});
		}
	}
	BOOL8 EExclusiveStruct::FindVariable(const EName& InVarName, EStructVariable& OutVar) const noexcept
	{
		constexpr UINT32 MaxUint32 = (UINT32)(-1);
		// The name lookup runs in every configuration. The old non-editor branch read a const
		// map through operator[], which does not exist on it, and the shortcut it stood for
		// would have used element 0 for a missing name - a miss has to stay a miss.
		if (UINT32 VarIndex = MaxUint32; StructVariableNames.FindValue(InVarName, VarIndex))
		{
			if (VarIndex < StructVariables.Num())
			{
				OutVar = StructVariables.GetRef(VarIndex);
				return TRUE;
			}
		}
#if _EDITOR_ONLY
		EString ErrorInfo(PE_TEXT("[EExclusiveStruct::FindVariable] Could not find name["));
		ErrorInfo = ErrorInfo + InVarName.ToString() + PE_TEXT("] variable.");
		PE_FAILED((ENGINE_CORE_ERROR), (*ErrorInfo));
#endif	// _EDITOR_ONLY
		return FALSE;
	}
	void EExclusiveStruct::CopyFromOther(const EExclusiveStruct& Other) noexcept
	{
		ClearStruct();
		RawData->Reallocate(Other.RawData->HeapSize());
		if (Other.RawData->HeapSize() > 0u)
		{
			EMemory::Memcpy(RawData->HeapData(), Other.RawData->HeapData(), Other.RawData->HeapSize());
		}
		StructDataRef->CopyFromOther(*(Other.StructDataRef));
		StructDataRef->DataRef = RawData;
		for (const auto& IterVar : Other.StructVariableNames)
		{
			StructVariableNames.Add(IterVar.first, IterVar.second);
		}
		for (const auto& Var : Other.StructVariables)
		{
			EStructVariable& NewVar = StructVariables.AddDefaultGetRef();
			NewVar.StructData = StructDataRef;
			NewVar.Info = Var.Info;
			NewVar.MemOffset = Var.MemOffset;
			NewVar.Format = Var.Format;
		}
		InitStructSize = Other.InitStructSize;
	}
	void EExclusiveStruct::MoveFromOther(EExclusiveStruct&& Other) noexcept
	{
		ClearStruct();
		StructVariableNames = EMemory::Move(Other.StructVariableNames);
		StructVariables = EMemory::Move(Other.StructVariables);
		StructDataRef = EMemory::Move(Other.StructDataRef);
		RawData = EMemory::Move(Other.RawData);
		InitStructSize = Other.InitStructSize;
	}

	ESharedStruct::ESingleStruct::ESingleStruct() noexcept
		: AddStructVariableCommands{}
		, StructVariableNames{}
		, StructVariables{}
		, StructDataRef{ nullptr }
		, InitStructSize{ 0u }
	{
	}
	ESharedStruct::ESingleStruct::~ESingleStruct() noexcept
	{
		AddStructVariableCommands.DoCommands();
		AddStructVariableCommands.EmptyQueue();
	}
	ESharedStruct::ESharedStruct() noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		ClearStruct();
	}
	ESharedStruct::~ESharedStruct() noexcept
	{
		ClearStruct();
	}
	ESharedStruct::ESharedStruct(const ESharedStruct& Other) noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		CopyFromOther(Other);
	}
	ESharedStruct::ESharedStruct(ESharedStruct&& Other) noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		MoveFromOther(EMemory::Forward<ESharedStruct>(Other));
	}
	ESharedStruct& ESharedStruct::operator=(const ESharedStruct& Other) noexcept
	{
		CopyFromOther(Other);
		return (*this);
	}
	ESharedStruct& ESharedStruct::operator=(ESharedStruct&& Other) noexcept
	{
		MoveFromOther(EMemory::Forward<ESharedStruct>(Other));
		return (*this);
	}
	UINT32 ESharedStruct::GetMemSize() const noexcept
	{
		return (RawData->HeapSize());
	}
	UINT8* ESharedStruct::GetData() noexcept
	{
		return (RawData->HeapData());
	}
	const UINT8* ESharedStruct::GetData() const noexcept
	{
		return (RawData->HeapData());
	}
	UINT32 ESharedStruct::GetMemSize(const EName& InStructName) const noexcept
	{
		constexpr UINT32 MaxUint32 = (UINT32)(-1);
		if (UINT32 StIdx = MaxUint32; StructNames.FindValue(InStructName, StIdx))
		{
			if (StIdx < Structs.Num())
			{
				const ESingleStruct& St = Structs[StIdx];
				return (St.StructDataRef->MemSize);
			}
		}
		return 0u;
	}
	UINT8* ESharedStruct::GetData(const EName& InStructName) noexcept
	{
		constexpr UINT32 MaxUint32 = (UINT32)(-1);
		if (UINT32 StIdx = MaxUint32; StructNames.FindValue(InStructName, StIdx))
		{
			if (StIdx < Structs.Num())
			{
				const ESingleStruct& St = Structs[StIdx];
				UINT8* HeapData = GetData();
				HeapData += St.StructDataRef->MemOffset;
				return HeapData;
			}
		}
		return nullptr;
	}
	const UINT8* ESharedStruct::GetData(const EName& InStructName) const noexcept
	{
		constexpr UINT32 MaxUint32 = (UINT32)(-1);
		if (UINT32 StIdx = MaxUint32; StructNames.FindValue(InStructName, StIdx))
		{
			if (StIdx < Structs.Num())
			{
				const ESingleStruct& St = Structs[StIdx];
				const UINT8* HeapData = GetData();
				HeapData += St.StructDataRef->MemOffset;
				return HeapData;
			}
		}
		return nullptr;
	}
	void ESharedStruct::SetupStruct() noexcept
	{
		ClearStruct();
		BeginSetupVariable();
		DoSetupVariable();
		for (UINT32 StructIdx = 0u, NumStructs = Structs.Num(); StructIdx < NumStructs; StructIdx++)
		{
			ESingleStruct& St = Structs[StructIdx];
			const UINT32 StMemOffset = InitHeapSize;
			InitHeapSize += St.InitStructSize;
			St.StructDataRef->MemOffset = StMemOffset;
			St.StructDataRef->MemSize = St.InitStructSize;
		}
		UINT32 HeapSize = 0u;
		EndSetupVariable(HeapSize);
		Check(((HeapSize > 0u) && (HeapSize >= InitHeapSize)), (ENGINE_CORE_ERROR), (PE_TEXT("[ESharedStruct::SetupStruct] Invalid Heap Size")));
		if (HeapSize > 0u)
		{
			RawData->Reallocate(HeapSize);
			RawData->ZeroHeapMemory();
		}
		for (UINT32 StructIdx = 0u, NumStructs = Structs.Num(); StructIdx < NumStructs; StructIdx++)
		{
			ESingleStruct& St = Structs[StructIdx];
			St.AddStructVariableCommands.DoCommands();
			St.AddStructVariableCommands.EmptyQueue();
		}
	}
	void ESharedStruct::ClearStruct() noexcept
	{
		StructNames.Empty();
		Structs.Empty();
		RawData->Release();
		InitHeapSize = 0u;
	}
	void ESharedStruct::BeginSetupVariable() noexcept
	{

	}
	void ESharedStruct::EndSetupVariable(UINT32& OutHeapSize) noexcept
	{
		OutHeapSize = InitHeapSize;
	}
	void ESharedStruct::AddStructVariable(const EName& InStructName, const EName& InVarName, const EVariableFormat& InFormat, UINT32 InNumArrayMembers, UINT32 InInitMemSize, const UINT8* InInitValuePtr) noexcept
	{
		Check((InNumArrayMembers > 0u), (ENGINE_CORE_ERROR), (PE_TEXT("[EExclusiveStruct::AddStructVariable] Invalid NumMembers")));
		constexpr UINT32 MaxUint32 = (UINT32)(-1);
		UINT32 StIdx = MaxUint32;
		if (!StructNames.FindValue(InStructName, StIdx))
		{
			StIdx = Structs.Num();
			StructNames.Add(InStructName, StIdx);
			ESingleStruct& NewSt = Structs.AddDefaultGetRef();
			NewSt.StructDataRef = EMemory::MakeShared<EStructDataReference>();
			NewSt.StructDataRef->DataRef = RawData;
		}
		Check((StIdx < Structs.Num()), (ENGINE_CORE_ERROR), (PE_TEXT("[ESharedStruct::AddStructVariable] Invalid Struct Index")));
		ESingleStruct& St = Structs[StIdx];
#if _EDITOR_ONLY
		if (St.StructVariableNames.ContainsKey(InVarName))
		{
			PE_FAILED((ENGINE_CORE_ERROR), (PE_TEXT("[ESharedStruct::AddStructVariable] Struct variable is exist, so can not add same value.")));
			return;
		}
#endif	// _EDITOR_ONLY
		const UINT32 MemOffset = St.InitStructSize;
		St.InitStructSize += GetVariableFormatSize(InFormat) * InNumArrayMembers;
		St.StructVariableNames.Add(InVarName, St.StructVariables.Num());
		EStructVariable& NewVar = St.StructVariables.AddDefaultGetRef();
		NewVar.Setup(St.StructDataRef, MemOffset, InFormat, InNumArrayMembers);
		if ((!!InInitValuePtr) && (InInitMemSize > 0u))
		{
			EStructVariable* NewVarPtr = &NewVar;
			St.AddStructVariableCommands.EnqueueCommand([InInitValuePtr, InInitMemSize, NewVarPtr]()->void
			{
				NewVarPtr->SetValue(InInitValuePtr, InInitMemSize);
			});
		}
	}
	BOOL8 ESharedStruct::FindVariable(const EName& InStructName, const EName& InVarName, EStructVariable& OutVar) const noexcept
	{
		constexpr UINT32 MaxUint32 = (UINT32)(-1);
		if (UINT32 StIdx = MaxUint32; StructNames.FindValue(InStructName, StIdx))
		{
			if (StIdx < Structs.Num())
			{
				const ESingleStruct& St = Structs[StIdx];
				if (UINT32 VarIdx = MaxUint32; St.StructVariableNames.FindValue(InVarName, VarIdx))
				{
					if (VarIdx < St.StructVariables.Num())
					{
						OutVar = St.StructVariables[VarIdx];
						return TRUE;
					}
				}
			}
		}
#if _EDITOR_ONLY
		EString ErrorInfo(PE_TEXT("[ESharedStruct::FindVariable] Could not find name[Struct:"));
		ErrorInfo = ErrorInfo + InStructName.ToString() + PE_TEXT(" Variable:") + InVarName.ToString() + PE_TEXT("] variable.");
		PE_FAILED((ENGINE_CORE_ERROR), (*ErrorInfo));
#endif	// _EDITOR_ONLY
		return FALSE;
	}
	void ESharedStruct::CopyFromOther(const ESharedStruct& Other) noexcept
	{
		ClearStruct();
		for (const auto& IterVar : Other.StructNames)
		{
			StructNames.Add(IterVar.first, IterVar.second);
		}
		RawData->Reallocate(Other.RawData->HeapSize());
		if (Other.RawData->HeapSize() > 0u)
		{
			EMemory::Memcpy(RawData->HeapData(), Other.RawData->HeapData(), Other.RawData->HeapSize());
		}
		for (const auto& St : Other.Structs)
		{
			ESingleStruct& NewSt = Structs.AddDefaultGetRef();
			NewSt.StructVariableNames = St.StructVariableNames;
			NewSt.StructDataRef = EMemory::MakeShared<EStructDataReference>();
			NewSt.StructDataRef->CopyFromOther(*(St.StructDataRef));
			NewSt.StructDataRef->DataRef = RawData;
			NewSt.InitStructSize = St.InitStructSize;
			for (const auto& StVar : St.StructVariables)
			{
				EStructVariable& NewVar = NewSt.StructVariables.AddDefaultGetRef();
				NewVar.StructData = NewSt.StructDataRef;
				NewVar.Info = StVar.Info;
				NewVar.MemOffset = StVar.MemOffset;
				NewVar.Format = StVar.Format;
			}
		}
		InitHeapSize = Other.InitHeapSize;
	}
	void ESharedStruct::MoveFromOther(ESharedStruct&& Other) noexcept
	{
		ClearStruct();
		StructNames = EMemory::Move(Other.StructNames);
		Structs = EMemory::Move(Other.Structs);
		RawData = EMemory::Move(Other.RawData);
		InitHeapSize = Other.InitHeapSize;
	}

	EStructArray::EStructArray() noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		ClearStruct();
	}
	EStructArray::~EStructArray() noexcept
	{
		ClearStruct();
	}
	EStructArray::EStructArray(const EStructArray& Other) noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		CopyFromOther(Other);
	}
	EStructArray::EStructArray(EStructArray&& Other) noexcept
	{
		RawData = EMemory::MakeShared<EStructAllocator>();
		MoveFromOther(EMemory::Forward<EStructArray>(Other));
	}
	EStructArray& EStructArray::operator=(const EStructArray& Other) noexcept
	{
		CopyFromOther(Other);
		return (*this);
	}
	EStructArray& EStructArray::operator=(EStructArray&& Other) noexcept
	{
		MoveFromOther(EMemory::Forward<EStructArray>(Other));
		return (*this);
	}
	UINT32 EStructArray::GetNumElements() const noexcept
	{
		return NumElements;
	}
	UINT32 EStructArray::GetMemSize() const noexcept
	{
		Check((StructDataRefs.Num<UINT32>() == NumElements), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::GetMemSize] Invalid NumElements")));
		return ((StructDataRefs.IsEmpty() || (NumElements == 0u)) ? (StructDataRefs[0]->GetSize() * NumElements) : 0u);
	}
	UINT8* EStructArray::GetData() noexcept
	{
		return (RawData->HeapData());
	}
	const UINT8* EStructArray::GetData() const noexcept
	{
		return (RawData->HeapData());
	}
	UINT32 EStructArray::GetMemSize(UINT32 InElementIndex) const noexcept
	{
		Check(((InElementIndex < NumElements) && (StructDataRefs.Num<UINT32>() == NumElements)), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::GetMemSize(ElementIndex)] Invalid Element Index")));
		return (StructDataRefs[InElementIndex]->GetSize());
	}
	UINT8* EStructArray::GetData(UINT32 InElementIndex) noexcept
	{
		Check(((InElementIndex < NumElements) && (StructDataRefs.Num<UINT32>() == NumElements)), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::GetMemSize(ElementIndex)] Invalid Element Index")));
		UINT8* DataPtr = GetData();
		DataPtr = DataPtr + (StructDataRefs[InElementIndex]->GetOffset());
		return DataPtr;
	}
	const UINT8* EStructArray::GetData(UINT32 InElementIndex) const noexcept
	{
		Check(((InElementIndex < NumElements) && (StructDataRefs.Num<UINT32>() == NumElements)), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::GetMemSize(ElementIndex)] Invalid Element Index")));
		const UINT8* DataPtr = GetData();
		DataPtr = DataPtr + (StructDataRefs[InElementIndex]->GetOffset());
		return DataPtr;
	}
	void EStructArray::Resize()
	{
		Check(((NumElements > 0u) && (StructDataRefs.Num() > 0u) && (StructDataRefs[0]) && (StructDataRefs[0]->GetSize() > 0u)), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::Resize] Invalid Base Struct")));
		TSharedPtr<EStructDataReference> BaseStData = StructDataRefs[0];
		const UINT32 StMemSize = BaseStData->GetSize();
		StructDataRefs.Empty(EMath::Max(1u, NumElements));
		if (NumElements > 0u)
		{
			RawData->Reallocate(StMemSize * NumElements);
			RawData->ZeroHeapMemory();
			for (UINT32 Index = 0u; Index < NumElements; Index++)
			{
				TSharedPtr<EStructDataReference> NewStData = EMemory::MakeShared<EStructDataReference>();
				NewStData->CopyFromOther(*(BaseStData));
				NewStData->MemOffset = Index * StMemSize;
				StructDataRefs.Add(NewStData);
			}
		}
		else
		{
			RawData->Release();
			StructDataRefs.Add(BaseStData);
		}
	}
	void EStructArray::SetupStruct() noexcept
	{
		ClearStruct();
		BeginSetupVariable();
		DoSetupVariable();
		UINT32 HeapSize = 0u;
		EndSetupVariable(HeapSize);
		Check(((HeapSize > 0u) && (HeapSize >= InitStructSize)), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::SetupStruct] Invalid Heap Size")));
		Check((StructDataRefs.Num() > 0u), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::SetupStruct] Invalid NumMembers")));
		StructDataRefs[0]->MemSize = HeapSize;
		Resize();
	}
	void EStructArray::ClearStruct() noexcept
	{
		StructVariableNames.Empty();
		StructVariables.Empty();
		StructDataRefs.Empty();
		RawData->Release();
		InitStructSize = 0;
		NumElements = 0;
	}
	void EStructArray::BeginSetupVariable() noexcept
	{

	}
	void EStructArray::EndSetupVariable(UINT32& OutHeapSize) noexcept
	{
		OutHeapSize = InitStructSize;
	}
	void EStructArray::AddStructVariable(const EName& InVarName, const EVariableFormat& InFormat, UINT32 InNumArrayMembers) noexcept
	{
		Check((InNumArrayMembers > 0u), (ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::AddStructVariable] Invalid NumMembers")));
#if _EDITOR_ONLY
		if (StructVariableNames.ContainsKey(InVarName))
		{
			PE_FAILED((ENGINE_CORE_ERROR), (PE_TEXT("[EStructArray::AddStructVariable] Struct variable is exist, so can not add same value.")));
			return;
		}
#endif	// _EDITOR_ONLY
		if (StructDataRefs.Num() == 0u)
		{
			TSharedPtr<EStructDataReference> NewData = EMemory::MakeShared<EStructDataReference>();
			NewData->DataRef = RawData;
			NewData->MemOffset = 0u;
			NewData->MemSize = 0u;
			StructDataRefs.Add(NewData);
		}
		const UINT32 MemOffset = InitStructSize;
		InitStructSize += GetVariableFormatSize(InFormat) * InNumArrayMembers;
		StructVariableNames.Add(InVarName, StructVariables.Num());
		EStructVariable& NewVar = StructVariables.AddDefaultGetRef();
		NewVar.Setup(StructDataRefs[0], MemOffset, InFormat, InNumArrayMembers);
	}
	BOOL8 EStructArray::FindVariable(UINT32 InElementIndex, const EName& InVarName, EStructVariable& OutVar) const noexcept
	{
		constexpr UINT32 MaxUint32 = (UINT32)(-1);
		if (InElementIndex >= StructDataRefs.Num<UINT32>())
		{
			return FALSE;
		}
		if (UINT32 VarIndex = MaxUint32; StructVariableNames.FindValue(InVarName, VarIndex))
		{
			if (VarIndex < StructVariables.Num())
			{
				const EStructVariable& StVar = StructVariables.GetRef(VarIndex);
				OutVar = StVar;
				OutVar.StructData = StructDataRefs[InElementIndex];
				return TRUE;
			}
		}
#if _EDITOR_ONLY
		EString ErrorInfo(PE_TEXT("[EStructArray::FindVariable] Could not find name["));
		ErrorInfo = ErrorInfo + InVarName.ToString() + PE_TEXT("] variable.");
		PE_FAILED((ENGINE_CORE_ERROR), (*ErrorInfo));
#endif	// _EDITOR_ONLY
		return FALSE;
	}
	void EStructArray::CopyFromOther(const EStructArray& Other) noexcept
	{
		ClearStruct();
		RawData->Reallocate(Other.RawData->HeapSize());
		if (Other.RawData->HeapSize() > 0u)
		{
			EMemory::Memcpy(RawData->HeapData(), Other.RawData->HeapData(), Other.RawData->HeapSize());
		}
		for (const auto& StDataRef : Other.StructDataRefs)
		{
			TSharedPtr<EStructDataReference> NewStDataRef = EMemory::MakeShared<EStructDataReference>();
			NewStDataRef->CopyFromOther(*(StDataRef));
			NewStDataRef->DataRef = RawData;
			StructDataRefs.Add(NewStDataRef);
		}
		for (const auto& IterVar : Other.StructVariableNames)
		{
			StructVariableNames.Add(IterVar.first, IterVar.second);
		}
		for (const auto& Var : Other.StructVariables)
		{
			EStructVariable& NewVar = StructVariables.AddDefaultGetRef();
			NewVar.StructData.reset();
			NewVar.Info = Var.Info;
			NewVar.MemOffset = Var.MemOffset;
			NewVar.Format = Var.Format;
		}
		InitStructSize = Other.InitStructSize;
		NumElements = Other.NumElements;
	}
	void EStructArray::MoveFromOther(EStructArray&& Other) noexcept
	{
		ClearStruct();
		StructVariableNames = EMemory::Move(Other.StructVariableNames);
		StructVariables = EMemory::Move(Other.StructVariables);
		StructDataRefs = EMemory::Move(Other.StructDataRefs);
		RawData = EMemory::Move(Other.RawData);
		InitStructSize = Other.InitStructSize;
		NumElements = Other.NumElements;
	}

	// ---------------------------------------------------------------------------
	// Container compile probe.
	//
	// TBitArray is a header with no translation unit of its own, so nothing instantiated it and
	// its member templates were never read by a compiler. It had drifted into a copy of TArray's
	// API that could not build at all, and no build said so.
	//
	// The probe below is what keeps that from happening again: it is instantiated, never called,
	// and it exists so a bit array that stops compiling stops compiling here, where the error
	// names the line, instead of inside whatever feature uses it first.
	// ---------------------------------------------------------------------------
	template<typename _TSizeType>
	void BitArrayCompileProbe()
	{
		TBitArray<_TSizeType> Bits;
		Bits.Add(TRUE);
		Bits.AddDefaultGetRef() = TRUE;
		Bits.SetNum((_TSizeType)70);
		Bits.Reserve((_TSizeType)256);
		Bits[0] = FALSE;
		Bits.Swap(0, 1);
		Bits.RemoveAtSwap((_TSizeType)0);
		Bits.Pop();
		Bits.FirstRef() = TRUE;
		Bits.LastRef() = FALSE;
		Bits.GetRef(0) = TRUE;
		INT32 FoundIndex = 0;
		(void)Bits.FindFirst(TRUE, FoundIndex);
		(void)Bits.FindFirst(TRUE);
		(void)Bits.Contains(TRUE);

		TBitArray<_TSizeType> Copy(Bits);
		Copy = Bits;
		Copy.Append(Bits);
		Copy.Append(EMemory::Move(Bits));
		Copy.Empty();
		Copy.Empty((_TSizeType)64);
		Copy.Reset();
		Copy.SetNum((_TSizeType)0);
		Copy.Shrink();

		TBitArray<_TSizeType> Moved(EMemory::Move(Copy));
		Moved = EMemory::Move(Copy);

		const TBitArray<_TSizeType>& ConstBits = Moved;
		(void)ConstBits.Num<UINT32>();
		(void)ConstBits.ElementNum<UINT32>();
		(void)ConstBits.Capacity<UINT32>();
		(void)ConstBits.ElementCapacity<UINT32>();
		(void)ConstBits.AllocateSize<UINT32>();
		(void)ConstBits.IsEmpty();
		(void)ConstBits.IsValidIndex(0);
		(void)ConstBits.GetData();
		for (auto It = ConstBits.begin(); It; ++It)
		{
			(void)((BOOL8)(*It));
		}
		for (auto It = ConstBits.rbegin(); It; ++It)
		{
			(void)((BOOL8)(*It));
		}
		for (auto It = Moved.begin(); It; ++It)
		{
			(*It) = TRUE;
		}
	}
	// Reading the class and reading the probe are the two halves of the check: the first reads
	// every non template member, the second every template one.
	template class TBitArray<INT32>;
	template void BitArrayCompileProbe<INT32>();

};
