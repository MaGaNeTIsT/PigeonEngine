#pragma once

#include "../../../Main/Main.h"
#include <DirectXMath.h>
#include "BuiltInType.h"
#include "../Math/Math.h"

namespace PigeonEngine
{

#pragma warning(push)
#pragma warning(disable : 26495)

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

	struct Matrix4x4
	{
		constexpr Matrix4x4(
			FLOAT m00, FLOAT m01, FLOAT m02, FLOAT m03,
			FLOAT m10, FLOAT m11, FLOAT m12, FLOAT m13,
			FLOAT m20, FLOAT m21, FLOAT m22, FLOAT m23,
			FLOAT m30, FLOAT m31, FLOAT m32, FLOAT m33)noexcept :
			_11(m00), _12(m01), _13(m02), _14(m03),
			_21(m10), _22(m11), _23(m12), _24(m13),
			_31(m20), _32(m21), _33(m22), _34(m23),
			_41(m30), _42(m31), _43(m32), _44(m33) {}
		constexpr Matrix4x4(const DirectX::XMFLOAT4X4& xm)noexcept :
			_11(xm._11), _12(xm._12), _13(xm._13), _14(xm._14),
			_21(xm._21), _22(xm._22), _23(xm._23), _24(xm._24),
			_31(xm._31), _32(xm._32), _33(xm._33), _34(xm._34),
			_41(xm._41), _42(xm._42), _43(xm._43), _44(xm._44) {}
		Matrix4x4()noexcept :
			_11(0.f), _12(0.f), _13(0.f), _14(0.f),
			_21(0.f), _22(0.f), _23(0.f), _24(0.f),
			_31(0.f), _32(0.f), _33(0.f), _34(0.f),
			_41(0.f), _42(0.f), _43(0.f), _44(0.f) {}
		Matrix4x4(DirectX::CXMMATRIX xm)
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, xm);
			_11 = tempM._11; _12 = tempM._12; _13 = tempM._13; _14 = tempM._14;
			_21 = tempM._21; _22 = tempM._22; _23 = tempM._23; _24 = tempM._24;
			_31 = tempM._31; _32 = tempM._32; _33 = tempM._33; _34 = tempM._34;
			_41 = tempM._41; _42 = tempM._42; _43 = tempM._43; _44 = tempM._44;
		}
		Matrix4x4(const Matrix4x4& mm)noexcept :
			_11(mm._11), _12(mm._12), _13(mm._13), _14(mm._14),
			_21(mm._21), _22(mm._22), _23(mm._23), _24(mm._24),
			_31(mm._31), _32(mm._32), _33(mm._33), _34(mm._34),
			_41(mm._41), _42(mm._42), _43(mm._43), _44(mm._44) {}
		static const Matrix4x4& Identity()
		{
			const static Matrix4x4 _StaticIdentityMatrix4x4(DirectX::XMMatrixIdentity());
			return _StaticIdentityMatrix4x4;
		}
		static Matrix4x4 Transpose(const Matrix4x4& mm) { return (Matrix4x4(DirectX::XMMatrixTranspose(mm.GetDirectXMatrix()))); }
		static Matrix4x4 MultiplyMatrix(const Matrix4x4& lm, const Matrix4x4& rm) { return (Matrix4x4(DirectX::XMMatrixMultiply(lm.GetDirectXMatrix(), rm.GetDirectXMatrix()))); }
		static Matrix4x4 TranslationMatrix(FLOAT offsetX, FLOAT offsetY, FLOAT offsetZ) { return (Matrix4x4(DirectX::XMMatrixTranslation(offsetX, offsetY, offsetZ))); }
		static Matrix4x4 ScalingMatrixFrom(FLOAT sclX, FLOAT sclY, FLOAT sclZ) { return (Matrix4x4(DirectX::XMMatrixScaling(sclX, sclY, sclZ))); }
		static Matrix4x4 RotationPitchYawRollMatrix(FLOAT pitch, FLOAT yaw, FLOAT roll) { return (Matrix4x4(DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll))); }
		static Matrix4x4 PerspectiveMatrix(FLOAT fovYDeg, FLOAT aspectRatio, FLOAT nearPlane, FLOAT farPlane) { return (Matrix4x4(DirectX::XMMatrixPerspectiveFovLH(EMath::DegreesToRadians(fovYDeg), aspectRatio, nearPlane, farPlane))); }
		static Matrix4x4 OrthographicMatrix(FLOAT left, FLOAT top, FLOAT right, FLOAT bottom, FLOAT nearPlane, FLOAT farPlane) { return (Matrix4x4(DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearPlane, farPlane))); }
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
		DirectX::XMFLOAT4X4 GetDirectXValue4x4()const
		{
			return (DirectX::XMFLOAT4X4(
				_11, _12, _13, _14,
				_21, _22, _23, _24,
				_31, _32, _33, _34,
				_41, _42, _43, _44));
		}
		DirectX::XMMATRIX GetDirectXMatrix()const
		{
			DirectX::XMFLOAT4X4 tempM(GetDirectXValue4x4());
			return (DirectX::XMLoadFloat4x4(&tempM));
		}
		DirectX::XMFLOAT4X4 GetDirectXTransposeValue4x4()const
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, DirectX::XMMatrixTranspose(GetDirectXMatrix()));
			return tempM;
		}
		void Reset() { (*this) = Matrix4x4::Identity(); }
		Matrix4x4 Inverse()const { return (Matrix4x4(DirectX::XMMatrixInverse(nullptr, GetDirectXMatrix()))); }
		Matrix4x4 Transpose()const { return (Matrix4x4(DirectX::XMMatrixTranspose(GetDirectXMatrix()))); }
		FLOAT operator()(UINT32 row, UINT32 column)const noexcept { return m[row][column]; }
		FLOAT& operator()(UINT32 row, UINT32 column)noexcept { return m[row][column]; }
		Matrix4x4& operator=(const Matrix4x4& mm)
		{
			_11 = mm._11; _12 = mm._12; _13 = mm._13; _14 = mm._14;
			_21 = mm._21; _22 = mm._22; _23 = mm._23; _24 = mm._24;
			_31 = mm._31; _32 = mm._32; _33 = mm._33; _34 = mm._34;
			_41 = mm._41; _42 = mm._42; _43 = mm._43; _44 = mm._44;
			return (*this);
		}
		Matrix4x4& operator*=(const Matrix4x4& mm)
		{
			(*this) = Matrix4x4::MultiplyMatrix((*this), mm);
			return (*this);
		}

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
		constexpr Quaternion(FLOAT qx, FLOAT qy, FLOAT qz, FLOAT qw)noexcept : x(qx), y(qy), z(qz), w(qw) {}
		constexpr Quaternion(const DirectX::XMFLOAT4& xq)noexcept : x(xq.x), y(xq.y), z(xq.z), w(xq.w) {}
		Quaternion()noexcept : x(0.f), y(0.f), z(0.f), w(1.f) {}
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
		Quaternion(const Quaternion& q)noexcept : x(q.x), y(q.y), z(q.z), w(q.w) {}
		Quaternion(FLOAT pitch, FLOAT yaw, FLOAT roll)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
		}
		static const Quaternion& Identity()
		{
			const static Quaternion _StaticIdentityQuaternion(DirectX::XMQuaternionIdentity());
			return _StaticIdentityQuaternion;
		}
		static Quaternion Inverse(const Quaternion& q) { return (Quaternion(DirectX::XMQuaternionInverse(DirectX::XMVectorSet(q.x, q.y, q.z, q.w)))); }
		static Quaternion Normalize(const Quaternion& q) { return (Quaternion(DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(q.x, q.y, q.z, q.w)))); }
		static Quaternion MultiplyQuaternion(const Quaternion& q1, const Quaternion& q2) { return (Quaternion(DirectX::XMQuaternionMultiply(DirectX::XMVectorSet(q1.x, q1.y, q1.z, q1.w), DirectX::XMVectorSet(q2.x, q2.y, q2.z, q2.w)))); }
		static Quaternion NLerp(const Quaternion& q1, const Quaternion& q2, FLOAT t)
		{
			return (Quaternion(DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(
				q1.x * (1.f - t) + q2.x * t,
				q1.y * (1.f - t) + q2.y * t,
				q1.z * (1.f - t) + q2.z * t,
				q1.w * (1.f - t) + q2.w * t))));
		}
		static Quaternion NLerp(const Quaternion& q1, const Quaternion& q2, DOUBLE t)
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Quaternion(DirectX::XMQuaternionNormalize(DirectX::XMVectorSet(
				q1.x * (1.f - ft) + q2.x * ft,
				q1.y * (1.f - ft) + q2.y * ft,
				q1.z * (1.f - ft) + q2.z * ft,
				q1.w * (1.f - ft) + q2.w * ft))));
		}
		static Quaternion SLerp(const Quaternion& q1, const Quaternion& q2, FLOAT t) { return (Quaternion(DirectX::XMQuaternionSlerp(DirectX::XMVectorSet(q1.x, q1.y, q1.z, q1.w), DirectX::XMVectorSet(q2.x, q2.y, q2.z, q2.w), t))); }
		static Quaternion SLerp(const Quaternion& q1, const Quaternion& q2, DOUBLE t) { return (Quaternion(DirectX::XMQuaternionSlerp(DirectX::XMVectorSet(q1.x, q1.y, q1.z, q1.w), DirectX::XMVectorSet(q2.x, q2.y, q2.z, q2.w), static_cast<FLOAT>(t)))); }
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
		DirectX::XMMATRIX GetDirectXMatrix()const { return (DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(x, y, z, w))); }
		DirectX::XMFLOAT4X4 GetDirectXValue4x4()const
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, GetDirectXMatrix());
			return tempM;
		}
		DirectX::XMFLOAT4X4 GetDirectXTransposeValue4x4()const
		{
			DirectX::XMFLOAT4X4 tempM;
			DirectX::XMStoreFloat4x4(&tempM, DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(x, y, z, w))));
			return tempM;
		}
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(x, y, z, w)); }
		DirectX::XMFLOAT4 GetDirectXValue4()const { return (DirectX::XMFLOAT4(x, y, z, w)); }
		Matrix4x4 GetMatrix()const { return (Matrix4x4(GetDirectXValue4x4())); }
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
		Quaternion& operator=(const Quaternion& q)
		{
			x = q.x; y = q.y; z = q.z; w = q.w;
			return (*this);
		}
		Quaternion& operator*=(const Quaternion& q)
		{
			DirectX::XMFLOAT4 tempQ;
			DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionMultiply(DirectX::XMVectorSet(x, y, z, w), DirectX::XMVectorSet(q.x, q.y, q.z, q.w)));
			x = tempQ.x; y = tempQ.y; z = tempQ.z; w = tempQ.w;
			return (*this);
		}

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
		constexpr Vector2(const DirectX::XMFLOAT2& xv)noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2(const DirectX::XMFLOAT3& xv)noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2(const DirectX::XMFLOAT4& xv)noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2(const DirectX::XMINT2& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)) {}
		constexpr Vector2(const DirectX::XMINT3& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)) {}
		constexpr Vector2(const DirectX::XMINT4& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)) {}
		constexpr Vector2(FLOAT v)noexcept : x(v), y(v) {}
		constexpr Vector2(FLOAT vx, FLOAT vy)noexcept : x(vx), y(vy) {}
		constexpr Vector2(INT32 v)noexcept : x(EMath::TruncToFloat(v)), y(EMath::TruncToFloat(v)) {}
		constexpr Vector2(INT32 vx, INT32 vy)noexcept : x(EMath::TruncToFloat(vx)), y(EMath::TruncToFloat(vy)) {}
		constexpr Vector2(DOUBLE v)noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)) {}
		constexpr Vector2(DOUBLE vx, DOUBLE vy)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)) {}
		Vector2()noexcept : x(0.f), y(0.f) {}
		Vector2(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, xv);
			x = tempV.x; y = tempV.y;
		}
		Vector2(const Vector2& vv)noexcept : x(vv.x), y(vv.y) {}
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
		static Vector2 Lerp(const Vector2& v1, const Vector2& v2, FLOAT t) { return (Vector2(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t)); }
		static Vector2 Reciprocal(const Vector2& vv) { return Vector2(1.f / vv.x, 1.f / vv.y); }
		void SetDirectXVector(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, xv);
			x = tempV.x; y = tempV.y;
		}
		void SetDirectXValue2(const DirectX::XMFLOAT2& xv) { x = xv.x; y = xv.y; }
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(x, y, 0.f, 0.f)); }
		DirectX::XMFLOAT2 GetDirectXValue2()const { return (DirectX::XMFLOAT2(x, y)); }
		DirectX::XMFLOAT3 GetDirectXValue3()const { return (DirectX::XMFLOAT3(x, y, 0.f)); }
		DirectX::XMFLOAT4 GetDirectXValue4()const { return (DirectX::XMFLOAT4(x, y, 0.f, 0.f)); }
		void Reset() { (*this) = Vector2::Zero(); }
		void Normalize()
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, DirectX::XMVector2Normalize(DirectX::XMVectorSet(x, y, 0.f, 0.f)));
			x = tempV.x; y = tempV.y;
		}
		FLOAT Dot(const Vector2& vv)const { return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(DirectX::XMVectorSet(x, y, 0.f, 0.f), DirectX::XMVectorSet(vv.x, vv.y, 0.f, 0.f)))); }
		FLOAT Length()const { return (DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSet(x, y, 0.f, 0.f)))); }
		FLOAT LengthSquare()const { return (DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(DirectX::XMVectorSet(x, y, 0.f, 0.f)))); }
		FLOAT Distance(const Vector2& vv)const
		{
			Vector2 tempV(x - vv.x, y - vv.y);
			return (DirectX::XMVectorGetX(DirectX::XMVector2Length(DirectX::XMVectorSet(tempV.x, tempV.y, 0.f, 0.f))));
		}
		FLOAT DistanceSquare(const Vector2& vv)const
		{
			Vector2 tempV(x - vv.x, y - vv.y);
			return (DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(DirectX::XMVectorSet(tempV.x, tempV.y, 0.f, 0.f))));
		}
		Vector2 Lerp(const Vector2& vv, FLOAT t)const { return (Vector2(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t)); }
		Vector2 Reciprocal()const { return Vector2(1.f / x, 1.f / y); }
		Vector2 operator-()const
		{
			return Vector2(-x, -y);
		}
		Vector2& operator=(FLOAT v)
		{
			x = y = v;
			return (*this);
		}
		Vector2& operator=(const Vector2& vv)
		{
			x = vv.x; y = vv.y;
			return (*this);
		}
		Vector2& operator+=(FLOAT v)
		{
			x += v; y += v;
			return (*this);
		}
		Vector2& operator-=(FLOAT v)
		{
			x -= v; y -= v;
			return (*this);
		}
		Vector2& operator*=(FLOAT v)
		{
			x *= v; y *= v;
			return (*this);
		}
		Vector2& operator/=(FLOAT v)
		{
			x /= v; y /= v;
			return (*this);
		}
		Vector2& operator+=(const Vector2& vv)
		{
			x += vv.x; y += vv.y;
			return (*this);
		}
		Vector2& operator-=(const Vector2& vv)
		{
			x -= vv.x; y -= vv.y;
			return (*this);
		}
		Vector2& operator*=(const Vector2& vv)
		{
			x *= vv.x; y *= vv.y;
			return (*this);
		}
		Vector2& operator/=(const Vector2& vv)
		{
			x /= vv.x; y /= vv.y;
			return (*this);
		}

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
		constexpr Vector3(const DirectX::XMFLOAT2& xv)noexcept : x(xv.x), y(xv.y), z(0.f) {}
		constexpr Vector3(const DirectX::XMFLOAT3& xv)noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3(const DirectX::XMFLOAT4& xv)noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3(const DirectX::XMINT2& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)), z(0.f) {}
		constexpr Vector3(const DirectX::XMINT3& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)), z(EMath::TruncToFloat(xv.z)) {}
		constexpr Vector3(const DirectX::XMINT4& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)), z(EMath::TruncToFloat(xv.z)) {}
		constexpr Vector3(FLOAT v)noexcept : x(v), y(v), z(v) {}
		constexpr Vector3(FLOAT vx, FLOAT vy)noexcept : x(vx), y(vy), z(0.f) {}
		constexpr Vector3(FLOAT vx, FLOAT vy, FLOAT vz)noexcept : x(vx), y(vy), z(vz) {}
		constexpr Vector3(INT32 v)noexcept : x(EMath::TruncToFloat(v)), y(EMath::TruncToFloat(v)), z(EMath::TruncToFloat(v)) {}
		constexpr Vector3(INT32 vx, INT32 vy)noexcept : x(EMath::TruncToFloat(vx)), y(EMath::TruncToFloat(vy)), z(0.f) {}
		constexpr Vector3(INT32 vx, INT32 vy, INT32 vz)noexcept : x(EMath::TruncToFloat(vx)), y(EMath::TruncToFloat(vy)), z(EMath::TruncToFloat(vz)) {}
		constexpr Vector3(DOUBLE v)noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		constexpr Vector3(DOUBLE vx, DOUBLE vy)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f) {}
		constexpr Vector3(DOUBLE vx, DOUBLE vy, DOUBLE vz)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		Vector3()noexcept : x(0.f), y(0.f), z(0.f) {}
		Vector3(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		Vector3(const Vector3& vv)noexcept : x(vv.x), y(vv.y), z(vv.z) {}
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
		static Vector3 Lerp(const Vector3& v1, const Vector3& v2, FLOAT t) { return (Vector3(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t)); }
		static Vector3 Lerp(const Vector3& v1, const Vector3& v2, DOUBLE t)
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
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(x, y, z, 0.f)); }
		DirectX::XMFLOAT2 GetDirectXValue2()const { return (DirectX::XMFLOAT2(x, y)); }
		DirectX::XMFLOAT3 GetDirectXValue3()const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4()const { return (DirectX::XMFLOAT4(x, y, z, 0.f)); }
		void Reset() { (*this) = Vector3::Zero(); }
		void Normalize()
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, DirectX::XMVector3Normalize(DirectX::XMVectorSet(x, y, z, 0.f)));
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		FLOAT Dot(const Vector3& vv)const { return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVectorSet(x, y, z, 0.f), DirectX::XMVectorSet(vv.x, vv.y, vv.z, 0.f)))); }
		FLOAT Length()const { return (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSet(x, y, z, 0.f)))); }
		FLOAT LengthSquare()const
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(x, y, z, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(tempV, tempV)));
		}
		FLOAT Distance(const Vector3& vv)const { return (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, 0.f)))); }
		FLOAT DistanceSquare(const Vector3& vv)const
		{
			DirectX::XMVECTOR tempV = DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, 0.f);
			return (DirectX::XMVectorGetX(DirectX::XMVector3Dot(tempV, tempV)));
		}
		Vector3 Lerp(const Vector3& vv, FLOAT t)const { return (Vector3(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t)); }
		Vector3 Lerp(const Vector3& vv, DOUBLE t)const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Vector3(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft));
		}
		Vector3 Cross(const Vector3& vv)const
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, DirectX::XMVector3Cross(DirectX::XMVectorSet(x, y, z, 0.f), DirectX::XMVectorSet(vv.x, vv.y, vv.z, 0.f)));
			return (Vector3(tempV.x, tempV.y, tempV.z));
		}
		Vector3 operator-()const
		{
			return Vector3(-x, -y, -z);
		}
		Vector3& operator=(FLOAT v)
		{
			x = y = z = v;
			return (*this);
		}
		Vector3& operator=(const Vector3& vv)
		{
			x = vv.x; y = vv.y; z = vv.z;
			return (*this);
		}
		Vector3& operator+=(FLOAT v)
		{
			x += v; y += v; z += v;
			return (*this);
		}
		Vector3& operator-=(FLOAT v)
		{
			x -= v; y -= v; z -= v;
			return (*this);
		}
		Vector3& operator*=(FLOAT v)
		{
			x *= v; y *= v; z *= v;
			return (*this);
		}
		Vector3& operator/=(FLOAT v)
		{
			x /= v; y /= v; z /= v;
			return (*this);
		}
		Vector3& operator+=(const Vector3& vv)
		{
			x += vv.x; y += vv.y; z += vv.z;
			return (*this);
		}
		Vector3& operator-=(const Vector3& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z;
			return (*this);
		}
		Vector3& operator*=(const Vector3 & vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z;
			return (*this);
		}
		Vector3& operator/=(const Vector3 & vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z;
			return (*this);
		}

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
		constexpr Vector4(const DirectX::XMFLOAT2& xv)noexcept : x(xv.x), y(xv.y), z(0.f), w(0.f) {}
		constexpr Vector4(const DirectX::XMFLOAT3& xv)noexcept : x(xv.x), y(xv.y), z(xv.z), w(0.f) {}
		constexpr Vector4(const DirectX::XMFLOAT4& xv)noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		constexpr Vector4(const DirectX::XMINT2& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)), z(0.f), w(0.f) {}
		constexpr Vector4(const DirectX::XMINT3& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)), z(EMath::TruncToFloat(xv.z)), w(0.f) {}
		constexpr Vector4(const DirectX::XMINT4& xv)noexcept : x(EMath::TruncToFloat(xv.x)), y(EMath::TruncToFloat(xv.y)), z(EMath::TruncToFloat(xv.z)), w(EMath::TruncToFloat(xv.w)) {}
		constexpr Vector4(FLOAT v)noexcept : x(v), y(v), z(v), w(v) {}
		constexpr Vector4(FLOAT vx, FLOAT vy)noexcept : x(vx), y(vy), z(0.f), w(0.f) {}
		constexpr Vector4(FLOAT vx, FLOAT vy, FLOAT vz)noexcept : x(vx), y(vy), z(vz), w(0.f) {}
		constexpr Vector4(FLOAT vx, FLOAT vy, FLOAT vz, FLOAT vw)noexcept : x(vx), y(vy), z(vz), w(vw) {}
		constexpr Vector4(INT32 v)noexcept : x(EMath::TruncToFloat(v)), y(EMath::TruncToFloat(v)), z(EMath::TruncToFloat(v)), w(EMath::TruncToFloat(v)) {}
		constexpr Vector4(INT32 vx, INT32 vy)noexcept : x(EMath::TruncToFloat(vx)), y(EMath::TruncToFloat(vy)), z(0.f), w(0.f) {}
		constexpr Vector4(INT32 vx, INT32 vy, INT32 vz)noexcept : x(EMath::TruncToFloat(vx)), y(EMath::TruncToFloat(vy)), z(EMath::TruncToFloat(vz)), w(0.f) {}
		constexpr Vector4(INT32 vx, INT32 vy, INT32 vz, INT32 vw)noexcept : x(EMath::TruncToFloat(vx)), y(EMath::TruncToFloat(vy)), z(EMath::TruncToFloat(vz)), w(EMath::TruncToFloat(vw)) {}
		constexpr Vector4(DOUBLE v)noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)), w(static_cast<FLOAT>(v)) {}
		constexpr Vector4(DOUBLE vx, DOUBLE vy)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(0.f), w(0.f) {}
		constexpr Vector4(DOUBLE vx, DOUBLE vy, DOUBLE vz)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(0.f) {}
		constexpr Vector4(DOUBLE vx, DOUBLE vy, DOUBLE vz, DOUBLE vw)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		Vector4()noexcept : x(0.f), y(0.f), z(0.f), w(0.f) {}
		Vector4(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		Vector4(const Vector4& vv)noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
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
		static Vector4 Lerp(const Vector4& v1, const Vector4& v2, FLOAT t) { return (Vector4(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t, v1.w * (1.f - t) + v2.w * t)); }
		static Vector4 Lerp(const Vector4& v1, const Vector4& v2, DOUBLE t)
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
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(x, y, z, w)); }
		DirectX::XMFLOAT2 GetDirectXValue2()const { return (DirectX::XMFLOAT2(x, y)); }
		DirectX::XMFLOAT3 GetDirectXValue3()const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4()const { return (DirectX::XMFLOAT4(x, y, z, w)); }
		void Reset() { (*this) = Vector4::Zero(); }
		void Normalize()
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, DirectX::XMVector4Normalize(DirectX::XMVectorSet(x, y, z, w)));
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		FLOAT Dot(const Vector4& vv)const { return (DirectX::XMVectorGetX(DirectX::XMVector4Dot(DirectX::XMVectorSet(x, y, z, w), DirectX::XMVectorSet(vv.x, vv.y, vv.z, vv.w)))); }
		FLOAT Length()const { return (DirectX::XMVectorGetX(DirectX::XMVector4Length(DirectX::XMVectorSet(x, y, z, w)))); }
		FLOAT LengthSquare()const { return (DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(DirectX::XMVectorSet(x, y, z, w)))); }
		FLOAT Distance(const Vector4& vv)const { return (DirectX::XMVectorGetX(DirectX::XMVector4Length(DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, w - vv.w)))); }
		FLOAT DistanceSquare(const Vector4& vv)const { return (DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(DirectX::XMVectorSet(x - vv.x, y - vv.y, z - vv.z, w - vv.w)))); }
		Vector4 Lerp(const Vector4& vv, FLOAT t)const { return (Vector4(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t, w * (1.f - t) + vv.w * t)); }
		Vector4 Lerp(const Vector4& vv, DOUBLE t)const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Vector4(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft, w * (1.f - ft) + vv.w * ft));
		}
		Vector4 operator-()const
		{
			return Vector4(-x, -y, -z, -w);
		}
		Vector4& operator=(FLOAT v)
		{
			x = y = z = w = v;
			return (*this);
		}
		Vector4& operator=(const Vector4& vv)
		{
			x = vv.x; y = vv.y; z = vv.z; w = vv.w;
			return (*this);
		}
		Vector4& operator+=(FLOAT v)
		{
			x += v; y += v; z += v; w += v;
			return (*this);
		}
		Vector4& operator-=(FLOAT v)
		{
			x -= v; y -= v; z -= v; w -= v;
			return (*this);
		}
		Vector4& operator*=(FLOAT v)
		{
			x *= v; y *= v; z *= v; w *= v;
			return (*this);
		}
		Vector4& operator/=(FLOAT v)
		{
			x /= v; y /= v; z /= v; w /= v;
			return (*this);
		}
		Vector4& operator+=(const Vector4& vv)
		{
			x += vv.x; y += vv.y; z += vv.z; w += vv.w;
			return (*this);
		}
		Vector4& operator-=(const Vector4& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w;
			return (*this);
		}
		Vector4& operator*=(const Vector4 & vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w;
			return (*this);
		}
		Vector4& operator/=(const Vector4 & vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w;
			return (*this);
		}

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
		constexpr Vector2Int(const DirectX::XMFLOAT2& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)) {}
		constexpr Vector2Int(const DirectX::XMFLOAT3& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)) {}
		constexpr Vector2Int(const DirectX::XMFLOAT4& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)) {}
		constexpr Vector2Int(const DirectX::XMINT2& xv)noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2Int(const DirectX::XMINT3& xv)noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2Int(const DirectX::XMINT4& xv)noexcept : x(xv.x), y(xv.y) {}
		constexpr Vector2Int(FLOAT v)noexcept : x(EMath::TruncToInt32(v)), y(EMath::TruncToInt32(v)) {}
		constexpr Vector2Int(FLOAT vx, FLOAT vy)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)) {}
		constexpr Vector2Int(INT32 v)noexcept : x(v), y(v) {}
		constexpr Vector2Int(INT32 vx, INT32 vy)noexcept : x(vx), y(vy) {}
		constexpr Vector2Int(UINT32 v)noexcept : x(static_cast<INT32>(v)), y(static_cast<INT32>(v)) {}
		constexpr Vector2Int(UINT32 vx, UINT32 vy)noexcept : x(static_cast<INT32>(vx)), y(static_cast<INT32>(vy)) {}
		constexpr Vector2Int(DOUBLE v)noexcept : x(EMath::TruncToInt32(v)), y(EMath::TruncToInt32(v)) {}
		constexpr Vector2Int(DOUBLE vx, DOUBLE vy)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)) {}
		constexpr Vector2Int(const Vector2& vv)noexcept : x(EMath::TruncToInt32(vv.x)), y(EMath::TruncToInt32(vv.y)) {}
		Vector2Int()noexcept : x(0), y(0) {}
		Vector2Int(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT2 tempV;
			DirectX::XMStoreFloat2(&tempV, xv);
			x = EMath::TruncToInt32(tempV.x); y = EMath::TruncToInt32(tempV.y);
		}
		Vector2Int(const Vector2Int& vv)noexcept : x(vv.x), y(vv.y) {}
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
			x = EMath::TruncToInt32(tempV.x); y = EMath::TruncToInt32(tempV.y);
		}
		void SetDirectXValue2(const DirectX::XMINT2& xv) { x = xv.x; y = xv.y; }
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(EMath::TruncToFloat(x), EMath::TruncToFloat(y), 0.f, 0.f)); }
		DirectX::XMINT2 GetDirectXValue2()const { return (DirectX::XMINT2(x, y)); }
		DirectX::XMINT3 GetDirectXValue3()const { return (DirectX::XMINT3(x, y, 0)); }
		DirectX::XMINT4 GetDirectXValue4()const { return (DirectX::XMINT4(x, y, 0, 0)); }
		void Reset() { (*this) = Vector2Int::Zero(); }
		Vector2Int operator-()const
		{
			return Vector2Int(-x, -y);
		}
		Vector2Int& operator=(FLOAT v)
		{
			x = y = EMath::TruncToInt32(v);
			return (*this);
		}
		Vector2Int& operator=(INT32 v)
		{
			x = y = v;
			return (*this);
		}
		Vector2Int& operator=(UINT32 v)
		{
			x = y = static_cast<INT32>(v);
			return (*this);
		}
		Vector2Int& operator=(DOUBLE v)
		{
			x = y = EMath::TruncToInt32(v);
			return (*this);
		}
		Vector2Int& operator=(const Vector2Int& vv)
		{
			x = vv.x; y = vv.y;
			return (*this);
		}
		Vector2Int& operator+=(INT32 v)
		{
			x += v; y += v;
			return (*this);
		}
		Vector2Int& operator-=(INT32 v)
		{
			x -= v; y -= v;
			return (*this);
		}
		Vector2Int& operator*=(INT32 v)
		{
			x *= v; y *= v;
			return (*this);
		}
		Vector2Int& operator/=(INT32 v)
		{
			x /= v; y /= v;
			return (*this);
		}
		Vector2Int& operator+=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x += iv; y += iv;
			return (*this);
		}
		Vector2Int& operator-=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x -= iv; y -= iv;
			return (*this);
		}
		Vector2Int& operator*=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x *= iv; y *= iv;
			return (*this);
		}
		Vector2Int& operator/=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x /= iv; y /= iv;
			return (*this);
		}
		Vector2Int& operator+=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x += iv; y += iv;
			return (*this);
		}
		Vector2Int& operator-=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x -= iv; y -= iv;
			return (*this);
		}
		Vector2Int& operator*=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x *= iv; y *= iv;
			return (*this);
		}
		Vector2Int& operator/=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x /= iv; y /= iv;
			return (*this);
		}
		Vector2Int& operator+=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x += iv; y += iv;
			return (*this);
		}
		Vector2Int& operator-=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x -= iv; y -= iv;
			return (*this);
		}
		Vector2Int& operator*=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x *= iv; y *= iv;
			return (*this);
		}
		Vector2Int& operator/=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x /= iv; y /= iv;
			return (*this);
		}
		Vector2Int& operator+=(const Vector2Int& vv)
		{
			x += vv.x; y += vv.y;
			return (*this);
		}
		Vector2Int& operator-=(const Vector2Int& vv)
		{
			x -= vv.x; y -= vv.y;
			return (*this);
		}
		Vector2Int& operator*=(const Vector2Int& vv)
		{
			x *= vv.x; y *= vv.y;
			return (*this);
		}
		Vector2Int& operator/=(const Vector2Int& vv)
		{
			x /= vv.x; y /= vv.y;
			return (*this);
		}

		union
		{
			struct
			{
				INT32 x, y;
			};
			INT32 v[2];
		};
	};

	struct Vector3Int
	{
		constexpr Vector3Int(const DirectX::XMFLOAT2& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)), z(0) {}
		constexpr Vector3Int(const DirectX::XMFLOAT3& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)), z(EMath::TruncToInt32(xv.z)) {}
		constexpr Vector3Int(const DirectX::XMFLOAT4& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)), z(EMath::TruncToInt32(xv.z)) {}
		constexpr Vector3Int(const DirectX::XMINT2& xv)noexcept : x(xv.x), y(xv.y), z(0) {}
		constexpr Vector3Int(const DirectX::XMINT3& xv)noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3Int(const DirectX::XMINT4& xv)noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Vector3Int(FLOAT v)noexcept : x(EMath::TruncToInt32(v)), y(EMath::TruncToInt32(v)), z(EMath::TruncToInt32(v)) {}
		constexpr Vector3Int(FLOAT vx, FLOAT vy)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(0) {}
		constexpr Vector3Int(FLOAT vx, FLOAT vy, FLOAT vz)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(EMath::TruncToInt32(vz)) {}
		constexpr Vector3Int(INT32 v)noexcept : x(v), y(v), z(v) {}
		constexpr Vector3Int(INT32 vx, INT32 vy)noexcept : x(vx), y(vy), z(0) {}
		constexpr Vector3Int(INT32 vx, INT32 vy, INT32 vz)noexcept : x(vx), y(vy), z(vz) {}
		constexpr Vector3Int(UINT32 v)noexcept : x(static_cast<INT32>(v)), y(static_cast<INT32>(v)), z(static_cast<INT32>(v)) {}
		constexpr Vector3Int(UINT32 vx, UINT32 vy)noexcept : x(static_cast<INT32>(vx)), y(static_cast<INT32>(vy)), z(0) {}
		constexpr Vector3Int(UINT32 vx, UINT32 vy, UINT32 vz)noexcept : x(static_cast<INT32>(vx)), y(static_cast<INT32>(vy)), z(static_cast<INT32>(vz)) {}
		constexpr Vector3Int(DOUBLE v)noexcept : x(EMath::TruncToInt32(v)), y(EMath::TruncToInt32(v)), z(EMath::TruncToInt32(v)) {}
		constexpr Vector3Int(DOUBLE vx, DOUBLE vy)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(0) {}
		constexpr Vector3Int(DOUBLE vx, DOUBLE vy, DOUBLE vz)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(EMath::TruncToInt32(vz)) {}
		constexpr Vector3Int(const Vector3& vv)noexcept : x(EMath::TruncToInt32(vv.x)), y(EMath::TruncToInt32(vv.y)), z(EMath::TruncToInt32(vv.z)) {}
		Vector3Int()noexcept : x(0), y(0), z(0) {}
		Vector3Int(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = EMath::TruncToInt32(tempV.x); y = EMath::TruncToInt32(tempV.y); z = EMath::TruncToInt32(tempV.z);
		}
		Vector3Int(const Vector3Int& vv)noexcept : x(vv.x), y(vv.y), z(vv.z) {}
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
			x = EMath::TruncToInt32(tempV.x); y = EMath::TruncToInt32(tempV.y); z = EMath::TruncToInt32(tempV.z);
		}
		void SetDirectXValue3(const DirectX::XMINT3& xv) { x = xv.x; y = xv.y; z = xv.z; }
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(EMath::TruncToFloat(x), EMath::TruncToFloat(y), EMath::TruncToFloat(z), 0.f)); }
		DirectX::XMINT2 GetDirectXValue2()const { return (DirectX::XMINT2(x, y)); }
		DirectX::XMINT3 GetDirectXValue3()const { return (DirectX::XMINT3(x, y, z)); }
		DirectX::XMINT4 GetDirectXValue4()const { return (DirectX::XMINT4(x, y, z, 0)); }
		void Reset() { (*this) = Vector3Int::Zero(); }
		Vector3Int operator-()const
		{
			return Vector3Int(-x, -y, -z);
		}
		Vector3Int& operator=(FLOAT v)
		{
			x = y = z = EMath::TruncToInt32(v);
			return (*this);
		}
		Vector3Int& operator=(INT32 v)
		{
			x = y = z = v;
			return (*this);
		}
		Vector3Int& operator=(UINT32 v)
		{
			x = y = z = static_cast<INT32>(v);
			return (*this);
		}
		Vector3Int& operator=(DOUBLE v)
		{
			x = y = z = EMath::TruncToInt32(v);
			return (*this);
		}
		Vector3Int& operator=(const Vector3Int& vv)
		{
			x = vv.x; y = vv.y; z = vv.z;
			return (*this);
		}
		Vector3Int& operator+=(INT32 v)
		{
			x += v; y += v; z += v;
			return (*this);
		}
		Vector3Int& operator-=(INT32 v)
		{
			x -= v; y -= v; z -= v;
			return (*this);
		}
		Vector3Int& operator*=(INT32 v)
		{
			x *= v; y *= v; z *= v;
			return (*this);
		}
		Vector3Int& operator/=(INT32 v)
		{
			x /= v; y /= v; z /= v;
			return (*this);
		}
		Vector3Int& operator+=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x += iv; y += iv; z += iv;
			return (*this);
		}
		Vector3Int& operator-=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x -= iv; y -= iv; z -= iv;
			return (*this);
		}
		Vector3Int& operator*=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x *= iv; y *= iv; z *= iv;
			return (*this);
		}
		Vector3Int& operator/=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x /= iv; y /= iv; z /= iv;
			return (*this);
		}
		Vector3Int& operator+=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x += iv; y += iv; z += iv;
			return (*this);
		}
		Vector3Int& operator-=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x -= iv; y -= iv; z -= iv;
			return (*this);
		}
		Vector3Int& operator*=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x *= iv; y *= iv; z *= iv;
			return (*this);
		}
		Vector3Int& operator/=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x /= iv; y /= iv; z /= iv;
			return (*this);
		}
		Vector3Int& operator+=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x += iv; y += iv; z += iv;
			return (*this);
		}
		Vector3Int& operator-=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x -= iv; y -= iv; z -= iv;
			return (*this);
		}
		Vector3Int& operator*=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x *= iv; y *= iv; z *= iv;
			return (*this);
		}
		Vector3Int& operator/=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x /= iv; y /= iv; z /= iv;
			return (*this);
		}
		Vector3Int& operator+=(const Vector3Int& vv)
		{
			x += vv.x; y += vv.y; z += vv.z;
			return (*this);
		}
		Vector3Int& operator-=(const Vector3Int& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z;
			return (*this);
		}
		Vector3Int& operator*=(const Vector3Int& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z;
			return (*this);
		}
		Vector3Int& operator/=(const Vector3Int& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z;
			return (*this);
		}

		union
		{
			struct
			{
				INT32 x, y, z;
			};
			INT32 v[3];
		};
	};

	struct Vector4Int
	{
		constexpr Vector4Int(const DirectX::XMFLOAT2& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)), z(0), w(0) {}
		constexpr Vector4Int(const DirectX::XMFLOAT3& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)), z(EMath::TruncToInt32(xv.z)), w(0) {}
		constexpr Vector4Int(const DirectX::XMFLOAT4& xv)noexcept : x(EMath::TruncToInt32(xv.x)), y(EMath::TruncToInt32(xv.y)), z(EMath::TruncToInt32(xv.z)), w(EMath::TruncToInt32(xv.w)) {}
		constexpr Vector4Int(const DirectX::XMINT2& xv)noexcept : x(xv.x), y(xv.y), z(0), w(0) {}
		constexpr Vector4Int(const DirectX::XMINT3& xv)noexcept : x(xv.x), y(xv.y), z(xv.z), w(0) {}
		constexpr Vector4Int(const DirectX::XMINT4& xv)noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		constexpr Vector4Int(FLOAT v)noexcept : x(EMath::TruncToInt32(v)), y(EMath::TruncToInt32(v)), z(EMath::TruncToInt32(v)), w(EMath::TruncToInt32(v)) {}
		constexpr Vector4Int(FLOAT vx, FLOAT vy)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(0), w(0) {}
		constexpr Vector4Int(FLOAT vx, FLOAT vy, FLOAT vz)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(EMath::TruncToInt32(vz)), w(0) {}
		constexpr Vector4Int(FLOAT vx, FLOAT vy, FLOAT vz, FLOAT vw)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(EMath::TruncToInt32(vz)), w(EMath::TruncToInt32(vw)) {}
		constexpr Vector4Int(INT32 v)noexcept : x(v), y(v), z(v), w(v) {}
		constexpr Vector4Int(INT32 vx, INT32 vy)noexcept : x(vx), y(vy), z(0), w(0) {}
		constexpr Vector4Int(INT32 vx, INT32 vy, INT32 vz)noexcept : x(vx), y(vy), z(vz), w(0) {}
		constexpr Vector4Int(INT32 vx, INT32 vy, INT32 vz, INT32 vw)noexcept : x(vx), y(vy), z(vz), w(vw) {}
		constexpr Vector4Int(UINT32 v)noexcept : x(static_cast<INT32>(v)), y(static_cast<INT32>(v)), z(static_cast<INT32>(v)), w(static_cast<INT32>(v)) {}
		constexpr Vector4Int(UINT32 vx, UINT32 vy)noexcept : x(static_cast<INT32>(vx)), y(static_cast<INT32>(vy)), z(0), w(0) {}
		constexpr Vector4Int(UINT32 vx, UINT32 vy, UINT32 vz)noexcept : x(static_cast<INT32>(vx)), y(static_cast<INT32>(vy)), z(static_cast<INT32>(vz)), w(0) {}
		constexpr Vector4Int(UINT32 vx, UINT32 vy, UINT32 vz, UINT32 vw)noexcept : x(static_cast<INT32>(vx)), y(static_cast<INT32>(vy)), z(static_cast<INT32>(vz)), w(static_cast<INT32>(vw)) {}
		constexpr Vector4Int(DOUBLE v)noexcept : x(EMath::TruncToInt32(v)), y(EMath::TruncToInt32(v)), z(EMath::TruncToInt32(v)), w(EMath::TruncToInt32(v)) {}
		constexpr Vector4Int(DOUBLE vx, DOUBLE vy)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(0), w(0) {}
		constexpr Vector4Int(DOUBLE vx, DOUBLE vy, DOUBLE vz)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(EMath::TruncToInt32(vz)), w(0) {}
		constexpr Vector4Int(DOUBLE vx, DOUBLE vy, DOUBLE vz, DOUBLE vw)noexcept : x(EMath::TruncToInt32(vx)), y(EMath::TruncToInt32(vy)), z(EMath::TruncToInt32(vz)), w(EMath::TruncToInt32(vw)) {}
		constexpr Vector4Int(const Vector4& vv)noexcept : x(EMath::TruncToInt32(vv.x)), y(EMath::TruncToInt32(vv.y)), z(EMath::TruncToInt32(vv.z)), w(EMath::TruncToInt32(vv.w)) {}
		Vector4Int()noexcept : x(0), y(0), z(0), w(0) {}
		Vector4Int(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = EMath::TruncToInt32(tempV.x); y = EMath::TruncToInt32(tempV.y); z = EMath::TruncToInt32(tempV.z); w = EMath::TruncToInt32(tempV.w);
		}
		Vector4Int(const Vector4Int& vv)noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
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
			x = EMath::TruncToInt32(tempV.x); y = EMath::TruncToInt32(tempV.y); z = EMath::TruncToInt32(tempV.z); w = EMath::TruncToInt32(tempV.w);
		}
		void SetDirectXValue4(const DirectX::XMINT4& xv) { x = xv.x; y = xv.y; z = xv.z; w = xv.w; }
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(EMath::TruncToFloat(x), EMath::TruncToFloat(y), EMath::TruncToFloat(z), EMath::TruncToFloat(w))); }
		DirectX::XMINT2 GetDirectXValue2()const { return (DirectX::XMINT2(x, y)); }
		DirectX::XMINT3 GetDirectXValue3()const { return (DirectX::XMINT3(x, y, z)); }
		DirectX::XMINT4 GetDirectXValue4()const { return (DirectX::XMINT4(x, y, z, w)); }
		void Reset() { (*this) = Vector4Int::Zero(); }
		Vector4Int operator-()const
		{
			return Vector4Int(-x, -y, -z, -w);
		}
		Vector4Int& operator=(FLOAT v)
		{
			x = y = z = w = EMath::TruncToInt32(v);
			return (*this);
		}
		Vector4Int& operator=(INT32 v)
		{
			x = y = z = w = v;
			return (*this);
		}
		Vector4Int& operator=(UINT32 v)
		{
			x = y = z = w = static_cast<INT32>(v);
			return (*this);
		}
		Vector4Int& operator=(DOUBLE v)
		{
			x = y = z = w = EMath::TruncToInt32(v);
			return (*this);
		}
		Vector4Int& operator=(const Vector4Int& vv)
		{
			x = vv.x; y = vv.y; z = vv.z; w = vv.w;
			return (*this);
		}
		Vector4Int& operator+=(INT32 v)
		{
			x += v; y += v; z += v; w += v;
			return (*this);
		}
		Vector4Int& operator-=(INT32 v)
		{
			x -= v; y -= v; z -= v; w -= v;
			return (*this);
		}
		Vector4Int& operator*=(INT32 v)
		{
			x *= v; y *= v; z *= v; w *= v;
			return (*this);
		}
		Vector4Int& operator/=(INT32 v)
		{
			x /= v; y /= v; z /= v; w /= v;
			return (*this);
		}
		Vector4Int& operator+=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x += iv; y += iv; z += iv; w += iv;
			return (*this);
		}
		Vector4Int& operator-=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x -= iv; y -= iv; z -= iv; w -= iv;
			return (*this);
		}
		Vector4Int& operator*=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x *= iv; y *= iv; z *= iv; w *= iv;
			return (*this);
		}
		Vector4Int& operator/=(FLOAT v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x /= iv; y /= iv; z /= iv; w /= iv;
			return (*this);
		}
		Vector4Int& operator+=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x += iv; y += iv; z += iv; w += iv;
			return (*this);
		}
		Vector4Int& operator-=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x -= iv; y -= iv; z -= iv; w -= iv;
			return (*this);
		}
		Vector4Int& operator*=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x *= iv; y *= iv; z *= iv; w *= iv;
			return (*this);
		}
		Vector4Int& operator/=(DOUBLE v)
		{
			INT32 iv = EMath::TruncToInt32(v);
			x /= iv; y /= iv; z /= iv; w /= iv;
			return (*this);
		}
		Vector4Int& operator+=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x += iv; y += iv; z += iv; w += iv;
			return (*this);
		}
		Vector4Int& operator-=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x -= iv; y -= iv; z -= iv; w -= iv;
			return (*this);
		}
		Vector4Int& operator*=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x *= iv; y *= iv; z *= iv; w *= iv;
			return (*this);
		}
		Vector4Int& operator/=(UINT32 v)
		{
			INT32 iv = static_cast<INT32>(v);
			x /= iv; y /= iv; z /= iv; w /= iv;
			return (*this);
		}
		Vector4Int& operator+=(const Vector4Int& vv)
		{
			x += vv.x; y += vv.y; z += vv.z; w += vv.w;
			return (*this);
		}
		Vector4Int& operator-=(const Vector4Int& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w;
			return (*this);
		}
		Vector4Int& operator*=(const Vector4Int& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w;
			return (*this);
		}
		Vector4Int& operator/=(const Vector4Int& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w;
			return (*this);
		}

		union
		{
			struct
			{
				INT32 x, y, z, w;
			};
			INT32 v[4];
		};
	};

	struct Color3
	{
		constexpr Color3(const DirectX::XMFLOAT3& xv)noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Color3(const DirectX::XMFLOAT4& xv)noexcept : x(xv.x), y(xv.y), z(xv.z) {}
		constexpr Color3(FLOAT v)noexcept : x(v), y(v), z(v) {}
		constexpr Color3(FLOAT vx, FLOAT vy, FLOAT vz)noexcept : x(vx), y(vy), z(vz) {}
		constexpr Color3(FLOAT vx, FLOAT vy, FLOAT vz, FLOAT vw)noexcept : x(vx), y(vy), z(vz) {}
		constexpr Color3(DOUBLE v)noexcept : x(static_cast<FLOAT>(v)), y(static_cast<FLOAT>(v)), z(static_cast<FLOAT>(v)) {}
		constexpr Color3(DOUBLE vx, DOUBLE vy, DOUBLE vz)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		constexpr Color3(DOUBLE vx, DOUBLE vy, DOUBLE vz, DOUBLE vw)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)) {}
		constexpr Color3(const Vector3& vv)noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		constexpr Color3(const Vector4& vv)noexcept : x(vv.x), y(vv.y), z(vv.z) {}
		Color3()noexcept : x(0.f), y(0.f), z(0.f) {}
		Color3(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT3 tempV;
			DirectX::XMStoreFloat3(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z;
		}
		Color3(const Color3& vv)noexcept : x(vv.x), y(vv.y), z(vv.z) {}
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
		static Color3 Lerp(const Color3& v1, const Color3& v2, FLOAT t) { return (Color3(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t)); }
		static Color3 Lerp(const Color3& v1, const Color3& v2, DOUBLE t)
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
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(x, y, z, 1.f)); }
		DirectX::XMFLOAT3 GetDirectXValue3()const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4()const { return (DirectX::XMFLOAT4(x, y, z, 1.f)); }
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
		Color3 Lerp(const Color3& vv, FLOAT t) const { return (Color3(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t)); }
		Color3 Lerp(const Color3& vv, DOUBLE t) const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Color3(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft));
		}
		Color3& operator=(FLOAT v)
		{
			x = y = z = v;
			return (*this);
		}
		Color3& operator=(DOUBLE v)
		{
			x = y = z = static_cast<FLOAT>(v);
			return (*this);
		}
		Color3& operator=(const Vector3& vv)
		{
			x = vv.x; y = vv.y; z = vv.z;
			return (*this);
		}
		Color3& operator=(const Vector4& vv)
		{
			x = vv.x; y = vv.y; z = vv.z;
			return (*this);
		}
		Color3& operator=(const Color3& vv)
		{
			x = vv.x; y = vv.y; z = vv.z;
			return (*this);
		}
		Color3& operator+=(const Vector3& vv)
		{
			x += vv.x; y += vv.y; z += vv.z;
			return (*this);
		}
		Color3& operator-=(const Vector3& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z;
			return (*this);
		}
		Color3& operator*=(const Vector3& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z;
			return (*this);
		}
		Color3& operator/=(const Vector3& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z;
			return (*this);
		}
		Color3& operator+=(const Vector4& vv)
		{
			x += vv.x; y += vv.y; z += vv.z;
			return (*this);
		}
		Color3& operator-=(const Vector4& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z;
			return (*this);
		}
		Color3& operator*=(const Vector4& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z;
			return (*this);
		}
		Color3& operator/=(const Vector4& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z;
			return (*this);
		}
		Color3& operator+=(const Color3& vv)
		{
			x += vv.x; y += vv.y; z += vv.z;
			return (*this);
		}
		Color3& operator-=(const Color3& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z;
			return (*this);
		}
		Color3& operator*=(const Color3& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z;
			return (*this);
		}
		Color3& operator/=(const Color3& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z;
			return (*this);
		}

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
		constexpr Color4(const DirectX::XMFLOAT3& xv)noexcept : x(xv.x), y(xv.y), z(xv.z), w(1.f) {}
		constexpr Color4(const DirectX::XMFLOAT4& xv)noexcept : x(xv.x), y(xv.y), z(xv.z), w(xv.w) {}
		constexpr Color4(FLOAT vx, FLOAT vy, FLOAT vz)noexcept : x(vx), y(vy), z(vz), w(1.f) {}
		constexpr Color4(FLOAT vx, FLOAT vy, FLOAT vz, FLOAT vw)noexcept : x(vx), y(vy), z(vz), w(vw) {}
		constexpr Color4(DOUBLE vx, DOUBLE vy, DOUBLE vz)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(1.f) {}
		constexpr Color4(DOUBLE vx, DOUBLE vy, DOUBLE vz, DOUBLE vw)noexcept : x(static_cast<FLOAT>(vx)), y(static_cast<FLOAT>(vy)), z(static_cast<FLOAT>(vz)), w(static_cast<FLOAT>(vw)) {}
		constexpr Color4(const Vector3& vv)noexcept : x(vv.x), y(vv.y), z(vv.z), w(1.f) {}
		constexpr Color4(const Vector4& vv)noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
		constexpr Color4(const Color3& vv)noexcept : x(vv.x), y(vv.y), z(vv.z), w(1.f) {}
		Color4()noexcept : x(0.f), y(0.f), z(0.f), w(1.f) {}
		Color4(DirectX::CXMVECTOR xv)
		{
			DirectX::XMFLOAT4 tempV;
			DirectX::XMStoreFloat4(&tempV, xv);
			x = tempV.x; y = tempV.y; z = tempV.z; w = tempV.w;
		}
		Color4(const Color4& vv)noexcept : x(vv.x), y(vv.y), z(vv.z), w(vv.w) {}
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
		static Color4 Lerp(const Color4& v1, const Color4& v2, FLOAT t) { return (Color4(v1.x * (1.f - t) + v2.x * t, v1.y * (1.f - t) + v2.y * t, v1.z * (1.f - t) + v2.z * t, v1.w * (1.f - t) + v2.w * t)); }
		static Color4 Lerp(const Color4& v1, const Color4& v2, DOUBLE t)
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
		DirectX::XMVECTOR GetDirectXVector()const { return (DirectX::XMVectorSet(x, y, z, w)); }
		DirectX::XMFLOAT3 GetDirectXValue3()const { return (DirectX::XMFLOAT3(x, y, z)); }
		DirectX::XMFLOAT4 GetDirectXValue4()const { return (DirectX::XMFLOAT4(x, y, z, w)); }
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
		Color4 Lerp(const Color4& vv, FLOAT t)const { return (Color4(x * (1.f - t) + vv.x * t, y * (1.f - t) + vv.y * t, z * (1.f - t) + vv.z * t, w * (1.f - t) + vv.w * t)); }
		Color4 Lerp(const Color4& vv, DOUBLE t)const
		{
			FLOAT ft = static_cast<FLOAT>(t);
			return (Color4(x * (1.f - ft) + vv.x * ft, y * (1.f - ft) + vv.y * ft, z * (1.f - ft) + vv.z * ft, w * (1.f - ft) + vv.w * ft));
		}
		Color4& operator=(const Vector3& vv)
		{
			x = vv.x; y = vv.y; z = vv.z; w = 1.f;
			return (*this);
		}
		Color4& operator=(const Vector4& vv)
		{
			x = vv.x; y = vv.y; z = vv.z; w = vv.w;
			return (*this);
		}
		Color4& operator=(const Color4& vv)
		{
			x = vv.x; y = vv.y; z = vv.z; w = vv.w;
			return (*this);
		}
		Color4& operator+=(const Vector3& vv)
		{
			x += vv.x; y += vv.y; z += vv.z;
			return (*this);
		}
		Color4& operator-=(const Vector3& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z;
			return (*this);
		}
		Color4& operator*=(const Vector3& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z;
			return (*this);
		}
		Color4& operator/=(const Vector3& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z;
			return (*this);
		}
		Color4& operator+=(const Vector4& vv)
		{
			x += vv.x; y += vv.y; z += vv.z; w += vv.w;
			return (*this);
		}
		Color4& operator-=(const Vector4& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w;
			return (*this);
		}
		Color4& operator*=(const Vector4& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w;
			return (*this);
		}
		Color4& operator/=(const Vector4& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w;
			return (*this);
		}
		Color4& operator+=(const Color4& vv)
		{
			x += vv.x; y += vv.y; z += vv.z; w += vv.w;
			return (*this);
		}
		Color4& operator-=(const Color4& vv)
		{
			x -= vv.x; y -= vv.y; z -= vv.z; w -= vv.w;
			return (*this);
		}
		Color4& operator*=(const Color4& vv)
		{
			x *= vv.x; y *= vv.y; z *= vv.z; w *= vv.w;
			return (*this);
		}
		Color4& operator/=(const Color4& vv)
		{
			x /= vv.x; y /= vv.y; z /= vv.z; w /= vv.w;
			return (*this);
		}

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

