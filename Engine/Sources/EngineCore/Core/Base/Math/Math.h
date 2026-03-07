#pragma once

#include "../../../Main/Main.h"
#include <memory>
#include "../DataStructure/BuiltInType.h"
#include <Config/ErrorCaption.h>
#include "../../../../Development/Alert/DevelopmentDefines.h"

namespace PigeonEngine
{

	class EMath final
	{
	public:
		/** Computes absolute value in a generic way */
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE TValueType	Abs(const TValueType InValue) { return ((InValue < ((TValueType)0)) ? (-InValue) : InValue); }

		/** Returns 1, 0, or -1 depending on relation of T to 0 */
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE TValueType	Sign(const TValueType InValue) { return ((InValue > ((TValueType)0)) ? ((TValueType)1) : ((InValue < ((TValueType)0)) ? ((TValueType)-1) : ((TValueType)0))); }

		/** Returns higher value in a generic way */
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE TValueType	Max(const TValueType A, const TValueType B) { return ((B < A) ? A : B); }

		/** Returns lower value in a generic way */
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE TValueType	Min(const TValueType A, const TValueType B) { return (A < B) ? A : B; }

		/** Returns value^2 in a generic way */
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE TValueType	Square(const TValueType InValue) { return (InValue * InValue); }

		/** Clamps X to be between Min and Max, inclusive */
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE TValueType	Clamp(const TValueType X, const TValueType Min, const TValueType Max) { return ((X < Min) ? Min : (X < Max) ? X : Max); }
	public:
		static PE_FORCEINLINE UINT32 AsUint32(const FLOAT InFloat)
		{
			UINT32 Uint{};
			static_assert(sizeof(InFloat) == sizeof(Uint), "The float and uint sizes must be equal");
			::memcpy_s(&Uint, sizeof(Uint), &InFloat, sizeof(InFloat));
			return Uint;
		}
		static PE_FORCEINLINE INT32 AsInt32(const FLOAT InFloat)
		{
			INT32 Int{};
			static_assert(sizeof(InFloat) == sizeof(Int), "The float and int sizes must be equal");
			::memcpy_s(&Int, sizeof(Int), &InFloat, sizeof(InFloat));
			return Int;
		}
		static PE_FORCEINLINE UINT64 AsUint64(const DOUBLE InDouble)
		{
			UINT64 Uint64{};
			static_assert(sizeof(InDouble) == sizeof(Uint64), "The double and uint64 sizes must be equal");
			::memcpy_s(&Uint64, sizeof(Uint64), &InDouble, sizeof(InDouble));
			return Uint64;
		}
		static PE_FORCEINLINE INT64 AsInt64(const DOUBLE InDouble)
		{
			INT64 Int64{};
			static_assert(sizeof(InDouble) == sizeof(Int64), "The double and int64 sizes must be equal");
			::memcpy_s(&Int64, sizeof(Int64), &InDouble, sizeof(InDouble));
			return Int64;
		}
		static PE_FORCEINLINE FLOAT AsFloat(const UINT32 InUint32)
		{
			FLOAT Float{};
			static_assert(sizeof(InUint32) == sizeof(Float), "The float and uint sizes must be equal");
			::memcpy_s(&Float, sizeof(Float), &InUint32, sizeof(InUint32));
			return Float;
		}
		static PE_FORCEINLINE FLOAT AsFloat(const INT32 InInt32)
		{
			FLOAT Float{};
			static_assert(sizeof(InInt32) == sizeof(Float), "The float and int sizes must be equal");
			::memcpy_s(&Float, sizeof(Float), &InInt32, sizeof(InInt32));
			return Float;
		}
		static PE_FORCEINLINE DOUBLE AsDouble(const UINT64 InUint64)
		{
			DOUBLE Double{};
			static_assert(sizeof(InUint64) == sizeof(Double), "The double and uint64 sizes must be equal");
			::memcpy_s(&Double, sizeof(Double), &InUint64, sizeof(InUint64));
			return Double;
		}
		static PE_FORCEINLINE DOUBLE AsDouble(const INT64 InInt64)
		{
			DOUBLE Double{};
			static_assert(sizeof(InInt64) == sizeof(Double), "The double and int64 sizes must be equal");
			::memcpy_s(&Double, sizeof(Double), &InInt64, sizeof(InInt64));
			return Double;
		}

