#pragma once

#include "../../Core/Headers/EMain.h"

namespace PigeonEngine
{
	struct Matrix4x4;
	struct Quaternion;
	struct Vector2;
	struct Vector3;
	struct Vector4;
	struct Vector2Int;
	struct Vector3Int;
	struct Vector4Int;
	struct Color3;
	struct Color4;

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
	Matrix4x4 operator*(const Matrix4x4& lm, const FLOAT& rv)
	{
		return (Matrix4x4(
			lm._11 * rv, lm._12 * rv, lm._13 * rv, lm._14 * rv,
			lm._21 * rv, lm._22 * rv, lm._23 * rv, lm._24 * rv,
			lm._31 * rv, lm._32 * rv, lm._33 * rv, lm._34 * rv,
			lm._41 * rv, lm._42 * rv, lm._43 * rv, lm._44 * rv));
	}
	Matrix4x4 operator*(const FLOAT& lv, const Matrix4x4& rm)
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
	Vector2 operator+(const Vector2& lv, const FLOAT& rv) { return (Vector2(lv.x + rv, lv.y + rv)); }
	Vector2 operator+(const FLOAT& lv, const Vector2& rv) { return (Vector2(lv + rv.x, lv + rv.y)); }
	Vector2 operator-(const Vector2& lv, const Vector2& rv) { return (Vector2(lv.x - rv.x, lv.y - rv.y)); }
	Vector2 operator-(const Vector2& lv, const FLOAT& rv) { return (Vector2(lv.x - rv, lv.y - rv)); }
	Vector2 operator-(const FLOAT& lv, const Vector2& rv) { return (Vector2(lv - rv.x, lv - rv.y)); }
	Vector2 operator*(const Vector2& lv, const Vector2& rv) { return (Vector2(lv.x * rv.x, lv.y * rv.y)); }
	Vector2 operator*(const Vector2& lv, const FLOAT& rv) { return (Vector2(lv.x * rv, lv.y * rv)); }
	Vector2 operator*(const FLOAT& lv, const Vector2& rv) { return (Vector2(lv * rv.x, lv * rv.y)); }
	Vector2 operator/(const Vector2& lv, const Vector2& rv) { return (Vector2(lv.x / rv.x, lv.y / rv.y)); }
	Vector2 operator/(const Vector2& lv, const FLOAT& rv) { return (Vector2(lv.x / rv, lv.y / rv)); }
	Vector2 operator/(const FLOAT& lv, const Vector2& rv) { return (Vector2(lv / rv.x, lv / rv.y)); }
	Vector3 operator+(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Vector3 operator+(const Vector3& lv, const FLOAT& rv) { return (Vector3(lv.x + rv, lv.y + rv, lv.z + rv)); }
	Vector3 operator+(const FLOAT& lv, const Vector3& rv) { return (Vector3(lv + rv.x, lv + rv.y, lv + rv.z)); }
	Vector3 operator-(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Vector3 operator-(const Vector3& lv, const FLOAT& rv) { return (Vector3(lv.x - rv, lv.y - rv, lv.z - rv)); }
	Vector3 operator-(const FLOAT& lv, const Vector3& rv) { return (Vector3(lv - rv.x, lv - rv.y, lv - rv.z)); }
	Vector3 operator*(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Vector3 operator*(const Vector3& lv, const FLOAT& rv) { return (Vector3(lv.x * rv, lv.y * rv, lv.z * rv)); }
	Vector3 operator*(const FLOAT& lv, const Vector3& rv) { return (Vector3(lv * rv.x, lv * rv.y, lv * rv.z)); }
	Vector3 operator/(const Vector3& lv, const Vector3& rv) { return (Vector3(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Vector3 operator/(const Vector3& lv, const FLOAT& rv) { return (Vector3(lv.x / rv, lv.y / rv, lv.z / rv)); }
	Vector3 operator/(const FLOAT& lv, const Vector3& rv) { return (Vector3(lv / rv.x, lv / rv.y, lv / rv.z)); }
	Vector4 operator+(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Vector4 operator+(const Vector4& lv, const FLOAT& rv) { return (Vector4(lv.x + rv, lv.y + rv, lv.z + rv, lv.w + rv)); }
	Vector4 operator+(const FLOAT& lv, const Vector4& rv) { return (Vector4(lv + rv.x, lv + rv.y, lv + rv.z, lv + rv.w)); }
	Vector4 operator-(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Vector4 operator-(const Vector4& lv, const FLOAT& rv) { return (Vector4(lv.x - rv, lv.y - rv, lv.z - rv, lv.w - rv)); }
	Vector4 operator-(const FLOAT& lv, const Vector4& rv) { return (Vector4(lv - rv.x, lv - rv.y, lv - rv.z, lv - rv.w)); }
	Vector4 operator*(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Vector4 operator*(const Vector4& lv, const FLOAT& rv) { return (Vector4(lv.x * rv, lv.y * rv, lv.z * rv, lv.w * rv)); }
	Vector4 operator*(const FLOAT& lv, const Vector4& rv) { return (Vector4(lv * rv.x, lv * rv.y, lv * rv.z, lv * rv.w)); }
	Vector4 operator/(const Vector4& lv, const Vector4& rv) { return (Vector4(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Vector4 operator/(const Vector4& lv, const FLOAT& rv) { return (Vector4(lv.x / rv, lv.y / rv, lv.z / rv, lv.w / rv)); }
	Vector4 operator/(const FLOAT& lv, const Vector4& rv) { return (Vector4(lv / rv.x, lv / rv.y, lv / rv.z, lv / rv.w)); }
	Vector2Int operator+(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x + rv.x, lv.y + rv.y)); }
	Vector2Int operator+(const Vector2Int& lv, const INT& rv) { return (Vector2Int(lv.x + rv, lv.y + rv)); }
	Vector2Int operator+(const INT& lv, const Vector2Int& rv) { return (Vector2Int(lv + rv.x, lv + rv.y)); }
	Vector2Int operator-(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x - rv.x, lv.y - rv.y)); }
	Vector2Int operator-(const Vector2Int& lv, const INT& rv) { return (Vector2Int(lv.x - rv, lv.y - rv)); }
	Vector2Int operator-(const INT& lv, const Vector2Int& rv) { return (Vector2Int(lv - rv.x, lv - rv.y)); }
	Vector2Int operator*(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x * rv.x, lv.y * rv.y)); }
	Vector2Int operator*(const Vector2Int& lv, const INT& rv) { return (Vector2Int(lv.x * rv, lv.y * rv)); }
	Vector2Int operator*(const INT& lv, const Vector2Int& rv) { return (Vector2Int(lv * rv.x, lv * rv.y)); }
	Vector2Int operator/(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x / rv.x, lv.y / rv.y)); }
	Vector2Int operator/(const Vector2Int& lv, const INT& rv) { return (Vector2Int(lv.x / rv, lv.y / rv)); }
	Vector2Int operator/(const INT& lv, const Vector2Int& rv) { return (Vector2Int(lv / rv.x, lv / rv.y)); }
	Vector3Int operator+(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Vector3Int operator+(const Vector3Int& lv, const INT& rv) { return (Vector3Int(lv.x + rv, lv.y + rv, lv.z + rv)); }
	Vector3Int operator+(const INT& lv, const Vector3Int& rv) { return (Vector3Int(lv + rv.x, lv + rv.y, lv + rv.z)); }
	Vector3Int operator-(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Vector3Int operator-(const Vector3Int& lv, const INT& rv) { return (Vector3Int(lv.x - rv, lv.y - rv, lv.z - rv)); }
	Vector3Int operator-(const INT& lv, const Vector3Int& rv) { return (Vector3Int(lv - rv.x, lv - rv.y, lv - rv.z)); }
	Vector3Int operator*(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Vector3Int operator*(const Vector3Int& lv, const INT& rv) { return (Vector3Int(lv.x * rv, lv.y * rv, lv.z * rv)); }
	Vector3Int operator*(const INT& lv, const Vector3Int& rv) { return (Vector3Int(lv * rv.x, lv * rv.y, lv * rv.z)); }
	Vector3Int operator/(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Vector3Int operator/(const Vector3Int& lv, const INT& rv) { return (Vector3Int(lv.x / rv, lv.y / rv, lv.z / rv)); }
	Vector3Int operator/(const INT& lv, const Vector3Int& rv) { return (Vector3Int(lv / rv.x, lv / rv.y, lv / rv.z)); }
	Vector4Int operator+(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Vector4Int operator+(const Vector4Int& lv, const INT& rv) { return (Vector4Int(lv.x + rv, lv.y + rv, lv.z + rv, lv.w + rv)); }
	Vector4Int operator+(const INT& lv, const Vector4Int& rv) { return (Vector4Int(lv + rv.x, lv + rv.y, lv + rv.z, lv + rv.w)); }
	Vector4Int operator-(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Vector4Int operator-(const Vector4Int& lv, const INT& rv) { return (Vector4Int(lv.x - rv, lv.y - rv, lv.z - rv, lv.w - rv)); }
	Vector4Int operator-(const INT& lv, const Vector4Int& rv) { return (Vector4Int(lv - rv.x, lv - rv.y, lv - rv.z, lv - rv.w)); }
	Vector4Int operator*(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Vector4Int operator*(const Vector4Int& lv, const INT& rv) { return (Vector4Int(lv.x * rv, lv.y * rv, lv.z * rv, lv.w * rv)); }
	Vector4Int operator*(const INT& lv, const Vector4Int& rv) { return (Vector4Int(lv * rv.x, lv * rv.y, lv * rv.z, lv * rv.w)); }
	Vector4Int operator/(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Vector4Int operator/(const Vector4Int& lv, const INT& rv) { return (Vector4Int(lv.x / rv, lv.y / rv, lv.z / rv, lv.w / rv)); }
	Vector4Int operator/(const INT& lv, const Vector4Int& rv) { return (Vector4Int(lv / rv.x, lv / rv.y, lv / rv.z, lv / rv.w)); }
	Color3 operator+(const Color3& lv, const FLOAT& rv) { return (Color3(lv.x + rv, lv.y + rv, lv.z + rv)); }
	Color3 operator+(const FLOAT& lv, const Color3& rv) { return (Color3(lv + rv.x, lv + rv.y, lv + rv.z)); }
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
	Color3 operator-(const Color3& lv, const FLOAT& rv) { return (Color3(lv.x - rv, lv.y - rv, lv.z - rv)); }
	Color3 operator-(const FLOAT& lv, const Color3& rv) { return (Color3(lv - rv.x, lv - rv.y, lv - rv.z)); }
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
	Color3 operator*(const Color3& lv, const FLOAT& rv) { return (Color3(lv.x * rv, lv.y * rv, lv.z * rv)); }
	Color3 operator*(const FLOAT& lv, const Color3& rv) { return (Color3(lv * rv.x, lv * rv.y, lv * rv.z)); }
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
	Color3 operator/(const Color3& lv, const FLOAT& rv) { return (Color3(lv.x / rv, lv.y / rv, lv.z / rv)); }
	Color3 operator/(const FLOAT& lv, const Color3& rv) { return (Color3(lv / rv.x, lv / rv.y, lv / rv.z)); }
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

	struct Matrix4x4
	{
		Matrix4x4() noexcept :
			_11(0.f), _12(0.f), _13(0.f), _14(0.f),
			_21(0.f), _22(0.f), _23(0.f), _24(0.f),
			_31(0.f), _32(0.f), _33(0.f), _34(0.f),
			_41(0.f), _42(0.f), _43(0.f), _44(0.f) {}
		Matrix4x4(
			const FLOAT& m00, const FLOAT& m01, const FLOAT& m02, const FLOAT& m03,
			const FLOAT& m10, const FLOAT& m11, const FLOAT& m12, const FLOAT& m13,
			const FLOAT& m20, const FLOAT& m21, const FLOAT& m22, const FLOAT& m23,
			const FLOAT& m30, const FLOAT& m31, const FLOAT& m32, const FLOAT& m33) noexcept :
			_11(m00), _12(m01), _13(m02), _14(m03),
			_21(m10), _22(m11), _23(m12), _24(m13),
			_31(m20), _32(m21), _33(m22), _34(m23),
			_41(m30), _42(m31), _43(m32), _44(m33) {}
		Matrix4x4(const DirectX::XMFLOAT4X4& xm) noexcept :
			_11(xm._11), _12(xm._12), _13(xm._13), _14(xm._14),
			_21(xm._21), _22(xm._22), _23(xm._23), _24(xm._24),
			_31(xm._31), _32(xm._32), _33(xm._33), _34(xm._34),
			_41(xm._41), _42(xm._42), _43(xm._43), _44(xm._44) {}
		Matrix4x4(DirectX::CXMMATRIX xm)
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, xm);
			_11 = tempM._11; _12 = tempM._12; _13 = tempM._13; _14 = tempM._14;
			_21 = tempM._21; _22 = tempM._22; _23 = tempM._23; _24 = tempM._24;
			_31 = tempM._31; _32 = tempM._32; _33 = tempM._33; _34 = tempM._34;
			_41 = tempM._41; _42 = tempM._42; _43 = tempM._43; _44 = tempM._44;
		}
		Matrix4x4(const Matrix4x4& mm) noexcept :
			_11(mm._11), _12(mm._12), _13(mm._13), _14(mm._14),
			_21(mm._21), _22(mm._22), _23(mm._23), _24(mm._24),
			_31(mm._31), _32(mm._32), _33(mm._33), _34(mm._34),
			_41(mm._41), _42(mm._42), _43(mm._43), _44(mm._44) {}
		Matrix4x4(const Quaternion& v)
		{
			(*this) = v.GetMatrix();
		}
		Matrix4x4(const Vector3& t, const Quaternion& r)
		{
			(*this) = r.GetMatrix() * TranslationMatrixFromVector3(t);
		}
		Matrix4x4(const Vector3& t, const Quaternion& r, const Vector3& s)
		{
			(*this) = ScalingMatrixFromVector3(s) * r.GetMatrix() * TranslationMatrixFromVector3(t);
		}
		static const Matrix4x4& Identity()
		{
			const static Matrix4x4 _StaticIdentityMatrix4x4(DirectX::XMMatrixIdentity());
			return _StaticIdentityMatrix4x4;
		}
		static Matrix4x4 Inverse(const Matrix4x4& mm, Vector4* det = nullptr)
		{
			if (det != nullptr)
			{
				DirectX::XMVECTOR tempV = DirectX::XMVectorZero();
				Matrix4x4 result(DirectX::XMMatrixInverse(&tempV, mm.GetDirectXMatrix()));
				(*det) = Vector4(tempV);
				return result;
			}
			return (Matrix4x4(DirectX::XMMatrixInverse(nullptr, mm.GetDirectXMatrix())));
		}
		static Matrix4x4 Transpose(const Matrix4x4& mm) { return (Matrix4x4(DirectX::XMMatrixTranspose(mm.GetDirectXMatrix()))); }
		static Matrix4x4 MultiplyMatrix(const Matrix4x4& lm, const Matrix4x4& rm) { return (Matrix4x4(DirectX::XMMatrixMultiply(lm.GetDirectXMatrix(), rm.GetDirectXMatrix()))); }
		static Matrix4x4 TranslationMatrix(const FLOAT& offsetX, const FLOAT& offsetY, const FLOAT& offsetZ) { return (Matrix4x4(DirectX::XMMatrixTranslation(offsetX, offsetY, offsetZ))); }
		static Matrix4x4 TranslationMatrixFromVector3(const Vector3& offset) { return (Matrix4x4(DirectX::XMMatrixTranslation(offset.x, offset.y, offset.z))); }
		static Matrix4x4 ScalingMatrixFrom(const FLOAT& sclX, const FLOAT& sclY, const FLOAT& sclZ) { return (Matrix4x4(DirectX::XMMatrixScaling(sclX, sclY, sclZ))); }
		static Matrix4x4 ScalingMatrixFromVector3(const Vector3& scl) { return (Matrix4x4(DirectX::XMMatrixScaling(scl.x, scl.y, scl.z))); }
		static Matrix4x4 RotationPitchYawRollMatrix(const FLOAT& pitch, const FLOAT& yaw, const FLOAT& roll) { return (Matrix4x4(DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll))); }
		static Matrix4x4 RotationPitchYawRollMatrixFromVector3(const Vector3& rot) { return (Matrix4x4(DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z))); }
		static Matrix4x4 PerspectiveMatrix(const FLOAT& fovYDeg, const FLOAT& aspectRatio, const FLOAT& nearPlane, const FLOAT& farPlane) { return (Matrix4x4(DirectX::XMMatrixPerspectiveFovLH(fovYDeg * EMath::GetDegToRad(), aspectRatio, nearPlane, farPlane))); }
		static Matrix4x4 OrthographicMatrix(const FLOAT& left, const FLOAT& top, const FLOAT& right, const FLOAT& bottom, const FLOAT& nearPlane, const FLOAT& farPlane) { return (Matrix4x4(DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearPlane, farPlane))); }
		void SetDirectXQuaternion(DirectX::CXMVECTOR xq)
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, DirectX::XMMatrixRotationQuaternion(xq));
			_11 = tempM._11; _12 = tempM._12; _13 = tempM._13; _14 = tempM._14;
			_21 = tempM._21; _22 = tempM._22; _23 = tempM._23; _24 = tempM._24;
			_31 = tempM._31; _32 = tempM._32; _33 = tempM._33; _34 = tempM._34;
			_41 = tempM._41; _42 = tempM._42; _43 = tempM._43; _44 = tempM._44;
		}
		void SetDirectXMatrix(DirectX::CXMMATRIX xm)
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, xm);
			_11 = tempM._11; _12 = tempM._12; _13 = tempM._13; _14 = tempM._14;
			_21 = tempM._21; _22 = tempM._22; _23 = tempM._23; _24 = tempM._24;
			_31 = tempM._31; _32 = tempM._32; _33 = tempM._33; _34 = tempM._34;
			_41 = tempM._41; _42 = tempM._42; _43 = tempM._43; _44 = tempM._44;
		}
		void SetDirectXValue4(const DirectX::XMFLOAT4& xq)
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&xq)));
			_11 = tempM._11; _12 = tempM._12; _13 = tempM._13; _14 = tempM._14;
			_21 = tempM._21; _22 = tempM._22; _23 = tempM._23; _24 = tempM._24;
			_31 = tempM._31; _32 = tempM._32; _33 = tempM._33; _34 = tempM._34;
			_41 = tempM._41; _42 = tempM._42; _43 = tempM._43; _44 = tempM._44;
		}
		void SetDirectXValue4x4(const DirectX::XMFLOAT4X4& xm)
		{
			_11 = xm._11; _12 = xm._12; _13 = xm._13; _14 = xm._14;
			_21 = xm._21; _22 = xm._22; _23 = xm._23; _24 = xm._24;
			_31 = xm._31; _32 = xm._32; _33 = xm._33; _34 = xm._34;
			_41 = xm._41; _42 = xm._42; _43 = xm._43; _44 = xm._44;
		}
		DirectX::XMFLOAT4X4 GetDirectXValue4x4() const
		{
			return (DirectX::XMFLOAT4X4(
				_11, _12, _13, _14,
				_21, _22, _23, _24,
				_31, _32, _33, _34,
				_41, _42, _43, _44));
		}
		DirectX::XMMATRIX GetDirectXMatrix() const
		{
			DirectX::XMFLOAT4X4 tempM(GetDirectXValue4x4());
			return (DirectX::XMLoadFloat4x4(&tempM));
		}
		DirectX::XMFLOAT4X4 GetDirectXTransposeValue4x4() const
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, DirectX::XMMatrixTranspose(GetDirectXMatrix()));
			return tempM;
		}
		Quaternion GetQuaternion() const { return Quaternion(GetDirectXValue4x4()); }
		void Reset() { (*this) = Matrix4x4::Identity(); }
		Matrix4x4 Inverse(Vector4* det = nullptr) const
		{
			if (det != nullptr)
			{
				DirectX::XMVECTOR tempV = DirectX::XMVectorZero();
				Matrix4x4 result(DirectX::XMMatrixInverse(&tempV, GetDirectXMatrix()));
				(*det) = Vector4(tempV);
				return result;
			}
			return (Matrix4x4(DirectX::XMMatrixInverse(nullptr, GetDirectXMatrix())));
		}
		Matrix4x4 Transpose() const { return (Matrix4x4(DirectX::XMMatrixTranspose(GetDirectXMatrix()))); }
		Vector4 MultiplyVector(const Vector4& v) const { return (Vector4(DirectX::XMVector4Transform(DirectX::XMVectorSet(v.x, v.y, v.z, v.w), GetDirectXMatrix()))); }
		Vector3 MultiplyPosition(const Vector3& v) const { return (Vector3(DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(v.x, v.y, v.z, 1.f), GetDirectXMatrix()))); }
		Vector3 MultiplyDirection(const Vector3& v) const { return (Vector3(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(v.x, v.y, v.z, 0.f), GetDirectXMatrix()))); }
		FLOAT	operator()(const UINT& row, const UINT& column) const noexcept { return m[row][column]; }
		FLOAT&	operator()(const UINT& row, const UINT& column) noexcept { return m[row][column]; }
		void operator=(const Matrix4x4& mm)
		{
			_11 = mm._11; _12 = mm._12; _13 = mm._13; _14 = mm._14;
			_21 = mm._21; _22 = mm._22; _23 = mm._23; _24 = mm._24;
			_31 = mm._31; _32 = mm._32; _33 = mm._33; _34 = mm._34;
			_41 = mm._41; _42 = mm._42; _43 = mm._43; _44 = mm._44;
		}
		void operator*=(const Matrix4x4& mm) { (*this) = (*this) * mm; }
		constexpr Matrix4x4() noexcept :
			_11(0.f), _12(0.f), _13(0.f), _14(0.f),
			_21(0.f), _22(0.f), _23(0.f), _24(0.f),
			_31(0.f), _32(0.f), _33(0.f), _34(0.f),
			_41(0.f), _42(0.f), _43(0.f), _44(0.f) {}
		constexpr Matrix4x4(
			const FLOAT& m00, const FLOAT& m01, const FLOAT& m02, const FLOAT& m03,
			const FLOAT& m10, const FLOAT& m11, const FLOAT& m12, const FLOAT& m13,
			const FLOAT& m20, const FLOAT& m21, const FLOAT& m22, const FLOAT& m23,
			const FLOAT& m30, const FLOAT& m31, const FLOAT& m32, const FLOAT& m33) noexcept :
			_11(m00), _12(m01), _13(m02), _14(m03),
			_21(m10), _22(m11), _23(m12), _24(m13),
			_31(m20), _32(m21), _33(m22), _34(m23),
			_41(m30), _42(m31), _43(m32), _44(m33) {}
		constexpr Matrix4x4(const DirectX::XMFLOAT4X4& xm) noexcept :
			_11(xm._11), _12(xm._12), _13(xm._13), _14(xm._14),
			_21(xm._21), _22(xm._22), _23(xm._23), _24(xm._24),
			_31(xm._31), _32(xm._32), _33(xm._33), _34(xm._34),
			_41(xm._41), _42(xm._42), _43(xm._43), _44(xm._44) {}
		constexpr Matrix4x4(const Matrix4x4& mm) noexcept :
			_11(mm._11), _12(mm._12), _13(mm._13), _14(mm._14),
			_21(mm._21), _22(mm._22), _23(mm._23), _24(mm._24),
			_31(mm._31), _32(mm._32), _33(mm._33), _34(mm._34),
			_41(mm._41), _42(mm._42), _43(mm._43), _44(mm._44) {}
		union
		{
			struct
			{
				FLOAT _11, _12, _13, _14;
				FLOAT _21, _22, _23, _24;
				FLOAT _31, _32, _33, _34;
				FLOAT _41, _42, _43, _44;
			};
			FLOAT m[4][4];
		};
	};

	struct Quaternion
	{
		Quaternion() noexcept : x(0.f), y(0.f), z(0.f), w(1.f) {}
		Quaternion(const DirectX::XMFLOAT4& xq) noexcept : x(xq.x), y(xq.y), z(xq.z), w(xq.w) {}
		Quaternion(DirectX::CXMVECTOR xq)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, xq);
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		Quaternion(const DirectX::XMFLOAT4X4& xm)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationMatrix(DirectX::XMLoadFloat4x4(&xm)));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		Quaternion(DirectX::CXMMATRIX xm)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationMatrix(xm));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		Quaternion(const Quaternion& q) noexcept : x(q.x), y(q.y), z(q.z), w(q.w) {}
		Quaternion(const Vector4& q) noexcept : x(q.x), y(q.y), z(q.z), w(q.w) {}
		Quaternion(const Vector3& axis, const FLOAT& radian)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(axis.x, axis.y, axis.z, 0.f), radian));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		Quaternion(const FLOAT& pitch, const FLOAT& yaw, const FLOAT& roll)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		Quaternion(const FLOAT& qx, const FLOAT& qy, const FLOAT& qz, const FLOAT& qw) noexcept : x(qx), y(qy), z(qz), w(qw) {}
		static const Quaternion& Identity()
		{
			const static Quaternion _StaticIdentityQuaternion(DirectX::XMQuaternionIdentity());
			return _StaticIdentityQuaternion;
		}
		static Quaternion Inverse(const Quaternion& q) { return (Quaternion(DirectX::XMQuaternionInverse(DirectX::XMVectorSet(q.x, q.y, q.z, q.w)))); }
		static Quaternion Normalize(const Quaternion& q) { return (Quaternion(DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(q.x, q.y, q.z, q.w)))); }
		static Quaternion MultiplyQuaternion(const Quaternion& q1, const Quaternion& q2) { return (Quaternion(DirectX::XMQuaternionMultiply(DirectX::XMVectorSet(q1.x, q1.y, q1.z, q1.w), DirectX::XMVectorSet(q2.x, q2.y, q2.z, q2.w)))); }
		static Quaternion RotationAxis(const Vector3& axis, const FLOAT& radian) { return (Quaternion(DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(axis.x, axis.y, axis.z, 0.f), radian))); }
		static Quaternion NLerp(const Quaternion& q1, const Quaternion& q2, const FLOAT& t)
		{
			return (Quaternion(DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(
				q1.x * (1.f - t) + q2.x * t,
				q1.y * (1.f - t) + q2.y * t,
				q1.z * (1.f - t) + q2.z * t,
				q1.w * (1.f - t) + q2.w * t))));
		}
		static Quaternion NLerp(const Quaternion& q1, const Quaternion& q2, const DOUBLE& t)
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Quaternion(DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(
				q1.x * (1.f - ft) + q2.x * ft,
				q1.y * (1.f - ft) + q2.y * ft,
				q1.z * (1.f - ft) + q2.z * ft,
				q1.w * (1.f - ft) + q2.w * ft))));
		}
		static Quaternion SLerp(const Quaternion& q1, const Quaternion& q2, const FLOAT& t) { return (Quaternion(DirectX::XMQuaternionSlerp(DirectX::XMVectorSet(q1.x, q1.y, q1.z, q1.w), DirectX::XMVectorSet(q2.x, q2.y, q2.z, q2.w), t))); }
		static Quaternion SLerp(const Quaternion& q1, const Quaternion& q2, const DOUBLE& t) { return (Quaternion(DirectX::XMQuaternionSlerp(DirectX::XMVectorSet(q1.x, q1.y, q1.z, q1.w), DirectX::XMVectorSet(q2.x, q2.y, q2.z, q2.w), static_cast<FLOAT>(t)))); }
		void SetDirectXVector(DirectX::CXMVECTOR xq)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, xq);
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		void SetDirectXMatrix(DirectX::CXMMATRIX xm)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationMatrix(xm));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		void SetDirectXValue4(const DirectX::XMFLOAT4& xq) { x = xq.x; y = xq.y; z = xq.z; w = xq.w; }
		void SetDirectXValue4x4(const DirectX::XMFLOAT4X4& xm)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationMatrix(DirectX::XMLoadFloat4x4(&xm)));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		DirectX::XMMATRIX GetDirectXMatrix() const { return (DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(x, y, z, w))); }
		DirectX::XMFLOAT4X4 GetDirectXValue4x4() const
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, GetDirectXMatrix());
			return tempM;
		}
		DirectX::XMFLOAT4X4 GetDirectXTransposeValue4x4() const
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(x, y, z, w))));
			return tempM;
		}
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(x, y, z, w)); }
		DirectX::XMFLOAT4 GetDirectXValue4() const { return (DirectX::XMFLOAT4(x, y, z, w)); }
		Matrix4x4 GetMatrix() const { return (Matrix4x4(GetDirectXValue4x4())); }
		void Reset() { (*this) = Quaternion::Identity(); }
		void Inverse()
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionInverse(DirectX::XMVectorSet(x, y, z, w)));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		void Normalize()
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(x, y, z, w)));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		Vector3 MultiplyVector(const Vector3& v) const
		{
			return (Vector3(DirectX::XMVector3Rotate(DirectX::XMVectorSet(v.x, v.y, v.z, 0.f), DirectX::XMVectorSet(x, y, z, w))));
		}
		void operator=(const Quaternion& q)
		{
			x = q.x; y = q.y; z = q.z; w = q.w;
		}
		void operator=(const Vector4& q)
		{
			x = q.x; y = q.y; z = q.z; w = q.w;
		}
		void operator*=(const Quaternion& q)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionMultiply(DirectX::XMVectorSet(x, y, z, w), DirectX::XMVectorSet(q.x, q.y, q.z, q.w)));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		void operator*=(const Vector4& q)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionMultiply(DirectX::XMVectorSet(x, y, z, w), DirectX::XMVectorSet(q.x, q.y, q.z, q.w)));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		constexpr Quaternion() noexcept : x(0.f), y(0.f), z(0.f), w(1.f) {}
		constexpr Quaternion(const DirectX::XMFLOAT4& xq) noexcept : x(xq.x), y(xq.y), z(xq.z), w(xq.w) {}
		constexpr Quaternion(const Quaternion& q) noexcept : x(q.x), y(q.y), z(q.z), w(q.w) {}
		constexpr Quaternion(const Vector4& q) noexcept : x(q.x), y(q.y), z(q.z), w(q.w) {}
		constexpr Quaternion(const FLOAT& qx, const FLOAT& qy, const FLOAT& qz, const FLOAT& qw) noexcept : x(qx), y(qy), z(qz), w(qw) {}
		union
		{
			struct
			{
				FLOAT x, y, z, w;
			};
			FLOAT v[4];
		};
	};

	struct Vector2
	{
		Vector2() noexcept : x(0.f), y(0.f) {}
		Vector2(const DirectX::XMFLOAT2& xv) noexcept : x(xv.x), y(xv.y) {}
		Vector2(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y) {}
		Vector2(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y) {}
		Vector2(const DirectX::XMINT2& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)) {}
		Vector2(const DirectX::XMINT3& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)) {}
		Vector2(const DirectX::XMINT4& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)) {}
		Vector2(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, xv);
			x = tempV.x; y = tempV.y;
		}
		Vector2(const FLOAT& v) noexcept : x(v), y(v) {}
		Vector2(const FLOAT& vx, const FLOAT& vy) noexcept : x(vx), y(vy) {}
		Vector2(const INT& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)) {}
		Vector2(const INT& vx, const INT& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)) {}
		Vector2(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)) {}
		Vector2(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)) {}
		Vector2(const Vector2& vv) noexcept : x(vv.x), y(vv.y) {}
		Vector2(const Vector3& vv) noexcept : x(vv.x), y(vv.y) {}
		Vector2(const Vector4& vv) noexcept : x(vv.x), y(vv.y) {}
		Vector2(const Vector2Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)) {}
		Vector2(const Vector3Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)) {}
		Vector2(const Vector4Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)) {}
		static const Vector2& Zero()
		{
			const static Vector2 _StaticVector2Zero(0.f, 0.f);
			return _StaticVector2Zero;
		}
		static const Vector2& One()
		{
			const static Vector2 _StaticVector2One(1.f, 1.f);
			return _StaticVector2One;
		}
		static const Vector2& XVector()
		{
			const static Vector2 _StaticVector2XVector(1.f, 0.f);
			return _StaticVector2XVector;
		}
		static const Vector2& YVector()
		{
			const static Vector2 _StaticVector2YVector(0.f, 1.f);
			return _StaticVector2YVector;
		}
		static Vector2 Normalize(const Vector2& vv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, DirectX::XMVector2Normalize(DirectX::XMVectorSet(vv.x, vv.y, 0.f, 0.f)));
			return (Vector2(tempV.x, tempV.y));
		}
		static FLOAT Dot(const Vector2& v1, const Vector2& v2) { return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(DirectX::XMVectorSet(v1.x, v1.y, 0.f, 0.f), DirectX::XMVectorSet(v2.x, v2.y, 0.f, 0.f)))); }
		static FLOAT Length(const Vector2& vv) { return (DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSet(vv.x, vv.y, 0.f, 0.f)))); }
		static FLOAT LengthSquare(const Vector2& vv)
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(vv.x, vv.y, 0.f, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(tempV, tempV)));
		}
		static FLOAT Distance(const Vector2& v1, const Vector2& v2) { return (DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSet(v1.x - v2.x, v1.y - v2.y, 0.f, 0.f)))); }
		static FLOAT DistanceSquare(const Vector2& v1, const Vector2& v2)
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(v1.x - v2.x, v1.y - v2.y, 0.f, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(tempV, tempV)));
		}
		static Vector2 Lerp(const Vector2& v1, const Vector2& v2, const FLOAT& t) { return (Vector2(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t)); }
		static Vector2 Reciprocal(const Vector2& vv) { return Vector2(1.f / vv.x, 1.f / vv.y); }
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, xv);
			x = tempV.x; y = tempV.y;
		}
		void SetDirectXValue2(const DirectX::XMFLOAT2& xv) { x = xv.x; y = xv.y; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(x, y, 0.f, 0.f)); }
		DirectX::XMFLOAT2 GetDirectXValue2() const { return (DirectX::XMFLOAT2(x, y)); }
		DirectX::XMFLOAT3 GetDirectXValue3() const { return (DirectX::XMFLOAT3(x, y, 0.f)); }
		DirectX::XMFLOAT4 GetDirectXValue4() const { return (DirectX::XMFLOAT4(x, y, 0.f, 0.f)); }
		void Reset() { (*this) = Vector2::Zero(); }
		void Normalize()
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, DirectX::XMVector2Normalize(DirectX::XMVectorSet(x, y, 0.f, 0.f)));
			x = tempV.x; y = tempV.y;
		}
		FLOAT Dot(const Vector2& vv) const { return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(DirectX::XMVectorSet(x, y, 0.f, 0.f), DirectX::XMVectorSet(vv.x, vv.y, 0.f, 0.f)))); }
		FLOAT Length() const { return (DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSet(x, y, 0.f, 0.f)))); }
		FLOAT LengthSquare() const { return (DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(DirectX::XMVectorSet(x, y, 0.f, 0.f)))); }
		FLOAT Distance(const Vector2& vv) const
		{
			Vector2 tempV(x - vv.x, y - vv.y);
			return (DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSet(tempV.x, tempV.y, 0.f, 0.f))));
		}
		FLOAT DistanceSquare(const Vector2& vv) const
		{
			Vector2 tempV(x - vv.x, y - vv.y);
			return (DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(DirectX::XMVectorSet(tempV.x, tempV.y, 0.f, 0.f))));
		}
		Vector2 Lerp(const Vector2& vv, const FLOAT& t) const { return (Vector2(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t)); }
		Vector2 Reciprocal() const { return Vector2(1.f / x, 1.f / y); }
		void operator=(const FLOAT& v) { x = y = v; }
		void operator=(const INT& v) { x = y = static_cast<FLOAT>(v); }
		void operator=(const DOUBLE& v) { x = y = static_cast<FLOAT>(v); }
		void operator=(const Vector2& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector3& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector4& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector2Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); }
		void operator=(const Vector3Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); }
		void operator=(const Vector4Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); }
		void operator+=(const FLOAT& v) { x += v; y += v; }
		void operator-=(const FLOAT& v) { x -= v; y -= v; }
		void operator*=(const FLOAT& v) { x *= v; y *= v; }
		void operator/=(const FLOAT& v) { x /= v; y /= v; }
		void operator+=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x += fv; y += fv;
		}
		void operator-=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x -= fv; y -= fv;
		}
		void operator*=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x *= fv; y *= fv;
		}
		void operator/=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x /= fv; y /= fv;
		}
		void operator+=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x += fv; y += fv;
		}
		void operator-=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x -= fv; y -= fv;
		}
		void operator*=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x *= fv; y *= fv;
		}
		void operator/=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x /= fv; y /= fv;
		}
		void operator+=(const Vector2& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector2& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector2& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector2& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector3& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector3& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector3& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector3& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector4& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector4& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector4& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector4& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector2Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); }
		void operator-=(const Vector2Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); }
		void operator*=(const Vector2Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); }
		void operator/=(const Vector2Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); }
		void operator+=(const Vector3Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); }
		void operator-=(const Vector3Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); }
		void operator*=(const Vector3Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); }
		void operator/=(const Vector3Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); }
		void operator+=(const Vector4Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); }
		void operator-=(const Vector4Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); }
		void operator*=(const Vector4Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); }
		void operator/=(const Vector4Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); }
		constexpr Vector2() noexcept : x(0.f), y(0.f) {}
		constexpr Vector2(const DirectX::XMFLOAT2& xv) noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2(const DirectX::XMINT2& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)) {}
		constexpr Vector2(const DirectX::XMINT3& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)) {}
		constexpr Vector2(const DirectX::XMINT4& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)) {}
		constexpr Vector2(const FLOAT& v) noexcept : x(v), y(v) {}
		constexpr Vector2(const FLOAT& vx, const FLOAT& vy) noexcept : x(vx), y(vy) {}
		constexpr Vector2(const INT& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)) {}
		constexpr Vector2(const INT& vx, const INT& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)) {}
		constexpr Vector2(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)) {}
		constexpr Vector2(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)) {}
		constexpr Vector2(const Vector2& vv) noexcept : x(vv.x), y(vv.y) {}
		constexpr Vector2(const Vector3& vv) noexcept : x(vv.x), y(vv.y) {}
		constexpr Vector2(const Vector4& vv) noexcept : x(vv.x), y(vv.y) {}
		constexpr Vector2(const Vector2Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)) {}
		constexpr Vector2(const Vector3Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)) {}
		constexpr Vector2(const Vector4Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)) {}
		union
		{
			struct
			{
				FLOAT x, y;
			};
			FLOAT v[2];
		};
	};

	struct Vector3
	{
		Vector3() noexcept : x(0.f), y(0.f), z(0.f) {}
		Vector3(const DirectX::XMFLOAT2& xv) noexcept : x(xv.x), y(xv.y), z(0.f) {}
		Vector3(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		Vector3(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		Vector3(const DirectX::XMINT2& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(0.f)  {}
		Vector3(const DirectX::XMINT3& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)) {}
		Vector3(const DirectX::XMINT4& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)) {}
		Vector3(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		Vector3(const FLOAT& v) noexcept : x(v), y(v), z(v) {}
		Vector3(const FLOAT& vx, const FLOAT& vy) noexcept : x(vx), y(vy), z(0.f) {}
		Vector3(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz) {}
		Vector3(const INT& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		Vector3(const INT& vx, const INT& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f) {}
		Vector3(const INT& vx, const INT& vy, const INT& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		Vector3(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		Vector3(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f) {}
		Vector3(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		Vector3(const Vector2& vv) noexcept : x(vv.x), y(vv.y), z(0.f) {}
		Vector3(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		Vector3(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		Vector3(const Vector2Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(0.f) {}
		Vector3(const Vector3Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)) {}
		Vector3(const Vector4Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)) {}
		static const Vector3& Zero()
		{
			const static Vector3 _StaticVector3Zero(0.f, 0.f, 0.f);
			return _StaticVector3Zero;
		}
		static const Vector3& One()
		{
			const static Vector3 _StaticVector3One(1.f, 1.f, 1.f);
			return _StaticVector3One;
		}
		static const Vector3& XVector()
		{
			const static Vector3 _StaticVector3XVector(1.f, 0.f, 0.f);
			return _StaticVector3XVector;
		}
		static const Vector3& YVector()
		{
			const static Vector3 _StaticVector3YVector(0.f, 1.f, 0.f);
			return _StaticVector3YVector;
		}
		static const Vector3& ZVector()
		{
			const static Vector3 _StaticVector3ZVector(0.f, 0.f, 1.f);
			return _StaticVector3ZVector;
		}
		static Vector3 Normalize(const Vector3& vv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, DirectX::XMVector3Normalize(DirectX::XMVectorSet(vv.x, vv.y, vv.z, 0.f)));
			return (Vector3(tempV.x, tempV.y, tempV.z));
		}
		static FLOAT Dot(const Vector3& v1, const Vector3& v2) { return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVectorSet(v1.x, v1.y, v1.z, 0.f), DirectX::XMVectorSet(v2.x, v2.y, v2.z, 0.f)))); }
		static FLOAT Length(const Vector3& vv) { return (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSet(vv.x, vv.y, vv.z, 0.f)))); }
		static FLOAT LengthSquare(const Vector3& vv)
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(vv.x, vv.y, vv.z, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(tempV, tempV)));
		}
		static FLOAT Distance(const Vector3& v1, const Vector3& v2) { return (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSet(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, 0.f)))); }
		static FLOAT DistanceSquare(const Vector3& v1, const Vector3& v2)
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(tempV, tempV)));
		}
		static Vector3 Lerp(const Vector3& v1, const Vector3& v2, const FLOAT& t) { return (Vector3(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t)); }
		static Vector3 Lerp(const Vector3& v1, const Vector3& v2, const DOUBLE& t)
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Vector3(v1.x * (1.f - ft) + v2.x * ft, v1.y * (1.f - ft) + v2.y * ft, v1.z * (1.f - ft) + v2.z * ft));
		}
		static Vector3 Cross(const Vector3& v1, const Vector3& v2)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, DirectX::XMVector3Cross(DirectX::XMVectorSet(v1.x, v1.y, v1.z, 0.f), DirectX::XMVectorSet(v2.x, v2.y, v2.z, 0.f)));
			return (Vector3(tempV.x, tempV.y, tempV.z));
		}
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		void SetDirectXValue3(const DirectX::XMFLOAT3& xv) { x = xv.x; y = xv.y; z = xv.z; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(x, y, z, 0.f)); }
		DirectX::XMFLOAT2 GetDirectXValue2() const { return (DirectX::XMFLOAT2(x, y)); }
		DirectX::XMFLOAT3 GetDirectXValue3() const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4() const { return (DirectX::XMFLOAT4(x, y, z, 0.f)); }
		void Reset() { (*this) = Vector3::Zero(); }
		void Normalize()
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, DirectX::XMVector3Normalize(DirectX::XMVectorSet(x, y, z, 0.f)));
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		FLOAT Dot(const Vector3& vv) const { return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVectorSet(x, y, z, 0.f), DirectX::XMVectorSet(vv.x, vv.y, vv.z, 0.f)))); }
		FLOAT Length() const { return (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSet(x, y, z, 0.f)))); }
		FLOAT LengthSquare() const
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(x, y, z, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(tempV, tempV)));
		}
		FLOAT Distance(const Vector3& vv) const { return (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, 0.f)))); }
		FLOAT DistanceSquare(const Vector3& vv) const
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(tempV, tempV)));
		}
		Vector3 Lerp(const Vector3& vv, const FLOAT& t) const { return (Vector3(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t)); }
		Vector3 Lerp(const Vector3& vv, const DOUBLE& t) const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Vector3(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft));
		}
		Vector3 Cross(const Vector3& vv) const
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, DirectX::XMVector3Cross(DirectX::XMVectorSet(x, y, z, 0.f), DirectX::XMVectorSet(vv.x, vv.y, vv.z, 0.f)));
			return (Vector3(tempV.x, tempV.y, tempV.z));
		}
		void operator=(const FLOAT& v) { x = y = z = v; }
		void operator=(const INT& v) { x = y = z = static_cast<FLOAT>(v); }
		void operator=(const DOUBLE& v) { x = y = z = static_cast<FLOAT>(v); }
		void operator=(const Vector2& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector3& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Vector4& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Vector2Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); }
		void operator=(const Vector3Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); z = static_cast<FLOAT>(vv.z); }
		void operator=(const Vector4Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); z = static_cast<FLOAT>(vv.z); }
		void operator+=(const FLOAT& v) { x += v; y += v; z += v; }
		void operator-=(const FLOAT& v) { x -= v; y -= v; z -= v; }
		void operator*=(const FLOAT& v) { x *= v; y *= v; z *= v; }
		void operator/=(const FLOAT& v) { x /= v; y /= v; z /= v; }
		void operator+=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x += fv; y += fv; z += fv;
		}
		void operator-=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x -= fv; y -= fv; z -= fv;
		}
		void operator*=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x *= fv; y *= fv; z *= fv;
		}
		void operator/=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x /= fv; y /= fv; z /= fv;
		}
		void operator+=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x += fv; y += fv; z += fv;
		}
		void operator-=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x -= fv; y -= fv; z -= fv;
		}
		void operator*=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x *= fv; y *= fv; z *= fv;
		}
		void operator/=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x /= fv; y /= fv; z /= fv;
		}
		void operator+=(const Vector2& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector2& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector2& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector2& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector3& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector3& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector3& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector3& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Vector4& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector4& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector4& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector4& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Vector2Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); }
		void operator-=(const Vector2Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); }
		void operator*=(const Vector2Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); }
		void operator/=(const Vector2Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); }
		void operator+=(const Vector3Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); z += static_cast<FLOAT>(vv.z); }
		void operator-=(const Vector3Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); z -= static_cast<FLOAT>(vv.z); }
		void operator*=(const Vector3Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); z *= static_cast<FLOAT>(vv.z); }
		void operator/=(const Vector3Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); z /= static_cast<FLOAT>(vv.z); }
		void operator+=(const Vector4Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); z += static_cast<FLOAT>(vv.z); }
		void operator-=(const Vector4Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); z -= static_cast<FLOAT>(vv.z); }
		void operator*=(const Vector4Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); z *= static_cast<FLOAT>(vv.z); }
		void operator/=(const Vector4Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); z /= static_cast<FLOAT>(vv.z); }
		constexpr Vector3() noexcept : x(0.f), y(0.f), z(0.f) {}
		constexpr Vector3(const DirectX::XMFLOAT2& xv) noexcept : x(xv.x), y(xv.y), z(0.f) {}
		constexpr Vector3(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3(const DirectX::XMINT2& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(0.f) {}
		constexpr Vector3(const DirectX::XMINT3& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)) {}
		constexpr Vector3(const DirectX::XMINT4& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)) {}
		constexpr Vector3(const FLOAT& v) noexcept : x(v), y(v), z(v) {}
		constexpr Vector3(const FLOAT& vx, const FLOAT& vy) noexcept : x(vx), y(vy), z(0.f) {}
		constexpr Vector3(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz) {}
		constexpr Vector3(const INT& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		constexpr Vector3(const INT& vx, const INT& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f) {}
		constexpr Vector3(const INT& vx, const INT& vy, const INT& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		constexpr Vector3(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		constexpr Vector3(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f) {}
		constexpr Vector3(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		constexpr Vector3(const Vector2& vv) noexcept : x(vv.x), y(vv.y), z(0.f) {}
		constexpr Vector3(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		constexpr Vector3(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		constexpr Vector3(const Vector2Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(0.f) {}
		constexpr Vector3(const Vector3Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)) {}
		constexpr Vector3(const Vector4Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)) {}
		union
		{
			struct
			{
				FLOAT x, y, z;
			};
			FLOAT v[3];
		};
	};

	struct Vector4
	{
		Vector4() noexcept : x(0.f), y(0.f), z(0.f), w(0.f) {}
		Vector4(const DirectX::XMFLOAT2& xv) noexcept : x(xv.x), y(xv.y), z(0.f), w(0.f) {}
		Vector4(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(0.f) {}
		Vector4(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		Vector4(const DirectX::XMINT2& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(0.f), w(0.f) {}
		Vector4(const DirectX::XMINT3& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)), w(0.f) {}
		Vector4(const DirectX::XMINT4& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)), w(static_cast<FLOAT>(xv.w)) {}
		Vector4(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		Vector4(const FLOAT& v) noexcept : x(v), y(v), z(v), w(v) {}
		Vector4(const FLOAT& vx, const FLOAT& vy) noexcept : x(vx), y(vy), z(0.f), w(0.f) {}
		Vector4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz), w(0.f) {}
		Vector4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(vx), y(vy), z(vz), w(vw) {}
		Vector4(const INT& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)), w(static_cast<FLOAT>(v)) {}
		Vector4(const INT& vx, const INT& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f), w(0.f) {}
		Vector4(const INT& vx, const INT& vy, const INT& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(0.f) {}
		Vector4(const INT& vx, const INT& vy, const INT& vz, const INT& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		Vector4(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)), w(static_cast<FLOAT>(v)) {}
		Vector4(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f), w(0.f) {}
		Vector4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(0.f) {}
		Vector4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		Vector4(const Vector2& vv) noexcept : x(vv.x), y(vv.y), z(0.f), w(0.f) {}
		Vector4(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(0.f) {}
		Vector4(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		Vector4(const Vector2Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(0.f), w(0.f) {}
		Vector4(const Vector3Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)), w(0.f) {}
		Vector4(const Vector4Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)), w(static_cast<FLOAT>(vv.w)) {}
		static const Vector4& Zero()
		{
			const static Vector4 _StaticVector4Zero(0.f, 0.f, 0.f, 0.f);
			return _StaticVector4Zero;
		}
		static const Vector4& One()
		{
			const static Vector4 _StaticVector4One(1.f, 1.f, 1.f, 1.f);
			return _StaticVector4One;
		}
		static const Vector4& XVector()
		{
			const static Vector4 _StaticVector4XVector(1.f, 0.f, 0.f, 0.f);
			return _StaticVector4XVector;
		}
		static const Vector4& YVector()
		{
			const static Vector4 _StaticVector4YVector(0.f, 1.f, 0.f, 0.f);
			return _StaticVector4YVector;
		}
		static const Vector4& ZVector()
		{
			const static Vector4 _StaticVector4ZVector(0.f, 0.f, 1.f, 0.f);
			return _StaticVector4ZVector;
		}
		static const Vector4& WVector()
		{
			const static Vector4 _StaticVector4WVector(0.f, 0.f, 0.f, 1.f);
			return _StaticVector4WVector;
		}
		static Vector4 Normalize(const Vector4& vv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, DirectX::XMVector4Normalize(DirectX::XMVectorSet(vv.x, vv.y, vv.z, vv.w)));
			return (Vector4(tempV.x, tempV.y, tempV.z, tempV.w));
		}
		static FLOAT Dot(const Vector4& v1, const Vector4& v2) { return (DirectX::XMVectorGetX(DirectX::XMVector4Dot(DirectX::XMVectorSet(v1.x, v1.y, v1.z, v1.w), DirectX::XMVectorSet(v2.x, v2.y, v2.z, v2.w)))); }
		static FLOAT Length(const Vector4& vv) { return (DirectX::XMVectorGetX(DirectX::XMVector4Length(DirectX::XMVectorSet(vv.x, vv.y, vv.z, vv.w)))); }
		static FLOAT LengthSquare(const Vector4& vv) { return (DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(DirectX::XMVectorSet(vv.x, vv.y, vv.z, vv.w)))); }
		static FLOAT Distance(const Vector4& v1, const Vector4& v2) { return (DirectX::XMVectorGetX(DirectX::XMVector4Length(DirectX::XMVectorSet(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w)))); }
		static FLOAT DistanceSquare(const Vector4& v1, const Vector4& v2) { return (DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(DirectX::XMVectorSet(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w)))); }
		static Vector4 Lerp(const Vector4& v1, const Vector4& v2, const FLOAT& t) { return (Vector4(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t, v1.w * (1.f - t) + v2.w * t)); }
		static Vector4 Lerp(const Vector4& v1, const Vector4& v2, const DOUBLE& t)
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Vector4(v1.x * (1.f - ft) + v2.x * ft, v1.y * (1.f - ft) + v2.y * ft, v1.z * (1.f - ft) + v2.z * ft, v1.w * (1.f - ft) + v2.w * ft));
		}
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		void SetDirectXValue4(const DirectX::XMFLOAT4& xv) { x = xv.x; y = xv.y; z = xv.z; w = xv.w; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(x, y, z, w)); }
		DirectX::XMFLOAT2 GetDirectXValue2() const { return (DirectX::XMFLOAT2(x, y)); }
		DirectX::XMFLOAT3 GetDirectXValue3() const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4() const { return (DirectX::XMFLOAT4(x, y, z, w)); }
		void Reset() { (*this) = Vector4::Zero(); }
		void Normalize()
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, DirectX::XMVector4Normalize(DirectX::XMVectorSet(x, y, z, w)));
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		FLOAT Dot(const Vector4& vv) const { return (DirectX::XMVectorGetX(DirectX::XMVector4Dot(DirectX::XMVectorSet(x, y, z, w), DirectX::XMVectorSet(vv.x, vv.y, vv.z, vv.w)))); }
		FLOAT Length() const { return (DirectX::XMVectorGetX(DirectX::XMVector4Length(DirectX::XMVectorSet(x, y, z, w)))); }
		FLOAT LengthSquare() const { return (DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(DirectX::XMVectorSet(x, y, z, w)))); }
		FLOAT Distance(const Vector4& vv) const { return (DirectX::XMVectorGetX(DirectX::XMVector4Length(DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, w - vv.w)))); }
		FLOAT DistanceSquare(const Vector4& vv) const { return (DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, w - vv.w)))); }
		Vector4 Lerp(const Vector4& vv, const FLOAT& t) const { return (Vector4(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t, w * (1.f - t) + vv.w * t)); }
		Vector4 Lerp(const Vector4& vv, const DOUBLE& t) const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Vector4(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft, w * (1.f - ft) + vv.w * ft));
		}
		void operator=(const FLOAT& v) { x = y = z = w = v; }
		void operator=(const INT& v) { x = y = z = w = static_cast<FLOAT>(v); }
		void operator=(const DOUBLE& v) { x = y = z = w = static_cast<FLOAT>(v); }
		void operator=(const Vector2& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector3& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Vector4& vv) { x = vv.x; y = vv.y; z = vv.z; w = vv.w; }
		void operator=(const Vector2Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); }
		void operator=(const Vector3Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); z = static_cast<FLOAT>(vv.z); }
		void operator=(const Vector4Int& vv) { x = static_cast<FLOAT>(vv.x); y = static_cast<FLOAT>(vv.y); z = static_cast<FLOAT>(vv.z); w = static_cast<FLOAT>(vv.w); }
		void operator+=(const FLOAT& v) { x += v; y += v; z += v; w += v; }
		void operator-=(const FLOAT& v) { x -= v; y -= v; z -= v; w -= v; }
		void operator*=(const FLOAT& v) { x *= v; y *= v; z *= v; w *= v; }
		void operator/=(const FLOAT& v) { x /= v; y /= v; z /= v; w /= v; }
		void operator+=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x += fv; y += fv; z += fv; w += fv;
		}
		void operator-=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x -= fv; y -= fv; z -= fv; w -= fv;
		}
		void operator*=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x *= fv; y *= fv; z *= fv; w *= fv;
		}
		void operator/=(const INT& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x /= fv; y /= fv; z /= fv; w /= fv;
		}
		void operator+=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x += fv; y += fv; z += fv; w += fv;
		}
		void operator-=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x -= fv; y -= fv; z -= fv; w -= fv;
		}
		void operator*=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x *= fv; y *= fv; z *= fv; w *= fv;
		}
		void operator/=(const DOUBLE& v)
		{
			FLOAT fv = static_cast<FLOAT>(v);
			x /= fv; y /= fv; z /= fv; w /= fv;
		}
		void operator+=(const Vector2& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector2& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector2& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector2& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector3& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector3& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector3& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector3& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Vector4& vv) { x += vv.x; y += vv.y; z += vv.z; w += vv.w; }
		void operator-=(const Vector4& vv) { x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w; }
		void operator*=(const Vector4& vv) { x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w; }
		void operator/=(const Vector4& vv) { x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w; }
		void operator+=(const Vector2Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); }
		void operator-=(const Vector2Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); }
		void operator*=(const Vector2Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); }
		void operator/=(const Vector2Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); }
		void operator+=(const Vector3Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); z += static_cast<FLOAT>(vv.z); }
		void operator-=(const Vector3Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); z -= static_cast<FLOAT>(vv.z); }
		void operator*=(const Vector3Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); z *= static_cast<FLOAT>(vv.z); }
		void operator/=(const Vector3Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); z /= static_cast<FLOAT>(vv.z); }
		void operator+=(const Vector4Int& vv) { x += static_cast<FLOAT>(vv.x); y += static_cast<FLOAT>(vv.y); z += static_cast<FLOAT>(vv.z); w += static_cast<FLOAT>(vv.w); }
		void operator-=(const Vector4Int& vv) { x -= static_cast<FLOAT>(vv.x); y -= static_cast<FLOAT>(vv.y); z -= static_cast<FLOAT>(vv.z); w -= static_cast<FLOAT>(vv.w); }
		void operator*=(const Vector4Int& vv) { x *= static_cast<FLOAT>(vv.x); y *= static_cast<FLOAT>(vv.y); z *= static_cast<FLOAT>(vv.z); w *= static_cast<FLOAT>(vv.w); }
		void operator/=(const Vector4Int& vv) { x /= static_cast<FLOAT>(vv.x); y /= static_cast<FLOAT>(vv.y); z /= static_cast<FLOAT>(vv.z); w /= static_cast<FLOAT>(vv.w); }
		constexpr Vector4() noexcept : x(0.f), y(0.f), z(0.f), w(0.f) {}
		constexpr Vector4(const DirectX::XMFLOAT2& xv) noexcept : x(xv.x), y(xv.y), z(0.f), w(0.f) {}
		constexpr Vector4(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(0.f) {}
		constexpr Vector4(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		constexpr Vector4(const DirectX::XMINT2& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(0.f), w(0.f) {}
		constexpr Vector4(const DirectX::XMINT3& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)), w(0.f) {}
		constexpr Vector4(const DirectX::XMINT4& xv) noexcept : x(static_cast<FLOAT>(xv.x)), y(static_cast<FLOAT>(xv.y)), z(static_cast<FLOAT>(xv.z)), w(static_cast<FLOAT>(xv.w)) {}
		constexpr Vector4(const FLOAT& v) noexcept : x(v), y(v), z(v), w(v) {}
		constexpr Vector4(const FLOAT& vx, const FLOAT& vy) noexcept : x(vx), y(vy), z(0.f), w(0.f) {}
		constexpr Vector4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz), w(0.f) {}
		constexpr Vector4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(vx), y(vy), z(vz), w(vw) {}
		constexpr Vector4(const INT& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)), w(static_cast<FLOAT>(v)) {}
		constexpr Vector4(const INT& vx, const INT& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f), w(0.f) {}
		constexpr Vector4(const INT& vx, const INT& vy, const INT& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(0.f) {}
		constexpr Vector4(const INT& vx, const INT& vy, const INT& vz, const INT& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		constexpr Vector4(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)), w(static_cast<FLOAT>(v)) {}
		constexpr Vector4(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f), w(0.f) {}
		constexpr Vector4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(0.f) {}
		constexpr Vector4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		constexpr Vector4(const Vector2& vv) noexcept : x(vv.x), y(vv.y), z(0.f), w(0.f) {}
		constexpr Vector4(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(0.f) {}
		constexpr Vector4(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		constexpr Vector4(const Vector2Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(0.f), w(0.f) {}
		constexpr Vector4(const Vector3Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)), w(0.f) {}
		constexpr Vector4(const Vector4Int& vv) noexcept : x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)), w(static_cast<FLOAT>(vv.w)) {}
		union
		{
			struct
			{
				FLOAT x, y, z, w;
			};
			FLOAT v[4];
		};
	};

	struct Vector2Int
	{
		Vector2Int() noexcept : x(0), y(0) {}
		Vector2Int(const DirectX::XMFLOAT2& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)) {}
		Vector2Int(const DirectX::XMFLOAT3& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)) {}
		Vector2Int(const DirectX::XMFLOAT4& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)) {}
		Vector2Int(const DirectX::XMINT2& xv) noexcept : x(xv.x), y(xv.y) {}
		Vector2Int(const DirectX::XMINT3& xv) noexcept : x(xv.x), y(xv.y) {}
		Vector2Int(const DirectX::XMINT4& xv) noexcept : x(xv.x), y(xv.y) {}
		Vector2Int(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, xv);
			x = static_cast<INT>(tempV.x); y = static_cast<INT>(tempV.y);
		}
		Vector2Int(const FLOAT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)) {}
		Vector2Int(const FLOAT& vx, const FLOAT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)) {}
		Vector2Int(const INT& v) noexcept : x(v), y(v) {}
		Vector2Int(const INT& vx, const INT& vy) noexcept : x(vx), y(vy) {}
		Vector2Int(const UINT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)) {}
		Vector2Int(const UINT& vx, const UINT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)) {}
		Vector2Int(const DOUBLE& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)) {}
		Vector2Int(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)) {}
		Vector2Int(const Vector2& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)) {}
		Vector2Int(const Vector3& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)) {}
		Vector2Int(const Vector4& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)) {}
		Vector2Int(const Vector2Int& vv) noexcept : x(vv.x), y(vv.y) {}
		Vector2Int(const Vector3Int& vv) noexcept : x(vv.x), y(vv.y) {}
		Vector2Int(const Vector4Int& vv) noexcept : x(vv.x), y(vv.y) {}
		static const Vector2Int& Zero()
		{
			const static Vector2Int _StaticVector2IntZero(0, 0);
			return _StaticVector2IntZero;
		}
		static const Vector2Int& One()
		{
			const static Vector2Int _StaticVector2IntOne(1, 1);
			return _StaticVector2IntOne;
		}
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, xv);
			x = static_cast<INT>(tempV.x); y = static_cast<INT>(tempV.y);
		}
		void SetDirectXValue2(const DirectX::XMINT2& xv) { x = xv.x; y = xv.y; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(static_cast<FLOAT>(x), static_cast<FLOAT>(y), 0.f, 0.f)); }
		DirectX::XMINT2 GetDirectXValue2() const { return (DirectX::XMINT2(x, y)); }
		DirectX::XMINT3 GetDirectXValue3() const { return (DirectX::XMINT3(x, y, 0)); }
		DirectX::XMINT4 GetDirectXValue4() const { return (DirectX::XMINT4(x, y, 0, 0)); }
		void Reset() { (*this) = Vector2Int::Zero(); }
		void operator=(const FLOAT& v) { x = y = static_cast<INT>(v); }
		void operator=(const INT& v) { x = y = v; }
		void operator=(const UINT& v) { x = y = static_cast<INT>(v); }
		void operator=(const DOUBLE& v) { x = y = static_cast<INT>(v); }
		void operator=(const Vector2& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); }
		void operator=(const Vector3& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); }
		void operator=(const Vector4& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); }
		void operator=(const Vector2Int& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector3Int& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector4Int& vv) { x = vv.x; y = vv.y; }
		void operator+=(const INT& v) { x += v; y += v; }
		void operator-=(const INT& v) { x -= v; y -= v; }
		void operator*=(const INT& v) { x *= v; y *= v; }
		void operator/=(const INT& v) { x /= v; y /= v; }
		void operator+=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv;
		}
		void operator-=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv;
		}
		void operator*=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv;
		}
		void operator/=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv;
		}
		void operator+=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv;
		}
		void operator-=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv;
		}
		void operator*=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv;
		}
		void operator/=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv;
		}
		void operator+=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv;
		}
		void operator-=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv;
		}
		void operator*=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv;
		}
		void operator/=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv;
		}
		void operator+=(const Vector2& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); }
		void operator-=(const Vector2& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); }
		void operator*=(const Vector2& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); }
		void operator/=(const Vector2& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); }
		void operator+=(const Vector3& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); }
		void operator-=(const Vector3& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); }
		void operator*=(const Vector3& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); }
		void operator/=(const Vector3& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); }
		void operator+=(const Vector4& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); }
		void operator-=(const Vector4& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); }
		void operator*=(const Vector4& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); }
		void operator/=(const Vector4& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); }
		void operator+=(const Vector2Int& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector2Int& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector2Int& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector2Int& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector3Int& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector3Int& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector3Int& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector3Int& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector4Int& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector4Int& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector4Int& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector4Int& vv) { x /= vv.x; y /= vv.y; }
		constexpr Vector2Int() noexcept : x(0), y(0) {}
		constexpr Vector2Int(const DirectX::XMFLOAT2& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)) {}
		constexpr Vector2Int(const DirectX::XMFLOAT3& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)) {}
		constexpr Vector2Int(const DirectX::XMFLOAT4& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)) {}
		constexpr Vector2Int(const DirectX::XMINT2& xv) noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2Int(const DirectX::XMINT3& xv) noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2Int(const DirectX::XMINT4& xv) noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2Int(const FLOAT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)) {}
		constexpr Vector2Int(const FLOAT& vx, const FLOAT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)) {}
		constexpr Vector2Int(const INT& v) noexcept : x(v), y(v) {}
		constexpr Vector2Int(const INT& vx, const INT& vy) noexcept : x(vx), y(vy) {}
		constexpr Vector2Int(const UINT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)) {}
		constexpr Vector2Int(const UINT& vx, const UINT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)) {}
		constexpr Vector2Int(const DOUBLE& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)) {}
		constexpr Vector2Int(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)) {}
		constexpr Vector2Int(const Vector2& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)) {}
		constexpr Vector2Int(const Vector3& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)) {}
		constexpr Vector2Int(const Vector4& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)) {}
		constexpr Vector2Int(const Vector2Int& vv) noexcept : x(vv.x), y(vv.y) {}
		constexpr Vector2Int(const Vector3Int& vv) noexcept : x(vv.x), y(vv.y) {}
		constexpr Vector2Int(const Vector4Int& vv) noexcept : x(vv.x), y(vv.y) {}
		union
		{
			struct
			{
				INT x, y;
			};
			INT v[2];
		};
	};

	struct Vector3Int
	{
		Vector3Int() noexcept : x(0), y(0), z(0) {}
		Vector3Int(const DirectX::XMFLOAT2& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(0) {}
		Vector3Int(const DirectX::XMFLOAT3& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)) {}
		Vector3Int(const DirectX::XMFLOAT4& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)) {}
		Vector3Int(const DirectX::XMINT2& xv) noexcept : x(xv.x), y(xv.y), z(0) {}
		Vector3Int(const DirectX::XMINT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		Vector3Int(const DirectX::XMINT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		Vector3Int(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = static_cast<INT>(tempV.x); y = static_cast<INT>(tempV.y); z = static_cast<INT>(tempV.z);
		}
		Vector3Int(const FLOAT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)) {}
		Vector3Int(const FLOAT& vx, const FLOAT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0) {}
		Vector3Int(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)) {}
		Vector3Int(const INT& v) noexcept : x(v), y(v), z(v) {}
		Vector3Int(const INT& vx, const INT& vy) noexcept : x(vx), y(vy), z(0) {}
		Vector3Int(const INT& vx, const INT& vy, const INT& vz) noexcept : x(vx), y(vy), z(vz) {}
		Vector3Int(const UINT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)) {}
		Vector3Int(const UINT& vx, const UINT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0) {}
		Vector3Int(const UINT& vx, const UINT& vy, const UINT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)) {}
		Vector3Int(const DOUBLE& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)) {}
		Vector3Int(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0) {}
		Vector3Int(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)) {}
		Vector3Int(const Vector2& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(0) {}
		Vector3Int(const Vector3& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)) {}
		Vector3Int(const Vector4& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)) {}
		Vector3Int(const Vector2Int& vv) noexcept : x(vv.x), y(vv.y), z(0) {}
		Vector3Int(const Vector3Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		Vector3Int(const Vector4Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		static const Vector3Int& Zero()
		{
			const static Vector3Int _StaticVector3IntZero(0, 0, 0);
			return _StaticVector3IntZero;
		}
		static const Vector3Int& One()
		{
			const static Vector3Int _StaticVector3IntOne(1, 1, 1);
			return _StaticVector3IntOne;
		}
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = static_cast<INT>(tempV.x); y = static_cast<INT>(tempV.y); z = static_cast<INT>(tempV.z);
		}
		void SetDirectXValue3(const DirectX::XMINT3& xv) { x = xv.x; y = xv.y; z = xv.z; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(z), 0.f)); }
		DirectX::XMINT2 GetDirectXValue2() const { return (DirectX::XMINT2(x, y)); }
		DirectX::XMINT3 GetDirectXValue3() const { return (DirectX::XMINT3(x, y, z)); }
		DirectX::XMINT4 GetDirectXValue4() const { return (DirectX::XMINT4(x, y, z, 0)); }
		void Reset() { (*this) = Vector3Int::Zero(); }
		void operator=(const FLOAT& v) { x = y = z = static_cast<INT>(v); }
		void operator=(const INT& v) { x = y = z = v; }
		void operator=(const UINT& v) { x = y = z = static_cast<INT>(v); }
		void operator=(const DOUBLE& v) { x = y = z = static_cast<INT>(v); }
		void operator=(const Vector2& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); }
		void operator=(const Vector3& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); z = static_cast<INT>(vv.z); }
		void operator=(const Vector4& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); z = static_cast<INT>(vv.z); }
		void operator=(const Vector2Int& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector3Int& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Vector4Int& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator+=(const INT& v) { x += v; y += v; z += v; }
		void operator-=(const INT& v) { x -= v; y -= v; z -= v; }
		void operator*=(const INT& v) { x *= v; y *= v; z *= v; }
		void operator/=(const INT& v) { x /= v; y /= v; z /= v; }
		void operator+=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv; z += iv;
		}
		void operator-=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv; z -= iv;
		}
		void operator*=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv; z *= iv;
		}
		void operator/=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv; z /= iv;
		}
		void operator+=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv; z += iv;
		}
		void operator-=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv; z -= iv;
		}
		void operator*=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv; z *= iv;
		}
		void operator/=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv; z /= iv;
		}
		void operator+=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv; z += iv;
		}
		void operator-=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv; z -= iv;
		}
		void operator*=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv; z *= iv;
		}
		void operator/=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv; z /= iv;
		}
		void operator+=(const Vector2& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); }
		void operator-=(const Vector2& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); }
		void operator*=(const Vector2& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); }
		void operator/=(const Vector2& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); }
		void operator+=(const Vector3& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); z += static_cast<INT>(vv.z); }
		void operator-=(const Vector3& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); z -= static_cast<INT>(vv.z); }
		void operator*=(const Vector3& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); z *= static_cast<INT>(vv.z); }
		void operator/=(const Vector3& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); z /= static_cast<INT>(vv.z); }
		void operator+=(const Vector4& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); z += static_cast<INT>(vv.z); }
		void operator-=(const Vector4& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); z -= static_cast<INT>(vv.z); }
		void operator*=(const Vector4& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); z *= static_cast<INT>(vv.z); }
		void operator/=(const Vector4& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); z /= static_cast<INT>(vv.z); }
		void operator+=(const Vector2Int& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector2Int& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector2Int& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector2Int& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector3Int& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector3Int& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector3Int& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector3Int& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Vector4Int& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector4Int& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector4Int& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector4Int& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		constexpr Vector3Int() noexcept : x(0), y(0), z(0) {}
		constexpr Vector3Int(const DirectX::XMFLOAT2& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(0) {}
		constexpr Vector3Int(const DirectX::XMFLOAT3& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)) {}
		constexpr Vector3Int(const DirectX::XMFLOAT4& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)) {}
		constexpr Vector3Int(const DirectX::XMINT2& xv) noexcept : x(xv.x), y(xv.y), z(0) {}
		constexpr Vector3Int(const DirectX::XMINT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3Int(const DirectX::XMINT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3Int(const FLOAT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)) {}
		constexpr Vector3Int(const FLOAT& vx, const FLOAT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0) {}
		constexpr Vector3Int(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)) {}
		constexpr Vector3Int(const INT& v) noexcept : x(v), y(v), z(v) {}
		constexpr Vector3Int(const INT& vx, const INT& vy) noexcept : x(vx), y(vy), z(0) {}
		constexpr Vector3Int(const INT& vx, const INT& vy, const INT& vz) noexcept : x(vx), y(vy), z(vz) {}
		constexpr Vector3Int(const UINT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)) {}
		constexpr Vector3Int(const UINT& vx, const UINT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0) {}
		constexpr Vector3Int(const UINT& vx, const UINT& vy, const UINT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)) {}
		constexpr Vector3Int(const DOUBLE& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)) {}
		constexpr Vector3Int(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0) {}
		constexpr Vector3Int(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)) {}
		constexpr Vector3Int(const Vector2& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(0) {}
		constexpr Vector3Int(const Vector3& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)) {}
		constexpr Vector3Int(const Vector4& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)) {}
		constexpr Vector3Int(const Vector2Int& vv) noexcept : x(vv.x), y(vv.y), z(0) {}
		constexpr Vector3Int(const Vector3Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		constexpr Vector3Int(const Vector4Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		union
		{
			struct
			{
				INT x, y, z;
			};
			INT v[3];
		};
	};

	struct Vector4Int
	{
		Vector4Int() noexcept : x(0), y(0), z(0), w(0) {}
		Vector4Int(const DirectX::XMFLOAT2& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(0), w(0) {}
		Vector4Int(const DirectX::XMFLOAT3& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)), w(0) {}
		Vector4Int(const DirectX::XMFLOAT4& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)), w(static_cast<INT>(xv.w)) {}
		Vector4Int(const DirectX::XMINT2& xv) noexcept : x(xv.x), y(xv.y), z(0), w(0) {}
		Vector4Int(const DirectX::XMINT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(0) {}
		Vector4Int(const DirectX::XMINT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		Vector4Int(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = static_cast<INT>(tempV.x); y = static_cast<INT>(tempV.y); z = static_cast<INT>(tempV.z); w = static_cast<INT>(tempV.w);
		}
		Vector4Int(const FLOAT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)), w(static_cast<INT>(v)) {}
		Vector4Int(const FLOAT& vx, const FLOAT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0), w(0) {}
		Vector4Int(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(0) {}
		Vector4Int(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(static_cast<INT>(vw)) {}
		Vector4Int(const INT& v) noexcept : x(v), y(v), z(v), w(v) {}
		Vector4Int(const INT& vx, const INT& vy) noexcept : x(vx), y(vy), z(0), w(0) {}
		Vector4Int(const INT& vx, const INT& vy, const INT& vz) noexcept : x(vx), y(vy), z(vz), w(0) {}
		Vector4Int(const INT& vx, const INT& vy, const INT& vz, const INT& vw) noexcept : x(vx), y(vy), z(vz), w(vw) {}
		Vector4Int(const UINT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)), w(static_cast<INT>(v)) {}
		Vector4Int(const UINT& vx, const UINT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0), w(0) {}
		Vector4Int(const UINT& vx, const UINT& vy, const UINT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(0) {}
		Vector4Int(const UINT& vx, const UINT& vy, const UINT& vz, const UINT& vw) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(static_cast<INT>(vw)) {}
		Vector4Int(const DOUBLE& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)), w(static_cast<INT>(v)) {}
		Vector4Int(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0), w(0) {}
		Vector4Int(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(0) {}
		Vector4Int(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(static_cast<INT>(vw)) {}
		Vector4Int(const Vector2& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(0), w(0) {}
		Vector4Int(const Vector3& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)), w(0) {}
		Vector4Int(const Vector4& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)), w(static_cast<INT>(vv.w)) {}
		Vector4Int(const Vector2Int& vv) noexcept : x(vv.x), y(vv.y), z(0), w(0) {}
		Vector4Int(const Vector3Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(0) {}
		Vector4Int(const Vector4Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		static const Vector4Int& Zero()
		{
			const static Vector4Int _StaticVector4IntZero(0, 0, 0, 0);
			return _StaticVector4IntZero;
		}
		static const Vector4Int& One()
		{
			const static Vector4Int _StaticVector4IntOne(1, 1, 1, 1);
			return _StaticVector4IntOne;
		}
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = static_cast<INT>(tempV.x); y = static_cast<INT>(tempV.y); z = static_cast<INT>(tempV.z); w = static_cast<INT>(tempV.w);
		}
		void SetDirectXValue4(const DirectX::XMINT4& xv) { x = xv.x; y = xv.y; z = xv.z; w = xv.w; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(z), static_cast<FLOAT>(w))); }
		DirectX::XMINT2 GetDirectXValue2() const { return (DirectX::XMINT2(x, y)); }
		DirectX::XMINT3 GetDirectXValue3() const { return (DirectX::XMINT3(x, y, z)); }
		DirectX::XMINT4 GetDirectXValue4() const { return (DirectX::XMINT4(x, y, z, w)); }
		void Reset() { (*this) = Vector4Int::Zero(); }
		void operator=(const FLOAT& v) { x = y = z = w = static_cast<INT>(v); }
		void operator=(const INT& v) { x = y = z = w = v; }
		void operator=(const UINT& v) { x = y = z = w = static_cast<INT>(v); }
		void operator=(const DOUBLE& v) { x = y = z = w = static_cast<INT>(v); }
		void operator=(const Vector2& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); }
		void operator=(const Vector3& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); z = static_cast<INT>(vv.z); }
		void operator=(const Vector4& vv) { x = static_cast<INT>(vv.x); y = static_cast<INT>(vv.y); z = static_cast<INT>(vv.z); w = static_cast<INT>(vv.w); }
		void operator=(const Vector2Int& vv) { x = vv.x; y = vv.y; }
		void operator=(const Vector3Int& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Vector4Int& vv) { x = vv.x; y = vv.y; z = vv.z; w = vv.w; }
		void operator+=(const INT& v) { x += v; y += v; z += v; w += v; }
		void operator-=(const INT& v) { x -= v; y -= v; z -= v; w -= v; }
		void operator*=(const INT& v) { x *= v; y *= v; z *= v; w *= v; }
		void operator/=(const INT& v) { x /= v; y /= v; z /= v; w /= v; }
		void operator+=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv; z += iv; w += iv;
		}
		void operator-=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv; z -= iv; w -= iv;
		}
		void operator*=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv; z *= iv; w *= iv;
		}
		void operator/=(const FLOAT& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv; z /= iv; w /= iv;
		}
		void operator+=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv; z += iv; w += iv;
		}
		void operator-=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv; z -= iv; w -= iv;
		}
		void operator*=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv; z *= iv; w *= iv;
		}
		void operator/=(const DOUBLE& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv; z /= iv; w /= iv;
		}
		void operator+=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x += iv; y += iv; z += iv; w += iv;
		}
		void operator-=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x -= iv; y -= iv; z -= iv; w -= iv;
		}
		void operator*=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x *= iv; y *= iv; z *= iv; w *= iv;
		}
		void operator/=(const UINT& v)
		{
			INT iv = static_cast<INT>(v);
			x /= iv; y /= iv; z /= iv; w /= iv;
		}
		void operator+=(const Vector2& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); }
		void operator-=(const Vector2& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); }
		void operator*=(const Vector2& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); }
		void operator/=(const Vector2& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); }
		void operator+=(const Vector3& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); z += static_cast<INT>(vv.z); }
		void operator-=(const Vector3& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); z -= static_cast<INT>(vv.z); }
		void operator*=(const Vector3& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); z *= static_cast<INT>(vv.z); }
		void operator/=(const Vector3& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); z /= static_cast<INT>(vv.z); }
		void operator+=(const Vector4& vv) { x += static_cast<INT>(vv.x); y += static_cast<INT>(vv.y); z += static_cast<INT>(vv.z); w += static_cast<INT>(vv.w); }
		void operator-=(const Vector4& vv) { x -= static_cast<INT>(vv.x); y -= static_cast<INT>(vv.y); z -= static_cast<INT>(vv.z); w -= static_cast<INT>(vv.w); }
		void operator*=(const Vector4& vv) { x *= static_cast<INT>(vv.x); y *= static_cast<INT>(vv.y); z *= static_cast<INT>(vv.z); w *= static_cast<INT>(vv.w); }
		void operator/=(const Vector4& vv) { x /= static_cast<INT>(vv.x); y /= static_cast<INT>(vv.y); z /= static_cast<INT>(vv.z); w /= static_cast<INT>(vv.w); }
		void operator+=(const Vector2Int& vv) { x += vv.x; y += vv.y; }
		void operator-=(const Vector2Int& vv) { x -= vv.x; y -= vv.y; }
		void operator*=(const Vector2Int& vv) { x *= vv.x; y *= vv.y; }
		void operator/=(const Vector2Int& vv) { x /= vv.x; y /= vv.y; }
		void operator+=(const Vector3Int& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector3Int& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector3Int& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector3Int& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Vector4Int& vv) { x += vv.x; y += vv.y; z += vv.z; w += vv.w; }
		void operator-=(const Vector4Int& vv) { x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w; }
		void operator*=(const Vector4Int& vv) { x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w; }
		void operator/=(const Vector4Int& vv) { x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w; }
		constexpr Vector4Int() noexcept : x(0), y(0), z(0), w(0) {}
		constexpr Vector4Int(const DirectX::XMFLOAT2& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(0), w(0) {}
		constexpr Vector4Int(const DirectX::XMFLOAT3& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)), w(0) {}
		constexpr Vector4Int(const DirectX::XMFLOAT4& xv) noexcept : x(static_cast<INT>(xv.x)), y(static_cast<INT>(xv.y)), z(static_cast<INT>(xv.z)), w(static_cast<INT>(xv.w)) {}
		constexpr Vector4Int(const DirectX::XMINT2& xv) noexcept : x(xv.x), y(xv.y), z(0), w(0) {}
		constexpr Vector4Int(const DirectX::XMINT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(0) {}
		constexpr Vector4Int(const DirectX::XMINT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		constexpr Vector4Int(const FLOAT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)), w(static_cast<INT>(v)) {}
		constexpr Vector4Int(const FLOAT& vx, const FLOAT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0), w(0) {}
		constexpr Vector4Int(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(0) {}
		constexpr Vector4Int(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(static_cast<INT>(vw)) {}
		constexpr Vector4Int(const INT& v) noexcept : x(v), y(v), z(v), w(v) {}
		constexpr Vector4Int(const INT& vx, const INT& vy) noexcept : x(vx), y(vy), z(0), w(0) {}
		constexpr Vector4Int(const INT& vx, const INT& vy, const INT& vz) noexcept : x(vx), y(vy), z(vz), w(0) {}
		constexpr Vector4Int(const INT& vx, const INT& vy, const INT& vz, const INT& vw) noexcept : x(vx), y(vy), z(vz), w(vw) {}
		constexpr Vector4Int(const UINT& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)), w(static_cast<INT>(v)) {}
		constexpr Vector4Int(const UINT& vx, const UINT& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0), w(0) {}
		constexpr Vector4Int(const UINT& vx, const UINT& vy, const UINT& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(0) {}
		constexpr Vector4Int(const UINT& vx, const UINT& vy, const UINT& vz, const UINT& vw) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(static_cast<INT>(vw)) {}
		constexpr Vector4Int(const DOUBLE& v) noexcept : x(static_cast<INT>(v)), y(static_cast<INT>(v)), z(static_cast<INT>(v)), w(static_cast<INT>(v)) {}
		constexpr Vector4Int(const DOUBLE& vx, const DOUBLE& vy) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(0), w(0) {}
		constexpr Vector4Int(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(0) {}
		constexpr Vector4Int(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<INT>(vx)), y(static_cast<INT>(vy)), z(static_cast<INT>(vz)), w(static_cast<INT>(vw)) {}
		constexpr Vector4Int(const Vector2& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(0), w(0) {}
		constexpr Vector4Int(const Vector3& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)), w(0) {}
		constexpr Vector4Int(const Vector4& vv) noexcept : x(static_cast<INT>(vv.x)), y(static_cast<INT>(vv.y)), z(static_cast<INT>(vv.z)), w(static_cast<INT>(vv.w)) {}
		constexpr Vector4Int(const Vector2Int& vv) noexcept : x(vv.x), y(vv.y), z(0), w(0) {}
		constexpr Vector4Int(const Vector3Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(0) {}
		constexpr Vector4Int(const Vector4Int& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		union
		{
			struct
			{
				INT x, y, z, w;
			};
			INT v[4];
		};
	};

	struct Color3
	{
		Color3() noexcept : x(0.f), y(0.f), z(0.f) {}
		Color3(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		Color3(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		Color3(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		Color3(const FLOAT& v) noexcept : x(v), y(v), z(v) {}
		Color3(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz) {}
		Color3(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(vx), y(vy), z(vz) {}
		Color3(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		Color3(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		Color3(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		Color3(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		Color3(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		Color3(const Color3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		Color3(const Color4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		static const Color3& Black()
		{
			const static Color3 _StaticColor3Black(0.f, 0.f, 0.f);
			return _StaticColor3Black;
		}
		static const Color3& White()
		{
			const static Color3 _StaticColor3White(1.f, 1.f, 1.f);
			return _StaticColor3White;
		}
		static const Color3& MiddleGray()
		{
			const static Color3 _StaticColor3MiddleGray(0.5f, 0.5f, 0.5f);
			return _StaticColor3MiddleGray;
		}
		static const Color3& Red()
		{
			const static Color3 _StaticColor3Red(1.f, 0.f, 0.f);
			return _StaticColor3Red;
		}
		static const Color3& Green()
		{
			const static Color3 _StaticColor3Green(0.f, 1.f, 0.f);
			return _StaticColor3Green;
		}
		static const Color3& Blue()
		{
			const static Color3 _StaticColor3Blue(0.f, 0.f, 1.f);
			return _StaticColor3Blue;
		}
		static Color3 Normalize(const Color3& vv)
		{
			if (vv.x > 0.f && vv.y > 0.f && vv.z > 0.f)
			{
				FLOAT maxV = EMath::Max(EMath::Max(vv.x, vv.y), vv.z);
				maxV = maxV == 0.f ? 1e-5f : maxV;
				return (Color3(vv.x / maxV, vv.y / maxV, vv.z / maxV));
			}
			return (Color3::Black());
		}
		static Color3 Lerp(const Color3& v1, const Color3& v2, const FLOAT& t) { return (Color3(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t)); }
		static Color3 Lerp(const Color3& v1, const Color3& v2, const DOUBLE& t)
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Color3(v1.x * (1.f - ft) + v2.x * ft, v1.y * (1.f - ft) + v2.y * ft, v1.z * (1.f - ft) + v2.z * ft));
		}
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		void SetDirectXValue3(const DirectX::XMFLOAT3& xv) { x = xv.x; y = xv.y; z = xv.z; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(x, y, z, 1.f)); }
		DirectX::XMFLOAT3 GetDirectXValue3() const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4() const { return (DirectX::XMFLOAT4(x, y, z, 1.f)); }
		void Reset() { (*this) = Color3::Black(); }
		void Normalize()
		{
			if (x > 0.f && y > 0.f && z > 0.f)
			{
				FLOAT maxV = EMath::Max(EMath::Max(x, y), z);
				maxV = maxV == 0.f ? 1e-5f : maxV;
				x = x / maxV; y = y / maxV; z = z / maxV;
			}
		}
		Color3 Lerp(const Color3& vv, const FLOAT& t) const { return (Color3(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t)); }
		Color3 Lerp(const Color3& vv, const DOUBLE& t) const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Color3(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft));
		}
		void operator=(const FLOAT& v) { x = y = z = v; }
		void operator=(const DOUBLE& v) { x = y = z = static_cast<FLOAT>(v); }
		void operator=(const Vector3& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Vector4& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Color3& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator=(const Color4& vv) { x = vv.x; y = vv.y; z = vv.z; }
		void operator+=(const Vector3& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector3& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector3& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector3& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Vector4& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector4& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector4& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector4& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Color3& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Color3& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Color3& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Color3& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Color4& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Color4& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Color4& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Color4& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		constexpr Color3() noexcept : x(0.f), y(0.f), z(0.f) {}
		constexpr Color3(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Color3(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Color3(const FLOAT& v) noexcept : x(v), y(v), z(v) {}
		constexpr Color3(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz) {}
		constexpr Color3(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(vx), y(vy), z(vz) {}
		constexpr Color3(const DOUBLE& v) noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		constexpr Color3(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		constexpr Color3(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		constexpr Color3(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		constexpr Color3(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		constexpr Color3(const Color3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		constexpr Color3(const Color4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		union
		{
			struct
			{
				FLOAT x, y, z;
			};
			FLOAT v[3];
			struct
			{
				FLOAT r, g, b;
			};
			FLOAT rgb[3];
		};
	};

	struct Color4
	{
		Color4() noexcept : x(0.f), y(0.f), z(0.f), w(1.f) {}
		Color4(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(1.f) {}
		Color4(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		Color4(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		Color4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz), w(1.f) {}
		Color4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(vx), y(vy), z(vz), w(vw) {}
		Color4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(1.f) {}
		Color4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		Color4(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(1.f) {}
		Color4(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		Color4(const Color3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(1.f) {}
		Color4(const Color4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		static const Color4& Black()
		{
			const static Color4 _StaticColor4Black(0.f, 0.f, 0.f, 1.f);
			return _StaticColor4Black;
		}
		static const Color4& White()
		{
			const static Color4 _StaticColor4White(1.f, 1.f, 1.f, 1.f);
			return _StaticColor4White;
		}
		static const Color4& MiddleGray()
		{
			const static Color4 _StaticColor4MiddleGray(0.5f, 0.5f, 0.5f, 1.f);
			return _StaticColor4MiddleGray;
		}
		static const Color4& Red()
		{
			const static Color4 _StaticColor4Red(1.f, 0.f, 0.f, 1.f);
			return _StaticColor4Red;
		}
		static const Color4& Green()
		{
			const static Color4 _StaticColor4Green(0.f, 1.f, 0.f, 1.f);
			return _StaticColor4Green;
		}
		static const Color4& Blue()
		{
			const static Color4 _StaticColor4Blue(0.f, 0.f, 1.f, 1.f);
			return _StaticColor4Blue;
		}
		static Color4 Normalize(const Color4& vv)
		{
			if (vv.x > 0.f && vv.y > 0.f && vv.z > 0.f)
			{
				FLOAT maxV = EMath::Max(EMath::Max(vv.x, vv.y), vv.z);
				maxV = maxV == 0.f ? 1e-5f : maxV;
				return (Color4(vv.x / maxV, vv.y / maxV, vv.z / maxV, vv.w));
			}
			return (Color4::Black());
		}
		static Color4 Lerp(const Color4& v1, const Color4& v2, const FLOAT& t) { return (Color4(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t, v1.w * (1.f - t) + v2.w * t)); }
		static Color4 Lerp(const Color4& v1, const Color4& v2, const DOUBLE& t)
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Color4(v1.x * (1.f - ft) + v2.x * ft, v1.y * (1.f - ft) + v2.y * ft, v1.z * (1.f - ft) + v2.z * ft, v1.w * (1.f - ft) + v2.w * ft));
		}
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		void SetDirectXValue4(const DirectX::XMFLOAT4& xv) { x = xv.x; y = xv.y; z = xv.z; w = xv.w; }
		DirectX::XMVECTOR GetDirectXVector() const { return (DirectX::XMVectorSet(x, y, z, w)); }
		DirectX::XMFLOAT3 GetDirectXValue3() const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4() const { return (DirectX::XMFLOAT4(x, y, z, w)); }
		void Reset() { (*this) = Color4::Black(); }
		void Normalize()
		{
			if (x > 0.f && y > 0.f && z > 0.f)
			{
				FLOAT maxV = EMath::Max(EMath::Max(x, y), z);
				maxV = maxV == 0.f ? 1e-5f : maxV;
				x = x / maxV; y = y / maxV; z = z / maxV;
			}
		}
		Color4 Lerp(const Color4& vv, const FLOAT& t) const { return (Color4(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t, w * (1.f - t) + vv.w * t)); }
		Color4 Lerp(const Color4& vv, const DOUBLE& t) const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Color4(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft, w * (1.f - ft) + vv.w * ft));
		}
		void operator=(const Vector3& vv) { x = vv.x; y = vv.y; z = vv.z; w = 1.f; }
		void operator=(const Vector4& vv) { x = vv.x; y = vv.y; z = vv.z; w = vv.w; }
		void operator=(const Color3& vv) { x = vv.x; y = vv.y; z = vv.z; w = 1.f; }
		void operator=(const Color4& vv) { x = vv.x; y = vv.y; z = vv.z; w = vv.w; }
		void operator+=(const Vector3& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Vector3& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Vector3& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Vector3& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Vector4& vv) { x += vv.x; y += vv.y; z += vv.z; w += vv.w; }
		void operator-=(const Vector4& vv) { x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w; }
		void operator*=(const Vector4& vv) { x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w; }
		void operator/=(const Vector4& vv) { x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w; }
		void operator+=(const Color3& vv) { x += vv.x; y += vv.y; z += vv.z; }
		void operator-=(const Color3& vv) { x -= vv.x; y -= vv.y; z -= vv.z; }
		void operator*=(const Color3& vv) { x *= vv.x; y *= vv.y; z *= vv.z; }
		void operator/=(const Color3& vv) { x /= vv.x; y /= vv.y; z /= vv.z; }
		void operator+=(const Color4& vv) { x += vv.x; y += vv.y; z += vv.z; w += vv.w; }
		void operator-=(const Color4& vv) { x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w; }
		void operator*=(const Color4& vv) { x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w; }
		void operator/=(const Color4& vv) { x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w; }
		constexpr Color4() noexcept : x(0.f), y(0.f), z(0.f), w(1.f) {}
		constexpr Color4(const DirectX::XMFLOAT3& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(1.f) {}
		constexpr Color4(const DirectX::XMFLOAT4& xv) noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		constexpr Color4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz) noexcept : x(vx), y(vy), z(vz), w(1.f) {}
		constexpr Color4(const FLOAT& vx, const FLOAT& vy, const FLOAT& vz, const FLOAT& vw) noexcept : x(vx), y(vy), z(vz), w(vw) {}
		constexpr Color4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(1.f) {}
		constexpr Color4(const DOUBLE& vx, const DOUBLE& vy, const DOUBLE& vz, const DOUBLE& vw) noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		constexpr Color4(const Vector3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(1.f) {}
		constexpr Color4(const Vector4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		constexpr Color4(const Color3& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(1.f) {}
		constexpr Color4(const Color4& vv) noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		union
		{
			struct
			{
				FLOAT x, y, z, w;
			};
			FLOAT v[4];
			struct
			{
				FLOAT r, g, b, a;
			};
			FLOAT rgba[4];
		};
	};
};