#pragma once

#include "../../../Main/Main.h"
#include "../DataStructure/BuiltInType.h"

namespace PigeonEngine
{
	class EMath
	{
	public:
		static const FLOAT& GetPI();
		static const FLOAT& GetDegToRad();
		static const FLOAT& GetRadToDeg();
	protected:
		static FLOAT	m_PI;
		static FLOAT	m_DegToRad;
		static FLOAT	m_RadToDeg;
	public:
		static BOOL			Lerp(const INT& x0, const INT& y0, const INT& x1, const INT& y1, const INT& t, INT& phi);
		static FLOAT		Lerp(const FLOAT& v0, const FLOAT& v1, const FLOAT& t);
		template<typename TValueType>
		static TValueType Max(const TValueType& v0, const TValueType& v1)
		{
			return ((v1 > v0) ? v1 : v0);
		}
		template<typename TValueType>
		static TValueType Min(const TValueType& v0, const TValueType& v1)
		{
			return ((v0 > v1) ? v1 : v0);
		}
		template<typename TValueType>
		static TValueType Clamp(const TValueType& v, const TValueType& min, const TValueType& max)
		{
			if (v > max)
			{
				return max;
			}
			else if (v < min)
			{
				return min;
			}
			return v;
		}
		static FLOAT	Abs(const FLOAT& v);
		static FLOAT	Mod(const FLOAT& numerator, const FLOAT& denominator);
		static DOUBLE	Mod(const DOUBLE& numerator, const DOUBLE& denominator);
		static FLOAT	Frac(const FLOAT& v);
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
	public:
		template<typename VType, typename IType>
		static VType LinearBezierCurve(const VType& p0, const VType& p1, const IType& t)
		{
			VType v0 = p0, v1 = p1;
			IType d = t;
			IType oneMinusD = static_cast<IType>(1) - d;
			return (v0 * oneMinusD + v1 * d);
		}
		template<typename VType, typename IType>
		static VType QuadraticBezierCurve(const VType& p0, const VType& p1, const VType& p2, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2;
			IType d = t; IType dSq = d * d;
			IType oneMinusD = static_cast<IType>(1) - d; IType oneMinusDSq = oneMinusD * oneMinusD;
			return (v0 * oneMinusDSq + v1 * (static_cast<IType>(2) * oneMinusD * d) + v2 * dSq);
		}
		template<typename VType, typename IType>
		static VType DerivativeQuadraticBezierCurve(const VType& p0, const VType& p1, const VType& p2, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2;
			IType d = t; IType oneMinusD = static_cast<IType>(1) - d;
			return ((v1 - v0) * (static_cast<IType>(2) * oneMinusD) + (v2 - v1) * (static_cast<IType>(2) * d));
		}
		template<typename VType, typename IType>
		static VType SecondDerivativeQuadraticBezierCurve(const VType& p0, const VType& p1, const VType& p2, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2;
			IType two = static_cast<IType>(2);
			return ((v2 - v1 * two + v0) * two);
		}
		template<typename VType, typename IType>
		static VType CubicBezierCurve(const VType& p0, const VType& p1, const VType& p2, const VType& p3, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2, v3 = p3;
			IType d = t; IType dSq = d * d; IType dCu = d * d * d;
			IType oneMinusD = static_cast<IType>(1) - d; IType oneMinusDSq = oneMinusD * oneMinusD; IType oneMinusDCu = oneMinusD * oneMinusD * oneMinusD;
			return (v0 * oneMinusDCu + v1 * (static_cast<IType>(3) * oneMinusDSq * d) + v2 * (static_cast<IType>(3) * oneMinusD * dSq) + v3 * dCu);
		}
		template<typename VType, typename IType>
		static VType DerivativeCubicBezierCurve(const VType& p0, const VType& p1, const VType& p2, const VType& p3, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2, v3 = p3;
			IType d = t; IType dSq = d * d;
			IType oneMinusD = static_cast<IType>(1) - d; IType oneMinusDSq = oneMinusD * oneMinusD;
			return ((v1 - v0) * (static_cast<IType>(3) * oneMinusDSq) + (v2 - v1) * (static_cast<IType>(6) * oneMinusD * d) + (v3 - v2) * (static_cast<IType>(3) * dSq));
		}
		template<typename VType, typename IType>
		static VType SecondDerivativeCubicBezierCurve(const VType& p0, const VType& p1, const VType& p2, const VType& p3, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2, v3 = p3;
			IType d = t; IType oneMinusD = static_cast<IType>(1) - d;
			IType two = static_cast<IType>(2), six = static_cast<IType>(6);
			return ((v2 - v1 * two + v0) * six * oneMinusD + (v3 - v2 * two + v1) * six * d);
		}
	private:
		EMath() {}
		EMath(const EMath&) {}
		~EMath() {}
		EMath& operator=(const EMath&) { return (*this); }
	public:
		static EMath* GetSingleton()
		{
			static EMath _MathSingletonObject;
			return (&_MathSingletonObject);
		}
	};
};