		/**
		* Converts an integer or a double to a float with truncation towards zero.
		* @param InValue	Input value to convert
		* @return			Truncated float.
		*/
		static PE_CONSTEXPR PE_FORCEINLINE FLOAT	TruncToFloat(const INT32 InInt32) { return ((FLOAT)InInt32); }
		static PE_CONSTEXPR PE_FORCEINLINE FLOAT	TruncToFloat(const INT64 InInt64) { return ((FLOAT)InInt64); }
		static PE_CONSTEXPR PE_FORCEINLINE FLOAT	TruncToFloat(const UINT32 InUint32) { return ((FLOAT)InUint32); }
		static PE_CONSTEXPR PE_FORCEINLINE FLOAT	TruncToFloat(const UINT64 InUint64) { return ((FLOAT)InUint64); }

		/**
		* Converts an integer or a double to a double with truncation towards zero.
		* @param InValue	Input value to convert
		* @return			Truncated double.
		*/
		static PE_CONSTEXPR PE_FORCEINLINE DOUBLE	TruncToDouble(const INT32 InInt32) { return ((DOUBLE)InInt32); }
		static PE_CONSTEXPR PE_FORCEINLINE DOUBLE	TruncToDouble(const INT64 InInt64) { return ((DOUBLE)InInt64); }
		static PE_CONSTEXPR PE_FORCEINLINE DOUBLE	TruncToDouble(const UINT32 InUint32) { return ((DOUBLE)InUint32); }
		static PE_CONSTEXPR PE_FORCEINLINE DOUBLE	TruncToDouble(const UINT64 InUint64) { return ((DOUBLE)InUint64); }

		/**
		* Converts a float to an integer value with truncation towards zero.
		* @param InFloat	Floating point value to convert
		* @return			Truncated integer value.
		*/
		static PE_FORCEINLINE FLOAT					TruncToFloat(const FLOAT InFloat) { return truncf(InFloat); }

		/**
		 * Converts a double to an integer value with truncation towards zero.
		 * @param InDouble	Floating point value to convert
		 * @return			Truncated integer value.
		 */
		static PE_FORCEINLINE DOUBLE				TruncToDouble(const DOUBLE InDouble) { return trunc(InDouble); }
		static PE_FORCEINLINE DOUBLE				TruncToFloat(const DOUBLE InDouble) { return TruncToDouble(InDouble); }	//TODO

		/**
		* Converts a float to an integer with truncation towards zero.
		* @param InValue	Floating point value to convert
		* @return			Truncated integer.
		*/
		static PE_CONSTEXPR PE_FORCEINLINE INT32	TruncToInt32(const FLOAT InFloat) { return ((INT32)InFloat); }
		static PE_CONSTEXPR PE_FORCEINLINE INT32	TruncToInt32(const DOUBLE InDouble) { return ((INT32)InDouble); }
		static PE_CONSTEXPR PE_FORCEINLINE INT64	TruncToInt64(const FLOAT InFloat) { return ((INT64)InFloat); }
		static PE_CONSTEXPR PE_FORCEINLINE INT64	TruncToInt64(const DOUBLE InDouble) { return ((INT64)InDouble); }
		static PE_CONSTEXPR PE_FORCEINLINE UINT32	TruncToUint32(const FLOAT InFloat) { return ((UINT32)InFloat); }
		static PE_CONSTEXPR PE_FORCEINLINE UINT32	TruncToUint32(const DOUBLE InDouble) { return ((UINT32)InDouble); }
		static PE_CONSTEXPR PE_FORCEINLINE UINT64	TruncToUint64(const FLOAT InFloat) { return ((UINT64)InFloat); }
		static PE_CONSTEXPR PE_FORCEINLINE UINT64	TruncToUint64(const DOUBLE InDouble) { return ((UINT64)InDouble); }

		/**
		* Converts a float to a nearest less or equal integer.
		* @param InValue	Floating point value to convert
		* @return			An integer less or equal to 'InFloat'.
		*/
		static PE_FORCEINLINE INT32 FloorToInt32(const FLOAT InFloat)
		{
			INT32 I = TruncToInt32(InFloat);
			I -= (((FLOAT)I) > InFloat);
			return I;
		}
		static PE_FORCEINLINE INT32 FloorToInt32(const DOUBLE InDouble)
		{
			INT32 I = TruncToInt32(InDouble);
			I -= (((DOUBLE)I) > InDouble);
			return I;
		}
		static PE_FORCEINLINE INT64 FloorToInt64(const DOUBLE InDouble)
		{
			INT64 I = TruncToInt64(InDouble);
			I -= (((DOUBLE)I) > InDouble);
			return I;
		}

		/**
		* Converts a float to the nearest less or equal integer.
		* @param InFloat	Floating point value to convert
		* @return			An integer less or equal to 'InFloat'.
		*/
		static PE_FORCEINLINE FLOAT		FloorToFloat(const FLOAT InFloat) { return floorf(InFloat); }

