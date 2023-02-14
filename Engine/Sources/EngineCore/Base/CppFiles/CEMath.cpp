#include "../Headers/CEMath.h"
#include "../../Core/Headers/CEMain.h"

namespace PigeonEngine
{
	FLOAT CMath::m_PI		= 3.1415926535897932f;
	FLOAT CMath::m_RadToDeg	= 57.295779513082321f;
	FLOAT CMath::m_DegToRad	= 0.0174532925199433f;
	const FLOAT& CMath::GetPI()
	{
		return (CMath::m_PI);
	}
	const FLOAT& CMath::GetDegToRad()
	{
		return (CMath::m_DegToRad);
	}
	const FLOAT& CMath::GetRadToDeg()
	{
		return (CMath::m_RadToDeg);
	}
	BOOL CMath::Lerp(const INT& x0, const INT& y0, const INT& x1, const INT& y1, const INT& t, INT& phi)
	{
		if (t < x0 || t > x1)
		{
			return FALSE;
		}
		phi = (INT)(((FLOAT)(t - x1)) / ((FLOAT)(x0 - x1)) * (FLOAT)y0 + ((FLOAT)(t - x0)) / ((FLOAT)(x1 - x0)) * (FLOAT)y1);
		return TRUE;
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
	FLOAT CMath::Frac(const FLOAT& v)
	{
		return (v - floorf(v));
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
};