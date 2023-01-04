#pragma once

#include "../../../../Entry/EngineMain.h"

namespace CustomType
{
	class Matrix4x4;
	class VectorBase;
	class Quaternion;
	class Vector2;
	class Vector3;
	class Vector4;
	class Vector2Int;
	class Vector4Int;

	class Matrix4x4
	{
	public:
		DirectX::XMMATRIX			GetXMMATRIX()const { return DirectX::XMLoadFloat4x4(&(this->m_Value)); }
		void						SetXMMATRIX(DirectX::CXMMATRIX m) { DirectX::XMStoreFloat4x4(&(this->m_Value), m); }

		const DirectX::XMFLOAT4X4&	GetXMFLOAT4X4()const { return m_Value; }
		DirectX::XMFLOAT4X4			GetGPUUploadFloat4x4() { return (this->Transpose().GetXMFLOAT4X4()); }
	public:
		static const Matrix4x4&		Identity() { return Matrix4x4::m_Identity; }
		static Matrix4x4			MultiplyMatrix(const Matrix4x4& l, const Matrix4x4& r);
		static Matrix4x4			PerspectiveMatrix(const FLOAT& fovYDeg, const FLOAT& aspectRatio, const FLOAT& nearPlane, const FLOAT& farPlane);
		static Matrix4x4			OrthographicMatrix(const FLOAT& left, const FLOAT& top, const FLOAT& right, const FLOAT& bottom, const FLOAT& nearPlane, const FLOAT& farPlane);
	public:
		void		ResetIdentity() { (*this) = Matrix4x4::m_Identity; }
		void		SetMatrix(const Matrix4x4& m) { (*this) = m; }
		Matrix4x4	Inverse();
		Matrix4x4	Transpose();
		Vector3		MultiplyVector(const Vector3& v);
		Vector4		MultiplyVector(const Vector4& v);
		Vector3		MultiplyPosition(const Vector3& v);
	public:
		Matrix4x4	operator*(const Matrix4x4& m);
		void		operator=(const Matrix4x4& m);
		void		operator*=(const Matrix4x4& m);
	public:
		Matrix4x4();
		Matrix4x4(const Matrix4x4& m);
		Matrix4x4(DirectX::XMFLOAT4X4 m);
		Matrix4x4(DirectX::CXMMATRIX m);
		Matrix4x4(const Quaternion& v);
		Matrix4x4(const Vector3& t, const Quaternion& r);
		Matrix4x4(const Vector3& t, const Quaternion& r, const Vector3& s);
		virtual ~Matrix4x4();
	protected:
		static Matrix4x4		GetIdentity();
	protected:
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
		virtual void				Reset()override { (*this) = Quaternion::m_Identity; }

		DirectX::XMMATRIX			GetXMMATRIX()const { return DirectX::XMMatrixRotationQuaternion(this->GetXMVECTOR()); }
		const DirectX::XMFLOAT4&	GetXMFLOAT4()const { return m_Value; }
		DirectX::XMFLOAT4X4			GetXMFLOAT4X4()const;
		DirectX::XMFLOAT4X4			GetGPUUploadFloat4x4();
	public:
		static const Quaternion&	Identity() { return Quaternion::m_Identity; }
		static Quaternion			Inverse(const Quaternion& v);
		static Quaternion			Normalize(const Quaternion& v);
		static Quaternion			MultiplyQuaternion(const Quaternion& q1, const Quaternion& q2);
		static Quaternion			RotationAxis(const Vector3& axis, const FLOAT& radian);
	public:
		const FLOAT&				X()const { return m_Value.x; }
		const FLOAT&				Y()const { return m_Value.y; }
		const FLOAT&				Z()const { return m_Value.z; }
		const FLOAT&				W()const { return m_Value.w; }
	public:
		Matrix4x4					GetMatrix();
		Quaternion					Inverse();
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
		Quaternion(const Vector3& axis, const FLOAT& radian);
		Quaternion(const FLOAT& x, const FLOAT& y, const FLOAT& z, const FLOAT& w);
		virtual ~Quaternion();
	protected:
		static Quaternion	GetIdentity();
	protected:
		static Quaternion	m_Identity;
		DirectX::XMFLOAT4	m_Value;
	};