		/**
		* Converts a double to a less or equal integer.
		* @param InDouble	Floating point value to convert
		* @return			The nearest integer value to 'InDouble'.
		*/
		static PE_FORCEINLINE DOUBLE	FloorToDouble(const DOUBLE InDouble) { return floor(InDouble); }
		static PE_FORCEINLINE DOUBLE	FloorToFloat(const DOUBLE InDouble) { return FloorToDouble(InDouble); }	//TODO

		/**
		 * Converts a float to the nearest integer. Rounds up when the fraction is .5
		 * @param InValue	Floating point value to convert
		 * @return			The nearest integer to 'InValue'.
		 */
		static PE_FORCEINLINE INT32		RoundToInt32(const FLOAT InFloat) { return FloorToInt32(InFloat + 0.5f); }
		static PE_FORCEINLINE INT32		RoundToInt32(const DOUBLE InDouble) { return FloorToInt32(InDouble + 0.5); }
		static PE_FORCEINLINE INT64		RoundToInt64(const DOUBLE InDouble) { return FloorToInt64(InDouble + 0.5); }

		/**
		* Converts a float to the nearest integer. Rounds up when the fraction is .5
		* @param InValue	Floating point value to convert
		* @return			The nearest integer to 'InValue'.
		*/
		static PE_FORCEINLINE FLOAT		RoundToFloat(const FLOAT InFloat) { return FloorToFloat(InFloat + 0.5f); }

		/**
		* Converts a double to the nearest integer. Rounds up when the fraction is .5
		* @param InValue	Floating point value to convert
		* @return			The nearest integer to 'InValue'.
		*/
		static PE_FORCEINLINE DOUBLE	RoundToDouble(const DOUBLE InDouble) { return FloorToDouble(InDouble + 0.5); }
		static PE_FORCEINLINE DOUBLE	RoundToFloat(const DOUBLE InDouble) { return RoundToDouble(InDouble); }

		/**
		* Converts a float to the nearest greater or equal integer.
		* @param InValue	Floating point value to convert
		* @return			An integer greater or equal to 'InValue'.
		*/
		static PE_FORCEINLINE INT32 CeilToInt32(const FLOAT InFloat)
		{
			INT32 I = TruncToInt32(InFloat);
			I += (((FLOAT)I) < InFloat);
			return I;
		}
		static PE_FORCEINLINE INT32 CeilToInt32(const DOUBLE InDouble)
		{
			INT32 I = TruncToInt32(InDouble);
			I += (((DOUBLE)I) < InDouble);
			return I;
		}
		static PE_FORCEINLINE INT64 CeilToInt64(const DOUBLE InDouble)
		{
			INT64 I = TruncToInt64(InDouble);
			I += (((DOUBLE)I) < InDouble);
			return I;
		}

		/**
		* Converts a float to the nearest greater or equal integer.
		* @param InValue	Floating point value to convert
		* @return			An integer greater or equal to 'InValue'.
		*/
		static PE_FORCEINLINE FLOAT		CeilToFloat(const FLOAT InFloat) { return ceilf(InFloat); }

		/**
		* Converts a double to the nearest greater or equal integer.
		* @param InValue	Floating point value to convert
		* @return			An integer greater or equal to 'InValue'.
		*/
		static PE_FORCEINLINE DOUBLE	CeilToDouble(const DOUBLE InDouble) { return ceil(InDouble); }
		static PE_FORCEINLINE DOUBLE	CeilToFloat(const DOUBLE InDouble) { return CeilToDouble(InDouble); }

		/**
		* Returns signed fractional part of a float.
		* @param InValue	Floating point value to convert
		* @return			A float between >=0 and < 1 for nonnegative input. A float between >= -1 and < 0 for negative input.
		*/
		static PE_FORCEINLINE FLOAT		Fractional(const FLOAT InValue) { return (InValue - TruncToFloat(InValue)); }
		static PE_FORCEINLINE DOUBLE	Fractional(const DOUBLE InValue) { return (InValue - TruncToDouble(InValue)); }

		/**
		* Returns the fractional part of a float.
		* @param InValue	Floating point value to convert
		* @return			A float between >=0 and < 1.
		*/
		static PE_FORCEINLINE FLOAT		Frac(const FLOAT InValue) { return (InValue - FloorToFloat(InValue)); }
		static PE_FORCEINLINE DOUBLE	Frac(const DOUBLE InValue) { return (InValue - FloorToDouble(InValue)); }

		/**
		* Breaks the given value into an integral and a fractional part.
		* @param InValue	Floating point value to convert
		* @param OutIntPart Floating point value that receives the integral part of the number.
		* @return			The fractional part of the number.
		*/
		static PE_FORCEINLINE FLOAT		Modf(const FLOAT InValue, FLOAT* OutIntPart) { return modff(InValue, OutIntPart); }

