#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

    class ETextureRGB9E5Common final
    {
    public:
        PE_CONSTEXPR_STATIC     INT32   RGB9E5_EXPONENT_BITS            = 5;
        PE_CONSTEXPR_STATIC     INT32   RGB9E5_MANTISSA_BITS            = 9;

        PE_CONSTEXPR_STATIC     INT32   RGB9E5_EXP_BIAS                 = 15;
        PE_CONSTEXPR_STATIC     INT32   RGB9E5_MAX_VALID_BIASED_EXP     = 31;

        PE_CONSTEXPR_STATIC     INT32   MAX_RGB9E5_EXP                  = (RGB9E5_MAX_VALID_BIASED_EXP - RGB9E5_EXP_BIAS);
        PE_CONSTEXPR_STATIC     INT32   RGB9E5_MANTISSA_VALUES          = (1 << RGB9E5_MANTISSA_BITS);
        PE_CONSTEXPR_STATIC     INT32   MAX_RGB9E5_MANTISSA             = (RGB9E5_MANTISSA_VALUES - 1);
        PE_CONSTEXPR_STATIC     FLOAT   MAX_RGB9E5                      = (((FLOAT)MAX_RGB9E5_MANTISSA) / RGB9E5_MANTISSA_VALUES * (1 << MAX_RGB9E5_EXP));
        PE_CONSTEXPR_STATIC     FLOAT   EPSILON_RGB9E5                  = ((1.0f / RGB9E5_MANTISSA_VALUES) / (1 << RGB9E5_EXP_BIAS));
    public:
        struct EBitsOfRGB9E5
        {
            UINT32  r               :   RGB9E5_MANTISSA_BITS;
            UINT32  g               :   RGB9E5_MANTISSA_BITS;
            UINT32  b               :   RGB9E5_MANTISSA_BITS;
            UINT32  BiasedExponent  :   RGB9E5_EXPONENT_BITS;
        };
        union RGB9E5
        {
            UINT32          Raw;
            EBitsOfRGB9E5   Field;
        };
    public:
        static void     Color3ToRGB9E5(const Color3& InRGB, RGB9E5& OutRGB9E5);
        static void     RGB9E5ToColor3(const RGB9E5 InRGB9E5, Color3& OutRGB);
    public:
        ETextureRGB9E5Common() {}
        ETextureRGB9E5Common(const ETextureRGB9E5Common&) {}
        ~ETextureRGB9E5Common() {}
        ETextureRGB9E5Common& operator=(const ETextureRGB9E5Common&) { return (*this); }
    };

};