	class Vector2 : public VectorBase
	{
	public:
		virtual DirectX::XMVECTOR	GetXMVECTOR()const override { return DirectX::XMLoadFloat2(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { DirectX::XMStoreFloat2(&(this->m_Value), v); }
		virtual void				Normalize()override;
		virtual void				Reset()override { (*this) = Vector2::m_Zero; }

		const DirectX::XMFLOAT2&	GetXMFLOAT2()const { return m_Value; }
		DirectX::XMFLOAT4			GetXMFLOAT4()const { return DirectX::XMFLOAT4(m_Value.x, m_Value.y, 0.f, 0.f); }
	public:
		static const Vector2&		Zero() { return Vector2::m_Zero; }
		static Vector2				Normalize(const Vector2& v);
		static FLOAT				Dot(const Vector2& v1, const Vector2& v2);
		static FLOAT				Length(const Vector2& v);
		static FLOAT				LengthSquare(const Vector2& v);
		static FLOAT				Distance(const Vector2& v1, const Vector2& v2);
		static FLOAT				DistanceSquare(const Vector2& v1, const Vector2& v2);
		static Vector2				Lerp(const Vector2& v1, const Vector2& v2, const FLOAT& t);
	public:
		FLOAT						Dot(const Vector2& v);
		FLOAT						Length();
		FLOAT						LengthSquare();
		FLOAT						Distance(const Vector2& v);
		FLOAT						DistanceSquare(const Vector2& v);
	public:
		const FLOAT&				X()const { return m_Value.x; }
		const FLOAT&				Y()const { return m_Value.y; }
	public:
		Vector2			operator+(const Vector2& v);
		Vector2			operator-(const Vector2& v);
		Vector2			operator*(const Vector2& v);
		Vector2			operator/(const Vector2& v);
		Vector2			operator+(const FLOAT& v);
		Vector2			operator-(const FLOAT& v);
		Vector2			operator*(const FLOAT& v);
		Vector2			operator/(const FLOAT& v);
		Vector2			operator-();
		void			operator=(const Vector2& v);
		void			operator+=(const Vector2& v);
		void			operator-=(const Vector2& v);
		void			operator*=(const Vector2& v);
		void			operator/=(const Vector2& v);
		void			operator=(const FLOAT& v);
		void			operator+=(const FLOAT& v);
		void			operator-=(const FLOAT& v);
		void			operator*=(const FLOAT& v);
		void			operator/=(const FLOAT& v);
	public:
		Vector2();
		Vector2(const Vector2& v);
		Vector2(const Vector2Int& v);
		Vector2(DirectX::CXMVECTOR v);
		Vector2(DirectX::XMFLOAT2 v);
		Vector2(const FLOAT& v);
		Vector2(const FLOAT& x, const FLOAT& y);
		Vector2(const INT& v);
		Vector2(const INT& x, const INT& y);
		virtual ~Vector2();
	protected:
		static Vector2				GetZero();
	protected:
		static Vector2				m_Zero;
		DirectX::XMFLOAT2			m_Value;
	};

	class Vector3 : public VectorBase
	{
	public:
		virtual DirectX::XMVECTOR	GetXMVECTOR()const override { return DirectX::XMLoadFloat3(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { DirectX::XMStoreFloat3(&(this->m_Value), v); }
		virtual void				Normalize()override;
		virtual void				Reset()override { (*this) = Vector3::m_Zero; }

		const DirectX::XMFLOAT3&	GetXMFLOAT3()const { return m_Value; }
		DirectX::XMFLOAT4			GetXMFLOAT4()const { return DirectX::XMFLOAT4(m_Value.x, m_Value.y, m_Value.z, 0.f); }
	public:
		static const Vector3&		Zero() { return Vector3::m_Zero; }
		static const Vector3&		One() { return Vector3::m_One; }
		static const Vector3&		XVector() { return Vector3::m_XVector; }
		static const Vector3&		YVector() { return Vector3::m_YVector; }
		static const Vector3&		ZVector() { return Vector3::m_ZVector; }
		static Vector3				Normalize(const Vector3& v);
		static FLOAT				Dot(const Vector3& v1, const Vector3& v2);
		static Vector3				Cross(const Vector3& v1, const Vector3& v2);
		static FLOAT				Length(const Vector3& v);
		static FLOAT				LengthSquare(const Vector3& v);
		static FLOAT				Distance(const Vector3& v1, const Vector3& v2);
		static FLOAT				DistanceSquare(const Vector3& v1, const Vector3& v2);
		static Vector3				Reciprocal(const Vector3& v);
		static Vector3				Lerp(const Vector3& v1, const Vector3& v2, const FLOAT& t);
	public:
		FLOAT						Dot(const Vector3& v);
		Vector3						Cross(const Vector3& v);
		FLOAT						Length();
		FLOAT						LengthSquare();
		FLOAT						Distance(const Vector3& v);
		FLOAT						DistanceSquare(const Vector3& v);
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
		Vector3(const Vector4& v);
		Vector3(DirectX::CXMVECTOR v);
		Vector3(DirectX::XMFLOAT3 v);
		Vector3(DirectX::XMFLOAT4 v);
		Vector3(const FLOAT& v);
		Vector3(const FLOAT& x, const FLOAT& y, const FLOAT& z);
		virtual ~Vector3();
	protected:
		static Vector3				GetStaticVector(const FLOAT& x, const FLOAT& y, const FLOAT& z);
	protected:
		DirectX::XMFLOAT3			m_Value;
	protected:
		static Vector3				m_Zero;
		static Vector3				m_One;
		static Vector3				m_XVector;
		static Vector3				m_YVector;
		static Vector3				m_ZVector;
	};

	class Vector4 : public VectorBase
	{
	public:
		virtual DirectX::XMVECTOR	GetXMVECTOR()const override { return DirectX::XMLoadFloat4(&(this->m_Value)); }
		virtual void				SetXMVECTOR(DirectX::CXMVECTOR v)override { DirectX::XMStoreFloat4(&(this->m_Value), v); }
		virtual void				Normalize()override {}
		virtual void				Reset()override { (*this) = Vector4::m_Zero; }

		const DirectX::XMFLOAT4&	GetXMFLOAT4()const { return m_Value; }
	public:
		static const Vector4&		Zero() { return Vector4::m_Zero; }
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
		Vector4(const Vector4Int& v);
		virtual ~Vector4();
	protected:
		static Vector4				GetZero();
	protected:
		static Vector4				m_Zero;
		DirectX::XMFLOAT4			m_Value;
	};

	class Vector2Int
	{
	public:
		static const Vector2Int&	Zero() { return Vector2Int::m_Zero; }
	public:
		virtual void				Reset() { (*this) = Vector2Int::m_Zero; }
	public:
		const INT&					X()const { return x; }
		const INT&					Y()const { return y; }
	public:
		Vector2Int		operator+(const Vector2Int& v);
		Vector2Int		operator-(const Vector2Int& v);
		Vector2Int		operator*(const Vector2Int& v);
		Vector2Int		operator/(const Vector2Int& v);
		Vector2Int		operator+(const INT& v);
		Vector2Int		operator-(const INT& v);
		Vector2Int		operator*(const INT& v);
		Vector2Int		operator/(const INT& v);
		Vector2Int		operator-();
		void			operator=(const Vector2Int& v);
		void			operator+=(const Vector2Int& v);
		void			operator-=(const Vector2Int& v);
		void			operator*=(const Vector2Int& v);
		void			operator/=(const Vector2Int& v);
		void			operator=(const INT& v);
		void			operator+=(const INT& v);
		void			operator-=(const INT& v);
		void			operator*=(const INT& v);
		void			operator/=(const INT& v);
		BOOL			operator==(const Vector2Int& v);
		BOOL			operator!=(const Vector2Int& v);
	public:
		Vector2Int();
		Vector2Int(const Vector2Int& v);
		Vector2Int(const INT& v);
		Vector2Int(const INT& x, const INT& y);
		Vector2Int(const FLOAT& v);
		Vector2Int(const FLOAT& x, const FLOAT& y);
		virtual ~Vector2Int();
	protected:
		static Vector2Int	GetZero();
	protected:
		static Vector2Int	m_Zero;
		INT					x;
		INT					y;
	};

	class Vector4Int
	{
	public:
		static const Vector4Int&	Zero() { return Vector4Int::m_Zero; }
	public:
		virtual void		Reset() { (*this) = Vector4Int::m_Zero; }
	public:
		const INT&			X()const { return x; }
		const INT&			Y()const { return y; }
		const INT&			Z()const { return z; }
		const INT&			W()const { return w; }
	public:
		Vector4Int		operator+(const Vector4Int& v);
		Vector4Int		operator-(const Vector4Int& v);
		Vector4Int		operator*(const Vector4Int& v);
		Vector4Int		operator/(const Vector4Int& v);
		Vector4Int		operator+(const INT& v);
		Vector4Int		operator-(const INT& v);
		Vector4Int		operator*(const INT& v);
		Vector4Int		operator/(const INT& v);
		Vector4Int		operator-();
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
		void			operator=(const Vector4& v);
		BOOL			operator==(const Vector4Int& v);
		BOOL			operator!=(const Vector4Int& v);
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
	protected:
		static Vector4Int	GetZero();
	protected:
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
		static const FLOAT&		GetPI();
		static const FLOAT&		GetDegToRad();
		static const FLOAT&		GetRadToDeg();
	protected:
		static FLOAT	m_PI;
		static FLOAT	m_DegToRad;
		static FLOAT	m_RadToDeg;
	public:
		static BOOL		Lerp(const INT& x0, const INT& y0, const INT& x1, const INT& y1, const INT& t, INT& phi);
		static FLOAT	Lerp(const FLOAT& v0, const FLOAT& v1, const FLOAT& t);
		static FLOAT	Max(const FLOAT& v0, const FLOAT& v1);
		static UINT		Max(const UINT& v0, const UINT& v1);
		static INT		Max(const INT& v0, const INT& v1);
		static FLOAT	Min(const FLOAT& v0, const FLOAT& v1);
		static UINT		Min(const UINT& v0, const UINT& v1);
		static INT		Min(const INT& v0, const INT& v1);
		static FLOAT	Abs(const FLOAT& v);
		static FLOAT	Clamp(const FLOAT& v, const FLOAT& min, const FLOAT& max);
		static INT		Clamp(const INT& v, const INT& min, const INT& max);
		static UINT		Clamp(const UINT& v, const UINT& min, const UINT& max);
		static FLOAT	Sin(const FLOAT& v);
		static FLOAT	Cos(const FLOAT& v);
		static void		SinCos(FLOAT& sinValue, FLOAT& cosValue, const FLOAT& v);
		static FLOAT	Exp2(const FLOAT& v);
		static INT		Exp2(const INT& v);
		static INT		Log2Floor(const INT& v);
		static INT		Log2Floor(const FLOAT& v);
		static INT		Log2Ceil(const INT& v);
		static INT		Log2Ceil(const FLOAT& v);
		static INT		PowerOfTwoFloor(FLOAT& output, const FLOAT& input);
		static INT		PowerOfTwoFloor(INT& output, const INT& input);
		static FLOAT	Sqrt(const FLOAT& v);
	};
}