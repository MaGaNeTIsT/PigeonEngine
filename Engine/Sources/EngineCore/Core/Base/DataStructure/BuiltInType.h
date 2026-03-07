#pragma once

#include <Base/BuiltInLanguage.h>
#include <type_traits>
#include <initializer_list>
//#include <functional>

namespace PigeonEngine
{

	typedef	bool						BOOL8;
	typedef	char						INT8;
	typedef	char						CHAR;
	typedef	wchar_t						WCHAR;
	typedef	unsigned char				UINT8;
	typedef	unsigned char				BYTE;
	typedef	short						SHORT;
	typedef	unsigned short				USHORT;
	typedef	unsigned short				WORD;
	typedef	int							INT32;
	typedef	unsigned int				UINT32;
	typedef	long						LONG;
	typedef	long long					INT64;
	typedef	unsigned long				ULONG;
	typedef	unsigned long long			UINT64;
	typedef	unsigned long				DWORD;
	typedef	unsigned long long			size_t;
	typedef	unsigned long long			SIZE_T;
	typedef	float						FLOAT;
	typedef	double						DOUBLE;

#define PE_TEXT(__S)					__S

#ifdef _FORCE_INTEGRAL_BOOL

#ifdef TRUE
#undef TRUE
#endif	// TRUE
#define TRUE							(0x1)

#ifdef FALSE
#undef FALSE
#endif	// FALSE
#define FALSE							(0x0)

#else	// !_FORCE_INTEGRAL_BOOL

#ifdef TRUE
#undef TRUE
#endif
#define TRUE							(true)

#ifdef FALSE
#undef FALSE
#endif
#define FALSE							(false)

#endif	// _FORCE_INTEGRAL_BOOL

#ifdef NULL
#undef NULL
#endif	// NULL
#define NULL							(0)

#ifdef RAND_MAX
#undef RAND_MAX
#endif	// RAND_MAX
#define RAND_MAX						0x7fff

#define PE_ARRAYSIZE(__Array)			(static_cast<UINT32>(sizeof((__Array)) / sizeof(*((__Array)))))

#define PE_FLOAT32_MAX					(3.402823466e+38f)
#define PE_FLOAT32_EPSILON				(1.192092896e-07f) 

#define PE_PI							(3.1415926535897932f)								/* Extra digits if needed: 3.1415926535897932384626433832795f */
#define PE_SMALL_NUMBER					(1.e-8f)
#define PE_KINDA_SMALL_NUMBER			(1.e-4f)
#define PE_BIG_NUMBER					(3.4e+38f)
#define PE_EULERS_NUMBER				(2.71828182845904523536f)
#define PE_GOLDEN_RATIO					(1.6180339887498948482045868343656381f)				/* Also known as divine proportion, golden mean, or golden section - related to the Fibonacci Sequence = (1 + sqrt(5)) / 2 */
#define PE_FLOAT_NON_FRACTIONAL			(8388608.f)											/* All single-precision floating point numbers greater than or equal to this have no fractional value. */

#define PE_DOUBLE_PI					(3.141592653589793238462643383279502884197169399)
#define PE_DOUBLE_SMALL_NUMBER			(1.e-8)
#define PE_DOUBLE_KINDA_SMALL_NUMBER	(1.e-4)
#define PE_DOUBLE_BIG_NUMBER			(3.4e+38)
#define PE_DOUBLE_EULERS_NUMBER			(2.7182818284590452353602874713526624977572)
#define PE_DOUBLE_GOLDEN_RATIO			(1.6180339887498948482045868343656381)				/* Also known as divine proportion, golden mean, or golden section - related to the Fibonacci Sequence = (1 + sqrt(5)) / 2 */
#define PE_DOUBLE_NON_FRACTIONAL		(4503599627370496.0)								/* All double-precision floating point numbers greater than or equal to this have no fractional value. 2^52 */

// Aux constants.
#define PE_INV_PI						(0.31830988618f)
#define PE_HALF_PI						(1.57079632679f)
#define PE_TWO_PI						(6.28318530717f)
#define PE_PI_SQUARED					(9.86960440108f)

#define PE_DOUBLE_INV_PI				(0.31830988618379067154)
#define PE_DOUBLE_HALF_PI				(1.57079632679489661923)
#define PE_DOUBLE_TWO_PI				(6.28318530717958647692)
#define PE_DOUBLE_PI_SQUARED			(9.86960440108935861883)

// Common square roots
#define PE_SQRT_2						(1.4142135623730950488016887242097f)
#define PE_SQRT_3						(1.7320508075688772935274463415059f)
#define PE_INV_SQRT_2					(0.70710678118654752440084436210485f)
#define PE_INV_SQRT_3					(0.57735026918962576450914878050196f)
#define PE_HALF_SQRT_2					(0.70710678118654752440084436210485f)
#define PE_HALF_SQRT_3					(0.86602540378443864676372317075294f)

#define PE_DOUBLE_SQRT_2				(1.4142135623730950488016887242097)
#define PE_DOUBLE_SQRT_3				(1.7320508075688772935274463415059)
#define PE_DOUBLE_INV_SQRT_2			(0.70710678118654752440084436210485)
#define PE_DOUBLE_INV_SQRT_3			(0.57735026918962576450914878050196)
#define PE_DOUBLE_HALF_SQRT_2			(0.70710678118654752440084436210485)
#define PE_DOUBLE_HALF_SQRT_3			(0.86602540378443864676372317075294)

// Magic numbers for numerical precision.
#define PE_DELTA						(0.00001f)
#define PE_DOUBLE_DELTA					(0.00001)

/**
 * Lengths of normalized vectors (These are half their maximum values
 * to assure that dot products with normalized vectors don't overflow).
 */
#define PE_FLOAT_NORMAL_THRESH			(0.0001f)
#define PE_DOUBLE_NORMAL_THRESH			(0.0001)

