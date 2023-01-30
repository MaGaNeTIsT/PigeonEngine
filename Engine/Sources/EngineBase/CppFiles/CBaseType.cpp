#include "../Headers/CBaseType.h"

namespace CustomType
{
	Matrix4x4 Matrix4x4::m_Identity = Matrix4x4::GetIdentity();
	Matrix4x4::Matrix4x4()
	{
		(*this) = Matrix4x4::m_Identity;
	}
	Matrix4x4::Matrix4x4(const Matrix4x4& m)
	{
		(*this) = m;
	}
	Matrix4x4::Matrix4x4(DirectX::XMFLOAT4X4 m)
	{
		this->m_Value = m;
	}
	Matrix4x4::Matrix4x4(DirectX::CXMMATRIX m)
	{
		this->SetXMMATRIX(m);
	}
	Matrix4x4::Matrix4x4(const Quaternion& v)
	{
		this->SetXMMATRIX(v.GetXMMATRIX());
	}
	Matrix4x4::Matrix4x4(const Vector3& t, const Quaternion& r)
	{
		this->SetXMMATRIX(DirectX::XMMatrixMultiply(r.GetXMMATRIX(), DirectX::XMMatrixTranslationFromVector(t.GetXMVECTOR())));
	}
	Matrix4x4::Matrix4x4(const Vector3& t, const Quaternion& r, const Vector3& s)
	{
		this->SetXMMATRIX(DirectX::XMMatrixScalingFromVector(s.GetXMVECTOR()) *
			r.GetXMMATRIX() *
			DirectX::XMMatrixTranslationFromVector(t.GetXMVECTOR()));
	}
	Matrix4x4::~Matrix4x4()
	{
	}
	Matrix4x4 Matrix4x4::MultiplyMatrix(const Matrix4x4& l, const Matrix4x4& r)
	{
		Matrix4x4 m(l.GetXMMATRIX() * r.GetXMMATRIX());
		return m;
	}
	Matrix4x4 Matrix4x4::PerspectiveMatrix(const FLOAT& fovYDeg, const FLOAT& aspectRatio, const FLOAT& nearPlane, const FLOAT& farPlane)
	{
		Matrix4x4 result(DirectX::XMMatrixPerspectiveFovLH(fovYDeg * CMath::GetDegToRad(), aspectRatio, nearPlane, farPlane));
		return result;
	}
	Matrix4x4 Matrix4x4::OrthographicMatrix(const FLOAT& left, const FLOAT& top, const FLOAT& right, const FLOAT& bottom, const FLOAT& nearPlane, const FLOAT& farPlane)
	{
		Matrix4x4 result(DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearPlane, farPlane));
		return result;
	}
	Matrix4x4 Matrix4x4::Inverse()
	{
		Matrix4x4 result(DirectX::XMMatrixInverse(nullptr, this->GetXMMATRIX()));
		return result;
	}
	Matrix4x4 Matrix4x4::Transpose()
	{
		Matrix4x4 result(DirectX::XMMatrixTranspose(this->GetXMMATRIX()));
		return result;
	}
	Vector3 Matrix4x4::MultiplyVector(const Vector3& v)
	{
		Vector3 result(DirectX::XMVector3TransformNormal(v.GetXMVECTOR(), this->GetXMMATRIX()));
		return result;
	}
	Vector3 Matrix4x4::MultiplyPosition(const Vector3& v)
	{
		Vector3 result(DirectX::XMVector3TransformCoord(v.GetXMVECTOR(), this->GetXMMATRIX()));
		return result;
	}
	Vector4 Matrix4x4::MultiplyVector(const Vector4& v)
	{
		Vector4 result(DirectX::XMVector4Transform(v.GetXMVECTOR(), this->GetXMMATRIX()));
		return result;
	}
	Matrix4x4 Matrix4x4::operator*(const Matrix4x4& m)
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
		Matrix4x4 m(DirectX::XMMatrixIdentity());
		return m;
	}



	Quaternion Quaternion::m_Identity = Quaternion::GetIdentity();
	Quaternion::Quaternion()
	{
		(*this) = Quaternion::m_Identity;
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
		this->SetXMVECTOR(DirectX::XMQuaternionRotationMatrix(m));
	}
	Quaternion::Quaternion(const Vector3& axis, const FLOAT& radian)
	{
		this->SetXMVECTOR(DirectX::XMQuaternionRotationAxis(axis.GetXMVECTOR(), radian));
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
		this->SetXMVECTOR(DirectX::XMQuaternionNormalize(this->GetXMVECTOR()));
	}
	DirectX::XMFLOAT4X4 Quaternion::GetXMFLOAT4X4()const
	{
		DirectX::XMFLOAT4X4 result;
		DirectX::XMStoreFloat4x4(&result, this->GetXMMATRIX());
		return result;
	}
	DirectX::XMFLOAT4X4 Quaternion::GetGPUUploadFloat4x4()
	{
		DirectX::XMFLOAT4X4 result;
		DirectX::XMStoreFloat4x4(&result, DirectX::XMMatrixTranspose(this->GetXMMATRIX()));
		return result;
	}
	Quaternion Quaternion::Inverse(const Quaternion& v)
	{
		Quaternion result(DirectX::XMQuaternionInverse(v.GetXMVECTOR()));
		return result;
	}
	Quaternion Quaternion::Normalize(const Quaternion& v)
	{
		Quaternion result(DirectX::XMQuaternionNormalize(v.GetXMVECTOR()));
		return result;
	}
	Quaternion Quaternion::MultiplyQuaternion(const Quaternion& q1, const Quaternion& q2)
	{
		Quaternion result(DirectX::XMQuaternionMultiply(q1.GetXMVECTOR(), q2.GetXMVECTOR()));
		return result;
	}
	Quaternion Quaternion::RotationAxis(const Vector3& axis, const FLOAT& radian)
	{
		Quaternion result(axis, radian);
		return result;
	}
	Quaternion Quaternion::NLerp(const Quaternion& v1, const Quaternion& v2, const FLOAT& t)
	{
		Quaternion result(
			v1.X() * (1.f - t) + v2.X() * t,
			v1.Y() * (1.f - t) + v2.Y() * t,
			v1.Z() * (1.f - t) + v2.Z() * t,
			v1.W() * (1.f - t) + v2.W() * t);
		result.Normalize();
		return result;
	}
	Quaternion Quaternion::NLerp(const Quaternion& v1, const Quaternion& v2, const DOUBLE& t)
	{
		FLOAT ft = static_cast<FLOAT>(t);
		Quaternion result(
			v1.X() * (1.f - ft) + v2.X() * ft,
			v1.Y() * (1.f - ft) + v2.Y() * ft,
			v1.Z() * (1.f - ft) + v2.Z() * ft,
			v1.W() * (1.f - ft) + v2.W() * ft);
		result.Normalize();
		return result;
	}
	Quaternion Quaternion::SLerp(const Quaternion& v1, const Quaternion& v2, const FLOAT& t)
	{
		Quaternion result(DirectX::XMQuaternionSlerp(v1.GetXMVECTOR(), v2.GetXMVECTOR(), t));
		return result;
	}
	Quaternion Quaternion::SLerp(const Quaternion& v1, const Quaternion& v2, const DOUBLE& t)
	{
		FLOAT ft = static_cast<FLOAT>(t);
		Quaternion result(DirectX::XMQuaternionSlerp(v1.GetXMVECTOR(), v2.GetXMVECTOR(), ft));
		return result;
	}
	Matrix4x4 Quaternion::GetMatrix()
	{
		Matrix4x4 result((*this));
		return result;
	}
	Quaternion Quaternion::Inverse()
	{
		Quaternion result(DirectX::XMQuaternionInverse(this->GetXMVECTOR()));
		return result;
	}
	Vector3 Quaternion::MultiplyVector(const Vector3& v)
	{
		Vector3 result(DirectX::XMVector3Rotate(v.GetXMVECTOR(), this->GetXMVECTOR()));
		return result;
	}
	Quaternion Quaternion::operator*(const FLOAT& v)
	{
		Quaternion result(this->X() * v, this->Y() * v, this->Z() * v, this->W() * v);
		return result;
	}
	Quaternion Quaternion::operator*(const Quaternion& v)
	{
		Quaternion result(DirectX::XMQuaternionMultiply(this->GetXMVECTOR(), v.GetXMVECTOR()));
		return result;
	}
	void Quaternion::operator=(const Quaternion& v)
	{
		this->m_Value = v.m_Value;
	}
	Quaternion Quaternion::GetIdentity()
	{
		Quaternion v(DirectX::XMQuaternionIdentity());
		return v;
	}



	Vector2 Vector2::m_Zero = Vector2::GetZero();
	Vector2::Vector2()
	{
		(*this) = Vector2::m_Zero;
	}
	Vector2::Vector2(const Vector2& v)
	{
		(*this) = v;
	}
	Vector2::Vector2(const Vector2Int& v)
	{
		this->m_Value.x = static_cast<FLOAT>(v.X());
		this->m_Value.y = static_cast<FLOAT>(v.Y());
	}
	Vector2::Vector2(DirectX::CXMVECTOR v)
	{
		this->SetXMVECTOR(v);
	}
	Vector2::Vector2(DirectX::XMFLOAT2 v)
	{
		this->m_Value = v;
	}
	Vector2::Vector2(const FLOAT& v)
	{
		this->m_Value.x = v;
		this->m_Value.y = v;
	}
	Vector2::Vector2(const FLOAT& x, const FLOAT& y)
	{
		this->m_Value.x = x;
		this->m_Value.y = y;
	}
	Vector2::Vector2(const INT& v)
	{
		this->m_Value.x = static_cast<FLOAT>(v);
		this->m_Value.y = static_cast<FLOAT>(v);
	}
	Vector2::Vector2(const INT& x, const INT& y)
	{
		this->m_Value.x = static_cast<FLOAT>(x);
		this->m_Value.y = static_cast<FLOAT>(y);
	}
	Vector2::~Vector2()
	{
	}
	void Vector2::Normalize()
	{
		this->SetXMVECTOR(DirectX::XMVector2Normalize(this->GetXMVECTOR()));
	}
	Vector2 Vector2::Normalize(const Vector2& v)
	{
		Vector2 result(DirectX::XMVector2Normalize(v.GetXMVECTOR()));
		return result;
	}
	FLOAT Vector2::Dot(const Vector2& v1, const Vector2& v2)
	{
		return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(v1.GetXMVECTOR(), v2.GetXMVECTOR())));
	}
	FLOAT Vector2::Length(const Vector2& v)
	{
		return (DirectX::XMVectorGetX(DirectX::XMVector2Length(v.GetXMVECTOR())));
	}
	FLOAT Vector2::LengthSquare(const Vector2& v)
	{
		DirectX::XMVECTOR result = v.GetXMVECTOR();
		return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(result, result)));
	}
	FLOAT Vector2::Distance(const Vector2& v1, const Vector2& v2)
	{
		Vector2 v(v1); v = v - v2;
		return (DirectX::XMVectorGetX(DirectX::XMVector2Length(v.GetXMVECTOR())));
	}
	FLOAT Vector2::DistanceSquare(const Vector2& v1, const Vector2& v2)
	{
		Vector2 v(v1); v = v - v2;
		DirectX::XMVECTOR result = v.GetXMVECTOR();
		return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(result, result)));
	}
	Vector2 Vector2::Lerp(const Vector2& v1, const Vector2& v2, const FLOAT& t)
	{
		Vector2 result(
			v1.X() * (1.f - t) + v2.X() * t,
			v1.Y() * (1.f - t) + v2.Y() * t);
		return result;
	}
	FLOAT Vector2::Dot(const Vector2& v)
	{
		return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(this->GetXMVECTOR(), v.GetXMVECTOR())));
	}
	FLOAT Vector2::Length()
	{
		return (DirectX::XMVectorGetX(DirectX::XMVector2Length(this->GetXMVECTOR())));
	}
	FLOAT Vector2::LengthSquare()
	{
		DirectX::XMVECTOR v = DirectX::XMLoadFloat2(&(this->m_Value));
		return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(v, v)));
	}
	FLOAT Vector2::Distance(const Vector2& v)
	{
		Vector2 result(this->GetXMFLOAT2());
		result = result - v;
		return (DirectX::XMVectorGetX(DirectX::XMVector2Length(result.GetXMVECTOR())));
	}
	FLOAT Vector2::DistanceSquare(const Vector2& v)
	{
		Vector2 result(this->GetXMFLOAT2());
		result = result - v;
		DirectX::XMVECTOR resultV = result.GetXMVECTOR();
		return (DirectX::XMVectorGetX(DirectX::XMVector2Dot(resultV, resultV)));
	}
	Vector2 Vector2::operator+(const Vector2& v)
	{
		Vector2 result(
			this->m_Value.x + v.m_Value.x,
			this->m_Value.y + v.m_Value.y);
		return result;
	}
	Vector2 Vector2::operator-(const Vector2& v)
	{
		Vector2 result(
			this->m_Value.x - v.m_Value.x,
			this->m_Value.y - v.m_Value.y);
		return result;
	}
	Vector2 Vector2::operator*(const Vector2& v)
	{
		Vector2 result(
			this->m_Value.x * v.m_Value.x,
			this->m_Value.y * v.m_Value.y);
		return result;
	}
	Vector2 Vector2::operator/(const Vector2& v)
	{
		Vector2 result(
			this->m_Value.x / v.m_Value.x,
			this->m_Value.y / v.m_Value.y);
		return result;
	}
	Vector2 Vector2::operator+(const FLOAT& v)
	{
		Vector2 result(
			this->m_Value.x + v,
			this->m_Value.y + v);
		return result;
	}
	Vector2 Vector2::operator-(const FLOAT& v)
	{
		Vector2 result(
			this->m_Value.x - v,
			this->m_Value.y - v);
		return result;
	}
	Vector2 Vector2::operator*(const FLOAT& v)
	{
		Vector2 result(
			this->m_Value.x * v,
			this->m_Value.y * v);
		return result;
	}
	Vector2 Vector2::operator/(const FLOAT& v)
	{
		Vector2 result(
			this->m_Value.x / v,
			this->m_Value.y / v);
		return result;
	}
	Vector2 Vector2::operator-()
	{
		Vector2 result(
			-this->m_Value.x,
			-this->m_Value.y);
		return result;
	}
	void Vector2::operator=(const Vector2& v)
	{
		this->m_Value = v.m_Value;
	}
	void Vector2::operator+=(const Vector2& v)
	{
		this->m_Value.x = this->m_Value.x + v.m_Value.x;
		this->m_Value.y = this->m_Value.y + v.m_Value.y;
	}
	void Vector2::operator-=(const Vector2& v)
	{
		this->m_Value.x = this->m_Value.x - v.m_Value.x;
		this->m_Value.y = this->m_Value.y - v.m_Value.y;
	}
	void Vector2::operator*=(const Vector2& v)
	{
		this->m_Value.x = this->m_Value.x * v.m_Value.x;
		this->m_Value.y = this->m_Value.y * v.m_Value.y;
	}
	void Vector2::operator/=(const Vector2& v)
	{
		this->m_Value.x = this->m_Value.x / v.m_Value.x;
		this->m_Value.y = this->m_Value.y / v.m_Value.y;
	}
	void Vector2::operator=(const FLOAT& v)
	{
		this->m_Value.x = v;
		this->m_Value.y = v;
	}
	void Vector2::operator+=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x + v;
		this->m_Value.y = this->m_Value.y + v;
	}
	void Vector2::operator-=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x - v;
		this->m_Value.y = this->m_Value.y - v;
	}
	void Vector2::operator*=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x * v;
		this->m_Value.y = this->m_Value.y * v;
	}
	void Vector2::operator/=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x / v;
		this->m_Value.y = this->m_Value.y / v;
	}
	Vector2 Vector2::GetZero()
	{
		Vector2 result(0.f, 0.f);
		return result;
	}



	Vector3 Vector3::m_Zero = Vector3::GetStaticVector(0.f, 0.f, 0.f);
	Vector3 Vector3::m_One = Vector3::GetStaticVector(1.f, 1.f, 1.f);
	Vector3 Vector3::m_XVector = Vector3::GetStaticVector(1.f, 0.f, 0.f);
	Vector3 Vector3::m_YVector = Vector3::GetStaticVector(0.f, 1.f, 0.f);
	Vector3 Vector3::m_ZVector = Vector3::GetStaticVector(0.f, 0.f, 1.f);
	Vector3::Vector3()
	{
		(*this) = Vector3::m_Zero;
	}
	Vector3::Vector3(const Vector3& v)
	{
		(*this) = v;
	}
	Vector3::Vector3(const Vector4& v)
	{
		this->m_Value.x = v.X();
		this->m_Value.y = v.Y();
		this->m_Value.z = v.Z();
	}
	Vector3::Vector3(DirectX::CXMVECTOR v)
	{
		this->SetXMVECTOR(v);
	}
	Vector3::Vector3(DirectX::XMFLOAT3 v)
	{
		this->m_Value.x = v.x;
		this->m_Value.y = v.y;
		this->m_Value.z = v.z;
	}
	Vector3::Vector3(DirectX::XMFLOAT4 v)
	{
		this->m_Value.x = v.x;
		this->m_Value.y = v.y;
		this->m_Value.z = v.z;
	}
	Vector3::Vector3(const FLOAT& v)
	{
		this->m_Value.x = v;
		this->m_Value.y = v;
		this->m_Value.z = v;
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
		this->SetXMVECTOR(DirectX::XMVector3Normalize(this->GetXMVECTOR()));
	}
	Vector3 Vector3::Normalize(const Vector3& v)
	{
		Vector3 result(DirectX::XMVector3Normalize(v.GetXMVECTOR()));
		return result;
	}
	FLOAT Vector3::Dot(const Vector3& v1, const Vector3& v2)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(v1.GetXMVECTOR(), v2.GetXMVECTOR()));
	}
	Vector3 Vector3::Cross(const Vector3& v1, const Vector3& v2)
	{
		Vector3 result(DirectX::XMVector3Cross(v1.GetXMVECTOR(), v2.GetXMVECTOR()));
		return result;
	}
	FLOAT Vector3::Length(const Vector3& v)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Length(v.GetXMVECTOR()));
	}
	FLOAT Vector3::LengthSquare(const Vector3& v)
	{
		DirectX::XMVECTOR vec = v.GetXMVECTOR();
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(vec, vec));
	}
	FLOAT Vector3::Distance(const Vector3& v1, const Vector3& v2)
	{
		Vector3 v(v1);
		return DirectX::XMVectorGetX(DirectX::XMVector3Length((v - v2).GetXMVECTOR()));
	}
	FLOAT Vector3::DistanceSquare(const Vector3& v1, const Vector3& v2)
	{
		Vector3 v(v1);
		DirectX::XMVECTOR vec = (v - v2).GetXMVECTOR();
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(vec, vec));
	}
	Vector3 Vector3::Reciprocal(const Vector3& v)
	{
		Vector3 result(1.f / v.X(), 1.f / v.Y(), 1.f / v.Z());
		return result;
	}
	Vector3 Vector3::Lerp(const Vector3& v1, const Vector3& v2, const FLOAT& t)
	{
		Vector3 result(
			v1.X() * (1.f - t) + v2.X() * t,
			v1.Y() * (1.f - t) + v2.Y() * t,
			v1.Z() * (1.f - t) + v2.Z() * t);
		return result;
	}
	Vector3 Vector3::Lerp(const Vector3& v1, const Vector3& v2, const DOUBLE& t)
	{
		FLOAT ft = static_cast<FLOAT>(t);
		Vector3 result(
			v1.X() * (1.f - ft) + v2.X() * ft,
			v1.Y() * (1.f - ft) + v2.Y() * ft,
			v1.Z() * (1.f - ft) + v2.Z() * ft);
		return result;
	}
	FLOAT Vector3::Dot(const Vector3& v)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(this->GetXMVECTOR(), v.GetXMVECTOR()));
	}
	Vector3 Vector3::Cross(const Vector3& v)
	{
		Vector3 result(DirectX::XMVector3Cross(this->GetXMVECTOR(), v.GetXMVECTOR()));
		return result;
	}
	FLOAT Vector3::Length()
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Length(this->GetXMVECTOR()));
	}
	FLOAT Vector3::LengthSquare()
	{
		DirectX::XMVECTOR vec = this->GetXMVECTOR();
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(vec, vec));
	}
	FLOAT Vector3::Distance(const Vector3& v)
	{
		Vector3 tv = v;
		return DirectX::XMVectorGetX(DirectX::XMVector3Length((tv - (*this)).GetXMVECTOR()));
	}
	FLOAT Vector3::DistanceSquare(const Vector3& v)
	{
		Vector3 tv = v;
		DirectX::XMVECTOR vec = (tv - v).GetXMVECTOR();
		return DirectX::XMVectorGetX(DirectX::XMVector3Dot(vec, vec));
	}
	Vector3 Vector3::operator+(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x + v.m_Value.x,
			this->m_Value.y + v.m_Value.y,
			this->m_Value.z + v.m_Value.z);
		return result;
	}
	Vector3 Vector3::operator-(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x - v.m_Value.x,
			this->m_Value.y - v.m_Value.y,
			this->m_Value.z - v.m_Value.z);
		return result;
	}
	Vector3 Vector3::operator*(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x * v.m_Value.x,
			this->m_Value.y * v.m_Value.y,
			this->m_Value.z * v.m_Value.z);
		return result;
	}
	Vector3 Vector3::operator/(const Vector3& v)
	{
		Vector3 result(
			this->m_Value.x / v.m_Value.x,
			this->m_Value.y / v.m_Value.y,
			this->m_Value.z / v.m_Value.z);
		return result;
	}
	Vector3 Vector3::operator+(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x + v,
			this->m_Value.y + v,
			this->m_Value.z + v);
		return result;
	}
	Vector3 Vector3::operator-(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x - v,
			this->m_Value.y - v,
			this->m_Value.z - v);
		return result;
	}
	Vector3 Vector3::operator*(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x * v,
			this->m_Value.y * v,
			this->m_Value.z * v);
		return result;
	}
	Vector3 Vector3::operator/(const FLOAT& v)
	{
		Vector3 result(
			this->m_Value.x / v,
			this->m_Value.y / v,
			this->m_Value.z / v);
		return result;
	}
	Vector3 Vector3::operator-()
	{
		Vector3 result(
			-this->m_Value.x,
			-this->m_Value.y,
			-this->m_Value.z);
		return result;
	}
	BOOL Vector3::operator==(const Vector3& v)
	{
		BOOL result = false;
		if (fabsf(this->m_Value.x - v.m_Value.x) < 0.00001f &&
			fabsf(this->m_Value.y - v.m_Value.y) < 0.00001f &&
			fabsf(this->m_Value.z - v.m_Value.z) < 0.00001f)
			result = true;
		return result;
	}
	BOOL Vector3::operator!=(const Vector3& v)
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
	Vector3 Vector3::GetStaticVector(const FLOAT& x, const FLOAT& y, const FLOAT& z)
	{
		return Vector3(x, y, z);
	}



	Vector4 Vector4::m_Zero = Vector4::GetZero();
	Vector4::Vector4()
	{
		(*this) = Vector4::m_Zero;
	}
	Vector4::Vector4(const Vector3& v)
	{
		this->m_Value.x = v.X();
		this->m_Value.y = v.Y();
		this->m_Value.z = v.Z();
		this->m_Value.w = 0.f;
	}
	Vector4::Vector4(const Vector4& v)
	{
		(*this) = v.m_Value;
	}
	Vector4::Vector4(DirectX::CXMVECTOR v)
	{
		this->SetXMVECTOR(v);
	}
	Vector4::Vector4(DirectX::XMFLOAT4 v)
	{
		this->m_Value = v;
	}
	Vector4::Vector4(const FLOAT& v)
	{
		this->m_Value.x = v;
		this->m_Value.y = v;
		this->m_Value.z = v;
		this->m_Value.w = v;
	}
	Vector4::Vector4(const FLOAT& x, const FLOAT& y, const FLOAT& z)
	{
		this->m_Value.x = x;
		this->m_Value.y = y;
		this->m_Value.z = z;
		this->m_Value.w = 0.f;
	}
	Vector4::Vector4(const FLOAT& x, const FLOAT& y, const FLOAT& z, const FLOAT& w)
	{
		this->m_Value.x = x;
		this->m_Value.y = y;
		this->m_Value.z = z;
		this->m_Value.w = w;
	}
	Vector4::Vector4(const INT& x, const INT& y, const INT& z, const INT& w)
	{
		this->m_Value.x = static_cast<FLOAT>(x);
		this->m_Value.y = static_cast<FLOAT>(y);
		this->m_Value.z = static_cast<FLOAT>(z);
		this->m_Value.w = static_cast<FLOAT>(w);
	}
	Vector4::Vector4(const Vector4Int& v)
	{
		this->m_Value.x = static_cast<FLOAT>(v.X());
		this->m_Value.y = static_cast<FLOAT>(v.Y());
		this->m_Value.z = static_cast<FLOAT>(v.Z());
		this->m_Value.w = static_cast<FLOAT>(v.W());
	}
	Vector4::~Vector4()
	{
	}
	Vector4 Vector4::operator+(const Vector4& v)
	{
		Vector4 result(
			this->m_Value.x + v.m_Value.x,
			this->m_Value.y + v.m_Value.y,
			this->m_Value.z + v.m_Value.z,
			this->m_Value.w + v.m_Value.w);
		return result;
	}
	Vector4 Vector4::operator-(const Vector4& v)
	{
		Vector4 result(
			this->m_Value.x - v.m_Value.x,
			this->m_Value.y - v.m_Value.y,
			this->m_Value.z - v.m_Value.z,
			this->m_Value.w - v.m_Value.w);
		return result;
	}
	Vector4 Vector4::operator*(const Vector4& v)
	{
		Vector4 result(
			this->m_Value.x * v.m_Value.x,
			this->m_Value.y * v.m_Value.y,
			this->m_Value.z * v.m_Value.z,
			this->m_Value.w * v.m_Value.w);
		return result;
	}
	Vector4 Vector4::operator/(const Vector4& v)
	{
		Vector4 result(
			this->m_Value.x / v.m_Value.x,
			this->m_Value.y / v.m_Value.y,
			this->m_Value.z / v.m_Value.z,
			this->m_Value.w / v.m_Value.w);
		return result;
	}
	Vector4 Vector4::operator+(const FLOAT& v)
	{
		Vector4 result(
			this->m_Value.x + v,
			this->m_Value.y + v,
			this->m_Value.z + v,
			this->m_Value.w + v);
		return result;
	}
	Vector4 Vector4::operator-(const FLOAT& v)
	{
		Vector4 result(
			this->m_Value.x - v,
			this->m_Value.y - v,
			this->m_Value.z - v,
			this->m_Value.w - v);
		return result;
	}
	Vector4 Vector4::operator*(const FLOAT& v)
	{
		Vector4 result(
			this->m_Value.x * v,
			this->m_Value.y * v,
			this->m_Value.z * v,
			this->m_Value.w * v);
		return result;
	}
	Vector4 Vector4::operator/(const FLOAT& v)
	{
		Vector4 result(
			this->m_Value.x / v,
			this->m_Value.y / v,
			this->m_Value.z / v,
			this->m_Value.w / v);
		return result;
	}
	Vector4 Vector4::operator-()
	{
		Vector4 result(
			-this->m_Value.x,
			-this->m_Value.y,
			-this->m_Value.z,
			-this->m_Value.w);
		return result;
	}
	void Vector4::operator=(const Vector4& v)
	{
		this->m_Value = v.m_Value;
	}
	void Vector4::operator+=(const Vector4& v)
	{
		this->m_Value.x = this->m_Value.x + v.m_Value.x;
		this->m_Value.y = this->m_Value.y + v.m_Value.y;
		this->m_Value.z = this->m_Value.z + v.m_Value.z;
		this->m_Value.w = this->m_Value.w + v.m_Value.w;
	}
	void Vector4::operator-=(const Vector4& v)
	{
		this->m_Value.x = this->m_Value.x - v.m_Value.x;
		this->m_Value.y = this->m_Value.y - v.m_Value.y;
		this->m_Value.z = this->m_Value.z - v.m_Value.z;
		this->m_Value.w = this->m_Value.w - v.m_Value.w;
	}
	void Vector4::operator*=(const Vector4& v)
	{
		this->m_Value.x = this->m_Value.x * v.m_Value.x;
		this->m_Value.y = this->m_Value.y * v.m_Value.y;
		this->m_Value.z = this->m_Value.z * v.m_Value.z;
		this->m_Value.w = this->m_Value.w * v.m_Value.w;
	}
	void Vector4::operator/=(const Vector4& v)
	{
		this->m_Value.x = this->m_Value.x / v.m_Value.x;
		this->m_Value.y = this->m_Value.y / v.m_Value.y;
		this->m_Value.z = this->m_Value.z / v.m_Value.z;
		this->m_Value.w = this->m_Value.w / v.m_Value.w;
	}
	void Vector4::operator=(const FLOAT& v)
	{
		this->m_Value.x = v;
		this->m_Value.y = v;
		this->m_Value.z = v;
		this->m_Value.w = v;
	}
	void Vector4::operator+=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x + v;
		this->m_Value.y = this->m_Value.y + v;
		this->m_Value.z = this->m_Value.z + v;
		this->m_Value.w = this->m_Value.w + v;
	}
	void Vector4::operator-=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x - v;
		this->m_Value.y = this->m_Value.y - v;
		this->m_Value.z = this->m_Value.z - v;
		this->m_Value.w = this->m_Value.w - v;
	}
	void Vector4::operator*=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x * v;
		this->m_Value.y = this->m_Value.y * v;
		this->m_Value.z = this->m_Value.z * v;
		this->m_Value.w = this->m_Value.w * v;
	}
	void Vector4::operator/=(const FLOAT& v)
	{
		this->m_Value.x = this->m_Value.x / v;
		this->m_Value.y = this->m_Value.y / v;
		this->m_Value.z = this->m_Value.z / v;
		this->m_Value.w = this->m_Value.w / v;
	}
	Vector4 Vector4::GetZero()
	{
		Vector4 v(0.f, 0.f, 0.f, 0.f);
		return v;
	}



	Vector2Int Vector2Int::m_Zero = Vector2Int::GetZero();
	Vector2Int::Vector2Int()
	{
		(*this) = Vector2Int::m_Zero;
	}
	Vector2Int::Vector2Int(const Vector2Int& v)
	{
		(*this) = v;
	}
	Vector2Int::Vector2Int(const INT& v)
	{
		this->x = v;
		this->y = v;
	}
	Vector2Int::Vector2Int(const INT& x, const INT& y)
	{
		this->x = x;
		this->y = y;
	}
	Vector2Int::Vector2Int(const FLOAT& v)
	{
		this->x = static_cast<INT>(v);
		this->y = static_cast<INT>(v);
	}
	Vector2Int::Vector2Int(const FLOAT& x, const FLOAT& y)
	{
		this->x = static_cast<INT>(x);
		this->y = static_cast<INT>(y);
	}
	Vector2Int::~Vector2Int()
	{
	}
	Vector2Int Vector2Int::operator+(const Vector2Int& v)
	{
		Vector2Int result(
			this->x + v.x,
			this->y + v.y);
		return result;
	}
	Vector2Int Vector2Int::operator-(const Vector2Int& v)
	{
		Vector2Int result(
			this->x - v.x,
			this->y - v.y);
		return result;
	}
	Vector2Int Vector2Int::operator*(const Vector2Int& v)
	{
		Vector2Int result(
			this->x * v.x,
			this->y * v.y);
		return result;
	}
	Vector2Int Vector2Int::operator/(const Vector2Int& v)
	{
		Vector2Int result(
			this->x / v.x,
			this->y / v.y);
		return result;
	}
	Vector2Int Vector2Int::operator+(const INT& v)
	{
		Vector2Int result(
			this->x + v,
			this->y + v);
		return result;
	}
	Vector2Int Vector2Int::operator-(const INT& v)
	{
		Vector2Int result(
			this->x - v,
			this->y - v);
		return result;
	}
	Vector2Int Vector2Int::operator*(const INT& v)
	{
		Vector2Int result(
			this->x * v,
			this->y * v);
		return result;
	}
	Vector2Int Vector2Int::operator/(const INT& v)
	{
		Vector2Int result(
			this->x / v,
			this->y / v);
		return result;
	}
	Vector2Int Vector2Int::operator-()
	{
		Vector2Int result(-this->x, -this->y);
		return result;
	}
	void Vector2Int::operator=(const Vector2Int& v)
	{
		this->x = v.x;
		this->y = v.y;
	}
	void Vector2Int::operator+=(const Vector2Int& v)
	{
		this->x = this->x + v.x;
		this->y = this->y + v.y;
	}
	void Vector2Int::operator-=(const Vector2Int& v)
	{
		this->x = this->x - v.x;
		this->y = this->y - v.y;
	}
	void Vector2Int::operator*=(const Vector2Int& v)
	{
		this->x = this->x * v.x;
		this->y = this->y * v.y;
	}
	void Vector2Int::operator/=(const Vector2Int& v)
	{
		this->x = this->x / v.x;
		this->y = this->y / v.y;
	}
	void Vector2Int::operator=(const INT& v)
	{
		this->x = v;
		this->y = v;
	}
	void Vector2Int::operator+=(const INT& v)
	{
		this->x = this->x + v;
		this->y = this->y + v;
	}
	void Vector2Int::operator-=(const INT& v)
	{
		this->x = this->x - v;
		this->y = this->y - v;
	}
	void Vector2Int::operator*=(const INT& v)
	{
		this->x = this->x * v;
		this->y = this->y * v;
	}
	void Vector2Int::operator/=(const INT& v)
	{
		this->x = this->x / v;
		this->y = this->y / v;
	}
	BOOL Vector2Int::operator==(const Vector2Int& v)
	{
		return ((this->x == v.x) && (this->y == v.y));
	}
	BOOL Vector2Int::operator!=(const Vector2Int& v)
	{
		return ((this->x != v.x) || (this->y != v.y));
	}
	Vector2Int Vector2Int::GetZero()
	{
		Vector2Int result(0, 0);
		return result;
	}



	Vector4Int Vector4Int::m_Zero = Vector4Int::GetZero();
	Vector4Int::Vector4Int()
	{
		(*this) = Vector4Int::m_Zero;
	}
	Vector4Int::Vector4Int(const Vector4Int& v)
	{
		(*this) = v;
	}
	Vector4Int::Vector4Int(const INT& v)
	{
		this->x = v;
		this->y = v;
		this->z = v;
		this->w = v;
	}
	Vector4Int::Vector4Int(const INT& x, const INT& y, const INT& z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = 0;
	}
	Vector4Int::Vector4Int(const INT& x, const INT& y, const INT& z, const INT& w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}
	Vector4Int::Vector4Int(const FLOAT& v)
	{
		this->x = static_cast<INT>(v);
		this->y = static_cast<INT>(v);
		this->z = static_cast<INT>(v);
		this->w = static_cast<INT>(v);
	}
	Vector4Int::Vector4Int(const FLOAT& x, const FLOAT& y, const FLOAT& z)
	{
		this->x = static_cast<INT>(x);
		this->y = static_cast<INT>(y);
		this->z = static_cast<INT>(z);
		this->w = 0;
	}
	Vector4Int::Vector4Int(const FLOAT& x, const FLOAT& y, const FLOAT& z, const FLOAT& w)
	{
		this->x = static_cast<INT>(x);
		this->y = static_cast<INT>(y);
		this->z = static_cast<INT>(z);
		this->w = static_cast<INT>(w);
	}
	Vector4Int::Vector4Int(const Vector3& v)
	{
		this->x = static_cast<INT>(v.X());
		this->y = static_cast<INT>(v.Y());
		this->z = static_cast<INT>(v.Z());
		this->w = 0;
	}
	Vector4Int::Vector4Int(const Vector4& v)
	{
		this->x = static_cast<INT>(v.X());
		this->y = static_cast<INT>(v.Y());
		this->z = static_cast<INT>(v.Z());
		this->w = static_cast<INT>(v.W());
	}
	Vector4Int::~Vector4Int()
	{
	}
	Vector4Int Vector4Int::operator+(const Vector4Int& v)
	{
		Vector4Int result(
			this->x + v.x,
			this->y + v.y,
			this->z + v.z,
			this->w + v.w);
		return result;
	}
	Vector4Int Vector4Int::operator-(const Vector4Int& v)
	{
		Vector4Int result(
			this->x - v.x,
			this->y - v.y,
			this->z - v.z,
			this->w - v.w);
		return result;
	}
	Vector4Int Vector4Int::operator*(const Vector4Int& v)
	{
		Vector4Int result(
			this->x * v.x,
			this->y * v.y,
			this->z * v.z,
			this->w * v.w);
		return result;
	}
	Vector4Int Vector4Int::operator/(const Vector4Int& v)
	{
		Vector4Int result(
			this->x / v.x,
			this->y / v.y,
			this->z / v.z,
			this->w / v.w);
		return result;
	}
	Vector4Int Vector4Int::operator+(const INT& v)
	{
		Vector4Int result(
			this->x + v,
			this->y + v,
			this->z + v,
			this->w + v);
		return result;
	}
	Vector4Int Vector4Int::operator-(const INT& v)
	{
		Vector4Int result(
			this->x - v,
			this->y - v,
			this->z - v,
			this->w - v);
		return result;
	}
	Vector4Int Vector4Int::operator*(const INT& v)
	{
		Vector4Int result(
			this->x * v,
			this->y * v,
			this->z * v,
			this->w * v);
		return result;
	}
	Vector4Int Vector4Int::operator/(const INT& v)
	{
		Vector4Int result(
			this->x / v,
			this->y / v,
			this->z / v,
			this->w / v);
		return result;
	}
	Vector4Int Vector4Int::operator-()
	{
		Vector4Int result(
			-this->x,
			-this->y,
			-this->z,
			-this->w);
		return result;
	}
	void Vector4Int::operator=(const Vector4Int& v)
	{
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;
	}
	void Vector4Int::operator+=(const Vector4Int& v)
	{
		this->x = this->x + v.x;
		this->y = this->y + v.y;
		this->z = this->z + v.z;
		this->w = this->w + v.w;
	}
	void Vector4Int::operator-=(const Vector4Int& v)
	{
		this->x = this->x - v.x;
		this->y = this->y - v.y;
		this->z = this->z - v.z;
		this->w = this->w - v.w;
	}
	void Vector4Int::operator*=(const Vector4Int& v)
	{
		this->x = this->x * v.x;
		this->y = this->y * v.y;
		this->z = this->z * v.z;
		this->w = this->w * v.w;
	}
	void Vector4Int::operator/=(const Vector4Int& v)
	{
		this->x = this->x / v.x;
		this->y = this->y / v.y;
		this->z = this->z / v.z;
		this->w = this->w / v.w;
	}
	void Vector4Int::operator=(const INT& v)
	{
		this->x = v;
		this->y = v;
		this->z = v;
		this->w = v;
	}
	void Vector4Int::operator+=(const INT& v)
	{
		this->x = this->x + v;
		this->y = this->y + v;
		this->z = this->z + v;
		this->w = this->w + v;
	}
	void Vector4Int::operator-=(const INT& v)
	{
		this->x = this->x - v;
		this->y = this->y - v;
		this->z = this->z - v;
		this->w = this->w - v;
	}
	void Vector4Int::operator*=(const INT& v)
	{
		this->x = this->x * v;
		this->y = this->y * v;
		this->z = this->z * v;
		this->w = this->w * v;
	}
	void Vector4Int::operator/=(const INT& v)
	{
		this->x = this->x / v;
		this->y = this->y / v;
		this->z = this->z / v;
		this->w = this->w / v;
	}
	void Vector4Int::operator=(const Vector4& v)
	{
		this->x = static_cast<INT>(v.X());
		this->y = static_cast<INT>(v.Y());
		this->z = static_cast<INT>(v.Z());
		this->w = static_cast<INT>(v.W());
	}
	BOOL Vector4Int::operator==(const Vector4Int& v)
	{
		return (
			this->x == v.x &&
			this->y == v.y &&
			this->z == v.z &&
			this->w == v.w);
	}
	BOOL Vector4Int::operator!=(const Vector4Int& v)
	{
		return (
			this->x != v.x ||
			this->y != v.y ||
			this->z != v.z ||
			this->w != v.w);
	}
	Vector4Int Vector4Int::GetZero()
	{
		Vector4Int result(0, 0, 0, 0);
		return result;
	}



	FLOAT CMath::m_PI		= 3.1415926535897932f;
	FLOAT CMath::m_RadToDeg = 57.295779513082321f;
	FLOAT CMath::m_DegToRad = 0.0174532925199433f;
	const FLOAT& CMath::GetPI()
	{
		return CMath::m_PI;
	}
	const FLOAT& CMath::GetDegToRad()
	{
		return CMath::m_DegToRad;
	}
	const FLOAT& CMath::GetRadToDeg()
	{
		return CMath::m_RadToDeg;
	}
	BOOL CMath::Lerp(const INT& x0, const INT& y0, const INT& x1, const INT& y1, const INT& t, INT& phi)
	{
		if (t < x0 || t > x1)
			return false;
		phi = (INT)(((FLOAT)(t - x1)) / ((FLOAT)(x0 - x1)) * (FLOAT)y0 + ((FLOAT)(t - x0)) / ((FLOAT)(x1 - x0)) * (FLOAT)y1);
		return true;
	}
	FLOAT CMath::Lerp(const FLOAT& v0, const FLOAT& v1, const FLOAT& t)
	{
		return (v0 * (1.f - t) + v1 * t);
	}
	FLOAT CMath::Max(const FLOAT& v0, const FLOAT& v1)
	{
		return fmaxf(v0, v1);
	}
	UINT CMath::Max(const UINT& v0, const UINT& v1)
	{
		return ((v0 > v1) ? v0 : v1);
	}
	INT CMath::Max(const INT& v0, const INT& v1)
	{
		return ((v0 > v1) ? v0 : v1);
	}
	FLOAT CMath::Min(const FLOAT& v0, const FLOAT& v1)
	{
		return fminf(v0, v1);
	}
	UINT CMath::Min(const UINT& v0, const UINT& v1)
	{
		return ((v0 < v1) ? v0 : v1);
	}
	INT CMath::Min(const INT& v0, const INT& v1)
	{
		return ((v0 < v1) ? v0 : v1);
	}
	FLOAT CMath::Abs(const FLOAT& v)
	{
		return fabsf(v);
	}
	FLOAT CMath::Clamp(const FLOAT& v, const FLOAT& min, const FLOAT& max)
	{
		return fmaxf(min, fminf(max, v));
	}
	INT CMath::Clamp(const INT& v, const INT& min, const INT& max)
	{
		return CMath::Max(min, CMath::Min(max, v));
	}
	UINT CMath::Clamp(const UINT& v, const UINT& min, const UINT& max)
	{
		return CMath::Max(min, CMath::Min(max, v));
	}
	FLOAT CMath::Mod(const FLOAT& numerator, const FLOAT& denominator)
	{
		return fmodf(numerator, denominator);
	}
	DOUBLE CMath::Mod(const DOUBLE& numerator, const DOUBLE& denominator)
	{
		return fmod(numerator, denominator);
	}
	FLOAT CMath::Sin(const FLOAT& v)
	{
		return sinf(v);
	}
	FLOAT CMath::Cos(const FLOAT& v)
	{
		return cosf(v);
	}
	void CMath::SinCos(FLOAT& sinValue, FLOAT& cosValue, const FLOAT& v)
	{
		sinValue = sinf(v);
		cosValue = cosf(v);
	}
	FLOAT CMath::Exp2(const FLOAT& v)
	{
		return exp2f(v);
	}
	INT CMath::Exp2(const INT& v)
	{
		return static_cast<INT>(exp2f(static_cast<FLOAT>(v)));
	}
	INT CMath::Log2Floor(const INT& v)
	{
		FLOAT e = log2f(static_cast<FLOAT>(v));
		e = floorf(e);
		return static_cast<INT>(e);
	}
	INT CMath::Log2Floor(const FLOAT& v)
	{
		FLOAT e = log2f(v);
		return static_cast<INT>(floorf(e));
	}
	INT CMath::Log2Ceil(const INT& v)
	{
		FLOAT e = log2f(static_cast<FLOAT>(v));
		e = ceilf(e);
		return static_cast<INT>(e);
	}
	INT CMath::Log2Ceil(const FLOAT& v)
	{
		FLOAT e = log2f(v);
		return static_cast<INT>(ceilf(e));
	}
	INT CMath::PowerOfTwoFloor(FLOAT& output, const FLOAT& input)
	{
		FLOAT e = log2f(input);
		e = floorf(e);
		output = exp2(e);
		return static_cast<INT>(e);
	}
	INT CMath::PowerOfTwoFloor(INT& output, const INT& input)
	{
		FLOAT e = log2f(static_cast<FLOAT>(input));
		e = floorf(e);
		output = static_cast<INT>(exp2f(e));
		return static_cast<INT>(e);
	}
	FLOAT CMath::Sqrt(const FLOAT& v)
	{
		return sqrtf(v);
	}
}