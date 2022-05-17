#include "../../Headers/Base/CBaseType.h"

namespace CustomType
{
	Matrix4x4 Matrix4x4::m_Identity = Matrix4x4::GetIdentity();
	Matrix4x4::Matrix4x4()
	{
		this->SetXMMATRIX(m_Identity.GetXMMATRIX());
	}
	Matrix4x4::Matrix4x4(const Matrix4x4& m)
	{
		this->SetXMMATRIX(m.GetXMMATRIX());
	}
	Matrix4x4::Matrix4x4(DirectX::CXMMATRIX m)
	{
		this->SetXMMATRIX(m);
	}
	Matrix4x4::Matrix4x4(const Quaternion& v)
	{
		this->SetXMMATRIX(v.GetXMMATRIX());
	}
	Matrix4x4::Matrix4x4(const Vector3& t, const Quaternion& r, const Vector3& s)
	{
		this->SetXMMATRIX(XMMatrixScalingFromVector(s.GetXMVECTOR()) *
			r.GetXMMATRIX() *
			XMMatrixTranslationFromVector(t.GetXMVECTOR()));
	}
	Matrix4x4::~Matrix4x4()
	{
	}
	Matrix4x4& Matrix4x4::MultiplyMatrix(const Matrix4x4& l, const Matrix4x4& r)
	{
		Matrix4x4 m(l.GetXMMATRIX() * r.GetXMMATRIX());
		return m;
	}
	Matrix4x4& Matrix4x4::Inverse()
	{
		XMMATRIX m = this->GetXMMATRIX();
		XMVECTOR det = XMMatrixDeterminant(m);
		Matrix4x4 result(XMMatrixInverse(&det, m));
		return result;
	}
	Matrix4x4& Matrix4x4::Transpose()
	{
		Matrix4x4 result(XMMatrixTranspose(this->GetXMMATRIX()));
		return result;
	}
	Vector3& Matrix4x4::MultiplyVector(const Vector3& v)
	{
		Vector3 result(XMVector3TransformNormal(v.GetXMVECTOR(), this->GetXMMATRIX()));
		return result;
	}
	Vector3& Matrix4x4::MultiplyPosition(const Vector3& v)
	{
		Vector3 result(XMVector3TransformCoord(v.GetXMVECTOR(), this->GetXMMATRIX()));
		return result;
	}
	Matrix4x4& Matrix4x4::operator*(const Matrix4x4& m)
	{
		Matrix4x4 result(MultiplyMatrix((*this), m));
		return result;
	}
	void Matrix4x4::operator=(const Matrix4x4& m)
	{
		this->m_Value = m.m_Value;
	}
	void Matrix4x4::operator*=(const Matrix4x4& m)
	{
		this->SetXMMATRIX(MultiplyMatrix((*this), m).GetXMMATRIX());
	}
	Matrix4x4 Matrix4x4::GetIdentity()
	{
		Matrix4x4 m(XMMatrixIdentity());
		return m;
	}