	//template<typename _TResult, typename... _TArguments>
	//using TFunction = std::function<_TResult(_TArguments...)>;

	template<BOOL8 _Con, typename _Ty = void>
	using TEnableIf = std::enable_if<_Con, _Ty>;

	template<BOOL8 _Con, typename _Ty = void>
	using TEnableIfType = std::enable_if_t<_Con, _Ty>;

	template<typename _Ty>
	using TRemoveRef = std::remove_reference<_Ty>;

	template<typename _Ty>
	using TRemoveRefType = std::remove_reference_t<_Ty>;

	template<typename _Ty>
	using TRemoveExtent = std::remove_extent<_Ty>;

	template<typename _Ty>
	using TRemoveExtentType = std::remove_extent_t<_Ty>;

	template<typename _Ty1, typename _Ty2>
	using TIsSame = std::is_same<_Ty1, _Ty2>;

	template<typename _Ty>
	using TIsFundamental = std::is_fundamental<_Ty>;

	template<typename _Ty>
	using TIsArithmetic = std::is_arithmetic<_Ty>;

	template<typename _Ty>
	using TIsScalar = std::is_scalar<_Ty>;

	template<typename _Ty>
	using TIsVoid = std::is_void<_Ty>;

	template<typename _Ty>
	using TIsNullPtr = std::is_null_pointer<_Ty>;

	template<typename _Ty>
	using TIsPtr = std::is_pointer<_Ty>;

	template<typename _Ty>
	using TIsArray = std::is_array<_Ty>;

	template<typename _Ty>
	using TIsIntegral = std::is_integral<_Ty>;

	template<typename _Ty>
	using TIsUnsigned = std::is_unsigned<_Ty>;

	template<typename _Ty>
	using TIsFloating = std::is_floating_point<_Ty>;

	template<typename _Ty>
	using TIsEnum = std::is_enum<_Ty>;

	template<typename _Ty>
	using TIsUnion = std::is_union<_Ty>;