		/**
		* Breaks the given value into an integral and a fractional part.
		* @param InValue	Floating point value to convert
		* @param OutIntPart Floating point value that receives the integral part of the number.
		* @return			The fractional part of the number.
		*/
		static PE_FORCEINLINE DOUBLE	Modf(const DOUBLE InValue, DOUBLE* OutIntPart) { return modf(InValue, OutIntPart); }

		static PE_FORCEINLINE FLOAT		Exp(const FLOAT InValue) { return expf(InValue); }
		static PE_FORCEINLINE DOUBLE	Exp(const DOUBLE InValue) { return exp(InValue); }
		static PE_FORCEINLINE FLOAT		Exp2(const FLOAT InValue) { return powf(2.f, InValue);/*exp2f(InValue)*/ }
		static PE_FORCEINLINE DOUBLE	Exp2(const DOUBLE InValue) { return pow(2.0, InValue);/*exp2(InValue)*/ }
		static PE_FORCEINLINE FLOAT		LogE(const FLOAT InValue) { return logf(InValue); }
		static PE_FORCEINLINE DOUBLE	LogE(const DOUBLE InValue) { return log(InValue); }
		static PE_FORCEINLINE FLOAT		LogX(const FLOAT InBase, const FLOAT InValue) { return (LogE(InValue) / LogE(InBase)); }
		static PE_FORCEINLINE DOUBLE	LogX(const DOUBLE InBase, const DOUBLE InValue) { return (LogE(InValue) / LogE(InBase)); }
		static PE_FORCEINLINE FLOAT		Log2(const FLOAT InValue) { return (LogE(InValue) * 1.4426950f); }
		static PE_FORCEINLINE DOUBLE	Log2(const DOUBLE InValue) { return (LogE(InValue) * 1.442695040888963387); }

		/**
		* Computes the base 2 logarithm for an integer value.
		* The result is rounded down to the nearest integer.
		*
		* @param InValue	The value to compute the log of
		* @return			Log2 of InValue. 0 if InValue is 0.
		*/
		static PE_FORCEINLINE UINT32 FloorLog2(UINT32 InValue)
		{
			UINT32 pos = 0;
			if (InValue >= 1 << 16) { InValue >>= 16; pos += 16; }
			if (InValue >= 1 << 8) { InValue >>= 8; pos += 8; }
			if (InValue >= 1 << 4) { InValue >>= 4; pos += 4; }
			if (InValue >= 1 << 2) { InValue >>= 2; pos += 2; }
			if (InValue >= 1 << 1) { pos += 1; }
			return pos;
		}

		/**
		 * Computes the base 2 logarithm for a 64-bit value.
		 * The result is rounded down to the nearest integer.
		 *
		 * @param InValue	The value to compute the log of
		 * @return			Log2 of InValue. 0 if InValue is 0.
		 */
		static PE_FORCEINLINE UINT64 FloorLog2_64(UINT64 InValue)
		{
			UINT64 pos = 0;
			if (InValue >= 1ull << 32) { InValue >>= 32; pos += 32; }
			if (InValue >= 1ull << 16) { InValue >>= 16; pos += 16; }
			if (InValue >= 1ull << 8) { InValue >>= 8; pos += 8; }
			if (InValue >= 1ull << 4) { InValue >>= 4; pos += 4; }
			if (InValue >= 1ull << 2) { InValue >>= 2; pos += 2; }
			if (InValue >= 1ull << 1) { pos += 1; }
			return pos;
		}

		/**
		 * Counts the number of leading zeros in the bit representation of the 32-bit value
		 *
		 * @param InValue the value to determine the number of leading zeros for
		 *
		 * @return the number of zeros before the first "on" bit
		 */
		static PE_FORCEINLINE UINT32 CountLeadingZeros(const UINT32 InValue)
		{
			if (InValue == 0u)
			{
				return 32u;
			}
			return (31u - FloorLog2(InValue));
		}

		/**
		 * Counts the number of leading zeros in the bit representation of the 64-bit value
		 *
		 * @param InValue the value to determine the number of leading zeros for
		 *
		 * @return the number of zeros before the first "on" bit
		 */
		static PE_FORCEINLINE UINT64 CountLeadingZeros64(const UINT64 InValue)
		{
			if (InValue == 0u)
			{
				return 64u;
			}
			return (63u - FloorLog2_64(InValue));
		}