	Quaternion Quaternion::m_Identity = Quaternion::GetIdentity();
	Quaternion::Quaternion()
	{
		(*this) = m_Identity;
	}
	Quaternion::Quaternion(const Quaternion& v)
	{
		(*this) = v;
	}
	Quaternion::Quaternion(DirectX::CXMVECTOR v)
	{
		this->SetXMVECTOR(v);
	}
	Quaternion::Quaternion(DirectX::CXMMATRIX m)
	{
		this->SetXMVECTOR(XMQuaternionRotationMatrix(m));
	}
	Quaternion::Quaternion(const Vector3& axis, const FLOAT& radius)
	{
		this->SetXMVECTOR(XMQuaternionRotationAxis(axis.GetXMVECTOR(), radius));
	}
	Quaternion::Quaternion(const FLOAT& x, const FLOAT& y, const FLOAT& z, const FLOAT& w)
	{
		this->m_Value.x = x;
		this->m_Value.y = y;
		this->m_Value.z = z;
		this->m_Value.w = w;
	}
	Quaternion::~Quaternion()
	{
	}
	void Quaternion::Normalize()
	{
		this->SetXMVECTOR(XMQuaternionNormalize(this->GetXMVECTOR()));
	}
	const DirectX::XMFLOAT4X4& Quaternion::GetXMFLOAT4X4()const
	{
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, this->GetXMMATRIX());
		return result;
	}
	Quaternion& Quaternion::Normalize(const Quaternion& v)
	{
		Quaternion result(XMQuaternionNormalize(v.GetXMVECTOR()));
		return result;
	}
	Quaternion& Quaternion::MultiplyQuaternion(const Quaternion& q1, const Quaternion& q2)
	{
		Quaternion result(XMQuaternionMultiply(q1.GetXMVECTOR(), q2.GetXMVECTOR()));
		return result;
	}
	Quaternion& Quaternion::RotationAxis(const Vector3& axis, const FLOAT& radius)
	{
		Quaternion result(axis, radius);
		return result;
	}
	Matrix4x4& Quaternion::GetMatrix()
	{
		Matrix4x4 result((*this));
		return result;
	}
	Vector3& Quaternion::MultiplyVector(const Vector3& v)
	{
		Vector3 result(XMVector3Rotate(v.GetXMVECTOR(), this->GetXMVECTOR()));
		return result;
	}
	Quaternion Quaternion::GetIdentity()
	{
		Quaternion v(XMQuaternionIdentity());
		return v;
	}


	Vector3 Vector3::m_Zero = Vector3::GetZero();
	Vector3::Vector3()
	{
		(*this) = m_Zero;
	}
	Vector3::Vector3(const Vector3& v)
	{
		(*this) = v;
	}
	Vector3::Vector3(DirectX::CXMVECTOR v)
	{
		this->SetXMVECTOR(v);
	}
	Vector3::Vector3(const FLOAT& x, const FLOAT& y, const FLOAT& z)
	{
		this->m_Value.x = x;
		this->m_Value.y = y;
		this->m_Value.z = z;
	}
	Vector3::~Vector3()
	{
	}
	void Vector3::Normalize()
	{
		this->SetXMVECTOR(XMVector3Normalize(this->GetXMVECTOR()));
	}
	Vector3& Vector3::Normalize(const Vector3& v)
	{
		Vector3 result(XMVector3Normalize(v.GetXMVECTOR()));
		return result;
	}
	Vector3& Vector3::Dot(const Vector3& v1, const Vector3& v2)
	{
		Vector3 result(XMVector3Dot(v1.GetXMVECTOR(), v2.GetXMVECTOR()));
		return result;
	}
	Vector3& Vector3::Cross(const Vector3& v1, const Vector3& v2)
	{
		Vector3 result(XMVector3Cross(v1.GetXMVECTOR(), v2.GetXMVECTOR()));
		return result;
	}
	Vector3& Vector3::operator+(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x + v.m_Value.x,
			this->m_Value.y + v.m_Value.y,
			this->m_Value.z + v.m_Value.z);
		return result;
	}
	Vector3& Vector3::operator-(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x - v.m_Value.x,
			this->m_Value.y - v.m_Value.y,
			this->m_Value.z - v.m_Value.z);
		return result;
	}
	Vector3& Vector3::operator*(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x * v.m_Value.x,
			this->m_Value.y * v.m_Value.y,
			this->m_Value.z * v.m_Value.z);
		return result;
	}
	Vector3& Vector3::operator/(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x / v.m_Value.x,
			this->m_Value.y / v.m_Value.y,
			this->m_Value.z / v.m_Value.z);
		return result;
	}
	Vector3& Vector3::operator+(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x + v,
			this->m_Value.y + v,
			this->m_Value.z + v);
		return result;
	}
	Vector3& Vector3::operator-(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x - v,
			this->m_Value.y - v,
			this->m_Value.z - v);
		return result;
	}
	Vector3& Vector3::operator*(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x * v,
			this->m_Value.y * v,
			this->m_Value.z * v);
		return result;
	}
	Vector3& Vector3::operator/(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x / v,
			this->m_Value.y / v,
			this->m_Value.z / v);
		return result;
	}
	BOOL& Vector3::operator==(const Vector3& v)
	{
		BOOL result = false;
		if (fabsf(this->m_Value.x - v.m_Value.x) < 0.00001f &&
			fabsf(this->m_Value.y - v.m_Value.y) < 0.00001f &&
			fabsf(this->m_Value.z - v.m_Value.z) < 0.00001f)
			result = true;
		return result;
	}
	BOOL& Vector3::operator!=(const Vector3& v)
	{
		BOOL result = true;
		if ((*this) == v)
			result = false;
		return result;
	}
	void Vector3::operator=(const Vector3& v)
	{
		this->m_Value = v.m_Value;
	}
	void Vector3::operator+=(const Vector3& v)
	{
		this->m_Value.x = this->m_Value.x + v.m_Value.x;
		this->m_Value.y = this->m_Value.y + v.m_Value.y;
		this->m_Value.z = this->m_Value.z + v.m_Value.z;
	}
	void Vector3::operator-=(const Vector3& v)
	{
		this->m_Value.x = this->m_Value.x - v.m_Value.x;
		this->m_Value.y = this->m_Value.y - v.m_Value.y;
		this->m_Value.z = this->m_Value.z - v.m_Value.z;
	}
	void Vector3::operator*=(const Vector3& v)
	{
		this->m_Value.x = this->m_Value.x * v.m_Value.x;
		this->m_Value.y = this->m_Value.y * v.m_Value.y;
		this->m_Value.z = this->m_Value.z * v.m_Value.z;
	}
	void Vector3::operator/=(const Vector3& v)
	{
		this->m_Value.x = this->m_Value.x / v.m_Value.x;
		this->m_Value.y = this->m_Value.y / v.m_Value.y;
		this->m_Value.z = this->m_Value.z / v.m_Value.z;
	}
	void Vector3::operator=(const FLOAT& v)
	{
		this->m_Value.x = v;
		this->m_Value.y = v;
		this->m_Value.z = v;
	}
	void Vector3::operator+=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x + v;
		this->m_Value.y = this->m_Value.y + v;
		this->m_Value.z = this->m_Value.z + v;
	}
	void Vector3::operator-=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x - v;
		this->m_Value.y = this->m_Value.y - v;
		this->m_Value.z = this->m_Value.z - v;
	}
	void Vector3::operator*=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x * v;
		this->m_Value.y = this->m_Value.y * v;
		this->m_Value.z = this->m_Value.z * v;
	}
	void Vector3::operator/=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x / v;
		this->m_Value.y = this->m_Value.y / v;
		this->m_Value.z = this->m_Value.z / v;
	}
	Vector3 Vector3::GetZero()
	{
		Vector3 v(0.f, 0.f, 0.f);
		return v;
	}
}