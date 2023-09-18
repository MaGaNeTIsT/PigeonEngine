#include "BaseType.h"

namespace PigeonEngine
{

	PE_INLINE Matrix4x4 MakeTranslationMatrix4x4(const Vector3& InTranslation) { return Matrix4x4(DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
	PE_INLINE Matrix4x4 MakeRotationMatrix4x4(const Quaternion& InRotation) { return Matrix4x4(InRotation.GetDirectXMatrix()); }
	PE_INLINE Matrix4x4 MakeScalingMatrix4x4(const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z)); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Quaternion& InRotation) { return Matrix4x4(InRotation.GetDirectXMatrix()); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Quaternion& InRotation) { return Matrix4x4(InRotation.GetDirectXMatrix() * DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z) * DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Quaternion& InRotation, const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z) * InRotation.GetDirectXMatrix()); }
	PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Quaternion& InRotation, const Vector3& InScaling) { return Matrix4x4(DirectX::XMMatrixScaling(InScaling.x, InScaling.y, InScaling.z) * InRotation.GetDirectXMatrix() * DirectX::XMMatrixTranslation(InTranslation.x, InTranslation.y, InTranslation.z)); }
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
	PE_INLINE Vector3 QuaternionTransformVector(const Quaternion& q, const Vector3& v) { return Vector3(DirectX::XMVector3Rotate(DirectX::XMVectorSet(v.x, v.y, v.z, 0.f), DirectX::XMVectorSet(q.x, q.y, q.z, q.w))); }
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

};