		/**
		 * Returns smallest N such that (1<<N)>=Arg.
		 * Note: CeilLogTwo(0)=0
		 */
		static PE_FORCEINLINE UINT32 CeilLogTwo(UINT32 InArg)
		{
			// if Arg is 0, change it to 1 so that we return 0
			InArg = InArg ? InArg : 1u;
			return (32u - CountLeadingZeros(InArg - 1u));
		}
		static PE_FORCEINLINE UINT64 CeilLogTwo64(UINT64 InArg)
		{
			// if Arg is 0, change it to 1 so that we return 0
			InArg = InArg ? InArg : 1u;
			return (64u - CountLeadingZeros64(InArg - 1u));
		}

		/** @return Rounds the given number up to the next highest power of two. */
		static PE_FORCEINLINE UINT32 RoundUpToPowerOfTwo(const UINT32 InArg)
		{
			return (1u << CeilLogTwo(InArg));
		}

		static PE_FORCEINLINE UINT64 RoundUpToPowerOfTwo64(const UINT64 InArg)
		{
			return (UINT64(1u) << CeilLogTwo64(InArg));
		}

		static PE_FORCEINLINE FLOAT FMod(const FLOAT X, const FLOAT Y)
		{
			const FLOAT AbsY = Abs(Y);
			if (AbsY <= PE_SMALL_NUMBER)	// Note: this constant should match that used by VectorMod() implementations
			{
				PE_CHECK((ENGINE_MATH_ERROR), ("FMath::FMod(X, Y) : Y is zero, this is invalid and would result in NaN!"), (Y != 0));
				return 0.f;
			}
			return fmodf(X, Y);
		}
		static PE_FORCEINLINE DOUBLE FMod(const DOUBLE X, const DOUBLE Y)
		{
			const DOUBLE AbsY = Abs(Y);
			if (AbsY <= PE_DOUBLE_SMALL_NUMBER)	// Note: this constant should match that used by VectorMod() implementations
			{
				PE_CHECK((ENGINE_MATH_ERROR), ("FMath::FMod(X, Y) : Y is zero, this is invalid and would result in NaN!"), (Y != 0));
				return 0.0;
			}
			return fmod(X, Y);
		}

		static PE_FORCEINLINE FLOAT		Sin(const FLOAT InValue) { return sinf(InValue); }
		static PE_FORCEINLINE DOUBLE	Sin(const DOUBLE InValue) { return sin(InValue); }
		static PE_FORCEINLINE FLOAT		ASin(const FLOAT InValue) { return asinf((InValue < -1.f) ? (-1.f) : ((InValue < 1.f) ? InValue : 1.f)); }
		static PE_FORCEINLINE DOUBLE	ASin(const DOUBLE InValue) { return asin((InValue < -1.0) ? (-1.0) : ((InValue < 1.0) ? InValue : 1.0)); }
		static PE_FORCEINLINE FLOAT		SinH(const FLOAT InValue) { return sinhf(InValue); }
		static PE_FORCEINLINE DOUBLE	SinH(const DOUBLE InValue) { return sinh(InValue); }
		static PE_FORCEINLINE FLOAT		Cos(const FLOAT InValue) { return cosf(InValue); }
		static PE_FORCEINLINE DOUBLE	Cos(const DOUBLE InValue) { return cos(InValue); }
		static PE_FORCEINLINE FLOAT		ACos(const FLOAT InValue) { return acosf((InValue < -1.f) ? (-1.f) : ((InValue < 1.f) ? InValue : 1.f)); }
		static PE_FORCEINLINE DOUBLE	ACos(const DOUBLE InValue) { return acos((InValue < -1.0) ? (-1.0) : ((InValue < 1.0) ? InValue : 1.0)); }
		static PE_FORCEINLINE FLOAT		Tan(const FLOAT InValue) { return tanf(InValue); }
		static PE_FORCEINLINE DOUBLE	Tan(const DOUBLE InValue) { return tan(InValue); }
		static PE_FORCEINLINE FLOAT		ATan(const FLOAT InValue) { return atanf(InValue); }
		static PE_FORCEINLINE DOUBLE	ATan(const DOUBLE InValue) { return atan(InValue); }

