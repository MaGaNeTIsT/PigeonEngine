#pragma once

#include "../../Entry/MyMain.h"

namespace CustomType
{
	class Vector3;
	class Vector4;
	class Quaternion;

	class Matrix4x4
	{
	public:
		DirectX::XMMATRIX			GetXMMATRIX()const { return DirectX::XMLoadFloat4x4(&(this->m_Value)); }
		void						SetXMMATRIX(DirectX::CXMMATRIX m) { DirectX::XMStoreFloat4x4(&(this->m_Value), m); }

		const DirectX::XMFLOAT4X4&	GetXMFLOAT4X4()const { return m_Value; }
		DirectX::XMFLOAT4X4			GetGPUUploadFloat4x4() { return (this->Transpose().GetXMFLOAT4X4()); }
	public:
		static const Matrix4x4&		Identity() { return m_Identity; }
		static Matrix4x4			MultiplyMatrix(const Matrix4x4& l, const Matrix4x4& r);
	public:
		void		ResetIdentity() { (*this) = m_Identity; }
		void		SetMatrix(const Matrix4x4& m) { (*this) = m; }
		Matrix4x4	Inverse();
		Matrix4x4	Transpose();
		Vector3		MultiplyVector(const Vector3& v);
		Vector3		MultiplyPosition(const Vector3& v);
	public:
		Matrix4x4	operator*(const Matrix4x4& m);
		void		operator=(const Matrix4x4& m);
		void		operator*=(const Matrix4x4& m);
	public:
		Matrix4x4();
		Matrix4x4(const Matrix4x4& m);
		Matrix4x4(DirectX::CXMMATRIX m);
		Matrix4x4(const Quaternion& v);
		Matrix4x4(const Vector3& t, const Quaternion& r);
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
		virtual DirectX::XMVECTOR	GetXMVECTOR()const						= 0;
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)		= 0;
	public:
		virtual void				Normalize()								= 0;
		virtual void				Reset()									= 0;
	};

	class Quaternion : public VectorBase
	{
	public:
		virtual DirectX::XMVECTOR	GetXMVECTOR()const override { return DirectX::XMLoadFloat4(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { DirectX::XMStoreFloat4(&(this->m_Value), v); }
		virtual void				Normalize()override;
		virtual void				Reset()override { (*this) = m_Identity; }

		DirectX::XMMATRIX			GetXMMATRIX()const { return DirectX::XMMatrixRotationQuaternion(this->GetXMVECTOR()); }
		const DirectX::XMFLOAT4&	GetXMFLOAT4()const { return m_Value; }
		DirectX::XMFLOAT4X4			GetXMFLOAT4X4()const;
		DirectX::XMFLOAT4X4			GetGPUUploadFloat4x4();
	public:
		static const Quaternion&	Identity() { return m_Identity; }
		static Quaternion			Normalize(const Quaternion& v);
		static Quaternion			MultiplyQuaternion(const Quaternion& q1, const Quaternion& q2);
		static Quaternion			RotationAxis(const Vector3& axis, const FLOAT& radius);
	public:
		const FLOAT&				X()const { return m_Value.x; }
		const FLOAT&				Y()const { return m_Value.y; }
		const FLOAT&				Z()const { return m_Value.z; }
		const FLOAT&				W()const { return m_Value.w; }
	public:
		Matrix4x4					GetMatrix();
		void						SetQuaternion(const Quaternion& v) { (*this) = v; }
		Vector3						MultiplyVector(const Vector3& v);
	public:
		Quaternion					operator*(const Quaternion& v);
		void						operator=(const Quaternion& v);
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
		virtual DirectX::XMVECTOR	GetXMVECTOR()const override { return DirectX::XMLoadFloat3(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { DirectX::XMStoreFloat3(&(this->m_Value), v); }
		virtual void				Normalize()override;
		virtual void				Reset()override { (*this) = m_Zero; }

		const DirectX::XMFLOAT3&	GetXMFLOAT3()const { return m_Value; }
		DirectX::XMFLOAT4			GetXMFLOAT4()const { return DirectX::XMFLOAT4(m_Value.x, m_Value.y, m_Value.z, 0.f); }
	public:
		static const Vector3&		Zero() { return m_Zero; }
		static Vector3				Normalize(const Vector3& v);
		static Vector3				Dot(const Vector3& v1, const Vector3& v2);
		static Vector3				Cross(const Vector3& v1, const Vector3& v2);
	public:
		const FLOAT&				X()const { return m_Value.x; }
		const FLOAT&				Y()const { return m_Value.y; }
		const FLOAT&				Z()const { return m_Value.z; }
	public:
		Vector3			operator+(const Vector3& v);
		Vector3			operator-(const Vector3& v);
		Vector3			operator*(const Vector3& v);
		Vector3			operator/(const Vector3& v);
		Vector3			operator+(const FLOAT& v);
		Vector3			operator-(const FLOAT& v);
		Vector3			operator*(const FLOAT& v);
		Vector3			operator/(const FLOAT& v);
		Vector3			operator-();
		BOOL			operator==(const Vector3& v);
		BOOL			operator!=(const Vector3& v);
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
		Vector3(DirectX::XMFLOAT3 v);
		Vector3(DirectX::XMFLOAT4 v);
		Vector3(const FLOAT& v);
		Vector3(const FLOAT& x, const FLOAT& y, const FLOAT& z);
		virtual ~Vector3();
	private:
		static Vector3				GetZero();
	private:
		static Vector3				m_Zero;
		DirectX::XMFLOAT3			m_Value;
	};

	class Vector4 : public VectorBase
	{
	public:
		virtual DirectX::XMVECTOR	GetXMVECTOR()const override { return DirectX::XMLoadFloat4(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { DirectX::XMStoreFloat4(&(this->m_Value), v); }
		virtual void				Normalize()override {}
		virtual void				Reset()override { (*this) = m_Zero; }

		const DirectX::XMFLOAT4&	GetXMFLOAT4()const { return m_Value; }
	public:
		static const Vector4&		Zero() { return m_Zero; }
	public:
		const FLOAT&				X()const { return m_Value.x; }
		const FLOAT&				Y()const { return m_Value.y; }
		const FLOAT&				Z()const { return m_Value.z; }
		const FLOAT&				W()const { return m_Value.w; }
	public:
		Vector4			operator+(const Vector4& v);
		Vector4			operator-(const Vector4& v);
		Vector4			operator*(const Vector4& v);
		Vector4			operator/(const Vector4& v);
		Vector4			operator+(const FLOAT& v);
		Vector4			operator-(const FLOAT& v);
		Vector4			operator*(const FLOAT& v);
		Vector4			operator/(const FLOAT& v);
		Vector4			operator-();
		void			operator=(const Vector4& v);
		void			operator+=(const Vector4& v);
		void			operator-=(const Vector4& v);
		void			operator*=(const Vector4& v);
		void			operator/=(const Vector4& v);
		void			operator=(const FLOAT& v);
		void			operator+=(const FLOAT& v);
		void			operator-=(const FLOAT& v);
		void			operator*=(const FLOAT& v);
		void			operator/=(const FLOAT& v);
	public:
		Vector4();
		Vector4(const Vector3& v);
		Vector4(const Vector4& v);
		Vector4(DirectX::CXMVECTOR v);
		Vector4(DirectX::XMFLOAT4 v);
		Vector4(const FLOAT& v);
		Vector4(const FLOAT& x, const FLOAT& y, const FLOAT& z);
		Vector4(const FLOAT& x, const FLOAT& y, const FLOAT& z, const FLOAT& w);
		Vector4(const INT& x, const INT& y, const INT& z, const INT& w);
		virtual ~Vector4();
	private:
		static Vector4				GetZero();
	private:
		static Vector4				m_Zero;
		DirectX::XMFLOAT4			m_Value;
	};

	class Vector4Int
	{
	public:
		static const Vector4Int&	Zero() { return m_Zero; }
	public:
		virtual void		Reset() { (*this) = m_Zero; }
	public:
		const INT&			X()const { return x; }
		const INT&			Y()const { return y; }
		const INT&			Z()const { return z; }
		const INT&			W()const { return w; }
	public:
		Vector4Int			operator+(const Vector4Int& v);
		Vector4Int			operator-(const Vector4Int& v);
		Vector4Int			operator*(const Vector4Int& v);
		Vector4Int			operator/(const Vector4Int& v);
		Vector4Int			operator+(const INT& v);
		Vector4Int			operator-(const INT& v);
		Vector4Int			operator*(const INT& v);
		Vector4Int			operator/(const INT& v);
		Vector4Int			operator-();
		void			operator=(const Vector4Int& v);
		void			operator+=(const Vector4Int& v);
		void			operator-=(const Vector4Int& v);
		void			operator*=(const Vector4Int& v);
		void			operator/=(const Vector4Int& v);
		void			operator=(const INT& v);
		void			operator+=(const INT& v);
		void			operator-=(const INT& v);
		void			operator*=(const INT& v);
		void			operator/=(const INT& v);
	public:
		Vector4Int();
		Vector4Int(const Vector4Int& v);
		Vector4Int(const INT& v);
		Vector4Int(const INT& x, const INT& y, const INT& z);
		Vector4Int(const INT& x, const INT& y, const INT& z, const INT& w);
		Vector4Int(const FLOAT& v);
		Vector4Int(const FLOAT& x, const FLOAT& y, const FLOAT& z);
		Vector4Int(const FLOAT& x, const FLOAT& y, const FLOAT& z, const FLOAT& w);
		Vector4Int(const Vector3& v);
		Vector4Int(const Vector4& v);
		virtual ~Vector4Int();
	private:
		static Vector4Int	GetZero();
	private:
		static Vector4Int	m_Zero;
		INT					x;
		INT					y;
		INT					z;
		INT					w;
	};

	class CMath
	{
	public:
		CMath() {}
		~CMath() {}
	public:
		template<typename T>
		T			Max(const T& v0, const T& v1);
		template<typename T>
		T			Min(const T& v0, const T& v1);
		BOOL		Lerp(const INT& x0, const INT& y0, const INT& x1, const INT& y1, const INT& t, INT& Phi);
		FLOAT		Lerp(const FLOAT& v0, const FLOAT& v1, const FLOAT& t);
	public:
		static const FLOAT&		GetPI() { return m_PI; }
		static const FLOAT&		GetDegToRad() { return m_DegToRad; }
		static const FLOAT&		GetRadToDeg() { return m_RadToDeg; }
	private:
		static FLOAT	m_PI;
		static FLOAT	m_DegToRad;
		static FLOAT	m_RadToDeg;
	};
}