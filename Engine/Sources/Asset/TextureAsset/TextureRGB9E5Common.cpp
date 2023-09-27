#include "TextureRGB9E5Common.h"

namespace PigeonEngine
{

	// OpenGL rgb9e5 reference implementation from
	// https://www.opengl.org/registry/specs/EXT/texture_shared_exponent.txt
	// but modified to only support little endian.

	struct EBitsOfIEEE754
	{
		UINT32	Mantissa		: 23;
		UINT32	BiasedExponent	: 8;
		UINT32	Negative		: 1;
	};

	union EFloat754
	{
		UINT32			Raw;
		FLOAT			Value;
		EBitsOfIEEE754	Field;
	};

	static FLOAT ClampRangeForRGB9E5(const FLOAT InValue)
	{
		if (InValue > 0.0f)
		{
			if (InValue >= ETextureRGB9E5Common::MAX_RGB9E5)
			{
				return (ETextureRGB9E5Common::MAX_RGB9E5);
			}
			else
			{
				return InValue;
			}
		}
		else
		{
			PE_FAILED((ENGINE_MATH_ERROR), ("NaN gets here too since comparisons with NaN always fail!"));
			return 0.0f;
		}
	}

	// Ok, FloorLog2 is not correct for the denorm and zero values, but we
	// are going to do a max of this value with the minimum rgb9e5 exponent
	// that will hide these problem cases.
	static INT32 FloorLog2(const FLOAT InValue)
	{
		EFloat754 Float;
		Float.Value = InValue;
		return (Float.Field.BiasedExponent - 127);
	}

	void ETextureRGB9E5Common::Color3ToRGB9E5(const Color3& InRGB, RGB9E5& OutRGB9E5)
	{
		RGB9E5 Retval;
		FLOAT MaxRGB;
		INT32 rm, gm, bm;
		FLOAT rc, gc, bc;
		INT32 ExpShared;
		FLOAT Denom;

		rc = ClampRangeForRGB9E5(InRGB.r);
		gc = ClampRangeForRGB9E5(InRGB.g);
		bc = ClampRangeForRGB9E5(InRGB.b);

		MaxRGB = EMath::Max(EMath::Max(rc, gc), bc);
		ExpShared = EMath::Max(-ETextureRGB9E5Common::RGB9E5_EXP_BIAS - 1, FloorLog2(MaxRGB)) + 1 + ETextureRGB9E5Common::RGB9E5_EXP_BIAS;
		Check((ExpShared <= ETextureRGB9E5Common::RGB9E5_MAX_VALID_BIASED_EXP));
		Check((ExpShared >= 0));

		// This pow function could be replaced by a table.
		Denom = EMath::Pow(2.f, EMath::TruncToFloat(ExpShared - ETextureRGB9E5Common::RGB9E5_EXP_BIAS - ETextureRGB9E5Common::RGB9E5_MANTISSA_BITS));

		INT32 MaxM = EMath::FloorToInt32(MaxRGB / Denom + 0.5f);
		if (MaxM == ETextureRGB9E5Common::MAX_RGB9E5_MANTISSA + 1)
		{
			Denom *= 2.f;
			ExpShared += 1;
			Check((ExpShared <= ETextureRGB9E5Common::RGB9E5_MAX_VALID_BIASED_EXP));
		}
		else
		{
			Check((MaxM <= ETextureRGB9E5Common::MAX_RGB9E5_MANTISSA));
		}

		rm = EMath::FloorToInt32(rc / Denom + 0.5f);
		gm = EMath::FloorToInt32(gc / Denom + 0.5f);
		bm = EMath::FloorToInt32(bc / Denom + 0.5f);

		Check((rm <= ETextureRGB9E5Common::MAX_RGB9E5_MANTISSA));
		Check((gm <= ETextureRGB9E5Common::MAX_RGB9E5_MANTISSA));
		Check((bm <= ETextureRGB9E5Common::MAX_RGB9E5_MANTISSA));
		Check((rm >= 0));
		Check((gm >= 0));
		Check((bm >= 0));

		Retval.Field.r = rm;
		Retval.Field.g = gm;
		Retval.Field.b = bm;
		Retval.Field.BiasedExponent = (UINT32)ExpShared;

		OutRGB9E5 = Retval;
	}
	void ETextureRGB9E5Common::RGB9E5ToColor3(const RGB9E5 InRGB9E5, Color3& OutRGB)
	{
		// This produces equivalent output to the rgb9e5 to float3 reference implementation from
		// https://www.opengl.org/registry/specs/EXT/texture_shared_exponent.txt but saves a pow.

		union { FLOAT f; UINT32 i; } Value;
		Value.i = 0x33800000u + (InRGB9E5.Field.BiasedExponent << 23u);
		const FLOAT Scale = Value.f;
		OutRGB.r = Scale * FLOAT(InRGB9E5.Field.r);
		OutRGB.g = Scale * FLOAT(InRGB9E5.Field.g);
		OutRGB.b = Scale * FLOAT(InRGB9E5.Field.b);
	}

};