		static FLOAT Atan2(const FLOAT Y, const FLOAT X)
		{
			//return atan2f(Y,X);
			// atan2f occasionally returns NaN with perfectly valid input (possibly due to a compiler or library bug).
			// We are replacing it with a minimax approximation with a max relative error of 7.15255737e-007 compared to the C library function.
			// On PC this has been measured to be 2x faster than the std C version.

			const FLOAT absX = Abs(X);
			const FLOAT absY = Abs(Y);
			const BOOL32 yAbsBigger = (absY > absX);
			FLOAT t0 = yAbsBigger ? absY : absX;	// Max(absY, absX)
			FLOAT t1 = yAbsBigger ? absX : absY;	// Min(absX, absY)

			if (t0 == 0.f)
			{
				return 0.f;
			}

			FLOAT t3 = t1 / t0;
			FLOAT t4 = t3 * t3;

			static const FLOAT c[7] =
			{
				+7.2128853633444123e-03f,
				-3.5059680836411644e-02f,
				+8.1675882859940430e-02f,
				-1.3374657325451267e-01f,
				+1.9856563505717162e-01f,
				-3.3324998579202170e-01f,
				+1.0f
			};

			t0 = c[0];
			t0 = t0 * t4 + c[1];
			t0 = t0 * t4 + c[2];
			t0 = t0 * t4 + c[3];
			t0 = t0 * t4 + c[4];
			t0 = t0 * t4 + c[5];
			t0 = t0 * t4 + c[6];
			t3 = t0 * t3;

			t3 = yAbsBigger ? (0.5f * PE_PI) - t3 : t3;
			t3 = (X < 0.0f) ? PE_PI - t3 : t3;
			t3 = (Y < 0.0f) ? -t3 : t3;

			return t3;
		}
		static DOUBLE Atan2(const DOUBLE Y, const DOUBLE X)
		{
			if (X == 0.0 && Y == 0.0)
			{
				return 0.0;
			}
			return atan2(Y, X);
		}

		// Note:  We use FASTASIN_HALF_PI instead of HALF_PI inside of FastASin(), since it was the value that accompanied the minimax coefficients below.
		// It is important to use exactly the same value in all places inside this function to ensure that FastASin(0.0f) == 0.0f.
		// For comparison:
		//		HALF_PI				== 1.57079632679f == 0x3fC90FDB
		//		FASTASIN_HALF_PI	== 1.5707963050f  == 0x3fC90FDA
#define PE_FASTASIN_HALF_PI		(1.5707963050f)

		PE_NODISCARD static PE_FORCEINLINE FLOAT FastAsin(const FLOAT InValue)
		{
			// Clamp input to [-1,1].
			BOOL8 nonnegative = (InValue >= 0.0f);
			FLOAT x = EMath::Abs(InValue);
			FLOAT omx = 1.0f - x;
			if (omx < 0.0f)
			{
				omx = 0.0f;
			}
			FLOAT root = EMath::Sqrt(omx);
			// 7-degree minimax approximation
			FLOAT result = ((((((-0.0012624911f * x + 0.0066700901f) * x - 0.0170881256f) * x + 0.0308918810f) * x - 0.0501743046f) * x + 0.0889789874f) * x - 0.2145988016f) * x + PE_FASTASIN_HALF_PI;
			result *= root;  // acos(|x|)
			// acos(x) = pi - acos(-x) when x < 0, asin(x) = pi/2 - acos(x)
			return (nonnegative ? (PE_FASTASIN_HALF_PI - result) : (result - PE_FASTASIN_HALF_PI));
		}

		static PE_FORCEINLINE void SinCos(FLOAT& OutSinValue, FLOAT& OutCosValue, const FLOAT InValue)
		{
			OutSinValue = Sin(InValue);
			OutCosValue = Cos(InValue);
		}
		static PE_FORCEINLINE void SinCos(DOUBLE& OutSinValue, DOUBLE& OutCosValue, const DOUBLE InValue)
		{
			OutSinValue = Sin(InValue);
			OutCosValue = Cos(InValue);
		}
		static PE_FORCEINLINE FLOAT			Sqrt(const FLOAT InValue) { return sqrtf(InValue); }
		static PE_FORCEINLINE DOUBLE		Sqrt(const DOUBLE InValue) { return sqrt(InValue); }
		static PE_FORCEINLINE FLOAT			Pow(const FLOAT A, const FLOAT B) { return powf(A, B); }
		static PE_FORCEINLINE DOUBLE		Pow(const DOUBLE A, const DOUBLE B) { return pow(A, B); }

		/** Computes a fully accurate inverse square root */
		static PE_FORCEINLINE FLOAT			InvSqrt(const FLOAT InValue) { return 1.0f / sqrtf(InValue); }
		static PE_FORCEINLINE DOUBLE		InvSqrt(const DOUBLE InValue) { return 1.0 / sqrt(InValue); }

		/** Computes a faster but less accurate inverse square root */
		static PE_FORCEINLINE FLOAT			InvSqrtEst(const FLOAT InValue) { return InvSqrt(InValue); }
		static PE_FORCEINLINE DOUBLE		InvSqrtEst(const DOUBLE InValue) { return InvSqrt(InValue); }

