#pragma once

#include "../../Entry/MyMain.h"

namespace CustomType
{
	class Vector3;
	class Quaternion;

	class Matrix4x4
	{
	public:
		DirectX::CXMMATRIX			GetXMMATRIX()const { return XMLoadFloat4x4(&(this->m_Value)); }
		void						SetXMMATRIX(DirectX::CXMMATRIX m) { XMStoreFloat4x4(&(this->m_Value), m); }

		const DirectX::XMFLOAT4X4&	GetXMFLOAT4x4()const { return m_Value; }
	public:
		static const Matrix4x4&		Identity() { return m_Identity; }
		static Matrix4x4&			MultiplyMatrix(const Matrix4x4& l, const Matrix4x4& r);
	public:
		void		ResetIdentity() { (*this) = m_Identity; }
		void		SetMatrix(const Matrix4x4& m) { (*this) = m; }
		Matrix4x4&	Inverse();
		Matrix4x4&	Transpose();
		Vector3&	MultiplyVector(const Vector3& v);
		Vector3&	MultiplyPosition(const Vector3& v);
	public:
		Matrix4x4&	operator*(const Matrix4x4& m);
		void		operator=(const Matrix4x4& m);
		void		operator*=(const Matrix4x4& m);
	public:
		Matrix4x4();
		Matrix4x4(const Matrix4x4& m);
		Matrix4x4(DirectX::CXMMATRIX m);
		Matrix4x4(const Quaternion& v);
		Matrix4x4(const Vector3& t, const Quaternion& r, const Vector3& s);
		virtual ~Matrix4x4();
	private:
		static Matrix4x4		GetIdentity();
	private:
		static Matrix4x4		m_Identity;
		DirectX::XMFLOAT4X4		m_Value;
	};

	class VectorBase
	{
	public:
		VectorBase() {}
		virtual ~VectorBase() {}
	public:
		virtual DirectX::CXMVECTOR	GetXMVECTOR()const						= 0;
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)		= 0;
	public:
		virtual void				Normalize()								= 0;
		virtual void				Reset()									= 0;
	};

	class Quaternion : public VectorBase
	{
	public:
		virtual DirectX::CXMVECTOR	GetXMVECTOR()const override { return XMLoadFloat4(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { XMStoreFloat4(&(this->m_Value), v); }
		virtual void				Normalize()override;
		virtual void				Reset()override { (*this) = m_Identity; }

		DirectX::CXMMATRIX			GetXMMATRIX()const { return XMMatrixRotationQuaternion(this->GetXMVECTOR()); }
		const DirectX::XMFLOAT4&	GetXMFLOAT4()const { return m_Value; }
		const DirectX::XMFLOAT4X4&	GetXMFLOAT4X4()const;
	public:
		static const Quaternion&	Identity() { return m_Identity; }
		static Quaternion&			Normalize(const Quaternion& v);
		static Quaternion&			MultiplyQuaternion(const Quaternion& q1, const Quaternion& q2);
		static Quaternion&			RotationAxis(const Vector3& axis, const FLOAT& radius);
	public:
		Matrix4x4&					GetMatrix();
		void						SetQuaternion(const Quaternion& v) { (*this) = v; }
		Vector3&					MultiplyVector(const Vector3& v);
	public:
		Quaternion();
		Quaternion(const Quaternion& v);
		Quaternion(DirectX::CXMVECTOR v);
		Quaternion(DirectX::CXMMATRIX m);
		Quaternion(const Vector3& axis, const FLOAT& radius);
		Quaternion(const FLOAT& x, const FLOAT& y, const FLOAT& z, const FLOAT& w);
		virtual ~Quaternion();
	private:
		static Quaternion	GetIdentity();
	private:
		static Quaternion	m_Identity;
		XMFLOAT4			m_Value;
	};

	class Vector3 : public VectorBase
	{
	public:
		virtual DirectX::CXMVECTOR	GetXMVECTOR()const override { return XMLoadFloat3(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { XMStoreFloat3(&(this->m_Value), v); }
		virtual void				Normalize()override;
		virtual void				Reset()override { (*this) = m_Zero; }

		const DirectX::XMFLOAT3&	GetXMFLOAT3()const { return m_Value; }
	public:
		static const Vector3&		Zero() { return m_Zero; }
		static Vector3&				Normalize(const Vector3& v);
		static Vector3&				Dot(const Vector3& v1, const Vector3& v2);
		static Vector3&				Cross(const Vector3& v1, const Vector3& v2);
	public:
		Vector3&		operator+(const Vector3& v);
		Vector3&		operator-(const Vector3& v);
		Vector3&		operator*(const Vector3& v);
		Vector3&		operator/(const Vector3& v);
		Vector3&		operator+(const FLOAT& v);
		Vector3&		operator-(const FLOAT& v);
		Vector3&		operator*(const FLOAT& v);
		Vector3&		operator/(const FLOAT& v);
		BOOL&			operator==(const Vector3& v);
		BOOL&			operator!=(const Vector3& v);
		void			operator=(const Vector3& v);
		void			operator+=(const Vector3& v);
		void			operator-=(const Vector3& v);
		void			operator*=(const Vector3& v);
		void			operator/=(const Vector3& v);
		void			operator=(const FLOAT& v);
		void			operator+=(const FLOAT& v);
		void			operator-=(const FLOAT& v);
		void			operator*=(const FLOAT& v);
		void			operator/=(const FLOAT& v);
	public:
		Vector3();
		Vector3(const Vector3& v);
		Vector3(DirectX::CXMVECTOR v);
		Vector3(const FLOAT& x, const FLOAT& y, const FLOAT& z);
		virtual ~Vector3();
	private:
		static Vector3				GetZero();
	private:
		static Vector3				m_Zero;
		DirectX::XMFLOAT3			m_Value;
	};
}