#pragma warning(pop)

	Matrix4x4 operator+(const Matrix4x4& lm, const Matrix4x4& rm);
	Matrix4x4 operator-(const Matrix4x4& lm, const Matrix4x4& rm);
	Matrix4x4 operator*(const Matrix4x4& lm, FLOAT rv);
	Matrix4x4 operator*(FLOAT lv, const Matrix4x4& rm);
	Matrix4x4 operator*(const Matrix4x4& lm, const Matrix4x4& rm);
	Matrix4x4 operator*(const Matrix4x4& lm, const Quaternion& rq);
	Matrix4x4 operator*(const Quaternion& lq, const Matrix4x4& rm);
	Quaternion operator*(const Quaternion& lq, const Quaternion& rq);
	Vector2 operator+(const Vector2& lv, const Vector2& rv);
	Vector2 operator+(const Vector2& lv, FLOAT rv);
	Vector2 operator+(FLOAT lv, const Vector2& rv);
	Vector2 operator-(const Vector2& lv, const Vector2& rv);
	Vector2 operator-(const Vector2& lv, FLOAT rv);
	Vector2 operator-(FLOAT lv, const Vector2& rv);
	Vector2 operator*(const Vector2& lv, const Vector2& rv);
	Vector2 operator*(const Vector2& lv, FLOAT rv);
	Vector2 operator*(FLOAT lv, const Vector2& rv);
	Vector2 operator/(const Vector2& lv, const Vector2& rv);
	Vector2 operator/(const Vector2& lv, FLOAT rv);
	Vector2 operator/(FLOAT lv, const Vector2& rv);
	Vector3 operator+(const Vector3& lv, const Vector3& rv);
	Vector3 operator+(const Vector3& lv, FLOAT rv);
	Vector3 operator+(FLOAT lv, const Vector3& rv);
	Vector3 operator-(const Vector3& lv, const Vector3& rv);
	Vector3 operator-(const Vector3& lv, FLOAT rv);
	Vector3 operator-(FLOAT lv, const Vector3& rv);
	Vector3 operator*(const Vector3& lv, const Vector3& rv);
	Vector3 operator*(const Vector3& lv, FLOAT rv);
	Vector3 operator*(FLOAT lv, const Vector3& rv);
	Vector3 operator/(const Vector3& lv, const Vector3& rv);
	Vector3 operator/(const Vector3& lv, FLOAT rv);
	Vector3 operator/(FLOAT lv, const Vector3& rv);
	Vector4 operator+(const Vector4& lv, const Vector4& rv);
	Vector4 operator+(const Vector4& lv, FLOAT rv);
	Vector4 operator+(FLOAT lv, const Vector4& rv);
	Vector4 operator-(const Vector4& lv, const Vector4& rv);
	Vector4 operator-(const Vector4& lv, FLOAT rv);
	Vector4 operator-(FLOAT lv, const Vector4& rv);
	Vector4 operator*(const Vector4& lv, const Vector4& rv);
	Vector4 operator*(const Vector4& lv, FLOAT rv);
	Vector4 operator*(FLOAT lv, const Vector4& rv);
	Vector4 operator/(const Vector4& lv, const Vector4& rv);
	Vector4 operator/(const Vector4& lv, FLOAT rv);
	Vector4 operator/(FLOAT lv, const Vector4& rv);
	Vector2Int operator+(const Vector2Int& lv, const Vector2Int& rv);
	Vector2Int operator+(const Vector2Int& lv, INT32 rv);
	Vector2Int operator+(INT32 lv, const Vector2Int& rv);
	Vector2Int operator-(const Vector2Int& lv, const Vector2Int& rv);
	Vector2Int operator-(const Vector2Int& lv, INT32 rv);
	Vector2Int operator-(INT32 lv, const Vector2Int& rv);
	Vector2Int operator*(const Vector2Int& lv, const Vector2Int& rv);
	Vector2Int operator*(const Vector2Int& lv, INT32 rv);
	Vector2Int operator*(INT32 lv, const Vector2Int& rv);
	Vector2Int operator/(const Vector2Int& lv, const Vector2Int& rv);
	Vector2Int operator/(const Vector2Int& lv, INT32 rv);
	Vector2Int operator/(INT32 lv, const Vector2Int& rv);
	Vector3Int operator+(const Vector3Int& lv, const Vector3Int& rv);
	Vector3Int operator+(const Vector3Int& lv, INT32 rv);
	Vector3Int operator+(INT32 lv, const Vector3Int& rv);
	Vector3Int operator-(const Vector3Int& lv, const Vector3Int& rv);
	Vector3Int operator-(const Vector3Int& lv, INT32 rv);
	Vector3Int operator-(INT32 lv, const Vector3Int& rv);
	Vector3Int operator*(const Vector3Int& lv, const Vector3Int& rv);
	Vector3Int operator*(const Vector3Int& lv, INT32 rv);
	Vector3Int operator*(INT32 lv, const Vector3Int& rv);
	Vector3Int operator/(const Vector3Int& lv, const Vector3Int& rv);
	Vector3Int operator/(const Vector3Int& lv, INT32 rv);
	Vector3Int operator/(INT32 lv, const Vector3Int& rv);
	Vector4Int operator+(const Vector4Int& lv, const Vector4Int& rv);
	Vector4Int operator+(const Vector4Int& lv, INT32 rv);
	Vector4Int operator+(INT32 lv, const Vector4Int& rv);
	Vector4Int operator-(const Vector4Int& lv, const Vector4Int& rv);
	Vector4Int operator-(const Vector4Int& lv, INT32 rv);
	Vector4Int operator-(INT32 lv, const Vector4Int& rv);
	Vector4Int operator*(const Vector4Int& lv, const Vector4Int& rv);
	Vector4Int operator*(const Vector4Int& lv, INT32 rv);
	Vector4Int operator*(INT32 lv, const Vector4Int& rv);
	Vector4Int operator/(const Vector4Int& lv, const Vector4Int& rv);
	Vector4Int operator/(const Vector4Int& lv, INT32 rv);
	Vector4Int operator/(INT32 lv, const Vector4Int& rv);
	Color3 operator+(const Color3& lv, FLOAT rv);
	Color3 operator+(FLOAT lv, const Color3& rv);
	Color3 operator+(const Color3& lv, const Color3& rv);
	Color3 operator+(const Color3& lv, const Vector3& rv);
	Color3 operator+(const Vector3& lv, const Color3& rv);
	Color4 operator+(const Color4& lv, const Color4& rv);
	Color4 operator+(const Color4& lv, const Vector4& rv);
	Color4 operator+(const Vector4& lv, const Color4& rv);
	Color3 operator+(const Color3& lv, const Vector4& rv);
	Color3 operator+(const Vector4& lv, const Color3& rv);
	Color3 operator+(const Color4& lv, const Vector3& rv);
	Color3 operator+(const Vector3& lv, const Color4& rv);
	Color3 operator+(const Color4& lv, const Color3& rv);
	Color3 operator+(const Color3& lv, const Color4& rv);
	Color3 operator-(const Color3& lv, FLOAT rv);
	Color3 operator-(FLOAT lv, const Color3& rv);
	Color3 operator-(const Color3& lv, const Color3& rv);
	Color3 operator-(const Color3& lv, const Vector3& rv);
	Color3 operator-(const Vector3& lv, const Color3& rv);
	Color4 operator-(const Color4& lv, const Color4& rv);
	Color4 operator-(const Color4& lv, const Vector4& rv);
	Color4 operator-(const Vector4& lv, const Color4& rv);
	Color3 operator-(const Color3& lv, const Vector4& rv);
	Color3 operator-(const Vector4& lv, const Color3& rv);
	Color3 operator-(const Color4& lv, const Vector3& rv);
	Color3 operator-(const Vector3& lv, const Color4& rv);
	Color3 operator-(const Color4& lv, const Color3& rv);
	Color3 operator-(const Color3& lv, const Color4& rv);
	Color3 operator*(const Color3& lv, FLOAT rv);
	Color3 operator*(FLOAT lv, const Color3& rv);
	Color3 operator*(const Color3& lv, const Color3& rv);
	Color3 operator*(const Color3& lv, const Vector3& rv);
	Color3 operator*(const Vector3& lv, const Color3& rv);
	Color4 operator*(const Color4& lv, const Color4& rv);
	Color4 operator*(const Color4& lv, const Vector4& rv);
	Color4 operator*(const Vector4& lv, const Color4& rv);
	Color3 operator*(const Color3& lv, const Vector4& rv);
	Color3 operator*(const Vector4& lv, const Color3& rv);
	Color3 operator*(const Color4& lv, const Vector3& rv);
	Color3 operator*(const Vector3& lv, const Color4& rv);
	Color3 operator*(const Color4& lv, const Color3& rv);
	Color3 operator*(const Color3& lv, const Color4& rv);
	Color3 operator/(const Color3& lv, FLOAT rv);
	Color3 operator/(FLOAT lv, const Color3& rv);
	Color3 operator/(const Color3& lv, const Color3& rv);
	Color3 operator/(const Color3& lv, const Vector3& rv);
	Color3 operator/(const Vector3& lv, const Color3& rv);
	Color4 operator/(const Color4& lv, const Color4& rv);
	Color4 operator/(const Color4& lv, const Vector4& rv);
	Color4 operator/(const Vector4& lv, const Color4& rv);
	Color3 operator/(const Color3& lv, const Vector4& rv);
	Color3 operator/(const Vector4& lv, const Color3& rv);
	Color3 operator/(const Color4& lv, const Vector3& rv);
	Color3 operator/(const Vector3& lv, const Color4& rv);
	Color3 operator/(const Color4& lv, const Color3& rv);
	Color3 operator/(const Color3& lv, const Color4& rv);

	extern PE_INLINE Matrix4x4 MakeTranslationMatrix4x4(const Vector3& InTranslation);
	extern PE_INLINE Matrix4x4 MakeRotationMatrix4x4(const Quaternion& InRotation);
	extern PE_INLINE Matrix4x4 MakeScalingMatrix4x4(const Vector3& InScaling);
	extern PE_INLINE Matrix4x4 MakeMatrix4x4(const Quaternion& InRotation);
	extern PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Quaternion& InRotation);
	extern PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Vector3& InScaling);
	extern PE_INLINE Matrix4x4 MakeMatrix4x4(const Quaternion& InRotation, const Vector3& InScaling);
	extern PE_INLINE Matrix4x4 MakeMatrix4x4(const Vector3& InTranslation, const Quaternion& InRotation, const Vector3& InScaling);
	extern PE_INLINE Matrix4x4 InverseMatrix4x4(const Matrix4x4& InMatrix, Vector4* OutDeterminant = nullptr);
	extern PE_INLINE Vector4 Matrix4x4TransformVector(const Matrix4x4& m, const Vector4& v);
	extern PE_INLINE Vector3 Matrix4x4TransformPosition(const Matrix4x4& m, const Vector3& v);
	extern PE_INLINE Vector3 Matrix4x4TransformDirection(const Matrix4x4& m, const Vector3& v);
	extern PE_INLINE Quaternion MakeQuaternion(const Matrix4x4& m);
	extern PE_INLINE Quaternion MakeQuaternion(const Vector4& v);
	extern PE_INLINE Quaternion MakeQuaternion(const Vector3& InAxis, FLOAT InRadian);
	extern PE_INLINE Vector3 QuaternionTransformVector(const Quaternion& q, const Vector3& v);
	extern PE_INLINE Vector4 MakeVector4(const Vector3& v, FLOAT w);
	extern PE_INLINE Color3 MakeColor3(const Color4& c);
	extern PE_INLINE Color4 MakeColor4(const Color3& c);
	extern PE_INLINE Vector2 MinVector2(const Vector2& A, const Vector2& B);
	extern PE_INLINE Vector3 MinVector3(const Vector3& A, const Vector3& B);
	extern PE_INLINE Vector4 MinVector4(const Vector4& A, const Vector4& B);
	extern PE_INLINE Vector2Int MinVector2Int(const Vector2Int& A, const Vector2Int& B);
	extern PE_INLINE Vector3Int MinVector3Int(const Vector3Int& A, const Vector3Int& B);
	extern PE_INLINE Vector4Int MinVector4Int(const Vector4Int& A, const Vector4Int& B);
	extern PE_INLINE Vector2 MaxVector2(const Vector2& A, const Vector2& B);
	extern PE_INLINE Vector3 MaxVector3(const Vector3& A, const Vector3& B);
	extern PE_INLINE Vector4 MaxVector4(const Vector4& A, const Vector4& B);
	extern PE_INLINE Vector2Int MaxVector2Int(const Vector2Int& A, const Vector2Int& B);
	extern PE_INLINE Vector3Int MaxVector3Int(const Vector3Int& A, const Vector3Int& B);
	extern PE_INLINE Vector4Int MaxVector4Int(const Vector4Int& A, const Vector4Int& B);

};