		/** Return true if value is NaN (not a number). */
		static PE_FORCEINLINE BOOL32 IsNaN(const FLOAT InValue)
		{
			return ((AsUint32(InValue) & 0x7FFFFFFFU) > 0x7F800000U);
		}
		static PE_FORCEINLINE BOOL32 IsNaN(const DOUBLE InValue)
		{
			return ((AsUint64(InValue) & 0x7FFFFFFFFFFFFFFFULL) > 0x7FF0000000000000ULL);
		}

		/** Return true if value is finite (not NaN and not Infinity). */
		static PE_FORCEINLINE BOOL32 IsFinite(const FLOAT InValue)
		{
			return ((AsUint32(InValue) & 0x7F800000U) != 0x7F800000U);
		}
		static PE_FORCEINLINE BOOL32 IsFinite(const DOUBLE InValue)
		{
			return ((AsUint64(InValue) & 0x7FF0000000000000ULL) != 0x7FF0000000000000ULL);
		}

		static PE_FORCEINLINE BOOL32 IsNegativeOrNegativeZero(const FLOAT InValue)
		{
			return (AsUint32(InValue) >= ((UINT32)0x80000000));	// Detects sign bit.
		}
		static PE_FORCEINLINE BOOL32 IsNegativeOrNegativeZero(const DOUBLE InValue)
		{
			return (AsUint64(InValue) >= ((UINT64)0x8000000000000000));	// Detects sign bit.
		}
		static PE_FORCEINLINE UINT32 CountBits(UINT64 InBits)
		{
			// https://en.wikipedia.org/wiki/Hamming_weight
			InBits -= (InBits >> 1) & 0x5555555555555555ull;
			InBits = (InBits & 0x3333333333333333ull) + ((InBits >> 2) & 0x3333333333333333ull);
			InBits = (InBits + (InBits >> 4)) & 0x0f0f0f0f0f0f0f0full;
			return ((InBits * 0x0101010101010101) >> 56);
		}

	public:
		/** Seeds global random number functions Rand() and FRand() */
		static PE_FORCEINLINE void		RandInit(const INT32 InSeed) { srand(InSeed); }

		/** Returns a random integer between 0 and RAND_MAX, inclusive */
		static PE_FORCEINLINE INT32		Rand() { return rand(); }

		/** Returns a random float between 0 and 1, inclusive. */
		static PE_FORCEINLINE FLOAT		FRand()
		{
			// FP32 mantissa can only represent 24 bits before losing precision
			constexpr INT32 RandMax = 0x00ffffff < RAND_MAX ? 0x00ffffff : RAND_MAX;
			return ((Rand() & RandMax) / ((FLOAT)RandMax));
		}

		/** Seeds future calls to SRand() */
		static void						SRandInit(const INT32 InSeed);

		/** Returns the current seed for SRand(). */
		static INT32					GetRandSeed();

		/** Returns a seeded random float in the range [0,1), using the seed from SRandInit(). */
		static FLOAT					SRand();
	public:
		template<typename _TValueTypeX, typename _TValueTypeY>
		static PE_FORCEINLINE BOOL32 Lerp(const _TValueTypeX& X0, const _TValueTypeY& Y0, const _TValueTypeX& X1, const _TValueTypeY& Y1, const _TValueTypeX& T, _TValueTypeY& OutY)
		{
			if ((X0 > X1) || (T < X0) || (T > X1))
			{
				return FALSE;
			}
			FLOAT TempT = (EMath::TruncToFloat(T) - EMath::TruncToFloat(X0)) / (EMath::TruncToFloat(X1) - EMath::TruncToFloat(X0));
			OutY = (_TValueTypeY)(EMath::TruncToFloat(Y0) + (TempT * (EMath::TruncToFloat(Y1) - EMath::TruncToFloat(Y0))));
			return TRUE;
		}
		template<typename _TValueType, typename _TType>
		static PE_FORCEINLINE _TValueType Lerp(const _TValueType& A, const _TValueType& B, const _TType& T)
		{
			return (A * (((_TType)1) - T) + B * T);
		}

