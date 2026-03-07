#include "BuiltInType.h"

namespace PigeonEngine
{

#ifdef _FORCE_INTEGRAL_BOOL

	BOOL32 operator&(const BOOL32& A, const BOOL32& B) { return BOOL32((A.BoolValue & 0x1) & (B.BoolValue & 0x1)); }
	BOOL32 operator|(const BOOL32& A, const BOOL32& B) { return BOOL32((A.BoolValue & 0x1) | (B.BoolValue & 0x1)); }
	BOOL8 operator&&(const BOOL32& A, const BOOL32& B) { return (((A.BoolValue & 0x1) & (B.BoolValue & 0x1)) != 0x0); }
	BOOL8 operator||(const BOOL32& A, const BOOL32& B) { return (((A.BoolValue & 0x1) | (B.BoolValue & 0x1)) != 0x0); }
	BOOL8 operator&&(const BOOL8& A, const BOOL32& B) { return (A && ((B.BoolValue & 0x1) != 0x0)); }
	BOOL8 operator||(const BOOL8& A, const BOOL32& B) { return (A || ((B.BoolValue & 0x1) != 0x0)); }
	BOOL8 operator&&(const BOOL32& A, const BOOL8& B) { return (((A.BoolValue & 0x1) != 0x0) && B); }
	BOOL8 operator||(const BOOL32& A, const BOOL8& B) { return (((A.BoolValue & 0x1) != 0x0) || B); }

	BOOL8 operator==(const BOOL8& A, const BOOL32& B)
	{
		if (A)
		{
			return ((B.BoolValue & 0x1) != 0x0);
		}
		else
		{
			return ((B.BoolValue & 0x1) == 0x0);
		}
	}
	BOOL8 operator!=(const BOOL8& A, const BOOL32& B)
	{
		if (A)
		{
			return ((B.BoolValue & 0x1) == 0x0);
		}
		else
		{
			return ((B.BoolValue & 0x1) != 0x0);
		}
	}
	BOOL8 operator==(const BOOL32& A, const BOOL8& B)
	{
		if (B)
		{
			return ((A.BoolValue & 0x1) != 0x0);
		}
		else
		{
			return ((A.BoolValue & 0x1) == 0x0);
		}
	}
	BOOL8 operator!=(const BOOL32& A, const BOOL8& B)
	{
		if (B)
		{
			return ((A.BoolValue & 0x1) == 0x0);
		}
		else
		{
			return ((A.BoolValue & 0x1) != 0x0);
		}
	}

#endif

};