#include "Math.h"

namespace PigeonEngine
{
	FLOAT EMath::m_PI			= 3.1415926535897932f;
	FLOAT EMath::m_RadToDeg		= 57.295779513082321f;
	FLOAT EMath::m_DegToRad		= 0.0174532925199433f;
	const FLOAT& EMath::GetPI()
	{
		return (EMath::m_PI);
	}
	const FLOAT& EMath::GetDegToRad()
	{
		return (EMath::m_DegToRad);
	}
	const FLOAT& EMath::GetRadToDeg()
	{
		return (EMath::m_RadToDeg);
	}
	BOOL EMath::Lerp(const INT& x0, const INT& y0, const INT& x1, const INT& y1, const INT& t, INT& phi)
	{
		if (t < x0 || t > x1)
		{
			return FALSE;
		}
		phi = (INT)(((FLOAT)(t - x1)) / ((FLOAT)(x0 - x1)) * (FLOAT)y0 + ((FLOAT)(t - x0)) / ((FLOAT)(x1 - x0)) * (FLOAT)y1);
		return TRUE;
	}
	FLOAT EMath::Lerp(const FLOAT& v0, const FLOAT& v1, const FLOAT& t)
	{
		return (v0 * (1.f - t) + v1 * t);
	}
	template<typename TValueType>
	TValueType EMath::Max(TValueType v0, TValueType v1)
	{
		return ((v1 > v0) ? v1 : v0);
	}
	template<typename TValueType>
	TValueType EMath::Min(TValueType v0, TValueType v1)
	{
		return ((v0 > v1) ? v1 : v0);
	}
	FLOAT EMath::Abs(const FLOAT& v)
	{
		return fabsf(v);
	}
	FLOAT EMath::Clamp(const FLOAT& v, const FLOAT& min, const FLOAT& max)
	{
		return fmaxf(min, fminf(max, v));
	}
	INT EMath::Clamp(const INT& v, const INT& min, const INT& max)
	{
		return EMath::Max(min, EMath::Min(max, v));
	}
	UINT EMath::Clamp(const UINT& v, const UINT& min, const UINT& max)
	{
		return EMath::Max(min, EMath::Min(max, v));
	}
	FLOAT EMath::Mod(const FLOAT& numerator, const FLOAT& denominator)
	{
		return fmodf(numerator, denominator);
	}
	DOUBLE EMath::Mod(const DOUBLE& numerator, const DOUBLE& denominator)
	{
		return fmod(numerator, denominator);
	}
	FLOAT EMath::Frac(const FLOAT& v)
	{
		return (v - floorf(v));
	}
	FLOAT EMath::Sin(const FLOAT& v)
	{
		return sinf(v);
	}
	FLOAT EMath::Cos(const FLOAT& v)
	{
		return cosf(v);
	}
	void EMath::SinCos(FLOAT& sinValue, FLOAT& cosValue, const FLOAT& v)
	{
		sinValue = sinf(v);
		cosValue = cosf(v);
	}
	FLOAT EMath::Exp2(const FLOAT& v)
	{
		return exp2f(v);
	}
	INT EMath::Exp2(const INT& v)
	{
		return static_cast<INT>(exp2f(static_cast<FLOAT>(v)));
	}
	INT EMath::Log2Floor(const INT& v)
	{
		FLOAT e = log2f(static_cast<FLOAT>(v));
		e = floorf(e);
		return static_cast<INT>(e);
	}
	INT EMath::Log2Floor(const FLOAT& v)
	{
		FLOAT e = log2f(v);
		return static_cast<INT>(floorf(e));
	}
	INT EMath::Log2Ceil(const INT& v)
	{
		FLOAT e = log2f(static_cast<FLOAT>(v));
		e = ceilf(e);
		return static_cast<INT>(e);
	}
	INT EMath::Log2Ceil(const FLOAT& v)
	{
		FLOAT e = log2f(v);
		return static_cast<INT>(ceilf(e));
	}
	INT EMath::PowerOfTwoFloor(FLOAT& output, const FLOAT& input)
	{
		FLOAT e = log2f(input);
		e = floorf(e);
		output = exp2(e);
		return static_cast<INT>(e);
	}
	INT EMath::PowerOfTwoFloor(INT& output, const INT& input)
	{
		FLOAT e = log2f(static_cast<FLOAT>(input));
		e = floorf(e);
		output = static_cast<INT>(exp2f(e));
		return static_cast<INT>(e);
	}
	FLOAT EMath::Sqrt(const FLOAT& v)
	{
		return sqrtf(v);
	}
};