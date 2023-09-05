#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	enum ELightType : UINT8
	{
		LIGHT_TYPE_DIRECTIONAL = 0,
		LIGHT_TYPE_POINT,
		LIGHT_TYPE_SPOT,
		LIGHT_TYPE_COUNT
	};

	struct ELightData
	{
		ELightData()noexcept : LightType(ELightType::LIGHT_TYPE_COUNT), LightColor(Color3::Black()), LightIntensity(1.f), CastShadow(FALSE), ShadowMapSize(Vector2Int(2, 2)) {}
		ELightData(const ELightData& Other)noexcept : LightType(Other.LightType), LightColor(Other.LightColor), LightIntensity(Other.LightIntensity), CastShadow(Other.CastShadow), ShadowMapSize(Other.ShadowMapSize) {}
		constexpr ELightData(ELightType	InLightType, FLOAT InColorR, FLOAT InColorG, FLOAT InColorB, FLOAT InIntensity, BOOL32 InCastShadow, INT32 InShadowMapWidth, INT32 InShadowMapHeight)noexcept : LightType(InLightType), LightColor(Color3(InColorR, InColorG, InColorB)), LightIntensity(InIntensity), CastShadow(InCastShadow), ShadowMapSize(Vector2Int(InShadowMapWidth, InShadowMapHeight)) {}
		ELightData& operator=(const ELightData& Other)
		{
			LightType		= Other.LightType;
			LightColor		= Other.LightColor;
			LightIntensity	= Other.LightIntensity;
			CastShadow		= Other.CastShadow;
			ShadowMapSize	= Other.ShadowMapSize;
			return (*this);
		}

		ELightType	LightType;
		Color3		LightColor;
		FLOAT		LightIntensity;
		BOOL32		CastShadow;
		Vector2Int	ShadowMapSize;
	};

	struct CascadeShadowData
	{


	};

};