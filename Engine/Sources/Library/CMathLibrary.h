#pragma once
#include <algorithm>
constexpr float PI = 3.14159265f;
constexpr double PI_D = 3.1415926535897932f;

template<typename T>
constexpr auto sq(const T& x)
{
	return x * x;
}

template<typename T>
T WrapAngle(const T& Theta)
{
	const T modded = fmod(Theta, (T)2.0 * (T)PI_D);
	return (modded > (T)PI_D ? (modded - (T)2.0 * (T)PI_D) : modded);
}

template<typename T>
constexpr T interpolate(const T& src, const T& dst, float alpha)
{
	return src + (dst - src) * alpha;
}

template<typename T>
constexpr T to_rad(T deg)
{
	return deg * PI / (T)180.0;
}