	public:
		template<typename VType, typename IType>
		static PE_FORCEINLINE VType LinearBezierCurve(const VType& p0, const VType& p1, const IType& t)
		{
			VType v0 = p0, v1 = p1;
			IType d = t;
			IType oneMinusD = static_cast<IType>(1) - d;
			return (v0 * oneMinusD + v1 * d);
		}
		template<typename VType, typename IType>
		static PE_FORCEINLINE VType QuadraticBezierCurve(const VType& p0, const VType& p1, const VType& p2, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2;
			IType d = t; IType dSq = d * d;
			IType oneMinusD = static_cast<IType>(1) - d; IType oneMinusDSq = oneMinusD * oneMinusD;
			return (v0 * oneMinusDSq + v1 * (static_cast<IType>(2) * oneMinusD * d) + v2 * dSq);
		}
		template<typename VType, typename IType>
		static PE_FORCEINLINE VType DerivativeQuadraticBezierCurve(const VType& p0, const VType& p1, const VType& p2, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2;
			IType d = t; IType oneMinusD = static_cast<IType>(1) - d;
			return ((v1 - v0) * (static_cast<IType>(2) * oneMinusD) + (v2 - v1) * (static_cast<IType>(2) * d));
		}
		template<typename VType, typename IType>
		static PE_FORCEINLINE VType SecondDerivativeQuadraticBezierCurve(const VType& p0, const VType& p1, const VType& p2, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2;
			IType two = static_cast<IType>(2);
			return ((v2 - v1 * two + v0) * two);
		}
		template<typename VType, typename IType>
		static PE_FORCEINLINE VType CubicBezierCurve(const VType& p0, const VType& p1, const VType& p2, const VType& p3, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2, v3 = p3;
			IType d = t; IType dSq = d * d; IType dCu = d * d * d;
			IType oneMinusD = static_cast<IType>(1) - d; IType oneMinusDSq = oneMinusD * oneMinusD; IType oneMinusDCu = oneMinusD * oneMinusD * oneMinusD;
			return (v0 * oneMinusDCu + v1 * (static_cast<IType>(3) * oneMinusDSq * d) + v2 * (static_cast<IType>(3) * oneMinusD * dSq) + v3 * dCu);
		}
		template<typename VType, typename IType>
		static PE_FORCEINLINE VType DerivativeCubicBezierCurve(const VType& p0, const VType& p1, const VType& p2, const VType& p3, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2, v3 = p3;
			IType d = t; IType dSq = d * d;
			IType oneMinusD = static_cast<IType>(1) - d; IType oneMinusDSq = oneMinusD * oneMinusD;
			return ((v1 - v0) * (static_cast<IType>(3) * oneMinusDSq) + (v2 - v1) * (static_cast<IType>(6) * oneMinusD * d) + (v3 - v2) * (static_cast<IType>(3) * dSq));
		}
		template<typename VType, typename IType>
		static PE_FORCEINLINE VType SecondDerivativeCubicBezierCurve(const VType& p0, const VType& p1, const VType& p2, const VType& p3, const IType& t)
		{
			VType v0 = p0, v1 = p1, v2 = p2, v3 = p3;
			IType d = t; IType oneMinusD = static_cast<IType>(1) - d;
			IType two = static_cast<IType>(2), six = static_cast<IType>(6);
			return ((v2 - v1 * two + v0) * six * oneMinusD + (v3 - v2 * two + v1) * six * d);
		}
	public:
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE const FLOAT		EnginePI() { return PE_PI; }
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE const DOUBLE	EnginePIDouble() { return PE_DOUBLE_PI; }

		/**
		* Converts radians to degrees.
		* @param	InRadVal	Value in radians.
		* @return				Value in degrees.
		*/
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE auto RadiansToDegrees(TValueType const& InRadVal) -> decltype(InRadVal* (180.f / PE_PI))
		{
			return InRadVal * (180.f / PE_PI);
		}

		static PE_FORCEINLINE FLOAT		RadiansToDegrees(FLOAT const& InRadVal) { return InRadVal * (180.f / PE_PI); }
		static PE_FORCEINLINE DOUBLE	RadiansToDegrees(DOUBLE const& InRadVal) { return InRadVal * (180.0 / PE_DOUBLE_PI); }

		/**
		* Converts degrees to radians.
		* @param	InDegVal	Value in degrees.
		* @return				Value in radians.
		*/
		template<typename TValueType>
		PE_NODISCARD static PE_CONSTEXPR PE_FORCEINLINE auto DegreesToRadians(TValueType const& InDegVal) -> decltype(InDegVal* (PE_PI / 180.f))
		{
			return InDegVal * (PE_PI / 180.f);
		}

		static PE_FORCEINLINE FLOAT		DegreesToRadians(FLOAT const& DegVal) { return DegVal * (PE_PI / 180.f); }
		static PE_FORCEINLINE DOUBLE	DegreesToRadians(DOUBLE const& DegVal) { return DegVal * (PE_DOUBLE_PI / 180.0); }
	private:
		EMath()noexcept {}
		EMath(const EMath&)noexcept {}
		EMath(EMath&&)noexcept {}
		~EMath() {}
		EMath& operator=(const EMath&)noexcept { return (*this); }
		EMath& operator=(EMath&&)noexcept { return (*this); }
	public:
		static EMath* GetSingleton()
		{
			static EMath _MathSingletonObject;
			return (&_MathSingletonObject);
		}
	};
};