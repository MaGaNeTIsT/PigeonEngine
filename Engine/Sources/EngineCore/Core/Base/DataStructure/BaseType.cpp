#include "BaseType.h"

namespace PigeonEngine
{

	Matrix4x4::Matrix4x4(const Quaternion& v)
	{
		(*this) = v.GetMatrix();
	}
	Matrix4x4::Matrix4x4(const Vector3& t, const Quaternion& r)
	{
		(*this) = r.GetMatrix() * TranslationMatrixFromVector3(t);
	}
	Matrix4x4::Matrix4x4(const Vector3& t, const Quaternion& r, const Vector3& s)
	{
		(*this) = ScalingMatrixFromVector3(s) * r.GetMatrix() * TranslationMatrixFromVector3(t);
	}
	Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& mm, Vector4* det)
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
	Matrix4x4 Matrix4x4::TranslationMatrixFromVector3(const Vector3& offset)
	{
		return (Matrix4x4(DirectX::XMMatrixTranslation(offset.x, offset.y, offset.z)));
	}
	Matrix4x4 Matrix4x4::ScalingMatrixFromVector3(const Vector3& scl)
	{
		return (Matrix4x4(DirectX::XMMatrixScaling(scl.x, scl.y, scl.z)));
	}
	Matrix4x4 Matrix4x4::RotationPitchYawRollMatrixFromVector3(const Vector3& rot)
	{
		return (Matrix4x4(DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z)));
	}
	Quaternion Matrix4x4::GetQuaternion()const
	{
		return Quaternion(GetDirectXValue4x4());
	}
	Matrix4x4 Matrix4x4::Inverse(Vector4* det)const
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
	Vector4 Matrix4x4::MultiplyVector(const Vector4& v)const
	{
		return (Vector4(DirectX::XMVector4Transform(DirectX::XMVectorSet(v.x, v.y, v.z, v.w), GetDirectXMatrix())));
	}
	Vector3 Matrix4x4::MultiplyPosition(const Vector3& v)const
	{
		return (Vector3(DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(v.x, v.y, v.z, 1.f), GetDirectXMatrix())));
	}
	Vector3 Matrix4x4::MultiplyDirection(const Vector3& v)const
	{
		return (Vector3(DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(v.x, v.y, v.z, 0.f), GetDirectXMatrix())));
	}
	void Matrix4x4::operator*=(const Matrix4x4& mm)
	{
		(*this) = (*this) * mm;
	}

	constexpr Quaternion::Quaternion(const Vector4& q)noexcept
		: x(q.x), y(q.y), z(q.z), w(q.w)
	{
	}
	Quaternion::Quaternion(const Vector3& axis, FLOAT radian)
	{
		DirectX::XMFLOAT4 tempQ;
		DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(axis.x, axis.y, axis.z, 0.f), radian));
		x = tempQ.x; y = tempQ.y;
		z = tempQ.z; w = tempQ.w;
	}
	Quaternion Quaternion::RotationAxis(const Vector3& axis, FLOAT radian)
	{
		return (Quaternion(DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(axis.x, axis.y, axis.z, 0.f), radian)));
	}
	Vector3 Quaternion::MultiplyVector(const Vector3& v)const
	{
		return (Vector3(DirectX::XMVector3Rotate(DirectX::XMVectorSet(v.x, v.y, v.z, 0.f), DirectX::XMVectorSet(x, y, z, w))));
	}
	void Quaternion::operator=(const Vector4& q)
	{
		x = q.x; y = q.y;
		z = q.z; w = q.w;
	}
	void Quaternion::operator*=(const Vector4& q)
	{
		DirectX::XMFLOAT4 tempQ;
		DirectX::XMStoreFloat4(&tempQ, DirectX::XMQuaternionMultiply(DirectX::XMVectorSet(x, y, z, w), DirectX::XMVectorSet(q.x, q.y, q.z, q.w)));
		x = tempQ.x; y = tempQ.y;
		z = tempQ.z; w = tempQ.w;
	}

	constexpr Vector2::Vector2(const Vector3& vv)noexcept
		: x(vv.x), y(vv.y)
	{
	}
	constexpr Vector2::Vector2(const Vector4& vv)noexcept
		: x(vv.x), y(vv.y)
	{
	}
	constexpr Vector2::Vector2(const Vector2Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y))
	{
	}
	constexpr Vector2::Vector2(const Vector3Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y))
	{
	}
	constexpr Vector2::Vector2(const Vector4Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y))
	{
	}
	void Vector2::operator=(const Vector3& vv)
	{
		x = vv.x;
		y = vv.y;
	}
	void Vector2::operator=(const Vector4& vv)
	{
		x = vv.x;
		y = vv.y;
	}
	void Vector2::operator=(const Vector2Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator=(const Vector3Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator=(const Vector4Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator+=(const Vector3& vv)
	{
		x += vv.x;
		y += vv.y;
	}
	void Vector2::operator-=(const Vector3& vv)
	{
		x -= vv.x;
		y -= vv.y;
	}
	void Vector2::operator*=(const Vector3& vv)
	{
		x *= vv.x;
		y *= vv.y;
	}
	void Vector2::operator/=(const Vector3& vv)
	{
		x /= vv.x;
		y /= vv.y;
	}
	void Vector2::operator+=(const Vector4& vv)
	{
		x += vv.x;
		y += vv.y;
	}
	void Vector2::operator-=(const Vector4& vv)
	{
		x -= vv.x;
		y -= vv.y;
	}
	void Vector2::operator*=(const Vector4& vv)
	{
		x *= vv.x;
		y *= vv.y;
	}
	void Vector2::operator/=(const Vector4& vv)
	{
		x /= vv.x;
		y /= vv.y;
	}
	void Vector2::operator+=(const Vector2Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator-=(const Vector2Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator*=(const Vector2Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator/=(const Vector2Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator+=(const Vector3Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator-=(const Vector3Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator*=(const Vector3Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator/=(const Vector3Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator+=(const Vector4Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator-=(const Vector4Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator*=(const Vector4Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
	}
	void Vector2::operator/=(const Vector4Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
	}

	constexpr Vector3::Vector3(const Vector2& vv)noexcept
		: x(vv.x), y(vv.y), z(0.f)
	{
	}
	constexpr Vector3::Vector3(const Vector4& vv)noexcept
		: x(vv.x), y(vv.y), z(vv.z)
	{
	}
	constexpr Vector3::Vector3(const Vector2Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(0.f)
	{
	}
	constexpr Vector3::Vector3(const Vector3Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z))
	{
	}
	constexpr Vector3::Vector3(const Vector4Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z))
	{
	}
	void Vector3::operator=(const Vector2& vv)
	{
		x = vv.x;
		y = vv.y;
	}
	void Vector3::operator=(const Vector4& vv)
	{
		x = vv.x;
		y = vv.y;
		z = vv.z;
	}
	void Vector3::operator=(const Vector2Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
	}
	void Vector3::operator=(const Vector3Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
		z = static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator=(const Vector4Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
		z = static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator+=(const Vector2& vv)
	{
		x += vv.x;
		y += vv.y;
	}
	void Vector3::operator-=(const Vector2& vv)
	{
		x -= vv.x;
		y -= vv.y;
	}
	void Vector3::operator*=(const Vector2& vv)
	{
		x *= vv.x;
		y *= vv.y;
	}
	void Vector3::operator/=(const Vector2& vv)
	{
		x /= vv.x;
		y /= vv.y;
	}
	void Vector3::operator+=(const Vector4& vv)
	{
		x += vv.x;
		y += vv.y;
		z += vv.z;
	}
	void Vector3::operator-=(const Vector4& vv)
	{
		x -= vv.x;
		y -= vv.y;
		z -= vv.z;
	}
	void Vector3::operator*=(const Vector4& vv)
	{
		x *= vv.x;
		y *= vv.y;
		z *= vv.z;
	}
	void Vector3::operator/=(const Vector4& vv)
	{
		x /= vv.x;
		y /= vv.y;
		z /= vv.z;
	}
	void Vector3::operator+=(const Vector2Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
	}
	void Vector3::operator-=(const Vector2Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
	}
	void Vector3::operator*=(const Vector2Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
	}
	void Vector3::operator/=(const Vector2Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
	}
	void Vector3::operator+=(const Vector3Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
		z += static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator-=(const Vector3Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
		z -= static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator*=(const Vector3Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
		z *= static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator/=(const Vector3Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
		z /= static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator+=(const Vector4Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
		z += static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator-=(const Vector4Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
		z -= static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator*=(const Vector4Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
		z *= static_cast<FLOAT>(vv.z);
	}
	void Vector3::operator/=(const Vector4Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
		z /= static_cast<FLOAT>(vv.z);
	}

	constexpr Vector4::Vector4(const Vector2& vv)noexcept
		: x(vv.x), y(vv.y), z(0.f), w(0.f)
	{
	}
	constexpr Vector4::Vector4(const Vector3& vv)noexcept
		: x(vv.x), y(vv.y), z(vv.z), w(0.f)
	{
	}
	constexpr Vector4::Vector4(const Vector2Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(0.f), w(0.f)
	{
	}
	constexpr Vector4::Vector4(const Vector3Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)), w(0.f)
	{
	}
	constexpr Vector4::Vector4(const Vector4Int& vv)noexcept
		: x(static_cast<FLOAT>(vv.x)), y(static_cast<FLOAT>(vv.y)), z(static_cast<FLOAT>(vv.z)), w(static_cast<FLOAT>(vv.w))
	{
	}
	void Vector4::operator=(const Vector2& vv)
	{
		x = vv.x;
		y = vv.y;
	}
	void Vector4::operator=(const Vector3& vv)
	{
		x = vv.x;
		y = vv.y;
		z = vv.z;
	}
	void Vector4::operator=(const Vector2Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
	}
	void Vector4::operator=(const Vector3Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
		z = static_cast<FLOAT>(vv.z);
	}
	void Vector4::operator=(const Vector4Int& vv)
	{
		x = static_cast<FLOAT>(vv.x);
		y = static_cast<FLOAT>(vv.y);
		z = static_cast<FLOAT>(vv.z);
		w = static_cast<FLOAT>(vv.w);
	}
	void Vector4::operator+=(const Vector2& vv)
	{
		x += vv.x;
		y += vv.y;
	}
	void Vector4::operator-=(const Vector2& vv)
	{
		x -= vv.x;
		y -= vv.y;
	}
	void Vector4::operator*=(const Vector2& vv)
	{
		x *= vv.x;
		y *= vv.y;
	}
	void Vector4::operator/=(const Vector2& vv)
	{
		x /= vv.x;
		y /= vv.y;
	}
	void Vector4::operator+=(const Vector3& vv)
	{
		x += vv.x;
		y += vv.y;
		z += vv.z;
	}
	void Vector4::operator-=(const Vector3& vv)
	{
		x -= vv.x;
		y -= vv.y;
		z -= vv.z;
	}
	void Vector4::operator*=(const Vector3& vv)
	{
		x *= vv.x;
		y *= vv.y;
		z *= vv.z;
	}
	void Vector4::operator/=(const Vector3& vv)
	{
		x /= vv.x;
		y /= vv.y;
		z /= vv.z;
	}
	void Vector4::operator+=(const Vector2Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
	}
	void Vector4::operator-=(const Vector2Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
	}
	void Vector4::operator*=(const Vector2Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
	}
	void Vector4::operator/=(const Vector2Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
	}
	void Vector4::operator+=(const Vector3Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
		z += static_cast<FLOAT>(vv.z);
	}
	void Vector4::operator-=(const Vector3Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
		z -= static_cast<FLOAT>(vv.z);
	}
	void Vector4::operator*=(const Vector3Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
		z *= static_cast<FLOAT>(vv.z);
	}
	void Vector4::operator/=(const Vector3Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
		z /= static_cast<FLOAT>(vv.z);
	}
	void Vector4::operator+=(const Vector4Int& vv)
	{
		x += static_cast<FLOAT>(vv.x);
		y += static_cast<FLOAT>(vv.y);
		z += static_cast<FLOAT>(vv.z);
		w += static_cast<FLOAT>(vv.w);
	}
	void Vector4::operator-=(const Vector4Int& vv)
	{
		x -= static_cast<FLOAT>(vv.x);
		y -= static_cast<FLOAT>(vv.y);
		z -= static_cast<FLOAT>(vv.z);
		w -= static_cast<FLOAT>(vv.w);
	}
	void Vector4::operator*=(const Vector4Int& vv)
	{
		x *= static_cast<FLOAT>(vv.x);
		y *= static_cast<FLOAT>(vv.y);
		z *= static_cast<FLOAT>(vv.z);
		w *= static_cast<FLOAT>(vv.w);
	}
	void Vector4::operator/=(const Vector4Int& vv)
	{
		x /= static_cast<FLOAT>(vv.x);
		y /= static_cast<FLOAT>(vv.y);
		z /= static_cast<FLOAT>(vv.z);
		w /= static_cast<FLOAT>(vv.w);
	}

	constexpr Vector2Int::Vector2Int(const Vector3Int& vv)noexcept
		: x(vv.x), y(vv.y)
	{
	}
	constexpr Vector2Int::Vector2Int(const Vector4Int& vv)noexcept
		: x(vv.x), y(vv.y)
	{
	}
	void Vector2Int::operator=(const Vector3Int& vv)
	{
		x = vv.x;
		y = vv.y;
	}
	void Vector2Int::operator=(const Vector4Int& vv)
	{
		x = vv.x;
		y = vv.y;
	}
	void Vector2Int::operator+=(const Vector3Int& vv)
	{
		x += vv.x;
		y += vv.y;
	}
	void Vector2Int::operator-=(const Vector3Int& vv)
	{
		x -= vv.x;
		y -= vv.y;
	}
	void Vector2Int::operator*=(const Vector3Int& vv)
	{
		x *= vv.x;
		y *= vv.y;
	}
	void Vector2Int::operator/=(const Vector3Int& vv)
	{
		x /= vv.x;
		y /= vv.y;
	}
	void Vector2Int::operator+=(const Vector4Int& vv)
	{
		x += vv.x;
		y += vv.y;
	}
	void Vector2Int::operator-=(const Vector4Int& vv)
	{
		x -= vv.x;
		y -= vv.y;
	}
	void Vector2Int::operator*=(const Vector4Int& vv)
	{
		x *= vv.x;
		y *= vv.y;
	}
	void Vector2Int::operator/=(const Vector4Int& vv)
	{
		x /= vv.x;
		y /= vv.y;
	}

	constexpr Vector3Int::Vector3Int(const Vector4Int& vv)noexcept
		: x(vv.x), y(vv.y), z(vv.z)
	{
	}
	void Vector3Int::operator=(const Vector4Int& vv)
	{
		x = vv.x;
		y = vv.y;
		z = vv.z;
	}
	void Vector3Int::operator+=(const Vector4Int& vv)
	{
		x += vv.x;
		y += vv.y;
		z += vv.z;
	}
	void Vector3Int::operator-=(const Vector4Int& vv)
	{
		x -= vv.x;
		y -= vv.y;
		z -= vv.z;
	}
	void Vector3Int::operator*=(const Vector4Int& vv)
	{
		x *= vv.x;
		y *= vv.y;
		z *= vv.z;
	}
	void Vector3Int::operator/=(const Vector4Int& vv)
	{
		x /= vv.x;
		y /= vv.y;
		z /= vv.z;
	}

	constexpr Color3::Color3(const Color4& vv)noexcept
		: x(vv.x), y(vv.y), z(vv.z)
	{
	}
	void Color3::operator=(const Color4& vv)
	{
		x = vv.x;
		y = vv.y;
		z = vv.z;
	}
	void Color3::operator+=(const Color4& vv)
	{
		x += vv.x;
		y += vv.y;
		z += vv.z;
	}
	void Color3::operator-=(const Color4& vv)
	{
		x -= vv.x;
		y -= vv.y;
		z -= vv.z;
	}
	void Color3::operator*=(const Color4& vv)
	{
		x *= vv.x;
		y *= vv.y;
		z *= vv.z;
	}
	void Color3::operator/=(const Color4& vv)
	{
		x /= vv.x;
		y /= vv.y;
		z /= vv.z;
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
	Vector2Int operator+(const Vector2Int& lv, INT rv) { return (Vector2Int(lv.x + rv, lv.y + rv)); }
	Vector2Int operator+(INT lv, const Vector2Int& rv) { return (Vector2Int(lv + rv.x, lv + rv.y)); }
	Vector2Int operator-(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x - rv.x, lv.y - rv.y)); }
	Vector2Int operator-(const Vector2Int& lv, INT rv) { return (Vector2Int(lv.x - rv, lv.y - rv)); }
	Vector2Int operator-(INT lv, const Vector2Int& rv) { return (Vector2Int(lv - rv.x, lv - rv.y)); }
	Vector2Int operator*(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x * rv.x, lv.y * rv.y)); }
	Vector2Int operator*(const Vector2Int& lv, INT rv) { return (Vector2Int(lv.x * rv, lv.y * rv)); }
	Vector2Int operator*(INT lv, const Vector2Int& rv) { return (Vector2Int(lv * rv.x, lv * rv.y)); }
	Vector2Int operator/(const Vector2Int& lv, const Vector2Int& rv) { return (Vector2Int(lv.x / rv.x, lv.y / rv.y)); }
	Vector2Int operator/(const Vector2Int& lv, INT rv) { return (Vector2Int(lv.x / rv, lv.y / rv)); }
	Vector2Int operator/(INT lv, const Vector2Int& rv) { return (Vector2Int(lv / rv.x, lv / rv.y)); }
	Vector3Int operator+(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z)); }
	Vector3Int operator+(const Vector3Int& lv, INT rv) { return (Vector3Int(lv.x + rv, lv.y + rv, lv.z + rv)); }
	Vector3Int operator+(INT lv, const Vector3Int& rv) { return (Vector3Int(lv + rv.x, lv + rv.y, lv + rv.z)); }
	Vector3Int operator-(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z)); }
	Vector3Int operator-(const Vector3Int& lv, INT rv) { return (Vector3Int(lv.x - rv, lv.y - rv, lv.z - rv)); }
	Vector3Int operator-(INT lv, const Vector3Int& rv) { return (Vector3Int(lv - rv.x, lv - rv.y, lv - rv.z)); }
	Vector3Int operator*(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z)); }
	Vector3Int operator*(const Vector3Int& lv, INT rv) { return (Vector3Int(lv.x * rv, lv.y * rv, lv.z * rv)); }
	Vector3Int operator*(INT lv, const Vector3Int& rv) { return (Vector3Int(lv * rv.x, lv * rv.y, lv * rv.z)); }
	Vector3Int operator/(const Vector3Int& lv, const Vector3Int& rv) { return (Vector3Int(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z)); }
	Vector3Int operator/(const Vector3Int& lv, INT rv) { return (Vector3Int(lv.x / rv, lv.y / rv, lv.z / rv)); }
	Vector3Int operator/(INT lv, const Vector3Int& rv) { return (Vector3Int(lv / rv.x, lv / rv.y, lv / rv.z)); }
	Vector4Int operator+(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x + rv.x, lv.y + rv.y, lv.z + rv.z, lv.w + rv.w)); }
	Vector4Int operator+(const Vector4Int& lv, INT rv) { return (Vector4Int(lv.x + rv, lv.y + rv, lv.z + rv, lv.w + rv)); }
	Vector4Int operator+(INT lv, const Vector4Int& rv) { return (Vector4Int(lv + rv.x, lv + rv.y, lv + rv.z, lv + rv.w)); }
	Vector4Int operator-(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x - rv.x, lv.y - rv.y, lv.z - rv.z, lv.w - rv.w)); }
	Vector4Int operator-(const Vector4Int& lv, INT rv) { return (Vector4Int(lv.x - rv, lv.y - rv, lv.z - rv, lv.w - rv)); }
	Vector4Int operator-(INT lv, const Vector4Int& rv) { return (Vector4Int(lv - rv.x, lv - rv.y, lv - rv.z, lv - rv.w)); }
	Vector4Int operator*(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x * rv.x, lv.y * rv.y, lv.z * rv.z, lv.w * rv.w)); }
	Vector4Int operator*(const Vector4Int& lv, INT rv) { return (Vector4Int(lv.x * rv, lv.y * rv, lv.z * rv, lv.w * rv)); }
	Vector4Int operator*(INT lv, const Vector4Int& rv) { return (Vector4Int(lv * rv.x, lv * rv.y, lv * rv.z, lv * rv.w)); }
	Vector4Int operator/(const Vector4Int& lv, const Vector4Int& rv) { return (Vector4Int(lv.x / rv.x, lv.y / rv.y, lv.z / rv.z, lv.w / rv.w)); }
	Vector4Int operator/(const Vector4Int& lv, INT rv) { return (Vector4Int(lv.x / rv, lv.y / rv, lv.z / rv, lv.w / rv)); }
	Vector4Int operator/(INT lv, const Vector4Int& rv) { return (Vector4Int(lv / rv.x, lv / rv.y, lv / rv.z, lv / rv.w)); }
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