	template<typename _Ty>
	using TIsClass = std::is_class<_Ty>;

	template<typename _Ty>
	using TIsFunction = std::is_function<_Ty>;

	template<typename _Ty>
	using TInitializerList = std::initializer_list<_Ty>;

#ifdef _FORCE_INTEGRAL_BOOL

	typedef	short						BOOL16;
	typedef	long long					BOOL64;

	struct BOOL32
	{
	public:
		constexpr BOOL32(const INT32 InValue)noexcept : BoolValue(InValue) {}
		BOOL32()noexcept : BoolValue(FALSE) {}
		BOOL32(const BOOL32& Other)noexcept : BoolValue(Other.BoolValue) {}
		BOOL32(const BOOL8& Other)noexcept : BoolValue(Other == true) {}
	public:
		BOOL32& operator=(const BOOL32& Other)
		{
			BoolValue = Other.BoolValue & 0x1;
			return (*this);
		}
		BOOL32& operator|=(const BOOL32& Other)
		{
			BoolValue = (BoolValue & 0x1) | (Other.BoolValue & 0x1);
			return (*this);
		}
		BOOL32& operator&=(const BOOL32& Other)
		{
			BoolValue = (BoolValue & 0x1) & (Other.BoolValue & 0x1);
			return (*this);
		}
		BOOL8 operator==(const BOOL32& Other)const
		{
			return ((BoolValue & 0x1) == (Other.BoolValue & 0x1));
		}
		BOOL8 operator!=(const BOOL32& Other)const
		{
			return ((BoolValue & 0x1) != (Other.BoolValue & 0x1));
		}
		BOOL32& operator=(const INT32& InValue)
		{
			BoolValue = InValue & 0x1;
			return (*this);
		}
		BOOL32& operator|=(const INT32& InValue)
		{
			BoolValue = (BoolValue & 0x1) | (InValue & 0x1);
			return (*this);
		}
		BOOL32& operator&=(const INT32& InValue)
		{
			BoolValue = (BoolValue & 0x1) & (InValue & 0x1);
			return (*this);
		}
		BOOL8 operator==(const INT32& InValue)const
		{
			return ((BoolValue & 0x1) == (InValue & 0x1));
		}
		BOOL8 operator!=(const INT32& InValue)const
		{
			return ((BoolValue & 0x1) != (InValue & 0x1));
		}
		operator BOOL8()const
		{
			return ((BoolValue & 0x1) != 0x0);
		}
		BOOL8 operator!()const
		{
			return ((BoolValue & 0x1) == 0x0);
		}
		BOOL32& operator=(const BOOL8& Other)
		{
			BoolValue = (Other == true);
			return (*this);
		}
		operator INT32()const
		{
			return BoolValue;
		}
	public:
		INT32	BoolValue;
	};

	BOOL32 operator&(const BOOL32& A, const BOOL32& B);
	BOOL32 operator|(const BOOL32& A, const BOOL32& B);
	BOOL8 operator&&(const BOOL32& A, const BOOL32& B);
	BOOL8 operator||(const BOOL32& A, const BOOL32& B);
	BOOL8 operator&&(const BOOL8& A, const BOOL32& B);
	BOOL8 operator||(const BOOL8& A, const BOOL32& B);
	BOOL8 operator&&(const BOOL32& A, const BOOL8& B);
	BOOL8 operator||(const BOOL32& A, const BOOL8& B);
	BOOL8 operator==(const BOOL8& A, const BOOL32& B);
	BOOL8 operator!=(const BOOL8& A, const BOOL32& B);
	BOOL8 operator==(const BOOL32& A, const BOOL8& B);
	BOOL8 operator!=(const BOOL32& A, const BOOL8& B);

#else	// !_FORCE_INTEGRAL_BOOL

	typedef	bool	BOOL16;
	typedef	bool	BOOL64;
	typedef bool	BOOL32;

#endif	// _FORCE_INTEGRAL_BOOL

};